#include <windows.h>  
#include <shldisp.h>  

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{  
    CoInitialize (0);  
    IShellDispatch4 *pdisp = NULL;  
    CoCreateInstance(CLSID_Shell, NULL, CLSCTX_ALL, __uuidof (IShellDispatch4), (void**)&pdisp); 
    pdisp->ToggleDesktop (); //����������л������  
    pdisp->Release();  
    CoUninitialize();  
};
