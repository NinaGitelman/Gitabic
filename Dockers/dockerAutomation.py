import docker
import time
import subprocess
import psutil
import threading
import os
import select
import fcntl

IMAGE_NAME = "peer"
LOCAL_INTERFACE = "wlo1"
METADATA_FILE_TO_DOWNLOAD_NAME = "testVideo.mp4.gitabic"


def get_ip_address(interface):
    addrs = psutil.net_if_addrs()
    if interface in addrs:
        for addr in addrs[interface]:
            if addr.family == 2:  # AF_INET (IPv4)
                return addr.address
    return None


def main():
    time.sleep(3)

    #client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')
    client = docker.DockerClient(base_url='unix:///var/run/docker.sock')
    print(client.images.list())

    threads = []
    for i in range(4):
        print(f"Creating container {i}")
        thread = threading.Thread(target=create_container, args=(i, client))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()


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
        # Add the -u flag to docker exec for unbuffered output
        command = f"docker exec -i {container.id} ./PeerLion"

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

        # Send input
        process.stdin.write("\n4\n")  # option to download from Gitabic file
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        # Read output using select for non-blocking I/O
        def read_output(process, container_id):
            stdout_open, stderr_open = True, True

            while stdout_open or stderr_open:
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

                        print(f"Container {container_id}: {line.strip()}")

                except (ValueError, OSError):
                    # Stream closed or other error
                    break

                # Check if process has exited
                if process.poll() is not None and not ready:
                    break

                time.sleep(0.01)  # Small sleep to prevent CPU spinning

        # Use a single thread for both stdout and stderr
        output_thread = threading.Thread(target=read_output, args=(process, curr_container))
        output_thread.daemon = True
        output_thread.start()

        # Wait for process to complete
        exit_code = process.wait()
        output_thread.join(timeout=2)

        print(f"Container {curr_container} process exited with code {exit_code}")

    except Exception as e:
        print(f"Error creating container {curr_container}: {e}")


if __name__ == '__main__':
    main()