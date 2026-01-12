#include "flow/android_wipe.h"
#include "ui/layout.h"
#include <cstdlib>

Stage run_android_wipe(WINDOW* mainwin, WINDOW* footer) {
    std::string mode = "fastboot";
    std::string detected;

    while (true) {
        draw_android(mainwin, mode, detected);
        draw_footer(footer, "1/2: Mode  R: Scan  ENTER: Wipe  B: Back");

        int ch = wgetch(mainwin);

        if (ch == '1') mode = "fastboot";
        else if (ch == '2') mode = "adb";
        else if (ch == 'b' || ch == 'B')
            return Stage::MAIN_MENU;
        else if (ch == 'q' || ch == 'Q')
            return Stage::EXIT;
        else if (ch == 'r' || ch == 'R') {
            draw_progress(mainwin, "Detecting Devices",
                          "Scanning for " + mode + " devices...");
            system(("bash scripts/detect-android.sh " + mode).c_str());
            detected = "detected-device"; // placeholder
        }
        else if ((ch == 10 || ch == KEY_ENTER) && !detected.empty()) {
            draw_progress(mainwin, "Wiping Android",
                          "Wiping " + detected);
            int rc = system(("bash scripts/android-wipe.sh " +
                              mode + " " + detected).c_str());

            show_result(mainwin, rc == 0,
                        rc == 0 ? "Android Wipe Complete" : "Android Wipe Failed",
                        detected);

            wgetch(mainwin);
            detected.clear();
        }
    }
}
