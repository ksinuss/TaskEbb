#include "curl_handle.hpp"

CurlHandle::CurlHandle() : handle_(curl_easy_init()) {
    if (!handle_) {
        throw std::runtime_error("Failed to initialize CURL handle");
    }
}

CurlHandle::~CurlHandle() {
    cleanup();
}

void CurlHandle::cleanup() noexcept {
    if (handle_) {
        curl_easy_cleanup(handle_);
        handle_ = nullptr;
    }
}

CurlHandle::CurlHandle(CurlHandle&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
}

CurlHandle& CurlHandle::operator=(CurlHandle&& other) noexcept {
    if (this != &other) {
        cleanup();
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

CurlHandle::operator CURL*() const noexcept {
    return handle_;
}
