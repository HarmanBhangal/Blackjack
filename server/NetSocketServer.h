#ifndef NET_SOCKET_SERVER_H
#define NET_SOCKET_SERVER_H

#include "NetSocket.h"
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

namespace SyncTools {

class NetSocketServer : public SyncObject {
private:
    int pipeFD[2];
    SignalEvent terminationSignal;
    sockaddr_in serverAddr;
public:
    NetSocketServer(int port);
    ~NetSocketServer();
    NetSocket acceptConnection(void);
    void shutdownServer(void);
};

}; // namespace SyncTools

#endif // NET_SOCKET_SERVER_H
