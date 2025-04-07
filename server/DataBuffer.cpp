#include <vector>
#include <string>

class DataBuffer {
public:
    std::vector<char> buffer;
    std::string toString() const {
        std::string str;
        for (size_t i = 0; i < buffer.size(); i++) {
            str.push_back(buffer[i]);
        }
        return str;
    }
    DataBuffer() { }
    DataBuffer(const std::string & input) {
        for (size_t i = 0; i < input.size(); i++) {
            buffer.push_back(input[i]);
        }
    }
    DataBuffer(void * ptr, int size) {
        char * cptr = static_cast<char*>(ptr);
        for (int i = 0; i < size; i++) {
            buffer.push_back(cptr[i]);
        }
    }
};
