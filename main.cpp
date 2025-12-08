import App.Presenter;
#include <iostream>

// Определения из CMake/Build system
#ifndef DATA_DIR
#define DATA_DIR "./"
#endif

int main(int argc, char** argv) {
    // Параметры объема
    const int W = 128;
    const int H = 256;
    const int D = 256;
    const std::string volPath = std::string(DATA_DIR) + "/vis_male_128x256x256_uint8.raw";

    App::Presenter app(W, H, D);
    
    if (!app.initialize(volPath)) {
        return -1;
    }

    app.run();

    return 0;
}