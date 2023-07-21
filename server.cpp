#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <thread>

const int PORT = 8080;
const int MAX_CLIENTS = 10;
const int BUFFER_SIZE = 1024;

std::vector<int> clientSockets;

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead;

    // Receive the username from the client
    std::string username;
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead <= 0) {
        std::cerr << "Client disconnected." << std::endl;
        close(clientSocket);
        return;
    }
    username = buffer;

    std::cout << "User " << username << " connected." << std::endl;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead <= 0) {
            std::cout << "User " << username << " disconnected." << std::endl;
            break;
        }

        std::string message = buffer;
        if (message == "/exit") {
            std::cout << "User " << username << " requested to exit." << std::endl;
            break;
        }

        // Send the message to all other connected clients
        for (int socket : clientSockets) {
            if (socket != clientSocket) {
                send(socket, message.c_str(), message.size(), 0);
            }
        }
    }

    // Remove the client socket from the vector
    auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
    if (it != clientSockets.end()) {
        clientSockets.erase(it);
    }

    // Close the client socket
    close(clientSocket);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

    // Create server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // Set up server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket to the specified IP and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Binding failed." << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        std::cerr << "Listening failed." << std::endl;
        return 1;
    }

    std::cout << "Server is running. Listening on port " << PORT << "..." << std::endl;

    while (true) {
        // Accept a new client connection
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize)) < 0) {
            std::cerr << "Accepting connection failed." << std::endl;
            continue;
        }

        // Add the client socket to the vector of client sockets
        clientSockets.push_back(clientSocket);

        // Handle the client in a separate thread
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // Detach the thread to run independently
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
