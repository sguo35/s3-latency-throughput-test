# s3-latency-throughput-test

## Consistency test
This test measures the time to see an object from when the uploader receives confirmation of upload completion.

Testing methodology: 
- We spin up an upload thread which uploads a new 1KB blob with random file name, then spawn new getter threads.
    - Upload thread marks the system time when the call to S3.PutObject() finishes
- spin up new threads every 100us to S3.GetObject(), if request succeds then we mark the system time and main thread stops creating new threads.
- Convergence time is difference between S3.PutObject() return system time and the getter thread's success system time
- Care is taken to ensure correct concurrent writes using atomic CAS operations, so only 1 getter thread (the first one) can write the system time.

A negative value indicates that the getter thread was able to confirm the file's existence before the uploader thread received a confirmation from AWS that the upload had succeeded. This could occur if the uploader experiences a write latency spike, since read latency is ~30ms but write latency can spike above 100+ms.