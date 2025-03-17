import docker
import time
import subprocess
import psutil
import threading

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

    #local comp:\
   # client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')
    client = docker.DockerClient(base_url='unix:///var/run/docker.sock')
    print(client.images.list())

    threads = []
    for i in range(3):
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
            stdin_open=True,
            tty=True,
            detach=True,
        )
        time.sleep(1)

        # Run the command inside the container
        command = f"docker exec -i {container.id} ./PeerLion"
        process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True)

        # Send input
        process.stdin.write("\n4\n")  # option to download from Gitabic file
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        # Use select to read output without blocking
        def read_all_output(proc, container_id):
            import select
            import sys

            # Set stdout and stderr to non-blocking mode
            import fcntl
            import os
            for stream in [proc.stdout, proc.stderr]:
                fd = stream.fileno()
                fl = fcntl.fcntl(fd, fcntl.F_GETFL)
                fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

            stdout_data = ""
            stderr_data = ""

            while proc.poll() is None:
                # Check if stdout or stderr has data to read
                reads = [proc.stdout.fileno(), proc.stderr.fileno()]
                readable, _, _ = select.select(reads, [], [], 0.1)

                for fd in readable:
                    if fd == proc.stdout.fileno():
                        data = proc.stdout.read(1024)
                        if data:
                            stdout_data += data
                            print(f"Logs from container {container_id}: {data}", end="")
                            sys.stdout.flush()
                    elif fd == proc.stderr.fileno():
                        data = proc.stderr.read(1024)
                        if data:
                            stderr_data += data
                            print(f"Errors from container {container_id}: {data}", end="")
                            sys.stdout.flush()

            # Read any remaining data after process exits
            remaining_stdout = proc.stdout.read()
            if remaining_stdout:
                stdout_data += remaining_stdout
                print(f"Final logs from container {container_id}: {remaining_stdout}", end="")

            remaining_stderr = proc.stderr.read()
            if remaining_stderr:
                stderr_data += remaining_stderr
                print(f"Final errors from container {container_id}: {remaining_stderr}", end="")

            return stdout_data, stderr_data

        stdout, stderr = read_all_output(process, curr_container)

        print(f"Container {curr_container} process exited with code {process.returncode}")
        print(f"Total output length: {len(stdout)} characters")

    except Exception as e:
        print(f"Error creating container {curr_container}: {e}")

if __name__ == '__main__':
    main()