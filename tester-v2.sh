#!/bin/bash
sudo yum -y update
sudo yum group install -y "Development Tools"
curl -sL https://rpm.nodesource.com/setup_12.x | sudo bash -
sudo yum install -y git libcurl-devel openssl-devel libuuid-devel pulseaudio-libs-devel cmake3 libssl-dev nodejs
wget https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.bz2
tar --bzip2 -xf boost_1_73_0.tar.bz2
cd boost_1_73_0 && sudo chmod +x bootstrap.sh && sudo ./bootstrap.sh && sudo ./b2 install
cd ~ && git clone https://github.com/sguo35/s3-latency-throughput-test.git
cd ~/s3-latency-throughput-test/async-client && bash build-cmake.sh && make