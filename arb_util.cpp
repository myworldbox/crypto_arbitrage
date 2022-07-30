#ifndef ARB_UTIL_H
#define ARB_UTIL_H

#include <iostream>
#include <string>
#include <curl/curl.h>

#include "cryptopp/cryptlib.h"
#include "cryptopp/hmac.h"
#include "cryptopp/modes.h"
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/base64.h"

using namespace std;
using namespace CryptoPP;

namespace
{
    std::size_t callback(
            const char* in,
            std::size_t size,
            std::size_t num,
            std::string* out)
    {
        const std::size_t totalBytes(size * num);
        out->append(in, totalBytes);
        return totalBytes;
    }
}

std::string curl_get(const std::string url) {
    // Response information.
    long http_code;
    CURLcode result;
    const std::unique_ptr<std::string> http_data(new std::string());

    CURL* curl = curl_easy_init();

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    if(ARB_DEBUG){
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    }

    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0);

    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "redshift crypto trading automated trader");

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, http_data.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if(result != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
          throw 20;
    }
    if (http_code == 200) {
        if(ARB_DEBUG){
            std::cout << "\nGot successful response from " << url << std::endl;
            std::cout << "HTTP data was:\n" << *http_data.get() << std::endl;
        }
    } else {
        std::cout << "Received " << http_code << " response code from " << url << endl;
        throw 30;
    }

    return *http_data;
}

//https://stackoverflow.com/questions/236129/the-most-elegant-way-to-iterate-the-words-of-a-string
std::vector<std::string> split(const std::string &text, char sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
  return tokens;
}

//https://stackoverflow.com/questions/39721005/how-to-create-a-hmac-256-using-the-crypto-library#answer-39742465
string hmac_512_sign(string key, string plain) {

    string mac, encoded;
    try {
		HMAC<SHA512 > hmac((byte*)key.c_str(), key.length());

		StringSource(plain, true,
			new HashFilter(hmac,
				new StringSink(mac)
			) // HashFilter
		); // StringSource

	} catch(const CryptoPP::Exception& e) {

		cerr << e.what() << endl;
    }

    encoded.clear();
    StringSource(mac, true,
            new HexEncoder(
                    new StringSink(encoded)
            ) // Base64Encoder
    ); // StringSource
    return encoded;
}

#endif