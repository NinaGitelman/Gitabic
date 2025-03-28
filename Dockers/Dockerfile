# Use the latest Ubuntu image
FROM ubuntu:jammy-20240530

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
COPY test19Kb.txt.gitabic /test19Kb.txt.gitabic
COPY testVideo.mp4.gitabic /testVideo.mp4.gitabic
COPY docker-desktop-amd64.deb.gitabic /docker-desktop-amd64.deb.gitabic

# Make the PeerLion file executable
RUN chmod +x /PeerLion

# Set default command to bash
CMD ["/bin/bash"]