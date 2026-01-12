#include "ui/layout.h"
#include "ui/colors.h"

void draw_box_with_title(WINDOW *win, const std::string &title, int color_pair)
{
    box(win, 0, 0);
    if (!title.empty())
    {
        wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
        mvwprintw(win, 0, 3, " %s ", title.c_str());
        wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
    }
}
