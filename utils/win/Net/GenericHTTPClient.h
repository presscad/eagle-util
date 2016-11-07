/*
 * $ Generic HTTP Client
 * ----------------------------------------------------------------------------------------------------------------
 *
 * name :          GenericHTTPClient
 *
 * version tag :     0.1.0
 *
 * description :    HTTP Client using WININET
 *
 * author :          Heo Yongsun ( gooshin@opentown.net )
 *
 * This code distributed under BSD LICENSE STYLE.
 */

#ifndef __GENERIC_HTTP_CLIENT
#define __GENERIC_HTTP_CLIENT

//#include <afxwin.h>
#include <windows.h>
#include <tchar.h>
#include <wininet.h>

// use stl
#include <vector>

// PRE-DEFINED CONSTANTS
#define __DEFAULT_AGENT_NAME	"MERONG(0.9/;p)"

// PRE-DEFINED BUFFER SIZE
//#define	__SIZE_HTTP_ARGUMENT_NAME	256
//#define __SIZE_HTTP_ARGUMENT_VALUE	1024

#define __HTTP_VERB_GET	"GET"
#define __HTTP_VERB_POST "POST"
#define __HTTP_ACCEPT_TYPE "*/*"
#define __HTTP_ACCEPT "Accept: */*\r\n"
//#define __SIZE_HTTP_BUFFER	100000
//#define __SIZE_HTTP_RESPONSE_BUFFER	100000
#define __SIZE_HTTP_HEAD_LINE	2048

#define __SIZE_BUFFER	1024
//#define __SIZE_SMALL_BUFFER	256

class GenericHTTPClient {
public:					
    typedef struct __GENERIC_HTTP_ARGUMENT{							// ARGUMENTS STRUCTURE
        std::string         name;
        std::vector<BYTE>   buffer;
        DWORD               dwType;
        __GENERIC_HTTP_ARGUMENT() {
            dwType = TypeUnknown;
        }
        BOOL operator==(const __GENERIC_HTTP_ARGUMENT &argV){
            return !name.compare(argV.name) && (buffer == argV.buffer);
        }
    } GenericHTTPArgument;

    enum RequestMethod{															// REQUEST METHOD
        RequestUnknown=0,
        RequestGetMethod=1,
        RequestPostMethod=2,
        RequestPostMethodMultiPartsFormData=3
    };

    enum TypePostArgument{													// POST TYPE 
        TypeUnknown=0,
        TypeNormal=1,
        TypeBinary=2
    };

    // CONSTRUCTOR & DESTRUCTOR
    GenericHTTPClient();
    virtual ~GenericHTTPClient();

    static GenericHTTPClient::RequestMethod GetMethod(int nMethod);
    static GenericHTTPClient::TypePostArgument GetPostArgumentType(int nType);

    // Connection handler	
    BOOL Connect(LPCSTR szAddress, LPCSTR szAgent = __DEFAULT_AGENT_NAME, unsigned short nPort = INTERNET_DEFAULT_HTTP_PORT, LPCSTR szUserAccount = NULL, LPCSTR szPassword = NULL);
    BOOL Close();
    VOID InitilizePostArguments();

    // HTTP Arguments handler	
    VOID AddPostArguments(LPCSTR szName, DWORD nValue);
    VOID AddPostArguments(LPCSTR szName, LPCSTR szValue, BOOL bBinary = FALSE);
    VOID AddPostArguments(LPCSTR szName, PBYTE pBuffer, DWORD dwBufferSize, BOOL bBinary = FALSE);

    // HTTP Method handler 
    BOOL Request(LPCSTR szURL, int nMethod = GenericHTTPClient::RequestGetMethod, LPCSTR szAgent = __DEFAULT_AGENT_NAME);
    BOOL RequestOfURI(LPCSTR szURI, int nMethod = GenericHTTPClient::RequestGetMethod);
    BOOL Response(std::string &strHeaderBuffer, std::string &strBuffer);	
    std::string QueryHTTPResponseHeader(){
        return _szHTTPResponseHeader;
    };
    std::string QueryHTTPResponse() {
        return _szHTTPResponseHTML;
    };

    // General Handler
    DWORD GetLastError();
    LPCTSTR GetContentType(LPCSTR szName);
    VOID ParseURL(LPCSTR szURL, std::string& szProtocol, std::string& szAddress, DWORD &dwPort, std::string& szURI);


protected:				
    std::vector<GenericHTTPArgument> _vArguments;				// POST ARGUMENTS VECTOR

    std::string _szHTTPResponseHTML;                            // RECEIVE HTTP BODY
    std::string _szHTTPResponseHeader;                          // RECEIVE HTTP HEADR

    HINTERNET _hHTTPOpen;			// internet open handle
    HINTERNET _hHTTPConnection;		// internet connection hadle
    HINTERNET _hHTTPRequest;		// internet request hadle

    DWORD		_dwError;			// LAST ERROR CODE
    LPCSTR		_szHost;			// HOST NAME
    DWORD		_dwPort;			// PORT

    // HTTP Method handler
    DWORD ResponseOfBytes(PBYTE pBuffer, DWORD dwSize);
    DWORD GetPostArguments(std::vector<BYTE> &postArguments);
    BOOL RequestPost(LPCSTR szURI);
    BOOL RequestPostMultiPartsFormData(LPCSTR szURI);
    BOOL RequestGet(LPCSTR szURI);
    DWORD AllocMultiPartsFormData(PBYTE &pInBuffer, LPCSTR szBoundary = "--MULTI-PARTS-FORM-DATA-BOUNDARY-");
    VOID FreeMultiPartsFormData(PBYTE &pBuffer);
    DWORD GetMultiPartsFormDataLength();
};

#endif	// #ifndef __GENERIC_HTTP_CLIENT
