#include "NetSocket.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace SyncTools;

int main(void) {
    std::cout << "I am a test client" << std::endl;
    std::string serverIP = "127.0.0.1";
    int serverPort = 2036;
    NetSocket clientSocket(serverIP, serverPort);
    clientSocket.openConnection();
    
    while (true) {
        std::cout << "Enter message: ";
        std::string msg;
        getline(std::cin, msg);
        if (msg == "done")
            break;
        DataBuffer outBuffer(msg);
        clientSocket.writeData(outBuffer);
        DataBuffer inBuffer;
        if (clientSocket.readData(inBuffer) == 0) {
            std::cout << "Server has terminated" << std::endl;
            break;
        }
        std::string reply = inBuffer.toString();
        std::cout << "Server replies: " << reply << std::endl;
    }
    clientSocket.closeConnection();
    return 0;
}
