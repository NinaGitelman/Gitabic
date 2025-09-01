# PeerLion

PeerLion is a peer-to-peer file sharing application that uses the BitTorrent protocol to download and share files. It uses a centralized server for signaling and peer discovery, and ICE for NAT traversal to establish direct peer-to-peer connections.

## Features

*   **Decentralized file sharing:** PeerLion uses the BitTorrent protocol to download and share files from multiple peers at once, making it a fast and efficient way to share large files.
*   **NAT traversal:** PeerLion uses ICE (Interactive Connectivity Establishment) to traverse NATs and firewalls, allowing peers to connect to each other directly even if they are behind a router.
*   **Command-line interface:** PeerLion provides a simple command-line interface for adding new torrents, and managing downloads.
*   **File integrity:** PeerLion uses SHA256 to verify the integrity of files, ensuring that they have not been corrupted during download.

## Dependencies

*   **libnice:** A library that provides an implementation of the ICE protocol.
*   **glib-2.0:** A core library that provides data structures, utilities, and a main loop implementation.

## How to build

1.  Install the dependencies:
    *   On macOS, you can use Homebrew to install the dependencies:
        ```
        brew install libnice glib pkg-config openssl
        ```
    *   On Linux, you can use your distribution's package manager to install the dependencies. For example, on Ubuntu, you can use the following command:
        ```
        sudo apt-get install libnice-dev libglib2.0-dev pkg-config libssl-dev
        ```
2.  Build the project using CMake:
    ```
    mkdir build
    cd build
    cmake ..
    make
    ```

## How to run

1.  Start the signaling server (check ServerLion README.md). The signaling server is responsible for coordinating the connection between peers. You can use any signaling server that you want, as long as it is accessible to all peers.
2.  Run the PeerLion application:
    ```
    ./PeerLion <server_address>
    ```
    Replace `<server_address>` with the address of your signaling server.

## Code Structure

*   **`main.cpp`:** The entry point of the application. It initializes the `TorrentCLI` and starts the main loop.
*   **`TorrentCLI`:** A class that provides a command-line interface for interacting with the application.
*   **`TorrentManager`:** A singleton class that manages all active torrents.
*   **`PeersConnectionManager`:** A class that manages the connection to the signaling server and the discovery of new peers.
*   **`ICEConnection`:** A class that implements the ICE protocol for NAT traversal.
*   **`FileIO`:** A class that handles all file I/O operations.
*   **`Encryptions`:** A directory that contains the implementation of the AES and SHA256 encryption algorithms.
*   **`NetworkUnit`:** A directory that contains the implementation of the networking protocols, including the BitTorrent protocol and the signaling protocol.
*   **`Torrent`:** A directory that contains the implementation of the BitTorrent protocol.
*   **`Utils`:** A directory that contains various utility classes.
