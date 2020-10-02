#!/bin/bash
sudo yum -y update
sudo yum group install -y "Development Tools"
sudo yum install -y git libcurl-devel openssl-devel libuuid-devel pulseaudio-libs-devel cmake3 libssl-dev
wget https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.bz2
tar --bzip2 -xf boost_1_73_0.tar.bz2
cd boost_1_73_0 && sudo chmod +x bootstrap.sh && sudo ./bootstrap.sh && sudo ./b2 install
