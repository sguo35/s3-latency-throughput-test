//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP client, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <vector>
#include <stdio.h>

#include "aws_signer.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::chrono::high_resolution_clock::time_point start;

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
class session
{
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;
    http::response_parser<http::string_body> parser;

public:
    session(net::io_context& io_context)
    : resolver_(io_context)
    , stream_(io_context)
    {
	    parser.body_limit(1844674407370955);
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* target,
        int version,
        std::string& auth_header,
        std::vector<std::pair<std::string, std::string>>& other_headers
        )
    {
        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
	    for(int i = 0; i < other_headers.size(); i++)
        {
            req_.set(other_headers[i].first, other_headers[i].second);
        }

        req_.set(http::field::authorization, auth_header);

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                this));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set a timeout on the operation
        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                this));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if(ec)
            return fail(ec, "connect");
        // Send the HTTP request to the remote host
        http::async_write(stream_, req_,
            beast::bind_front_handler(
                &session::on_write,
                this));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Receive the HTTP response
        http::async_read(stream_, buffer_, parser,
            beast::bind_front_handler(
                &session::on_read,
                this));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
	    // printf("%d bytes transferred ", bytes_transferred);
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Write the message to standard out
        //std::cout << res_ << std::endl;

        // Set a timeout on the operation
        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    }
};

void* runner(void* param) {
    net::io_context* io = (net::io_context*) param;
    io->run();
}

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc != 5 && argc != 6)
    {
        std::cerr <<
            "Usage: http-client-async <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
            "Example:\n" <<
            "    http-client-async www.example.com 443 /\n" <<
            "    http-client-async www.example.com 443 / 1.0\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];
    int version = argc == 6 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    char* access_key_cstr = getenv("AWS_ACCESS_KEY");
    char* secret_key_cstr = getenv("AWS_SECRET_KEY");

    std::string access_key(access_key_cstr);
    std::string secret_key(secret_key_cstr);
    boost::posix_time::ptime t = boost::posix_time::second_clock::universal_time();
    std::string host_str(host);
    std::string path(target);
    std::string http_verb("GET");
    char* body = "";
    size_t body_len = 0;
    std::string region("us-west-2");
    std::string service("s3");
    
    std::string auth_header = get_authorization_header(access_key, secret_key,
                    t, host_str, path, http_verb, body, body_len, region, service);
    auto headers = get_other_headers(t, host_str, path, http_verb, body, body_len);

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    // The session is constructed with a strand to
    // ensure that handlers do not execute concurrently.
    
    
    // start timing
    int num_connects = atoi(argv[5]);
    start = std::chrono::high_resolution_clock::now();
    std::cout.setstate(std::ios_base::badbit);

    for (int i = 0; i < num_connects; i++) {
        // prevent session from being destructed before io_service starts
        // okay to leak memory since this is a short benchmark
        session* s = new session(ioc);
        s->run(host, port, target, version, auth_header, headers);
    }

    // Run the I/O service. The call will return when
    // the get operation is complete.
    for (int i = 0; i < 0; i++) {
	pthread_t thread;
        pthread_create(&thread, NULL, runner, (void*) &ioc);
	pthread_detach(thread);
    }
    ioc.run();

    auto end = std::chrono::high_resolution_clock::now(); 

    double time_taken =  
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); 
    std::cout.clear();
    std::cout << "Took " << time_taken * 1e-6 << " ms " <<
        time_taken * 1e-9 << " sec. " << std::endl;

    return EXIT_SUCCESS;
}
