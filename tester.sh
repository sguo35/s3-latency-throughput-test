#!/bin/bash
sudo yum -y update
sudo yum group install -y "Development Tools"
sudo yum install -y git libcurl-devel openssl-devel libuuid-devel pulseaudio-libs-devel cmake3
cd ~ && git clone https://github.com/aws/aws-sdk-cpp.git
cd ~/aws-sdk-cpp && cmake3 . -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY=s3 && make && sudo make install
cd ~ && git clone https://github.com/sguo35/s3-latency-throughput-test.git
cd ~/s3-latency-throughput-test/aws-sdk && bash build-cmake.sh && make
AWS_PROFILE=rise ./s3-test > test_results.txt