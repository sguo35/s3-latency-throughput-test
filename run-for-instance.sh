#!/bin/bash

# copy AWS credentials to EC2 instance
scp -i $2 -r ~/.aws $1:~/.aws

# build and execute benchmark
ssh -i $2 $1 << EOF
    sudo yum -y update;
    sudo yum group install -y "Development Tools";
    sudo yum install -y git libcurl-devel openssl-devel libuuid-devel pulseaudio-libs-devel cmake3;
    cd ~ && git clone https://github.com/aws/aws-sdk-cpp.git;
    cd ~/aws-sdk-cpp && cmake3 . -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY=s3 && make && sudo make install;
    cd ~ && git clone https://github.com/sguo35/s3-latency-throughput-test.git;
    cd ~/s3-latency-throughput-test/src && bash build-cmake.sh && make;
    AWS_PROFILE=rise ./s3-test > test_results.txt;
EOF

# copy results back
scp -i $2 $1:~/s3-latency-throughput-test/src/test_results.txt ~/s3-latency-throughput-test/src/test_results-$3.txt
