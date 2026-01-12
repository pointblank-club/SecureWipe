#include "core/disk.h"

#include <cstdio>
#include <sstream>
#include <map>
#include <vector>

std::vector<Disk> list_disks() {
    std::vector<Disk> disks;

    FILE* fp = popen("lsblk -P -o NAME,TYPE,SIZE,MODEL,SERIAL,ROTA", "r");
    if (!fp)
        return disks;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        std::string l(line);
        std::istringstream ss(l);
        std::string token;
        std::map<std::string, std::string> kv;

        while (ss >> token) {
            auto eq = token.find('=');
            if (eq == std::string::npos)
                continue;

            std::string k = token.substr(0, eq);
            std::string v = token.substr(eq + 1);

            if (!v.empty() && v.front() == '"' && v.back() == '"')
                v = v.substr(1, v.size() - 2);

            kv[k] = v;
        }

        if (kv["TYPE"] != "disk" && kv["TYPE"] != "loop")
            continue;

        Disk d;
        d.name   = kv["NAME"];
        d.node   = "/dev/" + d.name;
        d.model  = kv["MODEL"];
        d.serial = kv["SERIAL"];
        d.size   = kv["SIZE"];
        d.rota   = kv["ROTA"];

        disks.push_back(d);
    }

    pclose(fp);
    return disks;
}
