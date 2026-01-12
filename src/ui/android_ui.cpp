#include "ui/layout.h"
#include "ui/colors.h"

void draw_android(WINDOW *win,
                  const std::string &mode,
                  const std::string &device_info)
{
    werase(win);
    draw_box_with_title(win, "Android Device Wipe", COLOR_TITLE);

    int y = 3;
    mvwprintw(win, y++, 4, "Detection Mode:");
    wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
    mvwprintw(win, y++, 6, "> %s", mode.c_str());
    wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);

    y++;
    if (!device_info.empty())
    {
        wattron(win, COLOR_PAIR(COLOR_SUCCESS));
        mvwprintw(win, y++, 4, "Detected Device:");
        wattroff(win, COLOR_PAIR(COLOR_SUCCESS));
        mvwprintw(win, y++, 6, "%s", device_info.c_str());
    }
    else
    {
        wattron(win, COLOR_PAIR(COLOR_WARNING));
        mvwprintw(win, y++, 4, "No device detected");
        wattroff(win, COLOR_PAIR(COLOR_WARNING));
    }

    mvwprintw(win, getmaxy(win) - 2, 2,
              "1/2: Mode  R: Scan  ENTER: Wipe  B: Back");
    wrefresh(win);
}
