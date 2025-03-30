import docker
import time
import subprocess
import psutil
import threading
import os
import select
import fcntl
import re
import signal

# Configuration variables
IMAGE_NAME = "peer"
LOCAL_INTERFACE = "wlo1"
METADATA_FILE_TO_DOWNLOAD_NAME = "testVideo.mp4.gitabic"
PERIODIC_INPUT_INTERVAL = 20  # Interval in seconds
IN_AWS = True
NUM_PEERS = 18  # Increased to 25 containers
INTERNET_SERVER = True

# Aggressive resource limits for containers
CPU_LIMIT = "0.3"  # Reduced to 15% of one CPU core (t3.xlarge has 4 cores = 16 containers at 25%)
MEMORY_LIMIT = "1g"  # Reduced to 512MB per container (t3.xlarge has 16GB = ~32 containers at 512MB)
CONTAINER_STARTUP_DELAY = 5  # Seconds between container starts

# Global container tracking
active_containers = {}
container_lock = threading.Lock()


def get_ip_address(interface):
    addrs = psutil.net_if_addrs()
    if interface in addrs:
        for addr in addrs[interface]:
            if addr.family == 2:  # AF_INET (IPv4)
                return addr.address
    return None


def read_output(process, container_id, stop_event):
    stdout_open, stderr_open = True, True

    # Regex patterns
    progress_pattern = re.compile(r'Progress:\s*(?:\[.*\])?\s*\d+\.?\d*%')
    finished_pattern = re.compile(r'Finished downloading file!!!')
    error_pattern = re.compile(r'error|exception|failed', re.IGNORECASE)

    completed_download = False
    while not stop_event.is_set() and (stdout_open or stderr_open):
        readable = []
        if stdout_open:
            readable.append(process.stdout)
        if stderr_open:
            readable.append(process.stderr)

        if not readable:
            break

        try:
            ready, _, _ = select.select(readable, [], [], 0.1)

            for stream in ready:
                line = stream.readline()
                if not line:
                    if stream is process.stdout:
                        stdout_open = False
                    else:
                        stderr_open = False
                    continue

                # Process output selectively to reduce logging overhead
                if progress_pattern.search(line) and not completed_download:
                    if "100%" in line:
                        print(f"Container {container_id} PROGRESS: {line.strip()}")
                    # Only print occasional progress updates
                    elif re.search(r'(?:25|50|75)%', line):
                        print(f"Container {container_id} PROGRESS: {line.strip()}")
                elif finished_pattern.search(line):
                    completed_download = True
                    print(f"Container {container_id} FINISHED: {line.strip()}")
                elif error_pattern.search(line):
                    print(f"Container {container_id} ERROR: {line.strip()}")

        except (ValueError, OSError):
            break

        # Reduced CPU usage
        time.sleep(0.1)


def periodic_input(process, container_id, stop_event):
    time.sleep(5)  # Initial delay

    while not stop_event.is_set():
        try:
            process.stdin.write("\n1\n")
            process.stdin.flush()
        except Exception as e:
            print(f"Container {container_id}: Input error - {e}")
            break

        time.sleep(PERIODIC_INPUT_INTERVAL)


def cleanup_container(container, container_id):
    try:
        print(f"Cleaning up container {container_id}")
        container.stop(timeout=5)
        container.remove()

        with container_lock:
            if container_id in active_containers:
                del active_containers[container_id]
    except Exception as e:
        print(f"Error cleaning up container {container_id}: {e}")


def create_container(curr_container, client, result_event):
    try:
        # Run container with strict resource limits
        container_name = f"peer_{curr_container}"

        # Check if container already exists and remove it
        try:
            old_container = client.containers.get(container_name)
            old_container.stop()
            old_container.remove()
            print(f"Removed existing container {container_name}")
        except docker.errors.NotFound:
            pass

        # Create new container with optimized settings
        container = client.containers.run(
            image=IMAGE_NAME,
            name=container_name,
            stdin_open=True,
            tty=True,
            detach=True,
            cpu_quota=int(float(CPU_LIMIT) * 100000),
            mem_limit=MEMORY_LIMIT,
            restart_policy={"Name": "on-failure", "MaximumRetryCount": 2},
            # Additional optimizations
            oom_kill_disable=False,  # Let the kernel manage OOM
            pids_limit=100,  # Limit number of processes
            ulimits=[docker.types.Ulimit(name='nofile', soft=1024, hard=2048)]
        )

        with container_lock:
            active_containers[curr_container] = container

        print(f"Container {curr_container} created with ID: {container.id[:12]}")
        time.sleep(2)

        # Prepare exec command
        ip_address = get_ip_address(LOCAL_INTERFACE)
        if INTERNET_SERVER:
            command = f"docker exec -i {container.id} ./PeerLion"
        else:
            command = f"docker exec -i {container.id} ./PeerLion {ip_address}"

        env = os.environ.copy()
        env["PYTHONUNBUFFERED"] = "1"

        process = subprocess.Popen(
            command,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=0,
            env=env
        )

        # Set non-blocking mode
        for stream in [process.stdout, process.stderr]:
            fd = stream.fileno()
            fl = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        stop_event = threading.Event()

        # Start monitoring threads with reduced priority
        output_thread = threading.Thread(
            target=read_output,
            args=(process, curr_container, stop_event),
            name=f"output_{curr_container}"
        )
        output_thread.daemon = True
        output_thread.start()

        input_thread = threading.Thread(
            target=periodic_input,
            args=(process, curr_container, stop_event),
            name=f"input_{curr_container}"
        )
        input_thread.daemon = True
        input_thread.start()

        # Send initial input
        time.sleep(1)
        process.stdin.write("\n4\n")
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        # Wait for process
        exit_code = process.wait()
        stop_event.set()

        if exit_code != 0:
            print(f"Container {curr_container} exited with code {exit_code}")
            if exit_code in [134, 139]:
                print(f"Container {curr_container} likely crashed due to memory limits")
        else:
            print(f"Container {curr_container} completed successfully")

        # Signal completion
        result_event.set()

        # Clean up
        output_thread.join(timeout=1)
        input_thread.join(timeout=1)
        cleanup_container(container, curr_container)

    except Exception as e:
        print(f"Error with container {curr_container}: {e}")
        result_event.set()  # Signal completion even on error


def sigterm_handler(signum, frame):
    print("Received termination signal, cleaning up containers...")
    for container_id, container in list(active_containers.items()):
        cleanup_container(container, container_id)
    os._exit(0)


def main():
    print("Starting Docker container management for 25 containers...")

    # Setup signal handler for graceful shutdown
    signal.signal(signal.SIGTERM, sigterm_handler)
    signal.signal(signal.SIGINT, sigterm_handler)

    try:
        if IN_AWS:
            client = docker.DockerClient(base_url='unix:///var/run/docker.sock')
        else:
            client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')

        print(f"Connected to Docker daemon, verifying image: {IMAGE_NAME}")
        try:
            client.images.get(IMAGE_NAME)
        except docker.errors.ImageNotFound:
            print(f"Error: Image '{IMAGE_NAME}' not found. Please build or pull the image first.")
            return

    except Exception as e:
        print(f"Failed to connect to Docker daemon: {e}")
        return

    # Add a new approach - batch processing
    # Only run a maximum number of containers simultaneously
    MAX_CONCURRENT = 12  # Start with current max and increase gradually

    for batch_start in range(0, NUM_PEERS, MAX_CONCURRENT):
        batch_end = min(NUM_PEERS, batch_start + MAX_CONCURRENT)
        print(f"Starting batch {batch_start // MAX_CONCURRENT + 1}: containers {batch_start} to {batch_end - 1}")

        threads = []
        result_events = []

        # Start batch of containers
        for i in range(batch_start, batch_end):
            result_event = threading.Event()
            result_events.append(result_event)

            thread = threading.Thread(
                target=create_container,
                args=(i, client, result_event),
                name=f"container_{i}"
            )
            thread.start()
            threads.append(thread)
            time.sleep(CONTAINER_STARTUP_DELAY)  # Longer delay between starts

        # Wait for all containers in this batch to complete
        for event in result_events:
            event.wait()

        print(f"Completed batch {batch_start // MAX_CONCURRENT + 1}")

    print("All container batches have completed")


if __name__ == '__main__':
    main()