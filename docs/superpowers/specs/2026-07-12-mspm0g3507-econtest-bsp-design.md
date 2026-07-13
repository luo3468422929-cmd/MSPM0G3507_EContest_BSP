# MSPM0G3507 电赛 BSP 设计说明

## 目标

在用户创建的 CCS 空工程上构建一套基于 MSPM0 SDK DriverLib、SysConfig 和 TI Clang 的裸机模块库。比赛现场只修改 `empty.syscfg`、`BSP/Config/bsp_config.h` 和应用参数，即可组合电机、编码器、八路寻迹、OLED、UART 惯导及 PID 控制。

## 固定约束

- MCU：MSPM0G3507，LQFP-64。
- SDK：MSPM0 SDK 2.10.0.04。
- 编译器：TI Clang 4.0.4 LTS。
- SysConfig：1.26.2。
- 裸机 NoRTOS，不引入动态内存。
- 函数采用 `模块名_动作()`，不添加统一 `BSP_` 前缀。
- SysConfig 生成文件不手工修改。
- 应用层不直接调用 DriverLib。
- ISR 只完成采集、入队、计数和置标志。

## 分层

1. `BSP`：GPIO、定时器、PWM、ADC、UART、I2C/SPI 等硬件设备驱动。
2. `Components`：PID、滤波、环形缓冲、帧解析、SSD1306 等硬件无关组件。
3. `Services`：速度闭环、寻迹控制和协作式调度。
4. `App`：题目状态机和显示组织。
5. `Examples`、`Tests`：单模块板测例程和宿主机算法测试。

## 数据流

八路传感器经过滤波、阈值和加权位置计算生成偏差；寻迹 PID 将偏差转换为左右轮目标速度；双编码器速度 PID 生成 PWM；Motor 模块驱动 TB6612。UART ISR 将惯导字节放入环形缓冲，主循环解析 5 字节帧并更新统一姿态数据。OLED 以低频任务显示状态。

## 惯导协议

- `5A AA AzL AzH SUM`：Z 轴角速度，`raw / 32768 * 2000 deg/s`。
- `5A BB YawL YawH SUM`：Yaw，`raw / 32768 * 180 deg`。
- `SUM` 为前四字节 8 位累加和。
- 归零：发送解锁、Yaw 归零、保存三条 5 字节命令，命令间隔 100 ms。
- 解析器与 UART 收发解耦，可通过逐字节接口进行宿主机测试。

## 错误与安全

- 公共接口使用 `Status_t` 返回参数、初始化、超时、溢出和校验错误。
- 所有电机输出限幅；初始化和异常路径默认 STBY 关闭、PWM 为零。
- 丢线、惯导超时和显式急停均不在 ISR 中执行复杂逻辑。
- 环形缓冲满时丢弃新字节并累计溢出次数，不覆盖未处理数据。

## 验收

- 纯算法模块通过宿主机测试。
- CCS 工程中的每个模块可独立禁用。
- 完整工程能够由 SysConfig 重新生成并由 TI Clang 编译。
- 提供单模块自测、综合循迹 Demo、引脚表和中文快速上手文档。

