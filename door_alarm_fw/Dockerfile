# Use the base image
FROM docker.io/espressif/idf:v5.1.2

RUN apt update && apt install -y git python3 cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential xxd

# Set working directory
WORKDIR /

# Define default command
CMD ["bash"]