# eth_driver
网络设备驱动

## 编译
```bash
$ cd drivers
$ export PWD=$(pwd)
$ make
```
## 安装
```bash
$ cd drivers
#安装所有模块
$ make install
#安装eth
$ make install_eth
#安装xib
$ make install_xib

```
## 卸载
```bash
$ cd drivers
$ make clean
```