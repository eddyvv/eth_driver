#include <stdio.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <fcntl.h>

struct xtic_degug_reg_wr{
    unsigned int addr;
    unsigned int data;
};

#define XILINX_IOC_MAGIC                              'D'
#define XILINX_IOC_READ_REG                       _IOR(XILINX_IOC_MAGIC, 0xc0, unsigned long)
#define XILINX_IOC_WRITE_REG                      _IOW(XILINX_IOC_MAGIC, 0xc1, unsigned long)


int main(int argc, char *argv)
{
    struct xtic_degug_reg_wr *debug_reg;
    int fd = open("/dev/xtenet_eth", O_RDWR);
    if (fd < 0){
        printf("Open Device Failed!\n");
        return -1;
    }

    debug_reg->addr = 0x0;
    debug_reg->data = 0x0;
    if(0 != ioctl(fd, XILINX_IOC_READ_REG, &debug_reg)){
        printf(" ioctl() failed!\n");
        return -1;
    }
    return 0;
}

