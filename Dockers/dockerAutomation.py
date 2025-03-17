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

   # local comp: client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')
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
            stdin_open=True,
            tty=True,
            detach=True,
        )
        time.sleep(1)

        # Run the command inside the container
        ip_address = get_ip_address(LOCAL_INTERFACE)
        command = f"docker exec -i {container.id} ./PeerLion"
        process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True, bufsize=1)

        # Send input
        process.stdin.write("\n4\n")  # option to download from Gitabic file
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        # Use non-blocking reads with timeout
        def read_output(stream, container_id):
            while True:
                line = stream.readline()
                if not line and process.poll() is not None:
                    break
                if line:
                    print(f"Logs from container {container_id}: {line.strip()}")

        # Start threads to read stdout and stderr
        out_thread = threading.Thread(target=read_output, args=(process.stdout, curr_container))
        err_thread = threading.Thread(target=read_output, args=(process.stderr, curr_container))
        out_thread.daemon = True
        err_thread.daemon = True
        out_thread.start()
        err_thread.start()

        # Wait for process to complete
        exit_code = process.wait()
        out_thread.join(timeout=1)
        err_thread.join(timeout=1)

        print(f"Container {curr_container} process exited with code {exit_code}")

    except Exception as e:
        print(f"Error creating container {curr_container}: {e}")

if __name__ == '__main__':
    main()