// g++ client.cpp -o client -lws2_32
#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 5000;
const int BUFFER_SIZE = 1024;

// Поток приёма: постоянно ждёт сообщения от сервера
void receiveLoop(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytesReceived == SOCKET_ERROR) {
            cout << "\nrecv failed: " << WSAGetLastError() << endl;
            break;
        }
        if (bytesReceived == 0) {
            cout << "\nServer closed connection" << endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        cout << "\n<<< " << buffer << "\n> " << flush;
    }
}

void sendFile(SOCKET clientSocket, const string& filepath){
    ifstream file(filepath, ios::binary | ios::ate);
    if (!file.is_open()){
        cout << "Failed to open file: " << filepath << endl;
        return;
    }

    streamsize filesize = file.tellg();
    file.seekg(0, ios::beg);

    // получаем путь
    size_t slahPos = filepath.find_last_of("/\\");
    string filename = (slahPos == string::npos)? filepath: filepath.substr(slahPos + 1);

    // формируем загловок для сервера
    string header = "file: " + filename + ":" + to_string(filesize) + '\n';

    // send
    send(clientSocket, header.c_str(), (int)header.length(), 0);

    this_thread::sleep_for(chrono::milliseconds(100));

    // отправка файла частями
    char buffer[BUFFER_SIZE];
    while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0){
        int bytesToSend = (int)file.gcount();
        send(clientSocket, buffer, bytesToSend, 0);
    }

    file.close();
    cout << "File sent: " << filename << " (" << filesize << ") " << endl;

}
int main() {
    WSADATA wsaData;
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;

    // 1. Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed: " << WSAGetLastError() << endl;
        return 1;
    }

    // 2. Создание сокета
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // 3. Настройка адреса сервера
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    const char* serverIP = "127.0.0.1";
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    // 4. Connect
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "connect failed: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    cout << "Connected to server " << serverIP << ":" << PORT << endl;
    cout << "Type 'exit' to quit" << endl;

    // 5. Запускаем приём в отдельном потоке
    thread t(receiveLoop, clientSocket);
    t.detach();

    // 6. Основной поток — отправка
    while (true) {
        string message;
        cout << "> ";
        getline(cin, message);

        if (message.empty()) continue;

        if (message.rfind("file: ", 0) == 0){
            string filepath = message.substr(6);
            sendFile(clientSocket, filepath);
            continue;
        }

        send(clientSocket, message.c_str(), (int)message.length(), 0);

        if (message == "exit") break;
    }

    closesocket(clientSocket);
    WSACleanup();
    cout << "Client stopped" << endl;
    return 0;
}