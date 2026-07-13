# BSP 移植指南

## 移植边界

比赛现场优先只修改：

1. `empty.syscfg`：引脚和外设实例。
2. `BSP/Config/bsp_config.h`：宏映射、极性、方向和参数。
3. `Services/Src/*.c` 中的 PID 默认参数，或比赛应用中的在线参数接口。
4. `App`：题目状态机。

不要修改 DriverLib、SysConfig 生成文件、PID/滤波/协议算法内部逻辑。

## 迁移现有开源驱动的方法

1. 找出硬件访问代码，将 GPIO、Timer、ADC、UART 操作收进对应 BSP 模块。
2. 将全局变量改为模块私有 `static` 状态，通过 `GetData()` 返回只读快照。
3. 从 ISR 中移除 PID、OLED、延时和 `printf()`，只保留数据搬运与计数。
4. 将 `Car_Move()` 等业务接口拆成 `Motor_SetDutyPair()` 与上层速度服务。
5. 将 PID 参数和历史误差放进 `PID_t`，不要再创建左右轮专用重复函数。
6. 将寻迹的 L1/L2 等硬编码变量改为数组、通道数和权重表。
7. 每迁移一个模块先运行其 ModuleTest，再接入综合 App。

## 极性与方向校准

- 电机正方向错误：修改 `MOTOR_LEFT_REVERSED` 或 `MOTOR_RIGHT_REVERSED`。
- 编码器正负错误：修改 `ENCODER_LEFT_REVERSED` 或 `ENCODER_RIGHT_REVERSED`。
- 黑线输出为低：修改 `TRACK_BLACK_IS_HIGH`。
- LED/按键相反：修改 `LED_ACTIVE_HIGH`、`KEY_ACTIVE_LOW`。

这些调整不需要进入 `.c` 文件。

