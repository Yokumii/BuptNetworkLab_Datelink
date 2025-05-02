FROM ubuntu:22.04

RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    apt-get update --fix-missing && \
    apt-get upgrade -y

RUN apt-get install -y \
    build-essential \
    gcc \
    g++ \
    gdb \
    cmake \
    make \
    git \
    vim \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project

CMD ["/bin/bash"]