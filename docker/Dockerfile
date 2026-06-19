# Used Copilot/Claude to generate this Dockerfile
FROM nvidia/cuda:12.8.0-devel-ubuntu22.04

# Avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# CUDA environment
ENV CUDA_HOME=/usr/local/cuda
ENV PATH=${CUDA_HOME}/bin:${PATH}
ENV LD_LIBRARY_PATH=${CUDA_HOME}/lib64:${LD_LIBRARY_PATH}

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
    python3 \
    python3-pip \
    python3-dev \
    python3-venv \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Make python3 the default python
RUN ln -sf /usr/bin/python3 /usr/local/bin/python \
    && pip3 install --upgrade pip


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
# Change 'rlplays' and 'c' to your preferred username and password
RUN useradd -m -s /bin/bash rlplays \
    && echo "rlplays:c" | chpasswd \
    && echo "startxfce4" > /home/rlplays/.xsession \
    && chown rlplays:rlplays /home/rlplays/.xsession

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
# docker run -d -p 3399:3389 -p 2222:22 --gpus all --name rlplays_rdp rlplays:linux
# Ensure it runs by checking: docker ps
# Then RDP to: localhost:3399  (Username: rlplays  Password: c)
# Then SSH:    ssh rlplays@localhost -p 2222 (same username / password)

# To rebuild if you change this file:
# To clean first: docker rm -f rlplays_rdp 
#  and also: docker rmi rlplays:linux
# docker build -t rlplays:linux .
# Then run using the `docker run` command above.

