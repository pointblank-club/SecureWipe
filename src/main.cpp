#include <ncurses.h>
#include "ui/colors.h"
#include "flow/main_menu.h"
#include "flow/local_wipe.h"
#include "flow/android_wipe.h"

int main()
{
    initscr();
    start_color();
    use_default_colors();

    init_pair(COLOR_TITLE, COLOR_WHITE, -1);
    init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
    init_pair(COLOR_WARNING, COLOR_YELLOW, -1);
    init_pair(COLOR_ERROR, COLOR_RED, -1);
    init_pair(COLOR_INFO, COLOR_WHITE, -1);
    init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_GREEN);

    noecho();
    cbreak();
    curs_set(0);

    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW *mainwin = newwin(h - 2, w, 0, 0);
    WINDOW *footer = newwin(2, w, h - 2, 0);
    keypad(mainwin, TRUE);

    Stage stage = Stage::MAIN_MENU;
    while (stage != Stage::EXIT)
    {
        if (stage == Stage::MAIN_MENU)
            stage = run_main_menu(mainwin, footer);
        else if (stage == Stage::LOCAL_WIPE)
            stage = run_local_wipe(mainwin, footer);
        else if (stage == Stage::ANDROID)
            stage = run_android_wipe(mainwin, footer);
    }

    endwin();
    return 0;
}
