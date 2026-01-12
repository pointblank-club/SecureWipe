#include "ui/layout.h"
#include "ui/colors.h"

void draw_menu(WINDOW *win, int highlight,
               const std::vector<std::string> &choices)
{
    werase(win);
    draw_box_with_title(win, "Select Wipe Mode", COLOR_TITLE);

    int start_y = 3;
    for (size_t i = 0; i < choices.size(); i++)
    {
        if ((int)i == highlight)
        {
            wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
            mvwprintw(win, start_y + i * 2, 4, "> %s", choices[i].c_str());
            wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
        }
        else
        {
            mvwprintw(win, start_y + i * 2, 4, "  %s", choices[i].c_str());
        }
    }

    mvwprintw(win, getmaxy(win) - 2, 2,
              "UP/DOWN: Navigate  ENTER: Select  Q: Quit");

    wrefresh(win);
}
