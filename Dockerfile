FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Moscow
RUN apt-get update -y
RUN apt-get install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release \
    wget \
    unzip \
    clang-tidy \
    clang-format
WORKDIR /app
RUN wget -q -O - https://files.viva64.com/etc/pubkey.txt | apt-key add -
RUN wget -O /etc/apt/sources.list.d/viva64.list https://files.viva64.com/etc/viva64.list
RUN apt-get update -y 
ADD pkglist .
RUN apt-get install -y $(cat pkglist)
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
RUN mkdir sdk_build && cd sdk_build && cmake ../aws-sdk-cpp -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="s3" && make && make install
RUN rm -rf ./aws-sdk-cpp
RUN mkdir build && mkdir /output

