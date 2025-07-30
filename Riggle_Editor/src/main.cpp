#include "Editor/EditorApplication.h"
#include <iostream>

int main() {
    try {
        Riggle::EditorApplication app;
        return app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return -1;
    }
}