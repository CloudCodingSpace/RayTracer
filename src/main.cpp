#include "RayTracer/RayTracer.h"

int main(int argc, const char** argv) {
    RayTracer rt;
    rt.Init();
    rt.Run();
    rt.Cleanup();

    return 0;
}