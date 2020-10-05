#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

bool computeSHA256Hash(const std::string& unhashed, std::string& hashed);

// void hmac_sha256(
//     const unsigned char *text,      /* pointer to data stream        */
//     int                 text_len,   /* length of data stream         */
//     const unsigned char *key,       /* pointer to authentication key */
//     int                 key_len,    /* length of authentication key  */
//     void                *digest);    /* caller digest to be filled in */


// std::string get_signature_key(std::string secret_key, std::string key, std::string bucket_name,
//                     boost::posix_time::ptime date,
//                     std::string object_path, std::string http_verb);