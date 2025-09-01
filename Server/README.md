# ServerLion

ServerLion is a C++ server application that facilitates peer-to-peer (P2P) communication. It acts as a signaling server for the Interactive Connectivity Establishment (ICE) protocol and as a tracker for a file-sharing system.

## Features

*   **Multi-threaded TCP Server**: Handles multiple client connections concurrently.
*   **ICE Signaling**: Facilitates NAT traversal for P2P connections using ICE.
*   **File Tracker**: Keeps track of peers that have specific files, allowing clients to discover each other.
*   **SHA-256 Hashing**: Uses SHA-256 for creating unique identifiers for files and users.

## Project Structure

The project is organized into the following directories:

*   `Communication`: Contains the core server logic, including the multi-threaded server, message handlers, and ICE/tracker-specific message handling.
*   `Encryptions`: Includes cryptographic utilities, such as the SHA-256 implementation.
*   `NetworkUnit`: Contains network-related components, such as the TCP client socket and data storage for the tracker.
*   `Utils`: Provides utility classes for file operations and data serialization/deserialization.

## Building the Project

The project uses CMake to manage the build process. To build the project, you will need to have CMake and a C++ compiler (that supports C++20) installed.

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    ```
2.  **Create a build directory:**
    ```bash
    mkdir build && cd build
    ```
3.  **Run CMake:**
    ```bash
    cmake ..
    ```
4.  **Build the project:**
    ```bash
    make
    ```

This will create an executable named `ServerLion` in the `build` directory.

## How it Works

1.  **Server Initialization**: The `main` function creates an instance of `MultiThreadedServer` and starts it.
2.  **Client Connection**: The `MultiThreadedServer` listens for incoming TCP connections on port 4787. When a client connects, a new thread is created to handle it.
3.  **Message Handling**: The `MessageHandler` receives messages from clients and delegates them to the appropriate handler:
    *   **`IceMessagesHandler`**: Manages the exchange of ICE candidates between peers to establish a direct P2P connection.
    *   **`TrackerMessageHandling`**: Responds to requests for lists of peers who have a particular file.
4.  **Data Storage**: The `TrackerDataStorage` class stores information about which peers have which files. This data has a time-to-live (TTL) and is automatically removed after a certain period.

## Dependencies

The project has no external dependencies beyond the standard C++ library and a C++20 compatible compiler.

## Protocol

The communication between the server and clients follows a custom protocol defined in `NetworkUnit/ServerCom/Messages.h`. The messages are serialized into a byte stream before being sent over the network.
