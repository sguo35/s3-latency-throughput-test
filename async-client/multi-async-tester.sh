for i in {1..4};
do
	./async_client latency-throughput-test.s3-us-west-2.amazonaws.com 443 /test-file.txt &
done

