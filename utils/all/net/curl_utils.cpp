#include "curl_utils.h"
#include <vector>
#include <string>
#include <curl/curl.h>

NET_BEGIN_NAMESPACE

using namespace std;

struct CURL_CORE
{
    CURL *easy_handle{};
    string err;
    string proxy;
    vector<string> http_headers;
    vector<string> post_fields;
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
        throw exception("init libcurl failed");
    }
}

void CloseCurl()
{
    curl_global_cleanup();
}

CURL_HANDLE_PTR GetCurlHandle(const string& proxy)
{
    CURL_HANDLE_PTR p = make_shared<CURL_HANDLE_WRAPPER>(new CURL_CORE());
    CURLcode return_code;
    CURL_CORE& core = *static_cast<CURL_CORE *>(p->handle);

    core.easy_handle = curl_easy_init();
    if (NULL == core.easy_handle) {
        throw exception("get a easy handle failed");
    }

    core.err.resize(CURL_ERROR_SIZE + 1);
    return_code = curl_easy_setopt(core.easy_handle, CURLOPT_ERRORBUFFER, &core.err[0]);
    if (CURLE_OK != return_code) {
        throw exception("CURL set error buffer failed");
    }

#if 0
    return_code = curl_easy_setopt(core.easy_handle, CURLOPT_TIMEOUT, 20L);
    if (CURLE_OK != return_code) {
        throw exception("CURL set CURLOPT_TIMEOUT failed");
    }
#endif

    if (!proxy.empty()) {
        core.proxy = proxy;
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_PROXY, proxy.c_str());
        if (CURLE_OK != return_code) {
            throw exception(core.err.c_str());
        }
    }

    return p;
}

string CurlEncodeUrl(CURL_HANDLE_PTR easy_handle, const string& url)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    char *encoded_url = curl_easy_escape(core.easy_handle, url.c_str(), 0);
    if (NULL == encoded_url) {
        throw exception(core.err.c_str());
    }
    string result(encoded_url);
    curl_free(encoded_url);
    return result;
}

bool CurlSetMethod(CURL_HANDLE_PTR easy_handle, const string & method)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    CURLcode return_code = curl_easy_setopt(core.easy_handle, CURLOPT_CUSTOMREQUEST, method.c_str());
    return CURLE_OK == return_code;
}

bool CurlSetUrl(CURL_HANDLE_PTR easy_handle, const string & url)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    CURLcode return_code = curl_easy_setopt(core.easy_handle, CURLOPT_URL, url.c_str());
    return CURLE_OK == return_code;
}

bool CurlAppendHeader(CURL_HANDLE_PTR easy_handle, const std::string & header)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    core.http_headers.push_back(header);
    return true;
}

bool CurlAppendPostField(CURL_HANDLE_PTR easy_handle, const std::string & post_field)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    core.post_fields.push_back(post_field);
    return true;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    string& result = *(string *)userdata;
    size_t size0 = result.size();
    result.resize(size0 + size * nmemb);
    memcpy(&result[size0], ptr, size * nmemb);
    return size * nmemb;
}

string SendRequestAndReceive(CURL_HANDLE_PTR easy_handle, const string& url,
    size_t result_reserved_size)
{
    CURL_CORE& core = *static_cast<CURL_CORE*>(easy_handle->handle);
    string result;
    CURLcode return_code = CURLE_OK;

    if (!url.empty()) {
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_URL, url.c_str());
    }
    if (CURLE_OK == return_code) {
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_WRITEFUNCTION, write_callback);
    }
    if (CURLE_OK == return_code) {
        if (result_reserved_size > 0) {
            result.reserve(result_reserved_size);
        }
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_WRITEDATA, &result);
    }

    if (CURLE_OK == return_code && !core.http_headers.empty()) {
        struct curl_slist *headers{};
        for (auto& hdr : core.http_headers) {
            headers = curl_slist_append(headers, hdr.c_str());
        }
        return_code = curl_easy_setopt(core.easy_handle, CURLOPT_HTTPHEADER, headers);
        core.http_headers.clear();
    }

    if (CURLE_OK == return_code && !core.post_fields.empty()) {
        for (auto& field : core.post_fields) {
            curl_easy_setopt(core.easy_handle, CURLOPT_POSTFIELDS, field.c_str());
        }
        core.post_fields.clear();
    }

    if (CURLE_OK == return_code) {
        return_code = curl_easy_perform(core.easy_handle);
    }
    if (CURLE_OK != return_code) {
        throw exception(core.err.c_str());
    }

    return result;
}

NET_END_NAMESPACE
