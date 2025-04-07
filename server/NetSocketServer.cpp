#include "NetSocketServer.h"
#include <strings.h>
#include <iostream>
#include <errno.h>
#include <algorithm>

namespace SyncTools {

NetSocketServer::NetSocketServer(int port) {
    int sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0)
        throw std::string("Unable to create socket server");
    bzero((char*)&serverAddr, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockFD, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        throw std::string("Failed to bind socket to port");
    listen(sockFD, 5);
    setDescriptor(sockFD);
}
NetSocketServer::~NetSocketServer(void) {
    shutdownServer();
}
NetSocket NetSocketServer::acceptConnection(void) {
    MultiWait waiter(2, this, &terminationSignal);
    SyncObject * result = waiter.wait();
    if (result == &terminationSignal) {
        terminationSignal.reset();
        throw TerminationCode(2);
    }
    if (result == this) {
        int connectionFD = accept(getDescriptor(), NULL, 0);
        if (connectionFD < 0)
            throw std::string("Unexpected error in socket server");
        return NetSocket(connectionFD);
    } else {
        throw std::string("Unexpected error in socket server");
    }
}
void NetSocketServer::shutdownServer(void) {
    close(getDescriptor());
    shutdown(getDescriptor(), SHUT_RDWR);
    terminationSignal.trigger();
}

}; // namespace SyncTools
