@echo off
cd ..
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
echo Build complete! Running: build\Riggle_Editor\Release\Riggle_Editor.exe
start build\Riggle_Editor\Release\Riggle_Editor.exe
pause