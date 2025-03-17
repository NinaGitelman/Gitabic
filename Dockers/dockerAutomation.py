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

        # First send the input using docker exec
        input_cmd = f"docker exec -i {container_id} ./PeerLion"
        input_proc = subprocess.Popen(
            input_cmd,
            shell=True,
            stdin=subprocess.PIPE,
            text=True
        )

        # Send input commands
        input_proc.stdin.write("\n4\n")  # option to download from Gitabic file
        input_proc.stdin.write(f"{METADATA_FILE_TO_DOWNLOAD_NAME}\n")
        input_proc.stdin.flush()

        # Set up separate log monitoring thread that continues regardless of output
        def monitor_logs():
            last_timestamp = None
            no_new_output_count = 0
            max_no_output_counts = 12  # Wait for 60 seconds of no output before concluding

            while True:
                # Get all logs with timestamps
                logs = client.api.logs(container_id, timestamps=True, tail='all').decode('utf-8')

                # Extract the latest timestamp if available
                if logs:
                    log_lines = logs.strip().split('\n')
                    if log_lines:
                        # Timestamps are at the beginning of each line
                        latest_line = log_lines[-1]
                        if ' ' in latest_line:  # Format is typically "timestamp log_message"
                            current_timestamp = latest_line.split(' ')[0]

                            # If we have new output
                            if current_timestamp != last_timestamp:
                                print(f"Container {curr_container} logs ({len(log_lines)} lines):")
                                for line in log_lines:
                                    print(f"  {line}")
                                last_timestamp = current_timestamp
                                no_new_output_count = 0
                            else:
                                no_new_output_count += 1

                # If no new output for a while, print the status and latest stats
                if no_new_output_count >= max_no_output_counts:
                    container.reload()
                    print(f"Container {curr_container} status: {container.status}")
                    print(f"No new output for 60 seconds. Container may be stalled.")
                    print(f"Latest statistics: {client.api.stats(container_id, stream=False)}")
                    break

                # Wait 5 seconds before checking again
                time.sleep(5)

        # Start monitoring logs in a separate thread
        log_thread = threading.Thread(target=monitor_logs)
        log_thread.daemon = True
        log_thread.start()

        # Let the main process wait some time before continuing
        input_proc.wait(timeout=30)  # Wait up to 30 seconds for the exec process

        # Let the log monitoring continue for a while
        time.sleep(90)  # Allow 90 seconds for the log thread to capture output

        print(f"Container {curr_container} monitoring period ended")

    except Exception as e:
        import traceback
        print(f"Error with container {curr_container}: {e}")
        print(traceback.format_exc())

if __name__ == '__main__':
    main()