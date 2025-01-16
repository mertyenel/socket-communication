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
            cout << "Server disconnected\n";
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

int main(int argc, char* argv[]){
    if (argc != 3){
        cout << "Usage: ./client <SERVER_IP_ADRES> <PORT>\n"; 
        return -1;
    }

    char* server_ip_address = argv[1];
    int port = stoi(argv[2]);

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cout << "Socket creation error: " << errno << '\n';
        return -1;
    }
    
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(server_ip_address);
    
    if (connect(clientSocket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        cout << "Socket connect error: " << errno << '\n';
        close(clientSocket);
        return -1;
    }
    
    while (true) {

        cout << "Enter a message: ";

        string message;
        if (!getline(cin, message))
            break;

        if (!sendString(clientSocket, message))
            break;
        
        if (message == "Fin")
            break;
        
        if (!readString(clientSocket, message))
            break;
        
        cout << "Message from server: " << message << '\n';

        if (message == "Fin")
            break;
    }
    
    cout << "Client is shutting down...\n";
    close(clientSocket);
    
    return 0;
}