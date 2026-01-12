#include "ui/layout.h"

void draw_footer(WINDOW *win, const std::string &text)
{
    werase(win);
    mvwprintw(win, 0, 2, "%s", text.c_str());
    wrefresh(win);
}
