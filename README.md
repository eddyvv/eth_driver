# eth_driver

Linux内核版本需为5.10.0

## 编译
```bash
$ cd drivers
# 配置属性，如果为虚拟机下使用 PLATFORM=(uname -r)-vm，实体机使用PLATFORM=(uname -r)
$ export PWD=$(pwd) PLATFORM=(uname -r)
$ make
```
## 安装
```bash
$ cd drivers
#安装所有模块
$ make install
#安装eth
$ make install_eth
```
## 卸载
```bash
$ cd drivers
$ make clean
```

## 编译内核某个模块
```bash
make -C . M=./drivers/infiniband/core modules

sudo cp ./drivers/infiniband/core/*.ko /lib/modules/5.15.0/kernel/drivers/infiniband/core/
```
