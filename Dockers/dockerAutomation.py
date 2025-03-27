import docker
import time
import subprocess
import psutil
import threading
import os
import select
import fcntl
import re

IMAGE_NAME = "peer"
LOCAL_INTERFACE = "wlo1"
METADATA_FILE_TO_DOWNLOAD_NAME = "testVideo.mp4.gitabic"
PERIODIC_INPUT_INTERVAL = 10  # Interval in seconds
IN_AWS = False
NUM_PEERS = 2
INTERNET_SERVER = False


def get_ip_address(interface):
    addrs = psutil.net_if_addrs()
    if interface in addrs:
        for addr in addrs[interface]:
            if addr.family == 2:  # AF_INET (IPv4)
                return addr.address
    return None


def read_output(process, container_id, stop_event):
    stdout_open, stderr_open = True, True
    progress_printed = False

    # Regex pattern to match progress lines with optional progress bar and percentage
    progress_pattern = re.compile(r'Progress:\s*(?:\[.*\])?\s*\d+\.?\d*%')
    finished_pattern = re.compile(r'Finished downloading file!!!')

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

                # Debug print for ALL lines
                # print(f"Container {container_id} RAW LINE: {line.strip()}")

                # Check for progress line
                progress_match = progress_pattern.search(line)
                finished_match = finished_pattern.search(line)

                # Debug print for match statuses
                # print(f"Container {container_id} Progress Match: {bool(progress_match)}, Printed: {progress_printed}")

                # Only print progress line after input of "\n1\n"
                if progress_match:
                    print(f"Container {container_id} PROGRESS: {line.strip()}")

                # Print finished message
                if finished_match:
                    print(f"Container {container_id} FINISHED: {line.strip()}")


        except (ValueError, OSError):
            # Stream closed or other error
            break

        time.sleep(0.01)  # Small sleep to prevent CPU spinning


def periodic_input(process, container_id, stop_event):
    while not stop_event.is_set():
        try:
            process.stdin.write("\n1\n")
            process.stdin.write("1\n")
            process.stdin.flush()
        except Exception as e:
            print(f"Container {container_id}: Error sending input - {e}")

        time.sleep(PERIODIC_INPUT_INTERVAL)


def create_container(curr_container, client):
    try:
        # Run the container
        container = client.containers.run(
            image=IMAGE_NAME,
            name=f"peer_{curr_container}",
            stdin_open=True,
            tty=True,
            detach=True,
        )
        time.sleep(1)

        # Run the command inside the container with unbuffered output
        ip_address = get_ip_address(LOCAL_INTERFACE)
        if INTERNET_SERVER:
            command = f"docker exec -i {container.id} ./PeerLion"
        else:
            command = f"docker exec -i {container.id} ./PeerLion {ip_address}"

        # Set environment variable to disable Python output buffering
        env = os.environ.copy()
        env["PYTHONUNBUFFERED"] = "1"

        process = subprocess.Popen(
            command,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=0,  # Set to unbuffered
            env=env
        )

        # Set non-blocking mode for stdout and stderr
        for stream in [process.stdout, process.stderr]:
            fd = stream.fileno()
            fl = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        # Create stop event for threads
        stop_event = threading.Event()

        # Send initial input to download from Gitabic file
        process.stdin.write("\n4\n")  # option to download from Gitabic file
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        # Create threads for reading output and periodic input
        output_thread = threading.Thread(target=read_output, args=(process, curr_container, stop_event))
        output_thread.daemon = True
        output_thread.start()

        periodic_input_thread = threading.Thread(target=periodic_input, args=(process, curr_container, stop_event))
        periodic_input_thread.daemon = True
        periodic_input_thread.start()

        # Wait for process to complete
        exit_code = process.wait()

        # Stop threads
        stop_event.set()
        output_thread.join(timeout=2)
        periodic_input_thread.join(timeout=2)

        print(f"Container {curr_container} process exited with code {exit_code}")

    except Exception as e:
        print(f"Error creating container {curr_container}: {e}")


def main():
    time.sleep(3)

    if IN_AWS:
        client = docker.DockerClient(base_url='unix:///var/run/docker.sock')
    else:
        client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')
    print(client.images.list())

    threads = []
    for i in range(NUM_PEERS):
        print(f"Creating container {i}")
        thread = threading.Thread(target=create_container, args=(i, client))
        thread.start()
        threads.append(thread)

    # Keep main thread alive
    for thread in threads:
        thread.join()


if __name__ == '__main__':
    main()