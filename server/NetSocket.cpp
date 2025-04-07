#include <strings.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include "NetSocket.h"

namespace SyncTools {

NetSocket::NetSocket(const std::string & ipAddress, unsigned int port)
    : SyncObject(), isOpen(false)
{
    setDescriptor(socket(AF_INET, SOCK_STREAM, 0));
    if (getDescriptor() < 0)
        throw std::string("Unable to initialize socket");
    bzero((char*)&socketAddr, sizeof(sockaddr_in));
    if (!inet_aton(ipAddress.c_str(), &socketAddr.sin_addr))
        throw std::string("Invalid IP Address");
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(port);
}

NetSocket::NetSocket(int sockFD) : SyncObject(sockFD) {
    isOpen = true;
}

NetSocket::NetSocket(const NetSocket & s) : SyncObject(s) {
    isOpen = s.isOpen;
}

NetSocket & NetSocket::operator=(const NetSocket & rhs) {
    close(getDescriptor());
    socketAddr = rhs.socketAddr;
    setDescriptor(dup(rhs.getDescriptor()));
    isOpen = rhs.isOpen;
    return *this;
}

NetSocket::~NetSocket(void) {
    closeConnection();
}

int NetSocket::openConnection(void) {
    int ret = connect(getDescriptor(), (sockaddr*)&socketAddr, sizeof(socketAddr));
    if (ret != 0)
        throw std::string("Unable to open connection");
    isOpen = true;
    return ret;
}

int NetSocket::writeData(const DataBuffer & buffer) {
    if (!isOpen)
        return -1;
    char * raw = new char[buffer.buffer.size()];
    for (size_t i = 0; i < buffer.buffer.size(); i++)
        raw[i] = buffer.buffer[i];
    int bytesSent = write(getDescriptor(), raw, buffer.buffer.size());
    if (bytesSent <= 0)
        isOpen = false;
    delete[] raw;
    return bytesSent;
}

static const int MAX_DATA_SIZE = 256;
int NetSocket::readData(DataBuffer & buffer) {
    char raw[MAX_DATA_SIZE];
    if (!isOpen)
        return 0;
    buffer.buffer.clear();
    MultiWait waiter(2, this, &terminationEvent);
    SyncObject * result = waiter.wait();
    if (result == &terminationEvent) {
        terminationEvent.reset();
        return 0;
    }
    ssize_t received = recv(getDescriptor(), raw, MAX_DATA_SIZE, 0);
    for (int i = 0; i < received; i++)
        buffer.buffer.push_back(raw[i]);
    if (received <= 0)
        isOpen = false;
    return received;
}

bool NetSocket::isOpenStatus() {
    return isOpen;
}

void NetSocket::closeConnection(void) {
    close(getDescriptor());
    shutdown(getDescriptor(), SHUT_RDWR);
    isOpen = false;
    terminationEvent.trigger();
}

}; // namespace SyncTools
