import docker
import time
import subprocess
import psutil

IMAGE_NAME = "peer"
LOCAL_INTERFACE = "wlo1"
'''
Function tp get local ip - used to be able to run the peerLion and use local host server
'''
def get_ip_address(interface):
    addrs = psutil.net_if_addrs()
    if interface in addrs:
        for addr in addrs[interface]:
            if addr.family == 2:  # AF_INET (IPv4)
                return addr.address
    return None

def run_server():
    docker_command = "docker run --add-host=host.docker.internal:host-gateway -d -p 3125:3125 gitabic_poc_server_image"

    # Execute the command
    try:
        result = subprocess.run(docker_command, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"Command Output: {result.stdout.decode()}")
    except subprocess.CalledProcessError as e:
        print(f"Error: {e.stderr.decode()}")

def main():

    time.sleep(3)

    # Create a Docker client
    client = docker.from_env()
    # Example IP addresses
    containers = []
    for i in range(2):
        container = create_container(i, client)

        if container:
            containers.append(container)
            time.sleep(5)

    for container in containers:
        print(container.id)


def create_container(curr_container, client):

    try:
        # Start a container with interactive settings
        container = client.containers.run(
            image=IMAGE_NAME,
            stdin_open=True,
            tty=True,
            detach=True,

        )
        time.sleep(1)

        #Attach to the container's stdin
        socket = container.attach_socket(params={'stdin': 1, 'stream': 1})


        user_input = "nina\n"
        socket.send(user_input.encode())

        print("container: ", in_port)
        # Optionally, capture the output from stdout
        logs = container.logs(stdout=True, stderr=True).decode("utf-8")
        print(f"Logs from container {in_port}:\n{logs}")

        time.sleep(1)

        logs = container.logs(stdout=True, stderr=True).decode("utf-8")
        print(f"Logs from container {in_port}:\n{logs}")

    except Exception as e:
        print(f"Error creating container {curr_container}: {e}")
        return None

    return container


if __name__ == '__main__':
    print(get_ip_address(LOCAL_INTERFACE))
    #main()
