# Gitabic

Gitabic is a peer-to-peer file sharing application that uses a custom BitTorrent like protocol to download and share files. It consists of two main components:

*   **PeerLion:** A command-line client that allows users to download and share files.
*   **ServerLion:** A centralized server that acts as a signaling server for ICE and a tracker for peer discovery.

## Features

*   **Decentralized file sharing:** Gitabic uses the BitTorrent protocol to download and share files from multiple peers at once, making it a fast and efficient way to share large files.
*   **NAT traversal:** Gitabic uses ICE (Interactive Connectivity Establishment) to traverse NATs and firewalls, allowing peers to connect to each other directly even if they are behind a router.
*   **Command-line interface:** PeerLion provides a simple command-line interface for adding new torrents, and managing downloads.
*   **File integrity:** Gitabic uses SHA256 to verify the integrity of files, ensuring that they have not been corrupted during download.
*   **Multi-threaded TCP Server**: The ServerLion component handles multiple client connections concurrently.
*   **File Tracker**: The ServerLion keeps track of peers that have specific files, allowing clients to discover each other.

## Dependencies

### PeerLion (Client)

*   **libnice:** A library that provides an implementation of the ICE protocol.
*   **glib-2.0:** A core library that provides data structures, utilities, and a main loop implementation.

### ServerLion (Server)

*   A C++ compiler that supports C++20.

## How to build

Both projects were developed in Clion so it is possible to open them directly in Clion and run it from there.

### PeerLion (Client)

1.  Install the dependencies:
    *   On macOS, you can use Homebrew to install the dependencies:
        ```
        brew install libnice glib pkg-config openssl
        ```
    *   On Linux, you can use your distribution's package manager to install the dependencies. For example, on Ubuntu, you can use the following command:
        ```
        sudo apt-get install libnice-dev libglib2.0-dev pkg-config libssl-dev
        ```
2.  Build the project using CMake (Or in Clion directly):
    ```
    cd PeerLion
    mkdir build
    cd build
    cmake ..
    make
    ```

### ServerLion (Server)

1.  Build the project using CMake (Or in Clion directly):
    ```
    cd ServerLion
    mkdir build
    cd build
    cmake ..
    make
    ```

## How to run

1.  Start the signaling server:
    ```
    ./ServerLion/build/ServerLion
    ```
    The server will start listening for connections on port 4787.
2.  Run the PeerLion application:
    ```
    ./PeerLion/build/PeerLion <server_address>
    ```
    Replace `<server_address>` with the address of your signaling server (e.g. `127.0.0.1`).

## Docker Setup

This directory contains the necessary files to build and run the `PeerLion` application in Docker containers.

## Protocol

The communication between the server and clients follows a custom protocol defined in `ServerLion/NetworkUnit/ServerCom/Messages.h`. The messages are serialized into a byte stream before being sent over the network.

### Message Types

*   **ICE Messages**: Used for exchanging ICE candidates between peers.
*   **Tracker Messages**: Used for querying the tracker for a list of peers.

### Message Structure

Each message consists of a header and a payload. The header contains the message type and the size of the payload. The payload contains the actual data.

## Contributing

If you are interested in contributing or developing this project further, feel welcome to contact us!

## License

This project is licensed under the MIT License.

---

✨ This project was created as the **Magshimim final year project** by [Uri Tabic](https://github.com/UriTabic) and [Nina Gitelman](https://github.com/NinaGitelman).
