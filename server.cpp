#include <iostream>
#include <string>
#include <unordered_map>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class VotingServer {
public:
    VotingServer(int port) : port_(port) {}

    void start() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return;
        }

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port_);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        std::cout << "Server listening on port " << port_ << std::endl;

        while (true) {
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                continue;
            }

            std::cout << "Client connected" << std::endl;
            handleClient(clientSocket);
        }

        closesocket(serverSocket);
        WSACleanup();
    }

private:
    int port_;
    std::unordered_map<std::string, int> voteCount_;

    void handleClient(SOCKET clientSocket) {
        char buffer[1024];
        int bytesReceived;

        // Receive the HTTP request
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::cout << "Received " << bytesReceived << " bytes" << std::endl;
            std::string request(buffer, bytesReceived);
            std::cout << "Request: " << request << std::endl;

            // Parse the request (very basic parsing)
            if (request.find("POST") == 0) {
                // Extract the vote from the request body
                size_t bodyStart = request.find("\r\n\r\n");
                if (bodyStart != std::string::npos) {
                    std::string vote = request.substr(bodyStart + 4);
                    updateVoteCount(vote);
                }
            }

            // Send response
            std::string results = getVoteResults();
            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                                   "Access-Control-Allow-Headers: Content-Type\r\n"
                                   "\r\n" +
                                   results;

            send(clientSocket, response.c_str(), response.length(), 0);
            std::cout << "Sent response: " << results << std::endl;
        }

        closesocket(clientSocket);
        std::cout << "Client disconnected" << std::endl;
    }

    void updateVoteCount(const std::string& option) {
        voteCount_[option]++;
        std::cout << "Vote received for: " << option << std::endl;
    }

    std::string getVoteResults() {
        std::string results = "Vote Results:";
        for (const auto& pair : voteCount_) {
            results += " " + pair.first + ": " + std::to_string(pair.second) + ";";
        }
        return results;
    }
};

int main() {
    VotingServer server(8080);
    server.start();
    return 0;
}