# SysConfig 配置指南

## 比赛时换引脚

1. 双击 `empty.syscfg`，只修改带逻辑名的资源，不编辑生成的 `ti_msp_dl_config.c/.h`。
2. 保持逻辑名不变，例如 `MOTOR_AIN1`、`TRACK_I2C`、`LCD_SPI`。这样 `Bsp/board_pins.h` 和上层代码通常不用改。
3. 若必须改逻辑名，再同步修改 `Bsp/board_pins.h` 中对应别名。
4. 检查 SysConfig 的 PinMux 冲突与电源/时钟警告。
5. 运行 `Tests/Build/verify_build.ps1`，确认生成、编译、链接全部通过。

## 当前外设配置

- `MOTOR_PWM`：TIMG0，PA12=C0、PA13=C1，周期计数 1000。
- `ENCODER_LEFT_CAPTURE`：TIMA1_C0 / PA17，双边沿捕获中断。
- `ENCODER_RIGHT_CAPTURE`：TIMG8_C1 / PB19，双边沿捕获中断。
- `DEBUG_UART`：UART0，TX=PA10、RX=PA11，115200，RX 中断。
- `IMU_UART`：UART2，PA21/PA22，115200，RX 中断。
- `TRACK_I2C`：I2C0，SDA=PA28、SCL=PA31，Fast 400 kHz，控制器模式。
- `LCD_SPI`：SPI1，PB9/PB8，PICO-only，4 MHz，CPOL=1、CPHA=1。
- `SYSTICK`：1 ms 系统节拍。

PA2 不允许出现任何 `pin.$assign = "PA2"`。八路模块固定使用 I2C0 PA28/PA31。

## 换一种外设

例如更换 LCD：先在 SysConfig 建立新总线和控制引脚，再在 `Hardware/` 新建驱动，保持 `LCD_*` 公共接口，最后在 `Bsp/board_pins.h` 做逻辑名映射。不要让 `User/task.c` 直接调用 `DL_GPIO_*` 或 `DL_SPI_*`。
