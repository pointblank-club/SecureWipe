#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class OverwritePatternType
{
    ZERO,
    ONE,
    RANDOM,
    FIXED
};

struct OverwritePass
{
    OverwritePatternType type;
    uint8_t value;
};

struct OverwritePlan
{
    std::vector<OverwritePass> passes;
};

struct WipeResult
{
    int code;
    std::string message;
};

WipeResult wipe_overwrite_passes(const std::string &dev,
                                 const OverwritePlan &plan);

OverwritePlan make_guttmann_plan();
WipeResult wipe_ata_secure(const std::string &dev);
WipeResult wipe_crypto_erase(const std::string &dev);
