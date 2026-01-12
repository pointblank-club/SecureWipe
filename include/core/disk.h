#pragma once
#include <string>
#include <vector>

struct Disk
{
    std::string name;
    std::string node;
    std::string model;
    std::string serial;
    std::string size;
    std::string rota;
};

std::vector<Disk> list_disks();
