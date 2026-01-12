#include "core/disk.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

static bool have_lsblk()
{
    return system("lsblk --version >/dev/null 2>&1") == 0;
}

static std::vector<Disk> list_disks_lsblk()
{
    std::vector<Disk> disks;

    FILE* fp = popen("lsblk -P -o NAME,TYPE,SIZE,MODEL,SERIAL,ROTA", "r");
    if (!fp)
        return disks;

    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        std::string l(line);
        std::istringstream ss(l);
        std::string token;
        std::map<std::string, std::string> kv;

        while (ss >> token)
        {
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

static std::vector<Disk> list_disks_proc()
{
    std::vector<Disk> disks;
    std::ifstream f("/proc/partitions");

    if (!f.is_open())
        return disks;

    std::string line;
    while (std::getline(f, line))
    {
        std::istringstream ss(line);
        int major, minor;
        long long blocks;
        std::string name;

        if (!(ss >> major >> minor >> blocks >> name))
            continue;

        // Skip partitions (vda1, sda1, etc)
        if (name.back() >= '0' && name.back() <= '9')
            continue;

        Disk d;
        d.name = name;
        d.node = "/dev/" + name;
        d.size = std::to_string(blocks / 2048) + "M"; // rough
        d.model = "";
        d.serial = "";
        d.rota = "";

        disks.push_back(d);
    }

    return disks;
}

std::vector<Disk> list_disks()
{
    std::vector<Disk> disks;

    if (have_lsblk())
        disks = list_disks_lsblk();

    if (disks.empty())
        disks = list_disks_proc();

    return disks;
}
