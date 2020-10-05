#include <openssl/x509.h>
#include <openssl/hmac.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <openssl/evp.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/algorithm/hex.hpp>


const size_t HMAC_DIGEST_SIZE = 32;

// https://stackoverflow.com/a/40155962
bool computeSHA256Hash(const std::string& unhashed, std::string& hashed)
{
    bool success = false;

    EVP_MD_CTX* context = EVP_MD_CTX_create();

    if(context != NULL)
    {
        if(EVP_DigestInit_ex(context, EVP_sha256(), NULL))
        {
            if(EVP_DigestUpdate(context, unhashed.c_str(), unhashed.length()))
            {
                unsigned char hash[EVP_MAX_MD_SIZE];
                unsigned int lengthOfHash = 0;

                if(EVP_DigestFinal_ex(context, hash, &lengthOfHash))
                {
                    std::stringstream ss;
                    for(unsigned int i = 0; i < lengthOfHash; ++i)
                    {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
                    }

                    hashed = ss.str();
                    success = true;
                }
            }
        }

        EVP_MD_CTX_destroy(context);
    }

    return success;
}
// https://stackoverflow.com/a/35599923
// Convert string of chars to its representative string of hex numbers
void stream2hex(const std::string str, std::string& hexstr, bool capital = false)
{
    hexstr.resize(str.size() * 2);
    const size_t a = capital ? 'A' - 1 : 'a' - 1;

    for (size_t i = 0, c = str[0] & 0xFF; i < hexstr.size(); c = str[i / 2] & 0xFF)
    {
        hexstr[i++] = c > 0x9F ? (c / 16 - 9) | a : c / 16 | '0';
        hexstr[i++] = (c & 0xF) > 9 ? (c % 16 - 9) | a : c % 16 | '0';
    }
}

// Convert string of hex numbers to its equivalent char-stream
void hex2stream(const std::string hexstr, std::string& str)
{
    str.resize((hexstr.size() + 1) / 2);

    for (size_t i = 0, j = 0; i < str.size(); i++, j++)
    {
        str[i] = (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) << 4, j++;
        str[i] |= (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) & 0xF;
    }
}


// https://gist.github.com/tsupo/112188/3fe993ca2f05cba75b139bef6472b4503fb27a2d
void
hmac_sha256(
    const unsigned char *key,       /* pointer to authentication key */
    int                 key_len,    /* length of authentication key  */
    const unsigned char *text,      /* pointer to data stream        */
    int                 text_len,   /* length of data stream         */
    void                *digest)    /* caller digest to be filled in */
{
    unsigned char k_ipad[65];   /* inner padding -
                                 * key XORd with ipad
                                 */
    unsigned char k_opad[65];   /* outer padding -
                                 * key XORd with opad
                                 */
    unsigned char tk[SHA256_DIGEST_LENGTH];
    unsigned char tk2[SHA256_DIGEST_LENGTH];
    unsigned char bufferIn[1024];
    unsigned char bufferOut[1024];
    int           i;

    /* if key is longer than 64 bytes reset it to key=sha256(key) */
    if ( key_len > 64 ) {
        SHA256( key, key_len, tk );
        key     = tk;
        key_len = SHA256_DIGEST_LENGTH;
    }

    /*
     * the HMAC_SHA256 transform looks like:
     *
     * SHA256(K XOR opad, SHA256(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* start out by storing key in pads */
    memset( k_ipad, 0, sizeof k_ipad );
    memset( k_opad, 0, sizeof k_opad );
    memcpy( k_ipad, key, key_len );
    memcpy( k_opad, key, key_len );

    /* XOR key with ipad and opad values */
    for ( i = 0; i < 64; i++ ) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /*
     * perform inner SHA256
     */
    memset( bufferIn, 0x00, 1024 );
    memcpy( bufferIn, k_ipad, 64 );
    memcpy( bufferIn + 64, text, text_len );

    SHA256( bufferIn, 64 + text_len, tk2 );

    /*
     * perform outer SHA256
     */
    memset( bufferOut, 0x00, 1024 );
    memcpy( bufferOut, k_opad, 64 );
    memcpy( bufferOut + 64, tk2, SHA256_DIGEST_LENGTH );

    SHA256( bufferOut, 64 + SHA256_DIGEST_LENGTH, (unsigned char*) digest );
}

const std::string NEWLINE("\n");
const std::string SLASH_DELIM("/");
const std::string AWS_HMAC_STR_PREFIX("AWS4-HMAC-SHA256");
const std::string AWS_STR_TO_SIGN_SUFFIX("/aws4_request");
const std::string AWS_STR_SECRET_SIGNING_PREFIX("AWS4");
const std::string AWS_SIGNING_KEY_SUFFIX("aws4_request");
const std::string ISO8601_ANY_TIME_ZONE_SUFFIX("Z");

std::string calculate_signature(std::string secret_key, boost::posix_time::ptime time, 
                                std::string region, std::string service, std::string string_to_sign) {
                                    
    std::string date_key_hmac_key = AWS_STR_SECRET_SIGNING_PREFIX + secret_key;
    std::string date_key_hmac_data = boost::posix_time::to_iso_string(time).substr(0, 8);

    // calculate DateKey
    const unsigned char* date_key_key = (unsigned char*) date_key_hmac_key.c_str();
    const unsigned char* date_key_text = (unsigned char*) date_key_hmac_data.c_str();
    unsigned char date_key_digest[HMAC_DIGEST_SIZE];

    hmac_sha256(date_key_key, date_key_hmac_key.length(), date_key_text, date_key_hmac_data.length(), date_key_digest);

    // calculate DateRegionKey
    const unsigned char* date_region_key_text = (unsigned char*) region.c_str();
    unsigned char date_region_key_digest[HMAC_DIGEST_SIZE];

    hmac_sha256(date_key_digest, HMAC_DIGEST_SIZE, date_region_key_text, region.length(), date_region_key_digest);

    // calculate DateRegionServiceKey
    const unsigned char* date_region_service_key_text = (unsigned char*) service.c_str();
    unsigned char date_region_service_key_digest[HMAC_DIGEST_SIZE];

    hmac_sha256(date_region_key_digest, HMAC_DIGEST_SIZE, date_region_service_key_text, service.length(), date_region_service_key_digest);

    // calculate SigningKey
    const unsigned char* signing_key_text = (unsigned char*) AWS_SIGNING_KEY_SUFFIX.c_str();
    unsigned char signing_key_digest[HMAC_DIGEST_SIZE];

    hmac_sha256(date_region_service_key_digest, HMAC_DIGEST_SIZE, signing_key_text, AWS_SIGNING_KEY_SUFFIX.length(), signing_key_digest);

    // final signature
    const unsigned char* string_to_sign_text = (unsigned char*) string_to_sign.c_str();
    unsigned char signature_digest[HMAC_DIGEST_SIZE];

    hmac_sha256(signing_key_digest, HMAC_DIGEST_SIZE, string_to_sign_text, string_to_sign.length(), signature_digest);
    const std::string final_signature((char*) signature_digest, HMAC_DIGEST_SIZE);
    std::string final_signature_hex;

    stream2hex(final_signature, final_signature_hex);

    return final_signature_hex;
}

std::string get_string_to_sign(boost::posix_time::ptime time, std::string region,
                            std::string service, std::string canonical_request) {
    std::string scope = boost::posix_time::to_iso_string(time).substr(0, 8) + SLASH_DELIM + 
                        region + SLASH_DELIM + service + AWS_STR_TO_SIGN_SUFFIX;

    std::string hashed_canonical_request;
    computeSHA256Hash(canonical_request, hashed_canonical_request);

    std::string str_to_sign = AWS_HMAC_STR_PREFIX + NEWLINE + (boost::posix_time::to_iso_string(time) + ISO8601_ANY_TIME_ZONE_SUFFIX)
                            + NEWLINE + scope + NEWLINE + hashed_canonical_request;

    return str_to_sign;
}

const std::string HOST_HEADER("host");
const std::string CONTENT_SHA256_HEADER("x-amz-content-sha256");
const std::string DATE_HEADER("x-amz-date");
const std::string COLON_DELIM(":");
const std::string SEMICOLON_DELIM(";");

std::string get_canonical_request(boost::posix_time::ptime time, std::string host,
                                std::string path,
                                std::string http_verb, char* content, size_t content_length) {
    std::string canonical_query_str("");

    std::string canon_req("");
    canon_req += http_verb + NEWLINE;

    std::string canonical_uri = SLASH_DELIM + path;
    canon_req += canonical_uri + NEWLINE;

    // compute the sha256 hash of the content
    std::string content_str(content, content_length);
    std::string content_hash;
    computeSHA256Hash(content_str, content_hash);

    // not using any query string params
    canon_req += NEWLINE;

    std::string canonical_headers = HOST_HEADER + COLON_DELIM + host + NEWLINE;
    canonical_headers += CONTENT_SHA256_HEADER + COLON_DELIM + content_hash + NEWLINE;
    canonical_headers += DATE_HEADER + COLON_DELIM + (boost::posix_time::to_iso_string(time) + ISO8601_ANY_TIME_ZONE_SUFFIX) + NEWLINE;

    canon_req += canonical_headers;

    std::string signed_headers = HOST_HEADER + SEMICOLON_DELIM + CONTENT_SHA256_HEADER + SEMICOLON_DELIM + DATE_HEADER;
    canon_req += signed_headers + NEWLINE;

    // HashedPayload
    canon_req += content_hash;
}

int main(int, char**)
{
    std::string pw1(""), pw1hashed;

    computeSHA256Hash(pw1, pw1hashed);

    std::cout << pw1hashed << std::endl;

    //boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::ptime t = boost::posix_time::from_iso_string(std::string("20130524T000000Z"));
    std::string region("us-east-1");
    std::string service("s3");

    const char* c_req = ""
R"(GET
/test.txt

host:examplebucket.s3.amazonaws.com
range:bytes=0-9
x-amz-content-sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
x-amz-date:20130524T000000Z

host;range;x-amz-content-sha256;x-amz-date
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855)";
    std::string canonical_request(c_req);

    std::cout << "Original canonical req\n" << canonical_request << "\n" << std::endl;

    std::string generated_c_request = get_canonical_request(t, "examplebucket.s3.amazonaws.com",
                                "test.txt",
                                "GET", "", 0);
    std::cout << "Generated canonical req\n" << generated_c_request << "\n" << std::endl;

    std::string string_to_sign = get_string_to_sign(t, region, service, canonical_request);
    std::cout << string_to_sign << "\n" << std::endl;

    std::string secret_key("wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");

    std::string signature = calculate_signature(secret_key, t, 
                                region, service, string_to_sign);
    std::cout << signature << std::endl;
    return 0;
}

// std::string get_signature_key(std::string secret_key, std::string key, std::string bucket_name,
//                     std::string host, char* payload, char* payload_length,
//                     std::string object_path, std::string http_verb) {
//     // boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
//     // boost::posix_time::to_iso_extended_string(date)
//     std::string signed_headers = "host";

//     std::string StringToSign = http_verb + "\n" + "\n" + "\n" + date
//         + "\n" + "/" + bucket_name + "/" + object_path;

//     const char* string_to_sign_char_buf = StringToSign.c_str();
//     const char* secret_char_buf = secret_key.c_str();

//     char digest[HMAC_DIGEST_SIZE];

//     hmac_sha256(string_to_sign_char_buf, StringToSign.length(), secret_char_buf, secret_key.length(), digest);

//     // base64 encode the result
//     size_t base64_encoded_size = boost::beast::detail::base64::encoded_size(HMAC_DIGEST_SIZE);
//     char base64_encoded_signature[base64_encoded_size];
//     boost::beast::detail::base64::encode(base64_encoded_signature, digest, HMAC_DIGEST_SIZE);

//     std::string ret_s(base64_encoded_signature, base64_encoded_size);

//     return ret_s;
// }


#ifdef  TEST
#ifdef  _MSC_VER
#define Thread  __declspec( thread )
#else
#define Thread
#endif

void
printdump( const char *buffer, size_t sz )
{
    int             i, c;
    unsigned char   buf[80];

    for ( i = 0; (unsigned)i < sz; i++ ) {
        if ( (i != 0) && (i % 16 == 0) ) {
            buf[16] = NUL;
            fprintf( stderr, "    %s\n", buf );
        }
        if ( i % 16 == 0 )
            fprintf( stderr, "%08x:", &buffer[i] );
        c = buffer[i] & 0xFF;
        if ( (c >= ' ') && (c <= 0x7E) )
            buf[i % 16] = (unsigned char)c;
        else
            buf[i % 16] = '.';
        fprintf( stderr, " %02x", c & 0xFF );
    }
    if ( i % 16 == 0 )
        buf[16] = NUL;
    else {
        buf[i % 16] = NUL;
        for ( i = i % 16; i < 16; i++ )
            fputs( "   ", stderr );
    }
    fprintf( stderr, "    %s\n", buf );
}

static char b[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  /* 0000000000111111111122222222223333333333444444444455555555556666 */
  /* 0123456789012345678901234567890123456789012345678901234567890123 */
char    *
base64( const unsigned char *src, size_t sz )
{
    unsigned char               *pp, *p, *q;
    Thread static unsigned char *qq = NULL;
    size_t                      i, safe = sz;

    if ( qq ) {
        free( qq );
        qq = NULL;
    }
    if ( !src || (sz == 0) )
        return ( NULL );

    if ( (sz % 3) == 1 ) {
        p = (unsigned char *)malloc( sz + 2 );
        if ( !p )
            return ( NULL );
        memcpy( p, src, sz );
        p[sz] = p[sz + 1] = '=';
        sz += 2;
    }
    else if ( (sz % 3) == 2 ) {
        p = (unsigned char *)malloc( sz + 1 );
        if ( !p )
            return ( NULL );
        memcpy( p, src, sz );
        p[sz] = '=';
        sz++;
    }
    else
        p = (unsigned char *)src;

    q = (unsigned char *)malloc( (sz / 3) * 4 + 2 );
    if ( !q ) {
        if ( p != src )
            free( p );
        return ( NULL );
    }

    pp = p;
    qq = q;
    for ( i = 0; i < sz; i += 3 ) {
        q[0] = b[(p[0] & 0xFC) >> 2];
        q[1] = b[((p[0] & 0x03) << 4) | ((p[1] & 0xF0) >> 4)];
        q[2] = b[((p[1] & 0x0F) << 2) | ((p[2] & 0xC0) >> 6)];
        q[3] = b[p[2] & 0x3F];
        p += 3;
        q += 4;
    }
    *q = NUL;
    if ( (safe % 3) == 1 ) {
        *(q - 1) = '=';
        *(q - 2) = '=';
    }
    if ( (safe % 3) == 2 )
        *(q - 1) = '=';

    if ( pp != src )
        free( pp );
    return ( (char *)qq );
}

int
main( int argc, char *argv[] )
{
    // via http://docs.amazonwebservices.com/AWSECommerceService/latest/DG/index.html?rest-signature.html
    // see also:
    //    http://chalow.net/2009-05-09-1.html
    //     http://d.hatena.ne.jp/zorio/20090509/1241886502
    char        *p;
    const char  *key = "1234567890";
    char    req[2048];
    char    message[10240];
    char    digest[BUFSIZ];

    sprintf( req,
             "AWSAccessKeyId=%s&"
             "ItemId=%s&"
             "Operation=%s&"
             "ResponseGroup=%s&"
             "Service=%s&"
             "Timestamp=%s&"
             "Version=%s",
             "00000000000000000000",
             "0679722769",
             "ItemLookup",
             "ItemAttributes%2COffers%2CImages%2CReviews",
             "AWSECommerceService",
             "2009-01-01T12%3A00%3A00Z",
             "2009-01-06" );

    sprintf( message,
             "%s\n"
             "%s\n"
             "%s\n"
             "%s",
             "GET",
             "webservices.amazon.com",
             "/onca/xml",
             req );

    memset( digest, 0x00, BUFSIZ );
    hmac_sha256( message, strlen(message), key, strlen(key), digest );
    printdump( digest, BUFSIZ );

    { int c; fputs( ": ", stderr ); c = getchar(); }
    // 35 a7 1e f9 4d c0 cf 83 a1 37 bb 48 4a a8 2c d6
    // f7 4b 04 70 44 8a 35 9c 05 e0 aa 2f 9c 4d f7 18

    p = base64( digest, SHA256_DIGEST_LENGTH );
    if ( p )
        printf( "RESULT: %s\n", p );
    { int c; fputs( ": ", stderr ); c = getchar(); }
    // RESULT: Nace+U3Az4OhN7tISqgs1vdLBHBEijWcBeCqL5xN9xg=

    /*
        // via http://stackoverflow.com/questions/699041/hmacsha256-in-php-and-c-differ
        string password = "Password";
        string filename = "Filename";
        var hmacsha256 = new HMACSHA256(Encoding.UTF8.GetBytes(password));
        hmacsha256.ComputeHash(Encoding.UTF8.GetBytes(filename));
        foreach(byte test in hmacsha256.Hash){
            Console.Write(test.ToString("X2"));
        }
        // 5FE2AE06FF9828B33FE304545289A3F590BFD948CA9AB731C980379992EF41F1
    */

    key = "Password";
    sprintf( message, "Filename" );
    memset( digest, 0x00, BUFSIZ );
    hmac_sha256( message, strlen(message), key, strlen(key), digest );
    printdump( digest, BUFSIZ );

    { int c; fputs( ": ", stderr ); c = getchar(); }
    // 5f e2 ae 06 ff 98 28 b3 3f e3 04 54 52 89 a3 f5
    // 90 bf d9 48 ca 9a b7 31 c9 80 37 99 92 ef 41 f1

    p = base64( digest, SHA256_DIGEST_LENGTH );
    if ( p )
        printf( "RESULT: %s\n", p );
    { int c; fputs( ": ", stderr ); c = getchar(); }
    // X+KuBv+YKLM/4wRUUomj9ZC/2UjKmrcxyYA3mZLvQfE=
}
#endif
