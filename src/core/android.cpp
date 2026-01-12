#include "core/android.h"
#include "core/exec.h"

#include <sstream>
#include <vector>

std::vector<std::string> list_android_devices() {
    std::vector<std::string> devices;

    std::string out = run_cmd("adb devices -l");
    std::istringstream iss(out);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.find("device") != std::string::npos &&
            line.find("List") == std::string::npos) {
            devices.push_back(line);
            }
    }

    return devices;
}
