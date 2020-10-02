#include "asio_http/http_client.h"
#include "asio_http/http_request_result.h"

#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
  boost::asio::io_context context;

  asio_http::http_client  client({}, context);

  client.get([](asio_http::http_request_result result) { std::cout << result.get_body_as_string(); }, "www.google.com");

  context.run();
}