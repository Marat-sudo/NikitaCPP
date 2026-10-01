#include <iostream>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>

// тоже подключение библиотеки
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 5001;
const int BUFFER_SIZE = 1024;

int main(){
    WSADATA wsaDATA;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;

    int clientAddrLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    // 1 - инцализация winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaDATA) != 0) {
        // MAKEWORD(2, 2) - Winsock2.2
        cout << "Error WSAStartup failed." << endl;
        WSACleanup(); 
        return 1;
    }
    cout << "WSAStartup OK." << endl;

    // 2 - инцилизация сокета
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // AF_INET - семейство адресов IPv4 
    // SOCK_STREAM - поток для TCP 
    // IPPROTO_TCP = TCP

    if (listenSocket == INVALID_SOCKET){
        cout << "Error. socket failed. " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    cout << "Socket created." << endl;

    // 3 - Bind - привязка адреса к порту
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
        // (sockaddr*)&serverAddr - преобразует serverAddr в общий вид sockaddr 
        cout << "Error. bind failed." << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // 4 - listen - режим прослушивания

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR){
        // SOMAXCONN - макс подключений в системе
        cout << "Error. Listen failed." << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Listenning..." << endl;

    // 5 - accept - режим подключения
    clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET){
        cout << "Error. accept failed. " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Client connected!" << endl;


    // 6 - send/recv - обмен данными
    while (true)
    {
        
        memset(buffer, 0, BUFFER_SIZE);
        int butesReceived = recv(clientSocket, buffer, BUFFER_SIZE-1, 0);

        if (butesReceived == SOCKET_ERROR){
            cout << "Error. recv failed. " << WSAGetLastError() << endl;
            break;
        }

        if (butesReceived == 0){
            cout << "Client disconnect." << endl;
            break;
        }

        buffer[butesReceived] = '\0';

        cout << "Received: " << buffer << endl;

        string response = "Server got: ";
        response += buffer;
        send(clientSocket, response.c_str(), (int)response.length(), 0);
        // c_str() string -> char
        if (strcmp(buffer, "exit") == 0){
            cout << "Exit command received" << endl;
            break;
        }
        
        
    }
    // 7 - закрытие всех сокетов
    closesocket(clientSocket);
    closesocket(listenSocket);

    WSACleanup();
    cout << "Server Stoped" << endl;

    

    return 0;
}