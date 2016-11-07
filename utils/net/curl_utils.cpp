#include "curl_utils.h"
#include <exception>
#include <curl/curl.h>

NET_BEGIN_NAMESPACE

struct CURL_CORE
{
    CURL_CORE()
        : easy_handle(nullptr)
    {}

    CURL *easy_handle;
    std::string err;
    std::string proxy;
};

static void CurlCloseHandle(CURL_HANDLE *easy_handle)
{
    if (easy_handle) {
        CURL_CORE* handle = static_cast<CURL_CORE*>(easy_handle);
        curl_easy_cleanup(handle->easy_handle);
        handle->easy_handle = nullptr;
        delete handle;
    }
}

CURL_HANDLE_WRAPPER::~CURL_HANDLE_WRAPPER()
{
    CurlCloseHandle(handle);
    handle = nullptr;
}

void InitCurl()
{
    long flags = 0;
#ifdef _WIN32
    flags |= CURL_GLOBAL_WIN32;
#endif
    CURLcode return_code = curl_global_init(flags);
    if (CURLE_OK != return_code) {
        throw std::exception("init libcurl failed");
    }
}

void CloseCurl()
{
    curl_global_cleanup();
}

CURL_HANDLE_PTR GetCurlHandle(const std::string& proxy)
{
    CURL_HANDLE_PTR p = std::make_shared<CURL_HANDLE_WRAPPER>(new CURL_CORE());
    CURLcode return_code;
    CURL_CORE& core = *static_cast<CURL_CORE *>(p->handle);

    core.easy_handle = curl_easy_init();
    if (NULL == core.easy_handle) {
        throw std::exception("get a easy handle failed");
    }

    core.err.resize(CURL_ERROR_SIZE + 1);
    return_code = curl_easy_setopt(core.easy_handle, CURLOPT_ERRORBUFFER, &core.err[0]);
    if (CURLE_OK != return_code) {
        throw std::exception("CURL set error buffer failed");
    }

#if 0
    return_code = curl_easy_setopt(core.easy_handle, CURLOPT_TIMEOUT, 20L);
    if (CURLE_OK != return_code) {
        throw std::exception("CURL set CURLOPT_TIMEOUT failed");
    }
#endif

    if (!proxy.empty()) {
        core.proxy = proxy;
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_PROXY, proxy.c_str());
        if (CURLE_OK != return_code) {
            throw std::exception(core.err.c_str());
        }
    }

    return p;
}

std::string CurlEncodeUrl(CURL_HANDLE_PTR easy_handle, const std::string& url)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    char *encoded_url = curl_easy_escape(core.easy_handle, url.c_str(), 0);
    if (NULL == encoded_url) {
        throw std::exception(core.err.c_str());
    }
    std::string result(encoded_url);
    curl_free(encoded_url);
    return result;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::string& result = *(std::string *)userdata;
    size_t size0 = result.size();
    result.resize(size0 + size * nmemb);
    memcpy(&result[size0], ptr, size * nmemb);
    return size * nmemb;
}

std::string SendRequestAndReceive(CURL_HANDLE_PTR easy_handle, const std::string& url,
    size_t result_reserved_size)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    std::string result;
    CURLcode return_code;

    return_code = curl_easy_setopt(core.easy_handle, CURLOPT_URL, url.c_str());
    if (CURLE_OK == return_code) {
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_WRITEFUNCTION, write_callback);
    }
    if (CURLE_OK == return_code) {
        if (result_reserved_size > 0) {
            result.reserve(result_reserved_size);
        }
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_WRITEDATA, &result);
    }
    if (CURLE_OK == return_code) {
        return_code = curl_easy_perform(core.easy_handle);
    }
    if (CURLE_OK != return_code) {
        throw std::exception(core.err.c_str());
    }

    return result;
}

NET_END_NAMESPACE
