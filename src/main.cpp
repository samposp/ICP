#include "App.h"


App app;

int main()
{
    if (app.init())
        return app.run();
}