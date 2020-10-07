for i in {1..4};
do
	AWS_PROFILE=rise node s3-tester.js &
done

