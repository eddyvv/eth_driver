#include <stdio.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, char *argv)
{
    int fd = open("/dev/xtenet_eth", O_RDWR);
    if (fd < 0){
        printf("Open Device Failed!\n");
        return -1;
    }
    return 0;
}

