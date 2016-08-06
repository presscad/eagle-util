// GenericHTTPClient.cpp: implementation of the GenericHTTPClient class.
//
//////////////////////////////////////////////////////////////////////
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include "GenericHTTPClient.h"
#include <winreg.h>

#ifdef _DEBUG
static void MyDebug_Trace(LPCTSTR szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);

    TCHAR szBuffer[MAX_PATH * 2] = {0};
    int nBuf = _vstprintf(szBuffer, szFormat, args);

    if (nBuf < (sizeof(szBuffer) / sizeof(szBuffer[0]) - 1))
        OutputDebugString(szBuffer);
    else
        OutputDebugString(szFormat);

    va_end(args);
}

#define TRACE                    ::MyDebug_Trace
#define TRACE0(sz)              ::MyDebug_Trace(_T("%s"), _T(sz))
#define TRACE1(sz, p1)          ::MyDebug_Trace(_T(sz), p1)
#define TRACE2(sz, p1, p2)      ::MyDebug_Trace(_T(sz), p1, p2)
#define TRACE3(sz, p1, p2, p3)  ::MyDebug_Trace(_T(sz), p1, p2, p3)

#else

inline static void MyDebug_Trace(LPCTSTR, ...) { }
#define TRACE              1 ? (void)0 : ::MyDebug_Trace
#define TRACE0(sz)
#define TRACE1(sz, p1)
#define TRACE2(sz, p1, p2)
#define TRACE3(sz, p1, p2, p3)

#endif

GenericHTTPClient::GenericHTTPClient(){
    _hHTTPOpen=_hHTTPConnection=_hHTTPRequest=NULL;
}

GenericHTTPClient::~GenericHTTPClient(){
    _hHTTPOpen=_hHTTPConnection=_hHTTPRequest=NULL;
}

GenericHTTPClient::RequestMethod GenericHTTPClient::GetMethod(int nMethod){	
    return nMethod<=0 ? GenericHTTPClient::RequestGetMethod : static_cast<GenericHTTPClient::RequestMethod>(nMethod);
}

GenericHTTPClient::TypePostArgument GenericHTTPClient::GetPostArgumentType(int nType){
    return nType<=0 ? GenericHTTPClient::TypeNormal : static_cast<GenericHTTPClient::TypePostArgument>(nType);
}

BOOL GenericHTTPClient::Connect(LPCSTR szAddress, LPCSTR szAgent, unsigned short nPort, LPCSTR szUserAccount, LPCSTR szPassword){

    _hHTTPOpen=::InternetOpenA(szAgent,	// agent name
        INTERNET_OPEN_TYPE_PRECONFIG,	// proxy option
        "",								// proxy
        "",								// proxy bypass
        0);					            // flags

    if(!_hHTTPOpen){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    _hHTTPConnection=::InternetConnectA(	_hHTTPOpen,	// internet opened handle
        szAddress, // server name
        nPort, // ports
        szUserAccount, // user name
        szPassword, // password 
        INTERNET_SERVICE_HTTP, // service type
        INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,	// service option																														
        0);	// context call-back option

    if(!_hHTTPConnection){		
        _dwError=::GetLastError();
        ::CloseHandle(_hHTTPOpen);
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    if(::InternetAttemptConnect(NULL)!=ERROR_SUCCESS){		
        _dwError=::GetLastError();
        ::CloseHandle(_hHTTPConnection);
        ::CloseHandle(_hHTTPOpen);
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    return TRUE;															
}

BOOL GenericHTTPClient::Close(){

    if(_hHTTPRequest)
        ::InternetCloseHandle(_hHTTPRequest);

    if(_hHTTPConnection)
        ::InternetCloseHandle(_hHTTPConnection);

    if(_hHTTPOpen)
        ::InternetCloseHandle(_hHTTPOpen);

    return TRUE;
}

BOOL GenericHTTPClient::Request(LPCSTR szURL, int nMethod, LPCSTR szAgent){

    BOOL bReturn=TRUE;
    DWORD dwPort=0;
    std::string szProtocol;
    std::string szAddress;
    std::string szURI;
    DWORD dwResSize=0;

    ParseURL(szURL, szProtocol, szAddress, dwPort, szURI);

    if(Connect(szAddress.c_str(), szAgent, (unsigned short)dwPort)){
        if(!RequestOfURI(szURI.c_str(), nMethod)){
            bReturn=FALSE;
        }else{
            if(!Response(_szHTTPResponseHeader, _szHTTPResponseHTML)){
                bReturn=FALSE;
            }
        }
        Close();
    }else{
        bReturn=FALSE;
    }

    return bReturn;
}

BOOL GenericHTTPClient::RequestOfURI(LPCSTR szURI, int nMethod){

    BOOL bReturn=TRUE;

//  try{
        switch(nMethod){
        case	GenericHTTPClient::RequestGetMethod:
        default:
            bReturn=RequestGet(szURI);
            break;
        case	GenericHTTPClient::RequestPostMethod:		
            bReturn=RequestPost(szURI);
            break;
        case	GenericHTTPClient::RequestPostMethodMultiPartsFormData:
            bReturn=RequestPostMultiPartsFormData(szURI);
            break;
        }
/*
    }catch(CException *e){
#ifdef	_DEBUG
        TRACE(_T("\nEXCEPTION\n"));
        TCHAR szERROR[1024];
        e->GetErrorMessage(szERROR, 1024);
        TRACE(szERROR);
#endif
    }
*/

    return bReturn;
}

BOOL GenericHTTPClient::RequestGet(LPCSTR szURI){

    LPCSTR szAcceptTypes[] ={__HTTP_ACCEPT_TYPE, NULL};

    _hHTTPRequest=::HttpOpenRequestA(_hHTTPConnection,
        __HTTP_VERB_GET, // HTTP Verb
        szURI, // Object Name
        HTTP_VERSIONA, // Version
        "", // Reference
        szAcceptTypes, // Accept Type
        INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,
        0); // context call-back point

    if(!_hHTTPRequest){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    // REPLACE HEADER
    if(!::HttpAddRequestHeadersA( _hHTTPRequest, __HTTP_ACCEPT, strlen(__HTTP_ACCEPT), HTTP_ADDREQ_FLAG_REPLACE)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    // SEND REQUEST
    if(!::HttpSendRequest( _hHTTPRequest,	// handle by returned HttpOpenRequest
        NULL, // additional HTTP header
        0, // additional HTTP header length
        NULL, // additional data in HTTP Post or HTTP Put
        0) // additional data length
        ){
            _dwError=::GetLastError();
#ifdef	_DEBUG
            LPVOID     lpMsgBuffer;
            DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                ::GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPTSTR>(&lpMsgBuffer),
                0,
                NULL);
            OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
            LocalFree(lpMsgBuffer);		
#endif
            return FALSE;
    }

    return TRUE;
}

BOOL GenericHTTPClient::RequestPost(LPCSTR szURI){

    LPCSTR  szAcceptTypes[] = {__HTTP_ACCEPT_TYPE, NULL};
    std::vector<BYTE> postArguments;
    LPCSTR szContentType= "Content-Type: application/x-www-form-urlencoded\r\n";

    DWORD dwPostArgumentsLegnth=GetPostArguments(postArguments);

    _hHTTPRequest=::HttpOpenRequestA(_hHTTPConnection,
        __HTTP_VERB_POST, // HTTP Verb
        szURI, // Object Name
        HTTP_VERSIONA, // Version
        "", // Reference
        szAcceptTypes, // Accept Type
        INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT,
        0); // context call-back point

    if(!_hHTTPRequest){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(_T("\n%d : %s\n"), _dwError, reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    // REPLACE HEADER
    if(!::HttpAddRequestHeadersA(_hHTTPRequest, __HTTP_ACCEPT, strlen(__HTTP_ACCEPT), HTTP_ADDREQ_FLAG_REPLACE)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(_T("\n%d : %s\n"), _dwError, reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }


    // SEND REQUEST
    if(!::HttpSendRequestA( _hHTTPRequest,	// handle by returned HttpOpenRequest
        szContentType, // additional HTTP header
        strlen(szContentType), // additional HTTP header length
        (LPVOID)postArguments.data(), // additional data in HTTP Post or HTTP Put
        postArguments.size()) // additional data length
        ){
            _dwError=::GetLastError();
#ifdef	_DEBUG
            LPVOID     lpMsgBuffer;
            DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                ::GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPTSTR>(&lpMsgBuffer),
                0,
                NULL);
            TRACE(_T("\n%d : %s\n"), _dwError, reinterpret_cast<LPTSTR>(lpMsgBuffer));
            LocalFree(lpMsgBuffer);		
#endif
            return FALSE;
    }

    return TRUE;
}

BOOL GenericHTTPClient::RequestPostMultiPartsFormData(LPCSTR szURI){

    CONST CHAR *szAcceptType=__HTTP_ACCEPT_TYPE;	
    LPCSTR szContentType= "Content-Type: multipart/form-data; boundary=--MULTI-PARTS-FORM-DATA-BOUNDARY\r\n";		

    // ALLOCATE POST MULTI-PARTS FORM DATA ARGUMENTS
    PBYTE pPostBuffer=NULL;
    DWORD dwPostBufferLength=AllocMultiPartsFormData(pPostBuffer, "--MULTI-PARTS-FORM-DATA-BOUNDARY");

    _hHTTPRequest=::HttpOpenRequestA(_hHTTPConnection,
        __HTTP_VERB_POST, // HTTP Verb
        szURI, // Object Name
        HTTP_VERSIONA, // Version
        "", // Reference
        &szAcceptType, // Accept Type
        INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT,	// flags
        0); // context call-back point
    if(!_hHTTPRequest){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    // REPLACE HEADER
    if(!::HttpAddRequestHeadersA(_hHTTPRequest, __HTTP_ACCEPT, strlen(__HTTP_ACCEPT), HTTP_ADDREQ_FLAG_REPLACE)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    if(!::HttpAddRequestHeadersA( _hHTTPRequest, szContentType, strlen(szContentType), HTTP_ADDREQ_FLAG_ADD_IF_NEW)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    std::string szContentLength = "Content-Length: %d\r\n";
    if(!::HttpAddRequestHeadersA( _hHTTPRequest, szContentLength.c_str(), szContentLength.length(), HTTP_ADDREQ_FLAG_ADD_IF_NEW)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

#ifdef	_DEBUG
    TCHAR	szHTTPRequest[1024]=_T("");
    DWORD	dwHTTPRequestLength=1024;

    ::ZeroMemory(szHTTPRequest, dwHTTPRequestLength);
    if(!::HttpQueryInfo(_hHTTPRequest, HTTP_QUERY_RAW_HEADERS_CRLF, szHTTPRequest, &dwHTTPRequestLength, NULL)){
        _dwError=::GetLastError();
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(_T("\n%d : %s\n"), ::GetLastError(), reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
        //return FALSE;
    }

    TRACE(szHTTPRequest);
#endif

    // SEND REQUEST WITH HttpSendRequestEx and InternetWriteFile
    INTERNET_BUFFERS InternetBufferIn={0};
    InternetBufferIn.dwStructSize=sizeof(INTERNET_BUFFERS);
    InternetBufferIn.Next=NULL;	

    if(!::HttpSendRequestEx(_hHTTPRequest, &InternetBufferIn, NULL, HSR_INITIATE, 0)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        TRACE(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    DWORD dwOutPostBufferLength=0;
    if(!::InternetWriteFile(_hHTTPRequest, pPostBuffer, dwPostBufferLength, &dwOutPostBufferLength)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    if(!::HttpEndRequest(_hHTTPRequest, NULL, HSR_INITIATE, 0)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    // FREE POST MULTI-PARTS FORM DATA ARGUMENTS
    FreeMultiPartsFormData(pPostBuffer);

    return TRUE;
}

DWORD GenericHTTPClient::ResponseOfBytes(PBYTE pBuffer, DWORD dwSize){

    static DWORD dwBytes=0;

    if(!_hHTTPRequest){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return 0;
    }

    ::ZeroMemory(pBuffer, dwSize);
    if(!::InternetReadFile(	_hHTTPRequest,
        pBuffer,
        dwSize,
        &dwBytes)){
            _dwError=::GetLastError();
#ifdef	_DEBUG
            LPVOID     lpMsgBuffer;
            DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                ::GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPTSTR>(&lpMsgBuffer),
                0,
                NULL);
            OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
            LocalFree(lpMsgBuffer);		
#endif
            return 0;
    }

    return dwBytes;
}

BOOL GenericHTTPClient::Response(std::string &strHeaderBuffer, std::string &strBuffer){

    BYTE pResponseBuffer[2048]="";	
    DWORD dwNumOfBytesToRead=0;

    if(!_hHTTPRequest){
        _dwError=::GetLastError();
#ifdef	_DEBUG		
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }

    strBuffer.clear();
    while((dwNumOfBytesToRead=ResponseOfBytes(pResponseBuffer, sizeof(pResponseBuffer)))!=NULL){
        strBuffer.resize(strBuffer.size() + dwNumOfBytesToRead);
        memcpy((void *)(strBuffer.data() + strBuffer.size() - dwNumOfBytesToRead), pResponseBuffer, dwNumOfBytesToRead);		
    }

    char headerBuffer[1024 * 4];
    DWORD dwHeaderBufferLength = sizeof(headerBuffer);
    if(!::HttpQueryInfoA(_hHTTPRequest, HTTP_QUERY_RAW_HEADERS_CRLF, (LPVOID)headerBuffer, &dwHeaderBufferLength, NULL)){
        _dwError=::GetLastError();
#ifdef	_DEBUG
        LPVOID     lpMsgBuffer;
        DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ::GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&lpMsgBuffer),
            0,
            NULL);
        OutputDebugString(reinterpret_cast<LPTSTR>(lpMsgBuffer));
        LocalFree(lpMsgBuffer);		
#endif
        return FALSE;
    }
    strHeaderBuffer = headerBuffer;

    return !strBuffer.empty();
}

VOID GenericHTTPClient::AddPostArguments(LPCSTR szName, LPCSTR szValue, BOOL bBinary){

    GenericHTTPArgument	arg;

    arg.name = szName;

    int len = strlen(szValue);
    arg.buffer.resize((len + 1) * sizeof(szValue[0]));
    strncpy((char *)arg.buffer.data(), szValue, len);

    if(!bBinary)
        arg.dwType = GenericHTTPClient::TypeNormal;
    else
        arg.dwType = GenericHTTPClient::TypeBinary;	

    _vArguments.push_back(arg);

    return;
}

VOID GenericHTTPClient::AddPostArguments(LPCSTR szName, PBYTE pBuffer, DWORD dwBufferSize, BOOL bBinary) {
    GenericHTTPArgument	arg;

    arg.name = szName;

    arg.buffer.resize(dwBufferSize);
    memcpy(arg.buffer.data(), pBuffer, dwBufferSize);

    if(!bBinary)
        arg.dwType = GenericHTTPClient::TypeNormal;
    else
        arg.dwType = GenericHTTPClient::TypeBinary;	

    _vArguments.push_back(arg);

    return;
}

VOID GenericHTTPClient::AddPostArguments(LPCSTR szName, DWORD nValue){
    CHAR value[1024];

    sprintf(value, "%d", nValue);
    AddPostArguments(szName, value);

    return;
}

DWORD GenericHTTPClient::GetPostArguments(std::vector<BYTE> &postArguments){
    std::vector<GenericHTTPArgument>::iterator itArg;

    postArguments.clear();
    postArguments.reserve(1024 * 8);

    for(itArg=_vArguments.begin(); itArg<_vArguments.end(); ) {
        int oldLen = postArguments.size();
        int newLen = oldLen + itArg->name.length() + 1 + itArg->buffer.size();
        if (itArg+1 < _vArguments.end())
            newLen++;
        postArguments.resize(newLen);

        memcpy(postArguments.data() + oldLen, itArg->name.data(), itArg->name.length()); 
        oldLen += itArg->name.length();

        postArguments[oldLen] = '=';
        oldLen ++;

        memcpy(postArguments.data() + oldLen, itArg->buffer.data(), itArg->buffer.size());
        oldLen += itArg->buffer.size();

        if((++itArg)<_vArguments.end()){
            postArguments[oldLen] = '&';
        }
    }

    return postArguments.size();
}


VOID GenericHTTPClient::InitilizePostArguments(){
    _vArguments.clear();
    return;
}

VOID GenericHTTPClient::FreeMultiPartsFormData(PBYTE &pBuffer){

    if(pBuffer){
        ::LocalFree(pBuffer);
        pBuffer=NULL;
    }

    return;
}

DWORD GenericHTTPClient::AllocMultiPartsFormData(PBYTE &pInBuffer, LPCSTR szBoundary){

    if(pInBuffer){
        ::LocalFree(pInBuffer);
        pInBuffer=NULL;
    }

    pInBuffer=(PBYTE)::LocalAlloc(LPTR, GetMultiPartsFormDataLength());	
    std::vector<GenericHTTPArgument>::iterator itArgv;

    DWORD dwPosition=0;
    DWORD dwBufferSize=0;

    for(itArgv=_vArguments.begin(); itArgv<_vArguments.end(); ++itArgv){

        PBYTE pBuffer=NULL;

        // SET MULTI_PRATS FORM DATA BUFFER
        switch(itArgv->dwType){
        case	GenericHTTPClient::TypeNormal:
            pBuffer=(PBYTE)::LocalAlloc(LPTR, __SIZE_HTTP_HEAD_LINE*4);

            sprintf((char *)pBuffer,							
                "--%s\r\n"
                "Content-Disposition: form-data; name=\"%s\"\r\n"
                "\r\n"
                "%s\r\n",
                szBoundary,
                itArgv->name.c_str(),
                itArgv->buffer.data());

            dwBufferSize=strlen((char *)pBuffer);

            break;
        case	GenericHTTPClient::TypeBinary:
            DWORD	dwNumOfBytesToRead=0;
            DWORD	dwTotalBytes=0;

            HANDLE hFile=::CreateFileA(itArgv->name.c_str(), 
                GENERIC_READ, // desired access
                FILE_SHARE_READ, // share mode
                NULL, // security attribute
                OPEN_EXISTING, // create disposition
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, // flag and attributes
                NULL); // template file handle

            DWORD	dwSize=::GetFileSize(hFile, NULL);

            pBuffer=(PBYTE)::LocalAlloc(LPTR, __SIZE_HTTP_HEAD_LINE*3+dwSize+1);
            BYTE	pBytes[__SIZE_BUFFER+1]="";

            sprintf((char *)pBuffer,
                "--%s\r\n"
                "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                "Content-Type: %s\r\n"
                "\r\n",
                szBoundary,
                itArgv->name.c_str(), itArgv->buffer.data(),
                GetContentType((char *)itArgv->buffer.data())
                );

            DWORD	dwBufPosition=_tcslen((TCHAR*)pBuffer);	

            while(::ReadFile(hFile, pBytes, __SIZE_BUFFER, &dwNumOfBytesToRead, NULL) && dwNumOfBytesToRead>0 && dwTotalBytes<=dwSize){
                ::CopyMemory((pBuffer+dwBufPosition+dwTotalBytes), pBytes, dwNumOfBytesToRead);
                ::ZeroMemory(pBytes, __SIZE_BUFFER+1);
                dwTotalBytes+=dwNumOfBytesToRead;				
            }

            ::CopyMemory((pBuffer+dwBufPosition+dwTotalBytes), _T("\r\n"), _tcslen(_T("\r\n")));
            ::CloseHandle(hFile);

            dwBufferSize=dwBufPosition+dwTotalBytes+_tcslen(_T("\r\n"));
            break;			
        }

        ::CopyMemory((pInBuffer+dwPosition), pBuffer, dwBufferSize);
        dwPosition+=dwBufferSize;

        if(pBuffer){
            ::LocalFree(pBuffer);
            pBuffer=NULL;
        }
    }

    ::CopyMemory((pInBuffer+dwPosition), "--", 2);
    ::CopyMemory((pInBuffer+dwPosition+2), szBoundary, strlen(szBoundary));
    ::CopyMemory((pInBuffer+dwPosition+2+strlen(szBoundary)), "--\r\n", 3);

    return dwPosition+5+strlen(szBoundary);
}

DWORD GenericHTTPClient::GetMultiPartsFormDataLength(){
    std::vector<GenericHTTPArgument>::iterator itArgv;

    DWORD	dwLength=0;

    for(itArgv=_vArguments.begin(); itArgv<_vArguments.end(); ++itArgv){

        switch(itArgv->dwType){
        case	GenericHTTPClient::TypeNormal:
            dwLength+=__SIZE_HTTP_HEAD_LINE*4;
            break;
        case	GenericHTTPClient::TypeBinary:
            HANDLE hFile=::CreateFileA(itArgv->name.c_str(), 
                GENERIC_READ, // desired access
                FILE_SHARE_READ, // share mode
                NULL, // security attribute
                OPEN_EXISTING, // create disposition
                FILE_ATTRIBUTE_NORMAL, // flag and attributes
                NULL); // template file handle

            dwLength+=__SIZE_HTTP_HEAD_LINE*3+::GetFileSize(hFile, NULL);
            ::CloseHandle(hFile);			
            break;
        }

    }

    return dwLength;
}

LPCTSTR GenericHTTPClient::GetContentType(LPCSTR szName){

    static TCHAR	szReturn[1024]=_T("");
    LONG	dwLen=1024;
    DWORD	dwDot=0;

    for(dwDot=strlen(szName);dwDot>0;dwDot--){
        if(!strncmp((szName+dwDot),".", 1))
            break;
    }

    HKEY	hKEY;
    LPTSTR	szWord=(TCHAR *)(szName+dwDot);

    if(RegOpenKeyEx(HKEY_CLASSES_ROOT, szWord, 0, KEY_QUERY_VALUE, &hKEY)==ERROR_SUCCESS){
        if(RegQueryValueEx(hKEY, TEXT("Content Type"), NULL, NULL, (LPBYTE)szReturn, (unsigned long*)&dwLen)!=ERROR_SUCCESS)
            _tcsncpy(szReturn, _T("application/octet-stream"), _tcslen(_T("application/octet-stream")));
        RegCloseKey(hKEY);
    }else{
        _tcsncpy(szReturn, _T("application/octet-stream"), _tcslen(_T("application/octet-stream")));
    }

    return szReturn;
}

DWORD GenericHTTPClient::GetLastError(){

    return _dwError;
}

VOID GenericHTTPClient::ParseURL(LPCSTR szURL, std::string& szProtocol, std::string& szAddress, DWORD &dwPort, std::string& szURI){
    std::string strURL(szURL);
    std::string szPort;
    DWORD dwPosition=0;
    BOOL bFlag=FALSE;

    while(strlen(szURL)>0 && dwPosition<strlen(szURL) && strncmp((szURL+dwPosition), ":", 1))
        ++dwPosition;

    if(!strncmp((szURL+dwPosition+1), "/", 1)){	// is PROTOCOL
        szProtocol = strURL.substr(0, dwPosition);
        bFlag=TRUE;
    }else{	// is HOST 
        szProtocol = "http";
    }

    DWORD dwStartPosition=0;

    if(bFlag){
        dwStartPosition=dwPosition+=3;				
    }else{
        dwStartPosition=dwPosition=0;
    }

    bFlag=FALSE;
    while(strlen(szURL)>0 && dwPosition<strlen(szURL) && strncmp((szURL+dwPosition), "/", 1))
        ++dwPosition;

    DWORD dwFind=dwStartPosition;

    for(;dwFind<=dwPosition;dwFind++){
        if(!strncmp((szURL+dwFind), ":", 1)){ // find PORT
            bFlag=TRUE;
            break;
        }
    }

    if(bFlag){
        char sztmp[1024]= "";
        strncpy(sztmp, (szURL+dwFind+1), dwPosition-dwFind);
        dwPort=atol(sztmp);
        szAddress = strURL.substr(dwStartPosition, dwFind-dwStartPosition);
    }else if(szProtocol.compare("https") == 0){
        dwPort=INTERNET_DEFAULT_HTTPS_PORT;
        szAddress = strURL.substr(dwStartPosition, dwPosition-dwStartPosition);
    }else {
        dwPort=INTERNET_DEFAULT_HTTP_PORT;
        szAddress = strURL.substr(dwStartPosition, dwPosition-dwStartPosition);
    }

    if(dwPosition<strlen(szURL)){ // find URI
        szURI = strURL.substr(dwPosition);
    }else{
        szURI.clear();
    }

    return;
}
