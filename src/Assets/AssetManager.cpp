#include "Assets/AssetManager.h"

#include "Core/Log.h"

#include <fstream>
#include <sstream>

namespace bhs::assets {
    using bhs::core::Log;

std::string AssetManager::readTextFile(const std::string& relativePath) {
    std::ifstream file(relativePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        Log::fatal("Failed to open asset file: " + relativePath);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    if (file.fail() && !file.eof()) {
        Log::fatal("Failed to read asset file: " + relativePath);
    }

    return buffer.str();
}

} // namespace bhs::assets
