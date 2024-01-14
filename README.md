## 说明
这是一个基于raspberrypi pico的自动浇水工具
支持设置浇水周期与时长

## 演示

## 引脚定义

## 更新依赖
```shell
git submodule update --init --recursive
```
或者执行
```shell
git clone git@github.com:FreeRTOS/FreeRTOS.git --recurse-submodules

git clone git@github.com:raspberrypi/pico-sdk.git --recurse-submodules

git clone git@github.com:olikraus/u8g2.git
```

## 编译
```shell
mkdir build
cd build
cmake ..
make
```