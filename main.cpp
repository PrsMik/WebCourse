import App.Presenter;
#include <iostream>

#ifndef DATA_DIR
#define DATA_DIR "."
#endif

int main(int argc, char **argv)
{
    App::Presenter app; // Конструктор сам настроит пресеты

    if (!app.initialize())
    {
        return -1;
    }

    app.run();

    return 0;
}