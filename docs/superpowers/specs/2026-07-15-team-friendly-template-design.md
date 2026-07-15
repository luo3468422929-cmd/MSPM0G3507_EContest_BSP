# MSPM0G3507 电赛小白友好模板重构设计

## 目标

将现有 `MSPM0G3507_EContest_BSP` 重构为适合电赛小队协作的简洁模板。队友日常只需修改 SysConfig、用户参数、比赛状态机和单模块测试入口；底层仍保留 TI DriverLib、模块边界和自动编译验证。

## 范围

- 将当前 `App`、`BSP`、`Components`、`Services`、`Examples` 的可见结构收拢为 `User`、`Bsp`、`Hardware`、`Control`。
- 基于最终无冲突引脚表迁移 TB6612、双编码器、UART2 串口惯导和 ST7735S LCD。
- 在 12 路寻迹模块到货前，支持五路 GPIO 数字寻迹。
- 以 ST7735S SPI LCD 替换 SSD1306 OLED 显示链路。
- 保留 PID、滤波、环形缓冲和协议解析的可靠实现；将其收进所属模块，不要求队友日常接触。
- 更新 CCS、VS Code、自动构建、主机算法测试和入门文档。

本次不实现尚未到货的 12 路寻迹模块协议。到货后只新增寻迹输入适配层，不修改 `User/task.c` 和 `Control` 的寻迹控制接口。

## 目录与职责

```text
User/
  main.c            唯一主函数
  task.c/.h         比赛题目状态机、任务循环
  user_config.h     队友常调参数和模块开关
  test.c/.h         单模块测试选择入口
  project.h         User 层唯一总头文件

Bsp/
  board.c/.h        SysConfig 初始化、公共状态和急停
  timer.c/.h        1 ms 时基与非阻塞定时
  uart.c/.h         UART0 调试和 UART2 惯导字节收发
  spi.c/.h          SPI1 阻塞式发送与超时保护

Hardware/
  motor.c/.h        TB6612 双电机
  encoder.c/.h      双编码器采集与速度换算
  track.c/.h        五路数字循迹、滤波、标定、位置误差
  imu.c/.h          串口惯导缓存和帧解析
  lcd.c/.h          ST7735S 初始化、RGB565 绘图与文本
  key.c/.h          单按键消抖
  led.c/.h          LED

Control/
  pid.c/.h          位置式与增量式 PID
  car_control.c/.h  寻迹方向环、左右速度环、停车/急停接口
  scheduler.c/.h    5 ms、10 ms、20 ms、200 ms 周期判断
```

`Tests` 与 `docs` 不属于固件源码目录。现有的协议、环形缓冲和滤波实现可作为模块私有源文件保留，但不会再以 `Components`、`Services`、`Examples` 作为小队日常入口。

## 分层规则

```text
User -> Control -> Hardware -> Bsp -> TI DriverLib / SysConfig
```

- `User` 不调用 `DL_*`，只调用 Hardware 与 Control 的语义接口。
- `Control` 不访问 MCU 寄存器或 GPIO，只读取 Hardware 数据并输出控制目标。
- `Hardware` 实现具体模块协议，例如 TB6612、ST7735S、五路循迹和惯导。
- `Bsp` 只负责 MCU 外设、时基、总线和 SysConfig 生成对象。
- `User/project.h` 可包含所有 User 可用接口；非 User 源文件必须精确包含所需头文件，不使用全局大头文件。

## 固定硬件配置

### TB6612 与编码器

| 信号 | 引脚/外设 |
|---|---|
| PWMA / PWMB | PA12 / PA13，TIMG0_C0 / TIMG0_C1 |
| STBY | PB24 |
| AIN1 / AIN2 | PB18 / PA7 |
| BIN1 / BIN2 | PA8 / PA9 |
| 左编码器 A / B | PA17 / PA16，TIMA1_C0 + GPIO |
| 右编码器 A / B | PB19 / PB20，TIMG8_C1 + GPIO |

### 惯导与 LCD

| 模块 | 信号 | 引脚/外设 |
|---|---|---|
| 串口惯导 | 模块 TX / MCU TX | PA22 UART2_RX / PA21 UART2_TX |
| ST7735S LCD | SCK / MOSI | PB9 SPI1_SCK / PB8 SPI1_PICO |
| ST7735S LCD | CS / DC / RES / BL | PA27 / PA26 / PA25 / PA24 |

LCD 使用 128x160 RGB565、SPI mode 3、8 位 MSB first、软件 CS。首次上板使用 4 MHz；不使用完整帧缓冲，不实现 STM32 DMA 代码。

### 临时五路循迹与按键

| 用途 | 引脚 | 说明 |
|---|---|---|
| 五路循迹 CH1 / CH2 | PA0 / PA1 | 临时占用 I2C0 可选脚 |
| 五路循迹 CH3 | PA15 | 通过飞线替代原 PCB 上危险的 PA2 连接 |
| 五路循迹 CH4 / CH5 | PB6 / PB7 | 数字 GPIO 输入 |
| 临时用户按键 | PB2 | 内部上拉，按键另一端接 GND，低有效 |
| 禁止使用 | PA2 | 板载 ROSC 电阻相关引脚，永久保留 |

五路模块输出极性未知。`User/user_config.h` 必须提供 `TRACK_BLACK_IS_HIGH` 配置；`User/test.c` 必须提供可观察五路原始状态与判定状态的测试入口。

12 路模块到货后：拆除五路模块，PB2/PB3 改为 UART3；临时按键的杜邦线改接 PA15。

## 对外接口与任务模型

主函数固定为：

```c
int main(void)
{
    System_Init();
    Task_Init();
    while (1) {
        Task_Run();
        __WFI();
    }
}
```

`Task_Run()` 使用协作式周期任务：5 ms 按键、10 ms 寻迹/控制/电机、20 ms 惯导与命令、200 ms LCD 状态。中断仅负责 SysTick、编码器计数和 UART 收字节；不在中断中执行 PID、LCD 刷新或阻塞延时。

用户常用配置仅在 `User/user_config.h`：模块开关、传感器极性、机械参数、PID 参数、基础速度和状态机阈值。真实引脚复用仅在 `empty.syscfg` 中修改；生成的 `ti_msp_dl_config.*` 不得编辑。

## 验收标准

- SysConfig 与 TI Clang 完整构建成功，旧 `App`、`Components`、`Services`、`Examples` 固件入口不再参与默认工程。
- 主机测试覆盖 PID、五路寻迹位置计算、环形缓冲、惯导协议和调度周期。
- 自动静态检查确认：PA2 未被配置为外设输入输出；五路寻迹为 PA0/PA1/PA15/PB6/PB7；按键为 PB2；LCD 为 SPI1 PB8/PB9 + PA24~PA27。
- 上板测试顺序：LCD 色块与文本、五路黑白极性、按键、双电机架空、双编码器方向、UART2 惯导。
- `README.md` 首屏明确告诉队友：日常只改 `empty.syscfg`、`User/user_config.h`、`User/task.c`、`User/test.c`。
