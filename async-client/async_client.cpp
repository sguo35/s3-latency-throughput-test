#include "asio_http/http_client.h"
#include "asio_http/http_request_result.h"

#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
  boost::asio::io_context context;

  asio_http::http_client  client({}, context);

  std::string url = "throughput-latency-test.s3.us-west-2.amazonaws.com/test-file.txt";

  client.get([](asio_http::http_request_result result) { std::cout << result.get_body_as_string(); }, url);

  context.run();
}