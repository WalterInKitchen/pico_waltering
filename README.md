## 说明
这是一个基于raspberrypi pico的自动浇水工具,支持设置浇水周期与时长;

![image01](./assets/image01.jpg)

## 演示

## 引脚定义
LED：SSD1306 128x64

MCU：RP2040

GP8   --------------- I2C0(SDA)

GP9   --------------- I2C0(SCK)

GP14  --------------- (UP_KEY)

GP13  --------------- (OK_KEY)

GP12  --------------- (DOWN_KEY)

GP11  --------------- (CANCEL_KEY)

GP10  --------------- (PUMP OUT)

## 编译

**安装编译依赖**

参考：https://github.com/raspberrypi/pico-sdk

**编译**

```shell
mkdir build
cd build
cmake ..
make
```