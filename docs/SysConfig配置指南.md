# SysConfig 配置指南

## 修改原则

SysConfig 负责时钟、引脚复用、GPIO 方向、外设实例和中断；`bsp_config.h` 负责逻辑极性、方向反相、控制周期、尺寸与限幅。生成的 `ti_msp_dl_config.c/.h` 不允许手改。

## 默认配置

- SYSCLK：默认 32 MHz。
- SysTick：1 ms。
- MOTOR_PWM：TIMG0，PA12/PA13，计数周期 1000。
- OLED_I2C：I2C1，PB2/PB3。
- DEBUG_UART：UART0，115200，RX 中断。
- IMU_UART：UART3，115200，RX 中断。
- GPIO_A/GPIO_B：电机方向、编码器、LED、按键和数字寻迹。

## 开启八路模拟 ADC

默认工程为数字模式。使用八路模拟灰度阵列时：

1. 在 SysConfig 添加 ADC12 实例，建议命名 `TRACK_ADC`，选择 ADC0。
2. 选择 8 个支持 ADC 输入且不与 PWM、UART、I2C、SWD 冲突的引脚。
3. 配置 MEM0～MEM7 为重复序列或一次序列，最后一个转换结果启用 `MEM7_RESULT_LOADED` 标志。
4. 采样时间对高阻灰度输出建议从较长档开始，再根据速度优化。
5. 在 `bsp_config.h` 设置：

```c
#define TRACK_ANALOG_MODE 1
#define TRACK_ADC_INST ADC0
```

6. `ADCTrack_Read()` 会启动转换、等待 MEM7 完成并读取 MEM0～MEM7。
7. 上板先显示八路原始值，确认通道顺序，再执行黑白标定。

如果前端模块自带 ADC 并通过串口/I2C 输出，可调用 `Track_SetRawSamples()` 将八路数据送入同一滤波和位置计算链路。

## 切换 OLED SPI

1. 在 SysConfig 禁用或移除 OLED_I2C。
2. 添加 SPI Controller，配置 SCLK、MOSI，SSD1306 不要求 MISO。
3. 添加 CS、DC、RESET 三个输出 GPIO。
4. 在 `bsp_config.h` 设置 `OLED_USE_SPI 1`，并修改 SPI 实例及三个控制引脚宏。
5. `OLED` 与 `SSD1306` 的上层接口无需修改。

## 修改 PWM

PWM 比较值和 `MOTOR_PWM_PERIOD_COUNTS`、`MOTOR_MAX_DUTY` 必须一致。换用其他 Timer 时同步修改 `MOTOR_PWM_INST`、左右 CC 索引，并将 `Motor_Apply()` 中的 TimerG API 改为相同系列 Timer 的 DriverLib API；优先继续选择 TimerG，避免改逻辑。

## 重新生成后的检查

- 没有红色 PinMux 冲突。
- UART RX 中断已启用。
- 编码器 A 相启用双边沿中断。
- 电机方向脚与 STBY 是输出。
- 按键与寻迹脚是输入。
- SWD PA19/PA20 保留。
