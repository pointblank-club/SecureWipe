#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <ncurses.h>
#include <map>
#include <array>
#include <unistd.h>
#include <cstring>
#include "wipe_common.h"

enum class SelectedWipe
{
    GUTTMANN,
    ATA,
    CRYPTO
};

struct Disk
{
    std::string name;
    std::string node;
    std::string model;
    std::string serial;
    std::string size;
    std::string rota;
};

// Color pairs
#define COLOR_TITLE 1
#define COLOR_SUCCESS 2
#define COLOR_WARNING 3
#define COLOR_ERROR 4
#define COLOR_INFO 5
#define COLOR_HIGHLIGHT 6

std::string run_cmd(const std::string &cmd) {
    std::array<char, 1024> buf{};
    std::string out;
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return out;
    while (fgets(buf.data(), (int)buf.size(), fp)) out += buf.data();
    pclose(fp);
    return out;
}

// int run_system(const std::string &cmd) {
//     return system(cmd.c_str());
// }

// std::string run_cmd_capture(const std::string &cmd) {
//     return run_cmd(cmd);
// }

void split_tokens(const std::string &line, std::map<std::string, std::string> &m)
{
    std::istringstream ls(line);
    std::string token;
    while (ls >> token)
    {
        auto eq = token.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = token.substr(0, eq);
        std::string val = token.substr(eq + 1);
        if (val.size() && val.front() == '"' && val.back() == '"')
            val = val.substr(1, val.size() - 2);
        m[key] = val;
    }
}

std::vector<Disk> list_disks()
{
    std::vector<Disk> disks;
    FILE *fp = popen("lsblk -P -o NAME,TYPE,SIZE,MODEL,SERIAL,ROTA", "r");
    if (!fp)
        return disks;
    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        std::string l(line);
        std::map<std::string, std::string> kv;
        std::stringstream ss(l);
        std::string token;
        while (ss >> token)
        {
            auto eq = token.find('=');
            if (eq == std::string::npos)
                continue;
            std::string k = token.substr(0, eq);
            std::string v = token.substr(eq + 1);
            if (v.size() && v.front() == '"' && v.back() == '"')
                v = v.substr(1, v.size() - 2);
            kv[k] = v;
        }
        if (kv["TYPE"] != "disk" && kv["TYPE"] != "loop")
            continue;
        Disk d;
        d.name = kv["NAME"];
        d.node = "/dev/" + d.name;
        d.model = kv["MODEL"];
        d.serial = kv["SERIAL"];
        d.size = kv["SIZE"];
        d.rota = kv["ROTA"];
        disks.push_back(d);
    }
    pclose(fp);
    return disks;
}

std::vector<std::string> list_android_devices()
{
    std::string res = run_cmd("adb devices -l");
    std::istringstream iss(res);
    std::string line;
    std::vector<std::string> devs;
    while (std::getline(iss, line))
    {
        if (line.find("device") != std::string::npos && line.find("List") == std::string::npos)
            devs.push_back(line);
    }
    return devs;
}

void draw_box_with_title(WINDOW *win, const std::string &title, int color_pair = COLOR_TITLE)
{
    box(win, 0, 0);
    if (!title.empty())
    {
        wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
        mvwprintw(win, 0, 3, " %s ", title.c_str());
        wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
    }
}

void draw_header(WINDOW *win)
{
    werase(win);
    wattron(win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    int width = getmaxx(win);
    std::string title = "VOID - SECURE WIPE UTILITY";
    int x = (width - title.length()) / 2;
    mvwprintw(win, 1, x, "%s", title.c_str());
    wattroff(win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);

    std::string subtitle = "Secure Device Erasure & Attestation";
    x = (width - subtitle.length()) / 2;
    mvwprintw(win, 2, x, "%s", subtitle.c_str());

    wrefresh(win);
}

void draw_footer(WINDOW *win, const std::string &text)
{
    werase(win);
    mvwprintw(win, 0, 2, "%s", text.c_str());
    wrefresh(win);
}

void draw_menu(WINDOW *win, int highlight, const std::vector<std::string> &choices)
{
    werase(win);
    draw_box_with_title(win, "Select Wipe Mode", COLOR_TITLE);

    int start_y = 3;
    for (size_t i = 0; i < choices.size(); i++)
    {
        if ((int)i == highlight)
        {
            wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
            mvwprintw(win, start_y + (int)i * 2, 4, "> %s", choices[i].c_str());
            wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
        }
        else
        {
            mvwprintw(win, start_y + (int)i * 2, 4, "  %s", choices[i].c_str());
        }
    }

    mvwprintw(win, getmaxy(win) - 2, 2, "UP/DOWN: Navigate  ENTER: Select  Q: Quit");

    wrefresh(win);
}

void draw_disks(WINDOW *win, int highlight, const std::vector<Disk> &disks)
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
            {
                wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
            }

            std::string disk_type = (disks[i].rota == "1") ? "[HDD]" : "[SSD]";
            if (disks[i].node.find("nvme") != std::string::npos)
                disk_type = "[NVMe]";
            if (disks[i].node.find("loop") != std::string::npos)
                disk_type = "[Loop]";

            mvwprintw(win, start_y + (int)i * 3, 4, "> %s %s", disks[i].node.c_str(), disk_type.c_str());
            mvwprintw(win, start_y + (int)i * 3 + 1, 6, "Model: %s | Size: %s",
                      disks[i].model.empty() ? "Unknown" : disks[i].model.c_str(),
                      disks[i].size.c_str());

            if ((int)i == highlight)
            {
                wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_REVERSE | A_BOLD);
            }
        }
    }

    mvwprintw(win, getmaxy(win) - 2, 2, "UP/DOWN: Navigate  ENTER: Wipe  R: Refresh  B: Back");

    wrefresh(win);
}

void draw_progress(WINDOW *win, const std::string &title, const std::string &message, int percent = -1)
{
    werase(win);
    draw_box_with_title(win, title, COLOR_TITLE);

    int center_y = getmaxy(win) / 2;
    int center_x = (getmaxx(win) - message.length()) / 2;

    wattron(win, A_BOLD);
    mvwprintw(win, center_y, center_x, "%s", message.c_str());
    wattroff(win, A_BOLD);

    if (percent >= 0)
    {
        int bar_width = 40;
        int filled = (bar_width * percent) / 100;
        int bar_x = (getmaxx(win) - bar_width - 2) / 2;

        mvwprintw(win, center_y + 2, bar_x, "[");
        for (int i = 0; i < filled; i++)
        {
            mvwprintw(win, center_y + 2, bar_x + 1 + i, "=");
        }
        for (int i = filled; i < bar_width; i++)
        {
            mvwprintw(win, center_y + 2, bar_x + 1 + i, "-");
        }
        mvwprintw(win, center_y + 2, bar_x + bar_width + 1, "]");
        mvwprintw(win, center_y + 3, (getmaxx(win) - 4) / 2, "%d%%", percent);
    }

    wrefresh(win);
}

void show_result(WINDOW *win, bool success, const std::string &title, const std::string &message, const std::string &details = "")
{
    werase(win);
    draw_box_with_title(win, title, success ? COLOR_SUCCESS : COLOR_ERROR);

    int y = 3;
    int color = success ? COLOR_SUCCESS : COLOR_ERROR;

    wattron(win, COLOR_PAIR(color) | A_BOLD);
    std::string status = success ? "[SUCCESS]" : "[FAILED]";
    int x = (getmaxx(win) - status.length()) / 2;
    mvwprintw(win, y, x, "%s", status.c_str());
    wattroff(win, COLOR_PAIR(color) | A_BOLD);

    y += 2;
    x = (getmaxx(win) - message.length()) / 2;
    mvwprintw(win, y, x, "%s", message.c_str());

    if (!details.empty())
    {
        y += 2;
        mvwprintw(win, y, 4, "Details:");
        y++;
        std::istringstream iss(details);
        std::string line;
        while (std::getline(iss, line) && y < getmaxy(win) - 4)
        {
            mvwprintw(win, y++, 6, "%s", line.c_str());
        }
    }

    mvwprintw(win, getmaxy(win) - 2, 2, "Press any key to continue...");

    wrefresh(win);
}

void draw_android(WINDOW *win, const std::string &mode, const std::string &device_info = "")
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

    y += 2;
    mvwprintw(win, y++, 4, "Options:");
    mvwprintw(win, y++, 6, "[1] Switch to fastboot mode");
    mvwprintw(win, y++, 6, "[2] Switch to ADB mode");
    mvwprintw(win, y++, 6, "[R] Rescan devices");
    if (!device_info.empty())
    {
        mvwprintw(win, y++, 6, "[ENTER] Wipe device");
    }

    mvwprintw(win, getmaxy(win) - 2, 2, "B: Back  Q: Quit");

    wrefresh(win);
}

int main()
{
    initscr();
    start_color();
    use_default_colors();

    init_pair(COLOR_TITLE, COLOR_WHITE, -1);
    init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
    init_pair(COLOR_WARNING, COLOR_YELLOW, -1);
    init_pair(COLOR_ERROR, COLOR_RED, -1);
    init_pair(COLOR_INFO, COLOR_WHITE, -1);
    init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_GREEN);

    noecho();
    cbreak();
    curs_set(0);

    int h, w;
    getmaxyx(stdscr, h, w);

    WINDOW *header = newwin(4, w, 0, 0);
    WINDOW *mainwin = newwin(h - 6, w, 4, 0);
    WINDOW *footer = newwin(2, w, h - 2, 0);

    keypad(mainwin, TRUE);

    draw_header(header);

    std::vector<std::string> menu = {
        "Local Disks (NVMe, SSD, HDD)",
        "Android / USB Devices"};
    int menu_choice = 0;
    int stage = 0;

    while (true)
    {
        if (stage == 0)
        {
            draw_menu(mainwin, menu_choice, menu);
            draw_footer(footer, "Use arrow keys to navigate, press ENTER to select, Q to quit");

            int ch = wgetch(mainwin);
            if (ch == KEY_UP && menu_choice > 0)
                menu_choice--;
            else if (ch == KEY_DOWN && menu_choice < (int)menu.size() - 1)
                menu_choice++;
            else if (ch == 10 || ch == KEY_ENTER)
            {
                stage = (menu_choice == 0) ? 1 : 2;
            }
            else if (ch == 'q' || ch == 'Q')
                break;
        }

        else if (stage == 1)
        { // local disks
            auto disks = list_disks();
            int idx = 0;
            static int loopIndex = 0;

            while (true)
            {
                draw_disks(mainwin, idx, disks);
                draw_footer(footer, "Select a disk to wipe | R: Refresh | B: Back to menu");

                int ch = wgetch(mainwin);

                if (ch == KEY_UP && idx > 0)
                    idx--;
                else if (ch == KEY_DOWN && idx < (int)disks.size() - 1)
                    idx++;
                else if (ch == 'r' || ch == 'R')
                {
                    disks = list_disks();
                    if (!disks.empty() && idx >= (int)disks.size())
                        idx = (int)disks.size() - 1;
                }
                else if (ch == 'b' || ch == 'B')
                {
                    stage = 0;
                    break;
                }
                else if (ch == 'q' || ch == 'Q')
                {
                    endwin();
                    return 0;
                }
                else if (ch == 10 || ch == KEY_ENTER)
                {
                    if (disks.empty())
                        continue;

                    Disk &d = disks[idx];
                    bool is_loop = (d.node.rfind("/dev/loop", 0) == 0);

                    /* -------- confirmation prompt -------- */

                    std::string confirm_prompt;
                    if (!d.serial.empty() && !is_loop)
                        confirm_prompt = "Type device SERIAL to confirm wipe:";
                    else
                        confirm_prompt = "Type device node (" + d.node + ") to confirm wipe:";

                    werase(mainwin);
                    draw_box_with_title(mainwin, "Confirm Device Wipe", COLOR_SUCCESS);

                    mvwprintw(mainwin, 3, 4, "Selected: %s", d.node.c_str());
                    mvwprintw(mainwin, 4, 4, "Model:    %s",
                              d.model.empty() ? "Unknown" : d.model.c_str());
                    mvwprintw(mainwin, 5, 4, "Serial:   %s",
                              d.serial.empty() ? "N/A" : d.serial.c_str());
                    mvwprintw(mainwin, 6, 4, "Size:     %s", d.size.c_str());

                    wattron(mainwin, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
                    mvwprintw(mainwin, 8, 4,
                              "WARNING: ALL DATA WILL BE PERMANENTLY ERASED!");
                    wattroff(mainwin, COLOR_PAIR(COLOR_ERROR) | A_BOLD);

                    mvwprintw(mainwin, 10, 4, "%s", confirm_prompt.c_str());
                    wrefresh(mainwin);

                    echo();
                    curs_set(1);
                    char buf[256] = {};
                    mvwgetnstr(mainwin, 11, 4, buf, sizeof(buf) - 1);
                    noecho();
                    curs_set(0);

                    std::string confirm(buf);
                    bool confirmed = false;

                    if (d.serial.empty())
                        d.serial = "LOOP-" + std::to_string(loopIndex++);

                    if (!is_loop && confirm == d.serial)
                        confirmed = true;
                    else if (is_loop && confirm == d.node)
                        confirmed = true;

                    if (!confirmed)
                    {
                        show_result(mainwin, false, "Wipe Cancelled",
                                    "Confirmation mismatch",
                                    "Entered value did not match the required identifier.");
                        wgetch(mainwin);
                        continue;
                    }

                    /* -------- select wipe strategy -------- */

                    SelectedWipe wipe_type;

                    if (d.node.find("nvme") != std::string::npos)
                        wipe_type = SelectedWipe::CRYPTO;
                    else if (d.node.find("sd") != std::string::npos ||
                             d.model.find("ATA") != std::string::npos)
                        wipe_type = SelectedWipe::ATA;
                    else
                        wipe_type = SelectedWipe::GUTTMANN;

                    const char *method_str =
                        (wipe_type == SelectedWipe::GUTTMANN) ? "Overwrite (Guttmann 35-pass)" : (wipe_type == SelectedWipe::ATA) ? "ATA Secure Erase"
                                                                                                                                  : "Crypto Erase (discard)";

                    werase(mainwin);
                    draw_box_with_title(mainwin, "Confirm Wipe Method", COLOR_TITLE);
                    mvwprintw(mainwin, 3, 4, "Device: %s", d.node.c_str());
                    mvwprintw(mainwin, 4, 4, "Method: %s", method_str);
                    mvwprintw(mainwin, 6, 4, "Press ENTER to proceed, B to cancel");
                    wrefresh(mainwin);

                    int okc = wgetch(mainwin);
                    if (okc == 'b' || okc == 'B')
                        continue;
                    if (!(okc == 10 || okc == KEY_ENTER))
                        continue;

                    /* -------- execute wipe -------- */

                    draw_progress(mainwin, "Wiping Device",
                                  "Please wait... This may take several minutes");
                    wrefresh(mainwin);

                    WipeResult res;

                    if (wipe_type == SelectedWipe::GUTTMANN)
                    {
                        OverwritePlan plan = make_guttmann_plan();
                        res = wipe_overwrite_passes(d.node, plan);
                    }
                    else if (wipe_type == SelectedWipe::ATA)
                    {
                        res = wipe_ata_secure(d.node);
                    }
                    else
                    {
                        res = wipe_crypto_erase(d.node);
                    }

                    /* -------- post-wipe result -------- */

                    auto refreshed = list_disks();
                    bool still_exists = false;
                    for (auto &x : refreshed)
                    {
                        if (x.node == d.node)
                        {
                            still_exists = true;
                            break;
                        }
                    }

                    if (res.code == 0 && !still_exists)
                    {
                        show_result(mainwin, true, "Wipe Complete",
                                    "Device successfully wiped and removed",
                                    res.message);
                    }
                    else if (res.code == 0)
                    {
                        show_result(mainwin, true, "Wipe Complete",
                                    "Device wiped but still visible (expected for this media)",
                                    res.message);
                    }
                    else
                    {
                        show_result(mainwin, false, "Wipe Failed",
                                    "Wipe engine returned an error",
                                    res.message);
                    }

                    wgetch(mainwin);

                    disks = list_disks();
                    if (!disks.empty() && idx >= (int)disks.size())
                        idx = (int)disks.size() - 1;
                }
            }
        }

        else if (stage == 2)
        {
            std::string mode = "fastboot";
            std::string detected_device = "";

            while (true)
            {
                draw_android(mainwin, mode, detected_device);
                draw_footer(footer, "1/2: Mode | R: Scan | ENTER: Wipe | B: Back");

                int ch = wgetch(mainwin);
                if (ch == '1')
                    mode = "fastboot";
                else if (ch == '2')
                    mode = "adb";
                else if (ch == 'b' || ch == 'B')
                {
                    stage = 0;
                    break;
                }
                else if (ch == 'q' || ch == 'Q')
                {
                    endwin();
                    return 0;
                }

                else if (ch == 'r' || ch == 'R' || (ch == 10 && detected_device.empty()))
                {
                    draw_progress(mainwin, "Detecting Android Devices",
                                  "Scanning for " + mode + " devices...");

                    std::string detect_cmd = "bash /opt/sentinel/scripts/detect-android.sh " + mode + " > /tmp/sentinel-detect.log 2>&1";
                    system(detect_cmd.c_str());

                    std::ifstream log("/tmp/sentinel-detect.log");
                    std::string serial, line;
                    if (log.is_open())
                    {
                        while (std::getline(log, line))
                        {
                            if (line.find("Found") != std::string::npos)
                            {
                                auto pos = line.find_last_of(' ');
                                if (pos != std::string::npos)
                                    serial = line.substr(pos + 1);
                                break;
                            }
                        }
                        log.close();
                    }

                    detected_device = serial;

                    if (detected_device.empty())
                    {
                        show_result(mainwin, false, "Detection Failed",
                                    "No " + mode + " device found",
                                    "Make sure device is connected and in correct mode");
                        wgetch(mainwin);
                    }
                }
                else if (ch == 10 || ch == KEY_ENTER)
                {
                    if (detected_device.empty())
                        continue;

                    draw_progress(mainwin, "Wiping Android Device",
                                  "Wiping " + detected_device + " via " + mode);

                    std::string wipe_cmd = "bash /opt/sentinel/scripts/android-wipe.sh " + mode + " " + detected_device + " > /tmp/sentinel-android.log 2>&1";
                    int rc = system(wipe_cmd.c_str());

                    std::string details = "Device: " + detected_device + "\nMode: " + mode + "\nLog: /tmp/sentinel-android.log";
                    show_result(mainwin, rc == 0, "Android Wipe",
                                rc == 0 ? "Device wipe completed" : "Wipe operation failed",
                                details);

                    wgetch(mainwin);
                    detected_device = "";
                }
            }
        }
    }

    delwin(header);
    delwin(mainwin);
    delwin(footer);
    endwin();
    return 0;
}