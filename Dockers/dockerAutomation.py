import docker
import time
import subprocess
import psutil
import threading
import os
import select
import fcntl
import tkinter as tk
from tkinter import scrolledtext, ttk

IMAGE_NAME = "peer"
LOCAL_INTERFACE = "wlo1"
METADATA_FILE_TO_DOWNLOAD_NAME = "testVideo.mp4.gitabic"
PERIODIC_INPUT_INTERVAL = 10  # Interval in seconds
CONTAINER_NUM = 3
IN_AWS_SERVER = True

class DockerContainerMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("Docker Peer Container Monitor")
        self.root.geometry("800x600")

        # Create a style
        self.style = ttk.Style()
        self.style.configure("TLabel", font=('Helvetica', 10, 'bold'))
        self.style.configure("TFrame", background='#f0f0f0')

        # Main frame
        self.main_frame = ttk.Frame(root, padding="10")
        self.main_frame.pack(fill=tk.BOTH, expand=True)

        # Containers frame
        self.containers_frame = ttk.Frame(self.main_frame)
        self.containers_frame.pack(fill=tk.BOTH, expand=True)

        # Container to store container widgets
        self.container_widgets = {}

        # Docker client

        if IN_AWS_SERVER:
            # CHANGE TO SERVER
            self.client = docker.DockerClient(base_url='unix:///var/run/docker.sock')
        else:
            self.client = docker.DockerClient(base_url='unix:///home/user/.docker/desktop/docker.sock')

        # Start monitoring button
        self.start_button = ttk.Button(self.main_frame, text="Start Monitoring", command=self.start_monitoring)
        self.start_button.pack(pady=10)

    def create_container_widget(self, container_id):
        # Create a frame for this container
        container_frame = ttk.Frame(self.containers_frame, borderwidth=2, relief=tk.RAISED)
        container_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Container label
        label = ttk.Label(container_frame, text=f"Container {container_id}", style="TLabel")
        label.pack(pady=(5, 0))

        # Scrolled text area for output
        text_area = scrolledtext.ScrolledText(
            container_frame,
            wrap=tk.WORD,
            width=40,
            height=15,
            font=('Consolas', 10)
        )
        text_area.pack(padx=5, pady=5, fill=tk.BOTH, expand=True)

        return {
            'frame': container_frame,
            'label': label,
            'text_area': text_area
        }

    def get_ip_address(self, interface):
        addrs = psutil.net_if_addrs()
        if interface in addrs:
            for addr in addrs[interface]:
                if addr.family == 2:  # AF_INET (IPv4)
                    return addr.address
        return None

    def read_output(self, process, container_id, text_area, stop_event):
        stdout_open, stderr_open = True, True

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

                    # Update text area in the main thread
                    self.root.after(0, self.update_text_area, text_area, f"Container {container_id}: {line.strip()}\n")

            except (ValueError, OSError):
                # Stream closed or other error
                break

            time.sleep(0.01)  # Small sleep to prevent CPU spinning

    def update_text_area(self, text_area, message):
        text_area.insert(tk.END, message)
        text_area.see(tk.END)

    def periodic_input(self, process, container_id, stop_event):
        while not stop_event.is_set():
            try:
                process.stdin.write("1\n")
                process.stdin.flush()
                # Update text area in the main thread
                self.root.after(0, self.update_text_area,
                                self.container_widgets[container_id]['text_area'],
                                f"Container {container_id}: Sent input '1'\n"
                                )
            except Exception as e:
                self.root.after(0, self.update_text_area,
                                self.container_widgets[container_id]['text_area'],
                                f"Container {container_id}: Error sending input - {e}\n"
                                )

            time.sleep(PERIODIC_INPUT_INTERVAL)

    def create_container(self, curr_container):
        try:
            # Create widget for this container
            self.container_widgets[curr_container] = self.create_container_widget(curr_container)
            text_area = self.container_widgets[curr_container]['text_area']

            # Run the container
            container = self.client.containers.run(
                image=IMAGE_NAME,
                name=f"peer_{curr_container}",
                stdin_open=True,
                tty=True,
                detach=True,
            )
            time.sleep(1)

            # Run the command inside the container with unbuffered output
            ip_address = self.get_ip_address(LOCAL_INTERFACE)
            # CHANGE SERVER

            if IN_AWS_SERVER:
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
            output_thread = threading.Thread(target=self.read_output,
                                             args=(process, curr_container, text_area, stop_event))
            output_thread.daemon = True
            output_thread.start()

            periodic_input_thread = threading.Thread(target=self.periodic_input,
                                                     args=(process, curr_container, stop_event))
            periodic_input_thread.daemon = True
            periodic_input_thread.start()

            # Store threads and process for potential later management
            self.container_widgets[curr_container].update({
                'process': process,
                'output_thread': output_thread,
                'periodic_input_thread': periodic_input_thread,
                'stop_event': stop_event
            })

        except Exception as e:
            self.update_text_area(
                self.container_widgets[curr_container]['text_area'],
                f"Error creating container {curr_container}: {e}\n"
            )

    def start_monitoring(self):
        # Disable start button to prevent multiple starts
        self.start_button.config(state=tk.DISABLED)

        # Create containers
        for i in range(CONTAINER_NUM):
            self.create_container(i)


def main():
    root = tk.Tk()
    app = DockerContainerMonitor(root)
    root.mainloop()


if __name__ == '__main__':
    main()