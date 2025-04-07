#include <iostream>
#include <jsoncpp/json/json.h>

int main() {
    Json::Value root;
    root["username"] = "Alice";
    std::string output = root.toStyledString();
    std::cout << output << std::endl;
    return 0;
}
