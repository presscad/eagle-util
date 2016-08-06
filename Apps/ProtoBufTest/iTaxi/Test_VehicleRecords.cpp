//#include "afxwin.h"
#include "windows.h"
#include <iostream>
#include <fstream>
#include <string>
#include "VehicleRecords.pb.h"
#include "VehicleRecords_Col.h"

using namespace std;
using namespace com::sap::nic::itraffic;

const char POST_URL[] = "http://192.168.124.129:8000/hello";

static void GenerateProtoBuff(VehicleReports &vr, int num) {
    VehicleRecords_Col vrc;
    vrc.GenerateRecords(num);
    vrc.ToProtoBuf(vr);
}

int Test_VehicleRecords() {
    int nCase = 1;
    printf("%d: verify the convertion from auto-generated records to proto buffer\n", nCase++);
    {
        VehicleRecords_Col vr;
        vr.GenerateRecords(2);
        VehicleRecords_Col vr2(vr);

        VehicleReports pvr, pvr2;
        vr.ToProtoBuf(pvr);
        vr2.ToProtoBuf(pvr2);

        string s1 = pvr.DebugString();
        string s2 = pvr2.DebugString();
        assert(s1 == s2);
        printf(s1.c_str());
    }

    printf("%d: verify convertion from auto-generated proto buffer records to VehicleRecords_Col\n", nCase++);
    {
        VehicleRecords_Col t, vr;
        t.GenerateRecords(2);
        VehicleReports pvr, pvr2;
        t.ToProtoBuf(pvr);
        t.Clear();

        vr.FromProtoBuf(pvr);
        vr.ToProtoBuf(pvr2);
        string s1 = pvr.DebugString();
        string s2 = pvr2.DebugString();
        printf(s1.c_str()); printf(s2.c_str());
        assert(s1 == s2);
    }

    return 0;
}

#include "Utils\Net\GenericHTTPClient.h"

int Test_Get()
{
    GenericHTTPClient httpRequest;
    std::string szHead, szHTML;

    if (httpRequest.Request(POST_URL)) {
        szHead = httpRequest.QueryHTTPResponseHeader();
        szHTML = httpRequest.QueryHTTPResponse();
    }
    httpRequest.Close();
    return 0;
}

int Test_Post()
{
    GenericHTTPClient *pClient=new GenericHTTPClient();
    string szResult;
    DWORD dwRseLen = 0;

    VehicleReports items;
    GenerateProtoBuff(items, 1000);
    printf("Number of generated reports: %d\n", items.report_size()); // for debug only
    //printf(vr.DebugString().c_str());
    string strVR = items.SerializeAsString();

    pClient->InitilizePostArguments();
    pClient->AddPostArguments("VehicleReports", (PBYTE)strVR.c_str(), strVR.size(), TRUE);

    if (pClient->Request(POST_URL, GenericHTTPClient::RequestPostMethod)) {
        szResult = pClient->QueryHTTPResponse();
        printf("Response: %s\n", szResult.c_str());
    }
    pClient->Close();

    delete pClient;
    return 0;
}

#include "curl/curl.h"
// refer to http://curl.haxx.se/libcurl/c/simplepost.html
int Test_CurlPost()
{
    VehicleReports items;
    GenerateProtoBuff(items, 10);
    printf("Number of generated reports: %d\n", items.report_size()); // for debug only
    //printf(vr.DebugString().c_str());
    string strVR = "VehicleReports=" + items.SerializeAsString();

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, POST_URL);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strVR.c_str());

        /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
        itself */ 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strVR.size());

        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));

        /* always cleanup */ 
        curl_easy_cleanup(curl);
    }
    return 0;
}

int Test_HttpPostWithVehicleRecords()
{
    //Test_Get();
    //Test_Post();
    Test_CurlPost();
    return 0;
}
