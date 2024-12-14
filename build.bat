@echo off
SetLocal EnableDelayedExpansion

if not exist build (
    mkdir build
)

pushd build

cmake -S .. -B . -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_MAKE_PROGRAM=make
cmake --build . --parallel
copy compile_commands.json ..

popd

pause
