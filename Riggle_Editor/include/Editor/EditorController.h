#pragma once

#include <iostream>
#include <Riggle/Bone.h>

namespace Editor {

    class EditorController {
    public:
        void Run() {
            Riggle::Bone bone("RootBone");
            std::cout << "EditorController running. Bone name: " << bone.GetName() << std::endl;
        }
    };

} // namespace Riggle
