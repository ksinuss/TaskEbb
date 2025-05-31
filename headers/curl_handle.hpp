#ifndef CURL_HANDLE_HPP
#define CURL_HANDLE_HPP

#include <curl/curl.h>
#include <stdexcept>

class CurlHandle {
public:
    CurlHandle();

    ~CurlHandle();

    CurlHandle(const CurlHandle&) = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;

    CurlHandle(CurlHandle&& other) noexcept;
    CurlHandle& operator=(CurlHandle&& other) noexcept;

    operator CURL*() const noexcept;

private:
    void cleanup() noexcept;
    
    CURL* handle_;
};

#endif