#include "ui/layout.h"
#include "ui/colors.h"

void draw_header(WINDOW *win)
{
    werase(win);
    wattron(win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);

    int width = getmaxx(win);
    std::string title = "VOID - SECURE WIPE UTILITY";
    int x = (width - title.size()) / 2;
    mvwprintw(win, 1, x, "%s", title.c_str());

    wattroff(win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);

    std::string subtitle = "Secure Device Erasure & Attestation";
    x = (width - subtitle.size()) / 2;
    mvwprintw(win, 2, x, "%s", subtitle.c_str());

    wrefresh(win);
}
