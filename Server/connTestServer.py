import socket

def start_server(ip, port):
    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
            # Allow reusing the address
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            # Bind to the specified IP and port
            server_socket.bind((ip, port))
            print(f"Server listening on {ip}:{port}")
            
            # Listen for incoming connections
            server_socket.listen(5)  # Accept up to 5 queued connections
            
            while True:
                print("Waiting for a connection...")
                conn, addr = server_socket.accept()  # Accept a connection
                print(f"Connected by {addr}")
                
                # Handle the client
                with conn:
                    data = conn.recv(1024)  # Receive up to 1024 bytes
                    if data:
                        print(f"Received: {data.decode()}")
                        conn.sendall(b"Hello from Server!")  # Respond to the client
    except Exception as e:
        print(f"An error occurred: {e}")

# Replace with your public IP and desired port
# For example, use the public IP of your machine or server
public_ip = "18.207.118.96"  # Change this to your public IP or keep as 0.0.0.0 to bind to all interfaces
port = 4789           # Change this to your desired port

start_server(public_ip, port)