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



// тоже подключение библиотеки
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 5000;
const int BUFFER_SIZE = 1024;

vector<SOCKET> clients;
map<SOCKET, string> clientsIps;
mutex clientMutex;

void broadcast(const string &message, SOCKET sender){
    // рассылка сообщений
    lock_guard<mutex> lock(clientMutex);
    string senderIp = "0.0.0.0";
    auto it = clientsIps.find(sender);

    if (it != clientsIps.end()){
        senderIp = it->second;
    }
    // добавляем айпи адрес перед сообщением
    string fullMsg = "[" + senderIp + "]: " + message;
    for (SOCKET s: clients){
        if (s != sender){
            send(s, fullMsg.c_str(), (int)fullMsg.length(), 0);
        }
    }
}


void removeClient(SOCKET s){
    // удаляет клиента из списка 
    lock_guard<mutex> lock(clientMutex);
    clients.erase(remove(clients.begin(), clients.end(), s), clients.end());
}


void handleClient(SOCKET clientSocket, string clientIp){
    // обслуживает клиента
    char buffer[BUFFER_SIZE];

    {  // блокирует поток записи клиента в список только для него и после освобождает 
        lock_guard<mutex> lock(clientMutex);
        clients.push_back(clientSocket);
        clientsIps[clientSocket] = clientIp;
    }

    cout << "Clietnt connect" << endl;

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

        
        if (strcmp(buffer, "exit") == 0){
            cout << "Exit command received" << endl;
            break;
        }
        string response = "";
        response += buffer;
        broadcast(response, clientSocket);

        // send(clientSocket, response.c_str(), (int)response.length(), 0);
        // c_str() string -> char
         
    } 
    removeClient(clientSocket);
    closesocket(clientSocket);
    cout << "client disconect" << endl;
}



int main(){
    WSADATA wsaDATA;
    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;

    int clientAddrLen = sizeof(clientAddr);

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

    // 5. приём клиента в цикле
    while (true)
    {
        clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET){
            cout << "Error. accept failed. " << WSAGetLastError() << endl;
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
        /*
        AF_INET - семейство адресов
        &clientAddr.sin_addr - получаем айпи адрес клиента
        ipStr - сюда записываем айпи адрес в нужном формате
        INET_ADDRSTRLEN - размер буффера
        */ 
        string clietntIP = ipStr;

        // Для каждого клиента в поток
        thread t(handleClient, clientSocket, clietntIP);
        t.detach();
    }
    


    // 6 - send/recv - обмен данными
    
    // 7 - закрытие всех сокетов
    closesocket(listenSocket);

    WSACleanup();
    cout << "Server Stoped" << endl;

    

    return 0;
}