#include "Core/Application.h"
#include "Core/Log.h"

int main() {
    try {
        bhs::core::Application app;
        return app.run();
    } catch (const std::exception& ex) {
        bhs::core::Log::error(std::string("Fatal error in main: ") + ex.what());
        return 1;
    }
}
