#include "app/app.h"

#include <ncurses.h>

#include "flow/stage.h"
#include "flow/main_menu.h"
#include "flow/local_wipe.h"
#include "flow/android_wipe.h"

void App::run()
{
    Stage stage = Stage::MAIN_MENU;

    int h, w;
    getmaxyx(stdscr, h, w);

    WINDOW* mainwin = newwin(h - 2, w, 0, 0);
    WINDOW* footer  = newwin(2, w, h - 2, 0);

    keypad(mainwin, TRUE);

    while (stage != Stage::EXIT)
    {
        switch (stage)
        {
        case Stage::MAIN_MENU:
            stage = run_main_menu(mainwin, footer);
            break;

        case Stage::LOCAL_WIPE:
            stage = run_local_wipe(mainwin, footer);
            break;

        case Stage::ANDROID:
            stage = run_android_wipe(mainwin, footer);
            break;

        default:
            stage = Stage::EXIT;
        }
    }

    delwin(mainwin);
    delwin(footer);
}
