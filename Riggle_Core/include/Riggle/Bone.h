#pragma once

#include <string>

namespace Riggle {

    class Bone {
    public:
        Bone(const std::string& name) : name(name) {}

        const std::string& GetName() const { return name; }

    private:
        std::string name;
    };

} // namespace Riggle
