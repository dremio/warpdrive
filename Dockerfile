# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive

# TODO: Join steps to avoid multiple layers
RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
        build-essential pkg-config cmake autoconf libtool \
        libboost-filesystem-dev libboost-regex-dev libboost-system-dev \
        libbrotli-dev \
        libbz2-dev \
        libgflags-dev \
        liblz4-dev \
        libprotobuf-dev libprotoc-dev protobuf-compiler \
        libre2-dev \
        libsnappy-dev \
        libthrift-dev \
        libutf8proc-dev \
        libzstd-dev \
        rapidjson-dev \
        zlib1g-dev \
        unixodbc unixodbc-dev \
        git \
        libssl-dev \
        ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists*

# Build gRPC
RUN cd /tmp && \
    git clone -b v1.36.4 https://github.com/grpc/grpc && \
    cd grpc && \
    git submodule update --init && \
    mkdir -p cmake-build && cd cmake-build && \
    cmake .. && make && make install && \
    rm -rf /tmp/grpc

COPY . /opt/warpdrive
WORKDIR /opt/warpdrive

ARG github_access_token
RUN mkdir build && cd build && \
    cmake -DGITHUB_ACCESS_TOKEN=$github_access_token .. && make
