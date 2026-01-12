#pragma once
#include <string>
#include <vector>
#include <ncurses.h>
#include "core/disk.h"

void draw_box_with_title(WINDOW *win, const std::string &title, int color_pair);
void draw_header(WINDOW *);
void draw_footer(WINDOW *, const std::string &);
void draw_menu(WINDOW *, int, const std::vector<std::string> &);
void draw_disks(WINDOW *, int, const std::vector<Disk> &);
void draw_progress(WINDOW *, const std::string &, const std::string &, int percent = -1);
void show_result(WINDOW *, bool, const std::string &, const std::string &, const std::string & = "");
void draw_android(WINDOW *, const std::string &, const std::string & = "");
