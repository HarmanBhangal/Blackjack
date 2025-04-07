# Makefile for the new C++ server

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -I.
LIBS = -ljsoncpp -pthread

# Targets (list your new source files)
SRCS = GameServer.cpp ThreadWrapper.cpp NetSocket.cpp NetSocketServer.cpp SyncObject.cpp DataBuffer.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = game_server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

GameServer.o: GameServer.cpp ThreadWrapper.h NetSocketServer.h SyncObject.h 
	$(CXX) $(CXXFLAGS) -c GameServer.cpp -ljsoncpp

ThreadWrapper.o: ThreadWrapper.cpp ThreadWrapper.h SignalEvent.h
	$(CXX) $(CXXFLAGS) -c ThreadWrapper.cpp -ljsoncpp

NetSocket.o: NetSocket.cpp NetSocket.h
	$(CXX) $(CXXFLAGS) -c NetSocket.cpp -ljsoncpp

NetSocketServer.o: NetSocketServer.cpp NetSocketServer.h NetSocket.h
	$(CXX) $(CXXFLAGS) -c NetSocketServer.cpp -ljsoncpp

SyncObject.o: SyncObject.cpp SyncObject.h
	$(CXX) $(CXXFLAGS) -c SyncObject.cpp -ljsoncpp

DataBuffer.o: DataBuffer.cpp 
	$(CXX) $(CXXFLAGS) -c DataBuffer.cpp -ljsoncpp

clean:
	rm -f $(OBJS) $(TARGET)
