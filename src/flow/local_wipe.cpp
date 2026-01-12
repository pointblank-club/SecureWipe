#include "flow/local_wipe.h"

#include <string>

#include "core/disk.h"
#include "ui/layout.h"
#include "ui/colors.h"
#include "wipe/wipe_common.h"
#include "flow/stage.h"
#include "flow/wipe_type.h"

namespace
{
    bool confirm_wipe(WINDOW *win, Disk &d, int &loopIndex)
    {
        bool is_loop = (d.node.rfind("/dev/loop", 0) == 0);

        std::string prompt =
            (!d.serial.empty() && !is_loop)
                ? "Type device SERIAL to confirm wipe:"
                : "Type device node (" + d.node + ") to confirm wipe:";

        werase(win);
        draw_box_with_title(win, "Confirm Device Wipe", COLOR_SUCCESS);

        mvwprintw(win, 3, 4, "Selected: %s", d.node.c_str());
        mvwprintw(win, 4, 4, "Model: %s",
                  d.model.empty() ? "Unknown" : d.model.c_str());
        mvwprintw(win, 5, 4, "Serial: %s",
                  d.serial.empty() ? "N/A" : d.serial.c_str());
        mvwprintw(win, 6, 4, "Size: %s", d.size.c_str());

        wattron(win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
        mvwprintw(win, 8, 4,
                  "WARNING: ALL DATA WILL BE PERMANENTLY ERASED!");
        wattroff(win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);

        mvwprintw(win, 10, 4, "%s", prompt.c_str());
        wrefresh(win);

        echo();
        curs_set(1);

        char buf[256] = {};
        mvwgetnstr(win, 11, 4, buf, sizeof(buf) - 1);

        noecho();
        curs_set(0);

        if (d.serial.empty())
            d.serial = "LOOP-" + std::to_string(loopIndex++);

        std::string confirm(buf);

        return (!is_loop && confirm == d.serial) ||
               (is_loop && confirm == d.node);
    }

    SelectedWipe choose_wipe_method(const Disk &d)
{
    if (d.node.find("nvme") != std::string::npos)
        return SelectedWipe::CRYPTO;

    if (d.node.find("/dev/vd") == 0)
        return SelectedWipe::GUTTMANN;

    if (d.node.find("/dev/loop") == 0)
        return SelectedWipe::GUTTMANN;

    if (d.node.find("/dev/sd") == 0)
        return SelectedWipe::ATA;

    return SelectedWipe::GUTTMANN;
}


    WipeResult execute_wipe(WINDOW *win,
                            const Disk &d,
                            SelectedWipe wipe_type)
    {
        draw_progress(win,
                      "Wiping Device",
                      "Please wait... This may take several minutes");

        if (wipe_type == SelectedWipe::GUTTMANN)
        {
            OverwritePlan plan = make_guttmann_plan();
            return wipe_overwrite_passes(d.node, plan);
        }

        if (wipe_type == SelectedWipe::ATA)
            return wipe_ata_secure(d.node);

        return wipe_crypto_erase(d.node);
    }
}

Stage run_local_wipe(WINDOW *mainwin, WINDOW *footer)
{
    keypad(mainwin, TRUE);
    keypad(footer, TRUE);
    noecho();

    auto disks = list_disks();
    int idx = 0;
    static int loopIndex = 0;

    while (true)
    {
        draw_disks(mainwin, idx, disks);
        draw_footer(footer,
                    "UP/DOWN: Navigate  ENTER: Wipe  R: Refresh  B: Back");

        int ch = wgetch(mainwin);

        if (ch == KEY_UP && idx > 0)
        {
            idx--;
        }
        else if (ch == KEY_DOWN && idx < (int)disks.size() - 1)
        {
            idx++;
        }
        else if (ch == 'r' || ch == 'R')
        {
            disks = list_disks();
            if (idx >= (int)disks.size())
                idx = disks.empty() ? 0 : (int)disks.size() - 1;
        }
        else if (ch == 'b' || ch == 'B')
        {
            return Stage::MAIN_MENU;
        }
        else if (ch == 'q' || ch == 'Q')
        {
            return Stage::EXIT;
        }
        else if (ch == '\n' || ch == KEY_ENTER)
        {
            if (disks.empty())
                continue;

            Disk &d = disks[idx];

            if (!confirm_wipe(mainwin, d, loopIndex))
            {
                show_result(mainwin,
                            false,
                            "Wipe Cancelled",
                            "Confirmation mismatch");
                wgetch(mainwin);
                continue;
            }

            SelectedWipe wipe_type = choose_wipe_method(d);

            WipeResult res =
                execute_wipe(mainwin, d, wipe_type);

            show_result(mainwin,
                        res.code == 0,
                        res.code == 0 ? "Wipe Complete" : "Wipe Failed",
                        res.message);

            wgetch(mainwin);
            disks = list_disks();
        }
    }
}
