import docker
import time
import subprocess
import psutil
import threading
import os
import select
import fcntl
import re

# Configuration variables
IMAGE_NAME = "peer"
LOCAL_INTERFACE = "wlo1"
METADATA_FILE_TO_DOWNLOAD_NAME = "testVideo.mp4.gitabic"
PERIODIC_INPUT_INTERVAL = 3  # Interval in seconds
IN_AWS = False  # Changed to True since you're using AWS
NUM_PEERS = 4  # Number of containers to run (adjust as needed)
INTERNET_SERVER = False


# Resource limits for containers
CPU_LIMIT = "0.3"  # Limit each container to 30% of one CPU core
MEMORY_LIMIT = "1g"  # Limit each container to 1GB memory


def get_ip_address(interface):
    addrs = psutil.net_if_addrs()
    if interface in addrs:
        for addr in addrs[interface]:
            if addr.family == 2:  # AF_INET (IPv4)
                return addr.address
    return None


def read_output(process, container_id, stop_event, progress_dict, progress_lock):
    stdout_open, stderr_open = True, True

    # Regex patterns
    progress_pattern = re.compile(r'Progress:\s*(?:\[.*?\])?\s*(\d+\.?\d*)%')
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

                # Process the output line
                if progress_pattern.search(line) and not completed_download:
                    match = progress_pattern.search(line)
                    percent = float(match.group(1))
                    with progress_lock:
                        progress_dict[container_id] = {
                            'percent': percent,
                            'finished': False,
                            'error': None
                        }
                elif finished_pattern.search(line):
                    with progress_lock:
                        progress_dict[container_id] = {
                            'percent': 100.0,
                            'finished': True,
                            'error': None
                        }
                    completed_download = True
                elif error_pattern.search(line):
                    with progress_lock:
                        progress_dict[container_id] = {
                            'percent': progress_dict.get(container_id, {}).get('percent', 0.0),
                            'finished': False,
                            'error': line.strip()
                        }

        except (ValueError, OSError):
            break

        time.sleep(0.05)


def periodic_input(process, container_id, stop_event, progress_lock, progress_dict):
    time.sleep(5)

    while not stop_event.is_set():
        try:
            process.stdin.write("\n1\n")
            process.stdin.flush()
        except Exception as e:
            with progress_lock:
                progress_dict[container_id] = {
                    'percent': progress_dict.get(container_id, {}).get('percent', 0.0),
                    'finished': False,
                    'error': f"Input error: {str(e)}"
                }
            break

        time.sleep(PERIODIC_INPUT_INTERVAL)


def create_container(curr_container, client, progress_dict, progress_lock):
    try:
        container = client.containers.run(
            image=IMAGE_NAME,
            name=f"peer_{curr_container}",
            stdin_open=True,
            tty=True,
            detach=True,
            cpu_quota=int(float(CPU_LIMIT) * 100000),
            mem_limit=MEMORY_LIMIT,
            restart_policy={"Name": "on-failure", "MaximumRetryCount": 3}
        )
        time.sleep(2)

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

        for stream in [process.stdout, process.stderr]:
            fd = stream.fileno()
            fl = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        stop_event = threading.Event()

        time.sleep(1)
        process.stdin.write("\n4\n")
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        output_thread = threading.Thread(
            target=read_output,
            args=(process, curr_container, stop_event, progress_dict, progress_lock),
            name=f"output_{curr_container}"
        )
        output_thread.daemon = True
        output_thread.start()

        input_thread = threading.Thread(
            target=periodic_input,
            args=(process, curr_container, stop_event, progress_lock, progress_dict),
            name=f"input_{curr_container}"
        )
        input_thread.daemon = True
        input_thread.start()

        exit_code = process.wait()
        stop_event.set()

        if exit_code != 0:
            with progress_lock:
                progress_dict[curr_container] = {
                    'percent': progress_dict.get(curr_container, {}).get('percent', 0.0),
                    'finished': False,
                    'error': f"Exited with code {exit_code}"
                }

        try:
            container.stop(timeout=5)
            container.remove()
        except Exception as e:
            pass

    except Exception as e:
        with progress_lock:
            progress_dict[curr_container] = {
                'percent': 0.0,
                'finished': False,
                'error': str(e)
            }


def display_progress(progress_dict, progress_lock, stop_event, num_peers):
    while not stop_event.is_set():
        time.sleep(5)
        with progress_lock:
            current_progress = {k: v.copy() for k, v in progress_dict.items()}

        os.system('clear')
        print("Container Progress (Updated every 5 seconds):")
        for i in range(num_peers):
            status = current_progress.get(i, {})
            error = status.get('error')
            if error:
                print(f"Container {i}: ERROR - {error}")
            elif status.get('finished', False):
                print(f"Container {i}: FINISHED (100%)")
            else:
                print(
                    f"Container {i}: " + get_progress_bar(status.get('percent', 0.0)))
        print("--------------------------------------------")


def get_progress_bar(percentage, bar_length=50):
    percentage = max(0.0, min(100.0, percentage))
    filled_length = int(round(bar_length * percentage / 100))
    return f"[{'=' * filled_length}>{' ' * (bar_length - filled_length)}] {percentage:.1f}%"


def main():
    print("Starting Docker container management...")

    progress_dict = {}
    progress_lock = threading.Lock()
    display_stop_event = threading.Event()

    display_thread = threading.Thread(
        target=display_progress,
        args=(progress_dict, progress_lock, display_stop_event, NUM_PEERS),
        name="display_thread"
    )
    display_thread.daemon = True
    display_thread.start()

    try:
        if IN_AWS:
            client = docker.DockerClient(base_url='unix:///var/run/docker.sock')
        else:
            client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')
    except Exception as e:
        print(f"Failed to connect to Docker: {e}")
        return

    threads = []
    for i in range(NUM_PEERS):
        thread = threading.Thread(
            target=create_container,
            args=(i, client, progress_dict, progress_lock),
            name=f"container_{i}"
        )
        thread.start()
        threads.append(thread)
        time.sleep(3)

    for thread in threads:
        thread.join()

    display_stop_event.set()
    display_thread.join(timeout=5)
    print("All containers have completed execution")


if __name__ == '__main__':
    main()
