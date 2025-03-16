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

    client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')
    print(client.images.list())

    threads = []
    for i in range(2):
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
        command = f"docker exec -i {container.id} ./PeerLion {ip_address}"
        process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        # Send input
        process.stdin.write("\n4\n")  # option to download from Gitabic file
        process.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        process.stdin.flush()

        # Read output in real-time
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(f"Logs from container {curr_container}: {output.strip()}")

    except Exception as e:
        print(f"Error creating container {curr_container}: {e}")


if __name__ == '__main__':
    main()