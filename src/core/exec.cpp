#include "core/exec.h"

#include <array>
#include <cstdio>
#include <string>

std::string run_cmd(const std::string& cmd) {
    std::array<char, 1024> buf{};
    std::string out;

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp)
        return out;

    while (fgets(buf.data(), buf.size(), fp))
        out += buf.data();

    pclose(fp);
    return out;
}
