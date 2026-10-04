#include <iostream>
#include <string>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char* SERVER_IP = "127.0.0.1";
const int PORT = 5000;
const int BUFFER_SIZE = 1024;

int main() {
    WSADATA wsaData;
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // инцилизация winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Error WSAStartup failed." << endl;
        return 1;
    }
    cout << "WSAStartup OK." << endl;

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Error. socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
    cout << "Socket created." << endl;

    // настройка адреса сервера
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // устанавливаем коннект
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Error. connect failed: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    cout << "Connected to server!" << endl;

    while (true) {
        string userInput;
        cout << "Enter message (or 'exit'): ";
        getline(cin, userInput);

        if (userInput.empty()) {
            continue;
        }

        // отпрвка
        send(clientSocket, userInput.c_str(), (int)userInput.length(), 0);

        // приянли
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytesReceived == SOCKET_ERROR) {
            cout << "Error. recv failed: " << WSAGetLastError() << endl;
            break;
        }

        if (bytesReceived == 0) {
            cout << "Server disconnected." << endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        cout << "Server reply: " << buffer << endl;

        if (userInput == "exit") {
            cout << "Exiting" << endl;
            break; 
        }
    }

    // закрытие
    closesocket(clientSocket);
    WSACleanup();
    cout << "Client Stopped." << endl;

    return 0;
}