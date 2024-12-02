import socket

def send_hello_world(ip, port):
    message = "Hello, World!"
    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
            # Connect to the server
            client_socket.connect((ip, port))
            print(f"Connected to {ip}:{port}")
            
            # Send the message
            client_socket.sendall(message.encode())
            print(f"Sent: {message}")
            
            # Optionally, receive a response
            response = client_socket.recv(1024).decode()
            print(f"Received: {response}")
    except Exception as e:
        print(f"An error occurred: {e}")

# Replace with your desired IP and port
target_ip = "3.87.119.17"  # Change to the specific IP
target_port = 4789      # Change to the specific port

send_hello_world(target_ip, target_port)
