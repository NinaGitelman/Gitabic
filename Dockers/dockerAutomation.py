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
        print(f"Container {curr_container} started with ID: {container.id}")
        time.sleep(1)

        # Prepare the command to execute in the container
        exec_command = container.exec_run(
            cmd="./PeerLion",
            stdin=True,
            tty=True,
            stream=True
        )

        # Store the exec ID for later reference
        exec_id = exec_command.id
        print(f"Started PeerLion in container {curr_container} with exec ID: {exec_id}")

        # Send input to the container using docker API
        client.api.exec_start(
            exec_id=exec_id,
            detach=False,
            tty=True,
            stdin=True,
            socket=True
        )

        # Directly attach to the container for logs
        log_generator = container.logs(stream=True, follow=True)

        # Print all logs as they come in
        print(f"Streaming logs for container {curr_container}")
        for log_line in log_generator:
            print(f"Container {curr_container}: {log_line.decode('utf-8').rstrip()}")

        print(f"Log stream ended for container {curr_container}")

        # Check container status after logs end
        container.reload()
        print(f"Container {curr_container} final status: {container.status}")

        # Get the exit code if container has stopped
        if container.status == 'exited':
            exit_info = client.api.inspect_container(container.id)['State']
            print(f"Container {curr_container} exit code: {exit_info.get('ExitCode')}")
            print(f"Container {curr_container} error: {exit_info.get('Error', 'None')}")

    except Exception as e:
        import traceback
        print(f"Error with container {curr_container}: {e}")
        print(traceback.format_exc())
if __name__ == '__main__':
    main()