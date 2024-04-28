To use this Dockerfile, follow these steps:

1. Save the Dockerfile in the same directory as your firmware code (`face_tracking.ino`).

2. Open a terminal and navigate to the directory containing the Dockerfile and firmware code.

3. Build the Docker image by running the following command:
   ```
   docker build -t esp32-cam-firmware-arduino .
   ```

4. Once the image is built, you can run a container based on that image to build the firmware:
   ```
   docker run --rm -v $(pwd)/output:/app/output esp32-cam-firmware-arduino
   ```

   This command mounts the `output` directory from the host machine to the `/app/output` directory inside the container, allowing you to access the generated firmware binary.

5. After the container finishes running, you will find the `firmware.bin` file in the `output` directory on your host machine.

6. You can then flash the `firmware.bin` to your AI Thinker ESP32-Cam board using the appropriate flashing tool, such as `esptool.py`.
