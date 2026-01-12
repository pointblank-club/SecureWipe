#include "ui/layout.h"
#include "ui/colors.h"

void draw_disks(WINDOW *win, int highlight,
                const std::vector<Disk> &disks)
{
    werase(win);
    draw_box_with_title(win, "Available Disks", COLOR_TITLE);

    int start_y = 3;

    if (disks.empty())
    {
        wattron(win, COLOR_PAIR(COLOR_WARNING));
        mvwprintw(win, start_y, 4, "No disks detected");
        wattroff(win, COLOR_PAIR(COLOR_WARNING));
    }
    else
    {
        for (size_t i = 0; i < disks.size(); i++)
        {
            if ((int)i == highlight)
                wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);

            std::string type = (disks[i].rota == "1") ? "[HDD]" : "[SSD]";
            if (disks[i].node.find("nvme") != std::string::npos)
                type = "[NVMe]";
            if (disks[i].node.find("loop") != std::string::npos)
                type = "[Loop]";

            mvwprintw(win, start_y + i * 3, 4,
                      "> %s %s", disks[i].node.c_str(), type.c_str());

            mvwprintw(win, start_y + i * 3 + 1, 6,
                      "Model: %s | Size: %s",
                      disks[i].model.empty() ? "Unknown" : disks[i].model.c_str(),
                      disks[i].size.c_str());

            if ((int)i == highlight)
                wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
        }
    }

    mvwprintw(win, getmaxy(win) - 2, 2,
              "UP/DOWN: Navigate  ENTER: Wipe  R: Refresh  B: Back");

    wrefresh(win);
}
