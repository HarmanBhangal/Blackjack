// DataBuffer.h
#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <string>
#include <vector>
#include <jsoncpp/json/json.h>

class DataBuffer {
public:
    DataBuffer(const std::string &data);
    std::string toString() const;
    // ... add other public member functions and declarations here

private:
    std::vector<char> buffer; // or whatever your internal structure is
};

#endif // DATABUFFER_H
