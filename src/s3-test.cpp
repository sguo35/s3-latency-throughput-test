#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <chrono>
#include <thread>
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

    if (!region.empty())
    {
        config.region = region;
    }

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
        std::cout << temp_buffer[0] << std::endl;

        free(temp_buffer);

        return time_taken;
    }
    else
    {
        free(temp_buffer);
        auto err = get_object_outcome.GetError();
        std::cout << "Error: GetObject: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

        return -1;
    }
}

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string random_string( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

// helper function to upload file in new thread
void UploadRandomFile(const Aws::String bucketName, 
    Aws::String objectName,
    const Aws::String region,
    char* garbage_data,
    size_t BUFFER_SIZE,
    std::chrono::high_resolution_clock::time_point start_pt,
    std::atomic<double>* time_insert_finished) {
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
    auto sstream = std::make_shared<std::stringstream>();
    sstream->write(garbage_data, BUFFER_SIZE);
    request.SetBody(sstream);


    // start timing the request here
    Aws::S3::Model::PutObjectOutcome outcome = 
        s3_client.PutObject(request);

    if (outcome.IsSuccess()) {
        // outcome must be successful, stop timer
        auto now = std::chrono::high_resolution_clock::now();
        double curr_time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_pt).count();
        double max_val = std::numeric_limits<double>::max();
        time_insert_finished->compare_exchange_strong(
            max_val,
            curr_time,
            std::memory_order_seq_cst,
            std::memory_order_seq_cst
        );
    }
    else 
    {
        std::cout << "Error: PutObject: " << 
            outcome.GetError().GetMessage() << std::endl;
    }
}

void CheckFileExists(const Aws::String objectKey,
    const Aws::String fromBucket, const Aws::String region,
    std::chrono::high_resolution_clock::time_point start_pt,
    std::atomic<double>* time_object_exists) {
    Aws::Client::ClientConfiguration config("rise");

    if (!region.empty())
    {
        config.region = region;
    }

    Aws::S3::S3Client s3_client(config);
    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket(fromBucket);
    object_request.SetKey(objectKey);

    Aws::S3::Model::GetObjectOutcome get_object_outcome = 
        s3_client.GetObject(object_request);

    if (get_object_outcome.IsSuccess())
    {
        double curr_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start_pt
            ).count();
        double max_val = std::numeric_limits<double>::max();
        // first object to finish gets to set the value
        time_object_exists->compare_exchange_strong(
            max_val,
            curr_time
        );
    }
    else
    {
        auto err = get_object_outcome.GetError();
        std::cout << "Error: GetObject: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
}

void TestConsistencyTime(const Aws::String bucket_name,
                        const Aws::String region,
                        size_t object_size,
                        std::atomic<double>* time_object_exists,
                        std::atomic<double>* time_insert_finished) {
    // generate a random filename to use
    std::string filename = random_string(10);
    Aws::String aws_filename(filename.c_str(), filename.size());
    // generate garbage to upload
    char* temp_buffer = (char*) malloc(object_size);

    auto start_pt = std::chrono::high_resolution_clock::now();

    // start a new thread with upload
    std::thread uploader_thread(&UploadRandomFile, bucket_name, aws_filename, region, temp_buffer, object_size, start_pt, time_insert_finished);
    uploader_thread.detach();
    // loop every 500us until time_object_exists is no longer max value
    // this is okay to not lock on since we just care that some thread has written
    // to it
    double exists_time = time_object_exists->load();
    double max_val = std::numeric_limits<double>::max();
    while (exists_time == max_val) {
        std::thread checker_thread(&CheckFileExists, aws_filename, bucket_name, region, start_pt, time_object_exists);
        checker_thread.detach();
        exists_time = time_object_exists->load();
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    free(temp_buffer);
}



int main()
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    {
        const Aws::String bucket_name_unencrypted = "latency-throughput-test";
        const Aws::String bucket_name_encrypted = "latency-throughput-test-encrypted";
        const Aws::String object_name = "test-file.txt";
        const Aws::String region = "us-west-2";

        std::atomic<double> time_object_exists;
        time_object_exists.store((double) std::numeric_limits<double>::max());
        std::atomic<double> time_insert_finished;
        time_insert_finished.store((double) std::numeric_limits<double>::max());

        // test time for 2 requesters to see consistent views of a newly uploaded object
        // we do this by uploading a 1KB object and spinning up threads every 1ms
        // to check if the object exists, and if it does, we atomically set the time value
        for (size_t i = 0; i < 10; i++)
            TestConsistencyTime(bucket_name_unencrypted, region, 1024, &time_object_exists, &time_insert_finished);

        double exists_time = time_object_exists.load();
        double insert_finished_time = time_insert_finished.load();

        std::cout << "Consistency time was " << 1e-3 * (exists_time - insert_finished_time) << std::endl;

        // 1 KB, 1MB, 1 GB
        std::vector<size_t> buffer_sizes = {1024, 1024*1024, 1024*1024*1024};

        for (size_t i = 0; i < 2; i++) {
            Aws::String bucket_name;
            if (i == 0)
                bucket_name = bucket_name_unencrypted;
            else
                bucket_name = bucket_name_encrypted;

            std::cout << "Testing " << bucket_name << std::endl;

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

    }
    Aws::ShutdownAPI(options);

    return 0;
}