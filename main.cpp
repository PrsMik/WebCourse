// Сначала стандартные библиотеки
#include <iostream>

// Затем импорт модулей
import App.Presenter;

#ifndef DATA_DIR
#define DATA_DIR "."
#endif

int main(int argc, char **argv)
{
    // В браузере (Emscripten) std::cout будет выводиться в консоль JS (F12)
    std::cout << "Starting Wordle++..." << std::endl;

    App::Presenter app;

    if (!app.initialize())
    {
        std::cerr << "Initialization failed!" << std::endl;
        return -1;
    }

    app.run();

    return 0;
}