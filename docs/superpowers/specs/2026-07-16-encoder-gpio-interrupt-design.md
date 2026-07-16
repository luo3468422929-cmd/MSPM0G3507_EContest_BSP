# 编码器 GPIO 中断测速设计

## 目标

将当前编码器 A 相的 TIMA1/TIMG8 定时器捕获中断改为 GPIO 端口上升沿中断，复用参考工程的实现方式，同时保持已经验证过的 PA17/PA16、PB19/PB20 引脚和固定时间窗测速算法不变。

## 现状与边界

- 左编码器：A=PA17，B=PA16。
- 右编码器：A=PB19，B=PB20。
- 当前 A 相由 `CAPTURE1/CAPTURE2` 产生 TIMA1/TIMG8 捕获中断。
- 速度计算已经使用默认 50 ms 计数窗口，不能回退为单次 10 ms 速度。
- GPIO 中断只负责 A 相边沿计数，B 相仍在中断服务函数中读取并判定方向。

## 设计方案

### SysConfig

删除两个 CAPTURE 实例，把 PA17 加入 GPIOA 组、PB19 加入 GPIOB 组，并配置为：

- 数字输入；
- 上升沿中断；
- 不改变物理引脚编号；
- B 相保持普通数字输入。

SysConfig 生成的宏将由 `GPIO_A_ENCODER_LEFT_A_PIN` 和 `GPIO_B_ENCODER_RIGHT_A_PIN` 提供。

### BSP 映射层

`Bsp/board_pins.h` 删除 capture 实例和 capture IRQ 宏，改为 GPIO 端口、A 相引脚宏。编码器模块只依赖这些逻辑宏，不直接写 PA/PB 物理编号。

### 编码器中断层

`Hardware/encoder.c` 使用 `GROUP1_IRQHandler()`：

1. 读取 GPIOA 的待处理中断，若为左 A 相则读取左 B 相并调用 `Encoder_OnEdge()`；
2. 读取 GPIOB 的待处理中断，若为右 A 相则读取右 B 相并调用 `Encoder_OnEdge()`；
3. 清除对应 GPIO 中断标志；
4. `Encoder_Init()` 只使能 GPIOA/GPIOB 中断，不再启动或使能捕获定时器。

## 数据流

```text
A 相上升沿
  -> GPIOA/GPIOB 端口中断
  -> 读取对应 B 相
  -> Encoder_OnEdge() 累计有符号位置计数
  -> Encoder_UpdateSpeed() 统计最近 50 ms 脉冲
  -> PID 使用 rpm 和线速度
```

## 验证标准

- SysConfig 不再生成 `ENCODER_LEFT_CAPTURE` 或 `ENCODER_RIGHT_CAPTURE`。
- SysConfig 为 PA17/PB19 生成 GPIO 上升沿中断宏。
- ARM GCC 主机源码测试在 `-Wall -Wextra -Werror` 下通过。
- TI SysConfig 和完整 CCS 编译链接通过。
- 上板时静止轮子计数不应自增，手动转轮时对应轮计数变化，反向转动时符号相反。

