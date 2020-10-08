# C++ Async HTTP/S client

Install Boost and run `build-cmake.sh` to generate makefile, then `make` to compile. `async_client.cpp` and `async_client_insecure.cpp` contain code for the Boost Beast/ASIO implementation of an HTTP client and the driver/timer. Code is adapted from Boost example docs. `aws_signer.cpp` contains custom code for generating AWS authentication headers, since apparently there is no library that does this for us that exists. Code has not been cleaned up so there is misc. useless code all over the place.

Export `AWS_ACCESS_KEY` and `AWS_SECRET_KEY` as environment variables to run the client. `multi-async-tester.sh` has an example of how benchmarks are run.

For each connection spawned, a 5MB chunk is downloaded. For 100 connections, 500MB will be downloaded. Each process uses only 1 thread, although you can change the executor to use multiple threads in a single process by changing the `pthread_create loop` in `main`. Multiple threads will still only execute the number of connections specified, i.e. even with 64 threads if only 100 connections are specified, only 500MB will be downloaded. Because additional threads do not appear to provide any scaling benefit (lock contention issues?), we use new processes to test multi-core performance.

Latency testing is driven by the chrono in-built library. Peak throughput is determined by running `nload` and then running the testing script, then noting the max throughput recorded by `nload`.
