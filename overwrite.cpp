#include "wipe_common.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static int fill_random(void *buf, size_t len)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return -1;
    ssize_t r = read(fd, buf, len);
    close(fd);
    return (r == (ssize_t)len) ? 0 : -1;
}

static void fill_pattern(void *buf, size_t len, uint8_t v)
{
    memset(buf, v, len);
}

WipeResult wipe_overwrite_passes(const std::string &dev,
                                 const OverwritePlan &plan)
{
    int fd = open(dev.c_str(), O_WRONLY | O_DIRECT);
    if (fd < 0)
        return {-1, strerror(errno)};

    const size_t BS = 1024 * 1024;
    void *buf = nullptr;

    if (posix_memalign(&buf, 4096, BS) != 0)
    {
        close(fd);
        return {-2, "buffer alloc failed"};
    }

    for (size_t pass = 0; pass < plan.passes.size(); pass++)
    {
        lseek(fd, 0, SEEK_SET);

        while (true)
        {
            auto &p = plan.passes[pass];

            if (p.type == OverwritePatternType::ZERO)
                fill_pattern(buf, BS, 0x00);
            else if (p.type == OverwritePatternType::ONE)
                fill_pattern(buf, BS, 0xFF);
            else if (p.type == OverwritePatternType::FIXED)
                fill_pattern(buf, BS, p.value);
            else if (p.type == OverwritePatternType::RANDOM)
                if (fill_random(buf, BS) < 0)
                    break;

            ssize_t w = write(fd, buf, BS);
            if (w <= 0)
                break;
        }

        fsync(fd);
    }

    free(buf);
    close(fd);

    return {0, "multi-pass overwrite completed"};
}
