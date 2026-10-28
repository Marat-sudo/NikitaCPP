#include <iostream>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <mutex> // Нужна чтобы потоки не конфликтовали между собой
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <fstream>
#include <filesystem>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

const int PORT = 5000;
const int BUFFER_SIZE = 1024;

// Список подключённых клиентов и мьютекс для защиты
vector<SOCKET> clients;
mutex clientsMutex;

// Рассылка сообщения всем, кроме отправителя
void broadcast(const string& message, SOCKET sender) {
    lock_guard<mutex> lock(clientsMutex);
    for (SOCKET s : clients) {
        if (s != sender) {
            send(s, message.c_str(), (int)message.length(), 0);
        }
    }
}

// Удаление клиента из списка
void removeClient(SOCKET s) {
    lock_guard<mutex> lock(clientsMutex);
    clients.erase(remove(clients.begin(), clients.end(), s), clients.end());
}


// приём файлов от клиентов
void saveFile(SOCKET clientSocket, const string& filename, int fileSize, const char* initialData, int initialDataSize){
    system("mkdir downloads");  // создать папку
    string savePath = "downloads\\" + filename;
    ofstream file(savePath, ios::binary);

    if (!file.is_open()){
        cout << "Failed to open file: " << savePath << endl;
        return;
    }

    
    int totalRecived = 0;

    if (initialDataSize > 0) {
        file.write(initialData, initialDataSize);
        totalRecived += initialDataSize;
    }
    char buffer[BUFFER_SIZE];
    while (totalRecived < fileSize){
        int bytesToRead = min(BUFFER_SIZE, fileSize-totalRecived);
        int bytesReceived = recv(clientSocket, buffer, bytesToRead, 0);

        if (bytesReceived <= 0) break;

        file.write(buffer, bytesReceived);
        totalRecived += bytesReceived;
    }


    file.close();   
    cout << "File saved " << savePath << " (" << totalRecived << ")" << endl;
}



// Обслуживание одного клиента в отдельном потоке
void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];

    // Добавляем клиента в список
    {
        lock_guard<mutex> lock(clientsMutex);
        clients.push_back(clientSocket);
    }
    cout << "Client connected. Total: " << clients.size() << endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytesReceived == SOCKET_ERROR) {
            cout << "recv failed: " << WSAGetLastError() << endl;
            break;
        }
        if (bytesReceived == 0) {
            cout << "Client disconnected" << endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        
        if (strncmp(buffer, "file:", 5) == 0){
            string header = buffer;
            size_t firstColon = header.find(":");
            size_t secondColon = header.find(":", firstColon + 1);
            size_t newlinePos = header.find("\n");

            if (secondColon != string::npos && newlinePos != string::npos){
                string filename = header.substr(firstColon + 1, secondColon - firstColon - 1);
                
                if (!filename.empty() && filename[0] == ' ') {
                    filename = filename.substr(1);
                }

                int filesize = stoi(header.substr(secondColon + 1, newlinePos - secondColon - 1));

                cout << "Receiving file: " << filename << endl;

                int headerLen = newlinePos + 1; 
                const char* extraData = buffer + headerLen;
                int extraDataSize = bytesReceived - headerLen;

                saveFile(clientSocket, filename, filesize, extraData, extraDataSize);

                string ack = "Server: File " + filename + " received, size: " + to_string(filesize) + " bytes";
                send(clientSocket, ack.c_str(), (int)ack.length(), 0);
                continue;
            }

            
        }

        if (strncmp(buffer, "filelist", 8) == 0){
            string filesPath = "downloads";
            string answer = "downloads:";
            try {
                for (const auto& entry : fs::directory_iterator(filesPath)) {
                    answer = answer + '\n' + "<<< " + entry.path().filename().string();
                
                        
            }
            send(clientSocket, answer.c_str(), (int)answer.length(), 0);
            continue;

            } catch (const fs::filesystem_error& e) {
                string askError = string("File system Error: ") + e.what();
                send(clientSocket, askError.c_str(), (int)answer.length(), 0);
                continue;
                
            }
            
        }
        
        cout << "Received: " << buffer << endl;
        
        // Пересылаем всем остальным
        string message = "Client: ";
        message += buffer;
        broadcast(message, clientSocket);

        if (strcmp(buffer, "exit") == 0) {
            cout << "Exit command received" << endl;
            break;
        }
    }

    removeClient(clientSocket);
    closesocket(clientSocket);
    cout << "Client closed. Total: " << clients.size() << endl;
}

int main() {
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // 1. Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed: " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "WSAStartup OK" << endl;

    // 2. Создание сокета
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cout << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
    cout << "Socket created" << endl;

    // 3. Bind
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "bind failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    cout << "Bound to port " << PORT << endl;

    // 4. Listen
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    cout << "Listening..." << endl;

    // 5. Принимаем клиентов в цикле
    while (true) {
        clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cout << "accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        // Каждого клиента — в отдельный поток
        thread t(handleClient, clientSocket);
        t.detach();
    }

    closesocket(listenSocket);
    WSACleanup();
    cout << "Server stopped" << endl;
    return 0;
}