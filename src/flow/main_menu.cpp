#include "flow/main_menu.h"
#include "ui/layout.h"

Stage run_main_menu(WINDOW *mainwin, WINDOW *footer)
{
    std::vector<std::string> menu = {
        "Local Disks (NVMe, SSD, HDD)",
        "Android / USB Devices"};

    int choice = 0;

    while (true)
    {
        draw_menu(mainwin, choice, menu);
        draw_footer(footer, "UP/DOWN: Navigate  ENTER: Select  Q: Quit");

        int ch = wgetch(mainwin);

        if (ch == KEY_UP && choice > 0)
            choice--;
        else if (ch == KEY_DOWN && choice < (int)menu.size() - 1)
            choice++;
        else if (ch == 10 || ch == KEY_ENTER)
            return (choice == 0) ? Stage::LOCAL_WIPE : Stage::ANDROID;
        else if (ch == 'q' || ch == 'Q')
            return Stage::EXIT;
    }
}
