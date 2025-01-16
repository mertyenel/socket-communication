#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string> 
#include <cstdint>
using namespace std;
    
bool readRaw(int sckt, void* buffer, size_t bufsize) {
    char *ptr = static_cast<char*>(buffer);
    while (bufsize > 0) {
        ssize_t numBytes = recv(sckt, ptr, bufsize, 0);
        if (numBytes < 0) {
            cout << "Read error: " << errno << '\n';
            return false;
        }
        if (numBytes == 0) {
            cout << "Client disconnected\n";
            return false;
        }
        ptr += numBytes;
        bufsize -= numBytes;
    }
    return true;
}

bool sendRaw(int sckt, const void* buffer, size_t bufsize) {
    const char *ptr = static_cast<const char*>(buffer);
    while (bufsize > 0) {
        ssize_t numBytes = send(sckt, ptr, bufsize, 0);
        if (numBytes < 0) {
            cout << "Send error: " << errno << '\n';
            return false;
        }
        ptr += numBytes;
        bufsize -= numBytes;
    }
    return true;
}

bool readUint32(int sckt, uint32_t &value) {
    if (!readRaw(sckt, &value, sizeof(value))) return false;        
    value = ntohl(value);
    return true;
}

bool sendUint32(int sckt, uint32_t value) {
    value = htonl(value);
    return sendRaw(sckt, &value, sizeof(value));
}
 
bool readString(int sckt, string &message) {
    message.clear();
    uint32_t len;
    if (!readUint32(sckt, len)) return false;
    message.resize(len);
    return readRaw(sckt, const_cast<char*>(message.data()), len);
}


bool sendString(int sckt, const string &message) {
    uint32_t len = message.size();
    if (!sendUint32(sckt,len)) return false;
    return sendRaw(sckt, message.c_str(), len);
}

/* alternatively:

bool readString(int sckt, string &message) {
    char buffer[1024], ch;
    size_t len = 0;
    message.clear();
    while (true) {
        if (!readRaw(sckt, &ch, 1)) return false;
        if (ch == '\0') {
            if (len > 0) message.append(buffer, len);
            break;
        }
        if (len == sizeof(buffer)) {
            message.append(buffer, len);
            len = 0;
        }
        buffer[len++] = ch;
    }
    return true;    
}

bool sendString(int sckt, const string &message) {
    return sendRaw(sckt, message.c_str(), message.size()+1);    
}

*/

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: ./server <LISTEN_IP_ADDRESS> <PORT>\n"; 
        return -1;
    }
    
    char* listen_ip_address = argv[1];
    int port = stoi(argv[2]);
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cout << "Socket creation error: " << errno << '\n';
        return -1;
    }
    
    sockaddr_in serverAddress; //sockaddr_in: It is the data type that is used to store the address of the socket.
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(port); //This function is used to convert the unsigned int from machine byte order to network byte order.
    serverAddress.sin_addr.s_addr = inet_addr(listen_ip_address);
    
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    if (listen(serverSocket, 1) == -1) {
        cout << "Socket listen error: " << errno << '\n';
        close(serverSocket);
        return -1;
    }
    
    cout << "The server has been started. Listening for connection...\n";
    
    sockaddr_in clientAddr;
    socklen_t clientSize = sizeof(clientAddr);

    int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
    if (clientSocket < 0) {
        cout << "Connection accept error: " << errno << '\n';
        close(serverSocket);
        return -1;
    }

    cout << "Client connected\n";

    while (true) {
        
        std::string message;
        if (!readString(clientSocket, message))
            break;

        cout << "Message from client: " << message << '\n'; 

        if (message == "Fin")
            break;

        message.clear();

        cout << "Enter a message: ";
        if (!getline(cin, message))
            break;

        if (!sendString(clientSocket, message))
            break;

        if (message == "Fin")
            break;
    }
    
    cout << "Server is shutting down...\n";
    close(serverSocket);
    
    return 0;
}