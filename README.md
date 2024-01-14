## 准备SDK
### 克隆PICO_SDK到根目录
> git clone https://github.com/raspberrypi/pico-sdk.git --recurse-submodules
### 克隆FreeRTOS到根目录
> git clone https://github.com:FreeRTOS/FreeRTOS.git
### 克隆u8g2到根目录
> git clone https://github.com:olikraus/u8g2.git

## 编译
> mkdir build
> cd build
> cmake ..
> make