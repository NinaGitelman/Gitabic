# Use the latest Ubuntu image
FROM ubuntu:latest

# Update package lists and install required packages in one layer
RUN apt-get update && \
    apt-get install -y libnice-dev glib-2.0 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy the script, file and metadata file to the container's working directory
COPY PeerLion /PeerLion
COPY image.png /image.png
COPY image.png.gitabic /image.png.gitabic
COPY test.txt /test.txt
COPY test.txt.gitabic /test.txt.gitabic
COPY simple-example /simple-example

# Set default command to bash
CMD ["/bin/bash"]
