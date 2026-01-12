#include "ui/layout.h"

void draw_progress(WINDOW *win,
                   const std::string &title,
                   const std::string &message,
                   int percent)
{
    werase(win);
    draw_box_with_title(win, title, COLOR_WHITE);

    int y = getmaxy(win) / 2;
    int x = (getmaxx(win) - message.size()) / 2;

    wattron(win, A_BOLD);
    mvwprintw(win, y, x, "%s", message.c_str());
    wattroff(win, A_BOLD);

    if (percent >= 0)
    {
        int bar_width = 40;
        int filled = (bar_width * percent) / 100;
        int bar_x = (getmaxx(win) - bar_width - 2) / 2;

        mvwprintw(win, y + 2, bar_x, "[");
        for (int i = 0; i < bar_width; i++)
            mvwprintw(win, y + 2, bar_x + 1 + i,
                      i < filled ? "=" : "-");
        mvwprintw(win, y + 2, bar_x + bar_width + 1, "]");
    }

    wrefresh(win);
}
