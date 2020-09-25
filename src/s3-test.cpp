#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <chrono>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

// https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html
double TestPutObject(const Aws::String& bucketName, 
    const Aws::String& objectName,
    const Aws::String& region,
    const size_t BUFFER_SIZE)
{
    Aws::Client::ClientConfiguration config("rise");

    if (!region.empty())
    {
        config.region = region;
    }

    Aws::S3::S3Client s3_client(config);
    
    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucketName);
    request.SetKey(objectName);

    // send garbage data to s3
    char* garbage_data = (char*) malloc(BUFFER_SIZE);
    auto sstream = std::make_shared<std::stringstream>();
    sstream->write(garbage_data, BUFFER_SIZE);
    request.SetBody(sstream);


    // start timing the request here
    auto start = std::chrono::high_resolution_clock::now();
    Aws::S3::Model::PutObjectOutcome outcome = 
        s3_client.PutObject(request);

    if (outcome.IsSuccess()) {
        // outcome must be successful, stop timer
        auto end = std::chrono::high_resolution_clock::now(); 
        free(garbage_data);
        double time_taken =  
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); 

        std::cout << BUFFER_SIZE << "B transfer took " << time_taken * 1e-6 << " ms " <<
           time_taken * 1e-9 << " sec. " << std::endl;
        return time_taken;
    }
    else 
    {
        free(garbage_data);
        std::cout << "Error: PutObject: " << 
            outcome.GetError().GetMessage() << std::endl;
       
        return -1;
    }
}

double TestGetObject(const Aws::String& objectKey,
    const Aws::String& fromBucket, const Aws::String& region,
    const size_t BUFFER_SIZE)
{
    Aws::Client::ClientConfiguration config("rise");
    Aws::S3::S3Client s3_client(config);
    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket(fromBucket);
    object_request.SetKey(objectKey);

    char* temp_buffer = (char*) malloc(BUFFER_SIZE);

    auto start = std::chrono::high_resolution_clock::now();
    Aws::S3::Model::GetObjectOutcome get_object_outcome = 
        s3_client.GetObject(object_request);

    if (get_object_outcome.IsSuccess())
    {
        auto& retrieved_file = get_object_outcome.GetResultWithOwnership().
            GetBody();
        
        retrieved_file.getline(temp_buffer, BUFFER_SIZE);

        auto end = std::chrono::high_resolution_clock::now(); 

        double time_taken =  
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); 

        std::cout << BUFFER_SIZE << "B transfer took " << time_taken * 1e-6 << " ms " <<
           time_taken * 1e-9 << " sec. " << std::endl;

        // prevent gcc from optimizing out the transfer
        std::cout << temp_buffer[BUFFER_SIZE - 1] << std::endl;

        return time_taken;
    }
    else
    {
        auto err = get_object_outcome.GetError();
        std::cout << "Error: GetObject: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

        return -1;
    }
}

int main()
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    {
        const Aws::String bucket_name = "latency-throughput-test";
        const Aws::String object_name = "test-file.txt";
        const Aws::String region = "us-west-2";

        // 1 KB, 1MB, 1 GB
        std::vector<size_t> buffer_sizes = {1024, 1024*1024};//, 1024*1024*1024};
        for (auto iter = buffer_sizes.begin(); iter != buffer_sizes.end(); iter++) {
            std::cout << "-------" << std::endl;

            std::cout << "\n\nTesting write..." << std::endl;
            for (size_t i = 0; i < 10; i++)
                TestPutObject(bucket_name, object_name, region, *iter);

            std::cout << "\n\nTesting read..." << std::endl;
            for (size_t i = 0; i < 10; i++)
                TestGetObject(object_name, bucket_name, region, *iter);
        }

    }
    Aws::ShutdownAPI(options);

    return 0;
}