@echo off
SetLocal EnableDelayedExpansion

if not exist bin (
    mkdir bin
)

pushd bin

cmake ..
cmake --build . --parallel

popd

pause