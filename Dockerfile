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

FROM centos:centos7

RUN yum groupinstall -y "Development Tools" && \
     yum update -y && \
     yum install -y epel-release && \
     yum install -y zlib-devel && \
     yum install -y *protobuf* && \
     yum install -y openssl* && \
     yum install -y centos-release-scl && \
     yum install -y devtoolset-9-gcc* && \
     yum install -y unixODBC-devel && \
     yum install -y ninja-build && \
     yum install -y rapidjson-devel && \
     yum install -y cmake3 && \
     ln -s /usr/bin/cmake3 /usr/bin/cmake

SHELL [ "/usr/bin/scl", "enable", "devtoolset-9" ]

RUN cd /tmp && \
    git clone -b v1.36.4 https://github.com/grpc/grpc && \
    cd grpc && \
    git submodule update --init && \
    mkdir -p cmake-build && cd cmake-build && \
    cmake .. && make && make install && \
    rm -rf /tmp/grpc

RUN rm /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a && \
    ln -s /usr/lib64/libssl.a /usr/local/lib/libssl.a && \
    ln -s /usr/lib64/libcrypto.a /usr/local/lib/libcrypto.a

RUN yum install -y wget && \
    wget https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz && \
    tar -xzf boost_1_76_0.tar.gz && \
    cd boost_1_76_0 && \
    ./bootstrap.sh --prefix=/opt/boost && \
    ./b2 install --prefix=/opt/boost --with=all || true

RUN echo $'[ODBC] \n\
Trace=yes \n\
TraceFile=/tmp/odbctrace.log \n\

[ODBC Drivers] \n\
FlightSQL = Installed \n\
FlightSQL-Debug = Installed \n\
Dremio ODBC Driver 64-bit = Installed \n\

[FlightSQL] \n\
Description=FlightSQL Driver \n\
Driver=/opt/warpdrive/_build/release/libarrow-odbc.so \n\
FileUsage=1 \n\
UsageCount=1 \n\

[FlightSQL-Debug] \n\
Description=FlightSQL Driver \n\
Driver=/opt/warpdrive/_build/debug/libarrow-odbc.so \n\
FileUsage=1 \n\
UsageCount=1 \n\
' > /etc/odbcinst.ini

RUN echo $'[ODBC Data Sources] \n\
FlightSQL       = FlightSQL Driver \n\

[FlightSQL] \n\
Description     = FlightSQL Driver \n\
Driver          = FlightSQL \n\
host            = host.docker.internal \n\
port            = 32010 \n\
user            = dremio \n\
password        = dremio123 \n\
' > /root/.odbc.ini

# Enabled gcc 9 as default on bash
RUN echo 'source scl_source enable devtoolset-9' >> ~/.bashrc

WORKDIR /opt/warpdrive
