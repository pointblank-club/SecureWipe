#include "wipe/wipe_common.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>

WipeResult wipe_crypto_erase(const std::string &dev)
{
    int fd = open(dev.c_str(), O_RDWR);
    if (fd < 0)
        return {-1, strerror(errno)};

    unsigned long long bytes;
    if (ioctl(fd, BLKGETSIZE64, &bytes) < 0)
    {
        close(fd);
        return {-2, "BLKGETSIZE64 failed"};
    }

    unsigned long long range[2] = {0, bytes};
    if (ioctl(fd, BLKDISCARD, &range) < 0)
    {
        close(fd);
        return {-3, "BLKDISCARD failed"};
    }

    close(fd);
    return {0, "crypto erase / discard completed"};
}
