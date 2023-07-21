#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

const char* SERVER_IP = "127.0.0.1";
const int PORT = 8080;
const int BUFFER_SIZE = 1024;

bool shouldExit = false;

void receiveMessages(int clientSocket, const std::string& username) {
    char buffer[BUFFER_SIZE] = {0};

    while (!shouldExit) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead <= 0) {
            std::cerr << "Server connection closed." << std::endl;
            shouldExit = true; // To exit the receive loop if the server is disconnected
            break;
        }

        std::string receivedMessage = buffer;
        if (receivedMessage.find(username + ":") == 0) {
            // This is the response from the server containing the client's username
            std::cout << "Your username is: " << receivedMessage << std::endl;
        } else {
            // This is a message from the server or other clients
            std::cout << receivedMessage << std::endl;
        }
    }
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE] = {0};

    // Create client socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // Set up server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported." << std::endl;
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        return 1;
    }

    std::cout << "Connected to the chat server." << std::endl;

    // Ask for username
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    send(clientSocket, username.c_str(), username.size(), 0);

    std::cout << "You can start typing messages. Type '/exit' to leave the chat." << std::endl;

    // Start a new thread to receive and display messages from the server (and other clients)
    std::thread messageThread(receiveMessages, clientSocket, username);

    // Send messages to the server
    while (!shouldExit) {
        std::string message;
        std::getline(std::cin, message);
        if (message == "/exit") {
            shouldExit = true;
            send(clientSocket, message.c_str(), message.size(), 0); // Notify the server about the exit
            break;
        }

        std::string formattedMessage = username + ": " + message;
        send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
    }

    // Close the socket
    close(clientSocket);

    // Wait for the message thread to finish
    messageThread.join();

    return 0;
}
