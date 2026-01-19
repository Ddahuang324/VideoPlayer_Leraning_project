#include <iostream>
#include <nlohmann/json.hpp>
#include "common/Public.h"

using json = nlohmann::json;

int main() {
    std::cout << "YiboServer C++17 - Phase 1 Framework Setup\n";

    // 测试nlohmann/json
    json config;
    config["version"] = "1.0.0";
    config["phase"] = 1;
    std::cout << "Config: " << config.dump(2) << "\n";

    // 测试Buffer类
    Buffer msg = "Hello, YiboServer!";
    std::cout << "Message: " << msg << "\n";

    return 0;
}
