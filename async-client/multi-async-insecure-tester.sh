for i in {1..4};
do
	./async_client_insecure latency-throughput-test.s3-us-west-2.amazonaws.com 80 /test-file.txt 1.1 $1 &
done

