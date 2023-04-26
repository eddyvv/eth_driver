#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
struct xtic_degug_reg_wr{
    unsigned int addr;
    unsigned int data;
};

struct s_read_reg{
    int len;
    int addr[100];
    int val[100];
};

#define XILINX_IOC_MAGIC                              'D'
#define XILINX_IOC_READ_REG                       _IOR(XILINX_IOC_MAGIC, 0xc0, unsigned long)
#define XILINX_IOC_WRITE_REG                      _IOW(XILINX_IOC_MAGIC, 0xc1, unsigned long)
#define XILINX_IOC_READ_REG_ALL                   _IOR(XILINX_IOC_MAGIC, 0xc2, unsigned long)

void add_spaces(char* str) {
    int len =32;
    int i, j;
    for (i = len - 1, j = 0; i >= 0; i--, j++) {
        if (j % 4 == 0 && j != 1) {
            memmove(str + i + 1, str + i, len - i);
            str[i] = ' ';
            len++;
        }
    }
}

char* itoa(int num, char* str, int base) {
    int i = 0, b = 4;
    int is_negative = 0;

    // 处理负数情况
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    // 将数字转换成字符
    do {
        int digit = num % base;
        str[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        num /= base;
        if(i == 4 || i == 8)
            b = 4;
        else
            b = 5;
        if (i % b == 0 && num > 0) {
            str[i++] = ' ';
        }
    } while (num > 0);
    // add_spaces(str);
    // 如果是负数，添加负号
    if (is_negative) {
        str[i++] = '-';
    }

    // 反转字符串
    int j;
    for (j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }

    // 添加字符串结束符
    str[i] = '\0';

    return str;
}

int read_reg_val(int fd, struct s_read_reg *xxv)
{
    if(0 != ioctl(fd, XILINX_IOC_READ_REG_ALL, xxv)){
        printf(" ioctl() failed!\n");
        return -1;
    }
}

void print_reg_val(struct s_read_reg *xxv)
{
    char s[10];
    printf("read xxv reg\n");
    printf("addr\tval(H)\tval(b)\n\n");
    for(int i=0; i < xxv->len; i++){
        itoa(xxv->val[i], s, 2);
        printf("0x%x\t0x%x\t0b%s\n", xxv->addr[i], xxv->val[i], s);
    }
    printf("\n\n");
}

int read_reg_test(int fd, struct xtic_degug_reg_wr *debug_reg)
{
    debug_reg->addr = 0x0;
    debug_reg->data = 0x0;
    if(0 != ioctl(fd, XILINX_IOC_READ_REG, debug_reg)){
        printf(" ioctl() failed!\n");
        return -1;
    }
}
#define XXV_REG 1
#define AXIDMA_REG 2
void set_reg_addr(int cmd, struct s_read_reg *xxv)
{
    switch(cmd)
    {
        case XXV_REG:
            xxv->len = 9;
            break;
        case AXIDMA_REG:
            xxv->len = 9;
            break;
        default:
            xxv->len = 9;
        break;
    }
    for(int i = 0;i < xxv->len; i++){
        xxv->addr[i] = i*4;
    }
}

int main(int argc, char *argv)
{
    int i;
    struct s_read_reg xxv;

    int fd = open("/dev/xtenet_eth", O_RDWR);
    if (fd < 0){
        printf("Open Device Failed!\n");
        return -1;
    }

    set_reg_addr(XXV_REG, &xxv);
    read_reg_val(fd, &xxv);
    print_reg_val(&xxv);
    // printf("read addr = 0x%x, data = 0x%x\n", debug_reg.addr, debug_reg.data);
    return 0;
}

