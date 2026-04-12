#define IO_IMPLEMENTATION

#include "../../hi/audiocontext.hpp"

int main() {
    ac::AudioContext ctx{};

    if (ctx.IsInitialized()) {
        return 1;
    }

    if (ctx.MasterGain() != 1.0f) {
        return 2;
    }

    ctx.SetMasterGain(8.0f);
    if (ctx.MasterGain() != 4.0f) {
        return 3;
    }

    ctx.SetCategoryGain(ac::Category::UI, -1.0f);
    if (ctx.CategoryGain(ac::Category::UI) != 0.0f) {
        return 4;
    }

    if (!ctx.Init()) {
        // Device may be unavailable in CI/headless environment.
        return 0;
    }

    if (!ctx.IsInitialized()) {
        return 5;
    }

    io::sleep_ms(50);
    ctx.Shutdown();

    if (ctx.IsInitialized()) {
        return 6;
    }

    return 0;
}

