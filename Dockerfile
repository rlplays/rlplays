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
    # Desktop environment + RDP server
    xfce4 \
    xfce4-goodies \
    xrdp \
    dbus-x11 \
    x11-xserver-utils \
    # Useful GUI apps
    xterm \
    mousepad \
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

# Create a non-root user for RDP login
# Change 'rdpuser' and 'changeme' to your preferred username and password
RUN useradd -m -s /bin/bash rdpuser \
    && echo "rdpuser:changeme" | chpasswd \
    && echo "startxfce4" > /home/rdpuser/.xsession \
    && chown rdpuser:rdpuser /home/rdpuser/.xsession

# Expose RDP port
EXPOSE 3389

# Start xrdp on container startup
CMD ["sh", "-c", "service xrdp start && tail -f /var/log/xrdp.log"]

# To build:
# docker build -t rlplays:linux .
# To run:
# docker run -d -p 3389:3389 --name rlplays_rdp rlplays:linux
# Then RDP to: localhost:3389
#   Username: rdpuser   Password: changeme
