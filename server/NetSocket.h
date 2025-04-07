#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "SyncObject.h"

namespace SyncTools {

class DataBuffer; // forward declaration

class NetSocket : public SyncObject {
private:
    sockaddr_in socketAddr;
    bool isOpen;
    SignalEvent terminationEvent;
public:
    NetSocket(const std::string & ipAddress, unsigned int port);
    NetSocket(int sockFD);
    NetSocket(const NetSocket & s);
    NetSocket & operator=(const NetSocket & s);
    ~NetSocket(void);
    
    int openConnection(void);
    int writeData(const DataBuffer & buffer);
    int readData(DataBuffer & buffer);
    void closeConnection(void);
    bool isOpenStatus(void);
};

class DataBuffer {
public:
    std::vector<char> buffer;
    std::string toString(void) const {
        std::string ret;
        for (size_t i = 0; i < buffer.size(); i++)
            ret.push_back(buffer[i]);
        return ret;
    }
    DataBuffer() { }
    DataBuffer(const std::string & input) {
        for (size_t i = 0; i < input.size(); i++)
            buffer.push_back(input[i]);
    }
    DataBuffer(void * p, int size) {
        char * cptr = static_cast<char*>(p);
        for (int i = 0; i < size; i++)
            buffer.push_back(cptr[i]);
    }
};

}; // namespace SyncTools

#endif // NET_SOCKET_H
