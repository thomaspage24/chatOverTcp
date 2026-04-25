FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
  g++ \
  cmake \
  libssl-dev \
  figlet \
  lolcat

# create a folder inside the container for our code
WORKDIR /app

# copy everything from our project into /app inside the container
COPY . .

# build the project
RUN cmake -B build -S . && cmake --build build

# default command — can be overridden by docker-compose
CMD ["./build/output/main"]
