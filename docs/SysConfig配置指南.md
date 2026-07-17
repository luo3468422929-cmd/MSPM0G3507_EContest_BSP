# SysConfig 配置指南

## 正确修改方式

1. 双击根目录 `empty.syscfg`；
2. 修改带逻辑名的引脚或外设实例；
3. 尽量保持 `MOTOR_AIN1`、`TRACK_I2C`、`LCD_SPI` 等逻辑名；
4. 保存后让 CCS/SysConfig 重新生成；
5. 运行 `Tests/Build/verify_build.ps1`；
6. 不编辑 `Debug/syscfg/ti_msp_dl_config.c/.h`。

若逻辑名也改变，再同步 `Bsp/board_pins.h` 的别名。

## 当前配置

- PWM：TIMG0，PA12=C0、PA13=C1，周期 1000。
- 编码器：PA17/PA16、PB19/PB20 均为 GPIO 输入、内部上拉、上升沿中断。
- UART0 调试：PA10 TX、PA11 RX，115200 RX 中断，板载 CH340。
- UART2 惯导：PA21 TX、PA22 RX，115200 RX 中断。
- I2C0 循迹：PA28 SDA、PA31 SCL，Fast 400 kHz。
- SPI1 LCD：PB9 SCK、PB8 MOSI，4 MHz，CPOL=1、CPHA=1。
- LCD 控制：PA27 CS、PA26 DC、PA25 RES；BL 直连 3.3 V。
- SysTick：1 ms。

## 保留引脚

- PA0、PA1：当前不配置，也不作为现场首选 I2C；确需使用时确认开发板连接并配置外部上拉。
- PA2：地猛星 ROSC 相关，保持不分配。
- PA24：LCD 不再使用，当前留空。
- PA19/PA20：SWD 调试。

## 换一种器件

先在 SysConfig 建立总线和控制脚，再在 `Hardware` 增加驱动。若能保持现有公共接口（例如继续提供 `Track_Update()` / `Track_GetData()`），`Control` 和比赛状态机可以不随协议变化。
