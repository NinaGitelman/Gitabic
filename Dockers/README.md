# Docker Setup for PeerLion

This directory contains the necessary files to build and run the `PeerLion` application in Docker containers.

## Files

*   `Dockerfile`: Defines the Docker image for the `PeerLion` application. It's based on Ubuntu and includes all the necessary dependencies and files.
*   `dockerAutomation.py`: A Python script for automating the creation and management of multiple `PeerLion` containers. This is useful for testing and performance monitoring.
*   `dockerAutomationVisual.py`: A graphical version of the automation script that provides a visual interface for monitoring the containers.
*   `DockerHelp.txt`: A text file with helpful commands and notes for working with the Docker containers.
*   `*.gitabic`: Data files that can be used as examples by the `PeerLion`.

## How to Use

### Building the Docker Image

To build the Docker image, run the following command in this directory:

```bash
docker build . -t peer -f Dockerfile
```

### Running the Containers

You can run the containers in several ways:

*   **Manually:** Use the commands in `DockerHelp.txt` to run the containers manually.
*   **Automated Script:** Use the `dockerAutomation.py` script to run multiple containers at once. You can configure the script by changing the variables at the top of the file.
*   **Visual Interface:** Use the `dockerAutomationVisual.py` script to run the containers and monitor their output in a graphical interface.

### `dockerAutomation.py`

This script will create and run a specified number of Docker containers. You can configure the following variables in the script:

*   `IMAGE_NAME`: The name of the Docker image to use.
*   `LOCAL_INTERFACE`: The local network interface to use.
*   `METADATA_FILE_TO_DOWNLOAD_NAME`: The name of the metadata file to download.
*   `WAIT_BETWEEN_CONTAINERS`: The time to wait between starting each container.
*   `PERIODIC_INPUT_INTERVAL`: The interval at which to send input to the containers.
*   `IN_AWS`: Set to `True` if running on AWS.
*   `NUM_PEERS`: The number of containers to run.
*   `INTERNET_SERVER`: Set to `True` if the container is an internet server.
*   `CPU_LIMIT`: The CPU limit for each container.
*   `MEMORY_LIMIT`: The memory limit for each container.

To run the script:

```bash
python3 dockerAutomation.py
```

### `dockerAutomationVisual.py`

This script provides a graphical interface for monitoring the containers. To run the script:

```bash
python3 dockerAutomationVisual.py
```

This will open a window with a "Start Monitoring" button. Clicking this button will start the containers and display their output in separate text areas.
