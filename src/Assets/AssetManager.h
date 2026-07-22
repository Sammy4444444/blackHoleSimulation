#pragma once

#include <string>

namespace bhs::assets {

class AssetManager {
public:
    static std::string readTextFile(const std::string& relativePath);
};

} // namespace bhs::assets
