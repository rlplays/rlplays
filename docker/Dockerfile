# Used Copilot/Claude to generate this Dockerfile
FROM ubuntu:22.04

# Avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Update apt and install packages
USER root
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    git \
    wget \
    libx11-dev \
    xorg-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    curl \
    jq \
    libc++1 \
    libc++abi1 \
    xfce4 \
    xfce4-goodies \
    xrdp \
    dbus-x11 \
    x11-xserver-utils \
    xterm \
    mousepad \
    openssh-server \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*


# Install CMake 3.27 (or latest stable version)
RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-x86_64.sh -q -O /tmp/cmake-install.sh \
    && chmod u+x /tmp/cmake-install.sh \
    && mkdir -p /opt/cmake \
    && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake \
    && ln -s /opt/cmake/bin/* /usr/local/bin/ \
    && rm /tmp/cmake-install.sh

# Configure xrdp to use XFCE
RUN echo "startxfce4" > /etc/skel/.xsession \
    && sed -i 's/port=3389/port=3389/' /etc/xrdp/xrdp.ini \
    && adduser xrdp ssl-cert

# Create a non-root user for RDP/SSH login
# Change 'rdpuser' and 'changeme' to your preferred username and password
RUN useradd -m -s /bin/bash rdpuser \
    && echo "rdpuser:changeme" | chpasswd \
    && echo "startxfce4" > /home/rdpuser/.xsession \
    && chown rdpuser:rdpuser /home/rdpuser/.xsession

# Configure SSH
RUN ssh-keygen -A \
    && sed -i 's/^#\?PasswordAuthentication.*/PasswordAuthentication yes/' /etc/ssh/sshd_config \
    && sed -i 's/^#\?PermitRootLogin.*/PermitRootLogin no/' /etc/ssh/sshd_config \
    && sed -i 's/^#\?UsePAM.*/UsePAM no/' /etc/ssh/sshd_config \
    && echo "PasswordAuthentication yes" >> /etc/ssh/sshd_config \
    && echo "UsePAM no" >> /etc/ssh/sshd_config

# Expose RDP and SSH ports
EXPOSE 3389 22

# Start SSH and xrdp on container startup
# /var/run/sshd must be created at runtime (tmpfs is remounted)
CMD ["sh", "-c", "mkdir -p /var/run/sshd && /usr/sbin/sshd && service xrdp start && tail -f /var/log/xrdp.log"]

# To build:
# docker build -t rlplays:linux .

# To run (might need `sudo`):
# docker run -d -p 3399:3389 -p 2222:22 --name rlplays_rdp rlplays:linux
# Ensure it runs by checking: docker ps
# Then RDP to: localhost:3399  (Username: rdpuser  Password: changeme)
# Then SSH:    ssh rdpuser@localhost -p 2222

# To rebuild if you change this file:
# Run this if you change the file quite a bit: docker rm -f rlplays_rdp
# docker build -t rlplays:linux .
# docker rm -f rlplays_rdp
# Then run using the command above.

