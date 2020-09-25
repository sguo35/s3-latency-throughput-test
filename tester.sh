#!/bin/bash
yum -y update
yum install -y git g++ libcurl-devel openssl-devel libuuid-devel pulseaudio-libs-devel cmake
cd ~
git clone https://github.com/aws/aws-sdk-cpp.git
cd aws-sdk-cpp
cmake . -D CMAKE_BUILD_TYPE=Release -D BUILD_ONLY="s3"
make
make install
cd ~
git clone git@github.com:sguo35/s3-latency-throughput-test.git
cd s3-latency-throughput-test/src
bash build-cmake.sh && make && AWS_PROFILE=rise ./s3-test