#include "wipe_common.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/hdreg.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

static int ata_cmd(int fd, unsigned char cmd, unsigned char *data)
{
    unsigned char args[4 + 512] = {0};
    args[0] = cmd;
    memcpy(args + 4, data, 512);
    return ioctl(fd, HDIO_DRIVE_CMD, args);
}

WipeResult wipe_ata_secure(const std::string &dev)
{
    int fd = open(dev.c_str(), O_RDWR);
    if (fd < 0)
        return {-1, strerror(errno)};

    unsigned char pass[512] = {0};
    strcpy((char *)pass, "P"); // minimal password

    if (ata_cmd(fd, 0xF1, pass) < 0)
    {
        close(fd);
        return {-2, "SECURITY_SET_PASSWORD failed"};
    }

    if (ata_cmd(fd, 0xF4, pass) < 0)
    {
        close(fd);
        return {-3, "SECURITY_ERASE_UNIT failed"};
    }

    close(fd);
    return {0, "ATA secure erase issued"};
}
