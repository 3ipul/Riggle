@echo off
cd ..
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
echo Build complete! Running: build\Riggle_Editor\Debug\Riggle_Editor.exe
start build\Riggle_Editor\Debug\Riggle_Editor.exe
pause