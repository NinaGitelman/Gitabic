# Use the latest Ubuntu image
FROM ubuntu:latest

# Update package lists and install required packages in one layer
RUN apt-get update && \
    apt-get install -y libnice-dev glib-2.0 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy the script, file and metadata file to the container's working directory
COPY PeerLion /PeerLion
COPY DockerHelp.txt /DockerHelp.txt
COPY DockerHelp.txt.gitabic /DockerHelp.txt.gitabic

# Set default command to bash
CMD ["/bin/bash"]
