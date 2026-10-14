#include <iostream>
#include <string>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <deque>
#include <mutex>
#include <conio.h>


#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char* SERVER_IP = "127.0.0.1";
const int PORT = 5000;
const int BUFFER_SIZE = 1024;

mutex consoleMutex;
deque<string> commingMsgs;
string userInput;


void redrawConsole(){
    cout << "\r" << string(120, ' ') << "\r";

    while(!commingMsgs.empty()){
        cout << commingMsgs.front() << endl;
        commingMsgs.pop_front();
    }

    cout << "text: " << userInput << flush;
}


void reciveLoop(SOCKET clientSocket){
    char buffer[BUFFER_SIZE];
    while (true){
        // приняли
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
        commingMsgs.push_back(string(buffer));
        redrawConsole();
        
    }
}


int main() {
    WSADATA wsaData;
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    

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

    {
        lock_guard<mutex> losk(consoleMutex);
        redrawConsole();
    }

    thread t(reciveLoop, clientSocket);
    t.detach();

    while (true) {
        if (_kbhit()){
            int ch = _getch();

            if (ch == '\r') {
                lock_guard<mutex> losk(consoleMutex);

                if (!userInput.empty()){
                    send(clientSocket, userInput.c_str(), (int)userInput.length(), 0);
                    commingMsgs.push_back(userInput);
                }

                if (userInput == "exit"){
                    break;
                }
                userInput.clear();
                redrawConsole();
            }

            else if (ch == '\b'){
                if (!userInput.empty()){
                    userInput.pop_back();
                    lock_guard<mutex> losk(consoleMutex);
                    redrawConsole();
                }

            }
            else {
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
    }

    // закрытие
    closesocket(clientSocket);
    WSACleanup();
    cout << "Client Stopped." << endl;

    return 0;
}