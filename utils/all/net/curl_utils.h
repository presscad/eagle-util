#ifndef _CURL_UTILS_H
#define _CURL_UTILS_H

#include <string>
#include <memory>

#define NET_BEGIN_NAMESPACE namespace net {
#define NET_END_NAMESPACE }

NET_BEGIN_NAMESPACE

typedef void CURL_HANDLE;

struct CURL_HANDLE_WRAPPER
{
    CURL_HANDLE_WRAPPER() = delete;
    explicit CURL_HANDLE_WRAPPER(CURL_HANDLE* h)
        : handle(h)
    {}
    ~CURL_HANDLE_WRAPPER();

    CURL_HANDLE* handle;
};
typedef std::shared_ptr<CURL_HANDLE_WRAPPER> CURL_HANDLE_PTR;


void InitCurl();
void CloseCurl();

CURL_HANDLE_PTR GetCurlHandle(const std::string& proxy);
std::string CurlEncodeUrl(CURL_HANDLE_PTR handle, const std::string& url);
bool CurlSetMethod(CURL_HANDLE_PTR handle, const std::string& method);
bool CurlSetUrl(CURL_HANDLE_PTR handle, const std::string& url);
bool CurlAppendHeader(CURL_HANDLE_PTR handle, const std::string& header);
bool CurlAppendPostField(CURL_HANDLE_PTR handle, const std::string& post_field);
std::string SendRequestAndReceive(CURL_HANDLE_PTR handle, const std::string& url = std::string(),
    size_t result_reserved_size = 8 * 1024);

NET_END_NAMESPACE

#endif // _CURL_UTILS_H
