#include "ui/layout.h"
#include "ui/colors.h"
#include <sstream>

void show_result(WINDOW *win, bool success,
                 const std::string &title,
                 const std::string &message,
                 const std::string &details)
{
    werase(win);
    draw_box_with_title(win, title,
                        success ? COLOR_SUCCESS : COLOR_ERROR);

    int y = 3;
    int color = success ? COLOR_SUCCESS : COLOR_ERROR;

    wattron(win, COLOR_PAIR(color) | A_BOLD);
    std::string status = success ? "[SUCCESS]" : "[FAILED]";
    mvwprintw(win, y, (getmaxx(win) - status.size()) / 2,
              "%s", status.c_str());
    wattroff(win, COLOR_PAIR(color) | A_BOLD);

    y += 2;
    mvwprintw(win, y, (getmaxx(win) - message.size()) / 2,
              "%s", message.c_str());

    if (!details.empty())
    {
        y += 2;
        mvwprintw(win, y++, 4, "Details:");
        std::istringstream iss(details);
        std::string line;
        while (std::getline(iss, line))
            mvwprintw(win, y++, 6, "%s", line.c_str());
    }

    mvwprintw(win, getmaxy(win) - 2, 2,
              "Press any key to continue...");
    wrefresh(win);
}
