FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    wget \
    xz-utils \
    make \
    libssl-dev \
    libcurl4-openssl-dev \
    libc6-dev \
    libstdc++-10-dev \
    python3 \
    python3-pip \
    git \
    libhdf5-dev \
    libhdf5-serial-dev \
    libatlas-base-dev \
    libjasper-dev \
    libqtgui4 \
    libqt4-test

# Download and install Arduino IDE
RUN wget https://downloads.arduino.cc/arduino-1.8.19-linux64.tar.xz && \
    tar -xf arduino-1.8.19-linux64.tar.xz && \
    mv arduino-1.8.19 /usr/local/arduino && \
    rm arduino-1.8.19-linux64.tar.xz

# Set up Arduino IDE environment
ENV PATH="/usr/local/arduino:${PATH}"

# Install ESP32 board support
RUN arduino --pref "boardsmanager.additional.urls=https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json" --save-prefs && \
    arduino --install-boards esp32:esp32

# Set the working directory
WORKDIR /app

# Copy the firmware code to the container
COPY . /app

# Compile the firmware
RUN arduino --verify --board esp32:esp32:ai_thinker face_tracking.ino

# Generate the firmware binary
RUN arduino --verify --board esp32:esp32:ai_thinker --pref build.path=./build face_tracking.ino && \
    cp ./build/face_tracking.ino.bin firmware.bin

# Copy the generated binary to a directory for easy access
RUN mkdir -p /app/output && \
    cp firmware.bin /app/output/