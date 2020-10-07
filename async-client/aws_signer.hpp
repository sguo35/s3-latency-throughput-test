#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

std::string get_authorization_header(std::string access_key, std::string secret_key, boost::posix_time::ptime t, std::string host,
                                    std::string path, std::string http_verb, char* body, size_t body_len,
                                    std::string region, std::string service);

std::vector<std::pair<std::string, std::string>> get_other_headers(boost::posix_time::ptime time, std::string host, std::string path,
                                                            std::string http_verb, char* content, size_t content_length);