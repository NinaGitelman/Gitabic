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
        container_id = container.id
        print(f"Container {curr_container} started with ID: {container_id}")
        time.sleep(1)

        # Use docker CLI directly via subprocess, but with full output capture
        command = f"docker exec -i {container_id} ./PeerLion"
        print(f"Executing command for container {curr_container}: {command}")

        # Run the process but capture full output using a different method
        with subprocess.Popen(
                command,
                shell=True,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,  # Merge stderr into stdout
                bufsize=0,  # Unbuffered
                universal_newlines=True
        ) as proc:
            # Send input
            proc.stdin.write("\n4\n")  # option to download from Gitabic file
            proc.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
            proc.stdin.flush()

            # Read output one character at a time with a timeout
            import select
            output_buffer = ""

            while proc.poll() is None:
                # Check if there's data to read within 0.1 seconds
                if select.select([proc.stdout], [], [], 0.1)[0]:
                    # Read up to 1024 characters at a time
                    data = proc.stdout.read(1024)
                    if data:
                        output_buffer += data
                        print(f"Container {curr_container}: {data}", end="", flush=True)

            # Read any remaining output after the process exits
            while True:
                data = proc.stdout.read(1024)
                if not data:
                    break
                output_buffer += data
                print(f"Container {curr_container} (final output): {data}", end="", flush=True)

            print(f"\nProcess for container {curr_container} completed.")
            print(f"Output length: {len(output_buffer)} characters")
            print(f"Exit code: {proc.returncode}")

        # Also get container logs to see if there's anything missed
        try:
            container.reload()
            print(f"Container {curr_container} final status: {container.status}")
            container_logs = container.logs().decode('utf-8')
            if container_logs:
                print(f"Additional container logs for {curr_container} (length: {len(container_logs)} chars):")
                print(container_logs)
        except Exception as e:
            print(f"Failed to get container logs for {curr_container}: {e}")

    except Exception as e:
        import traceback
        print(f"Error with container {curr_container}: {e}")
        print(traceback.format_exc())

if __name__ == '__main__':
    main()