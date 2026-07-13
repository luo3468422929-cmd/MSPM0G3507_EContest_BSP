# CCS 工程导入指南

## 工程环境

- CCS：70.5.0 Theia-based。
- Device：MSPM0G3507，LQFP-64。
- MSPM0 SDK：2.10.0.04。
- SysConfig：1.26.2。
- 编译器：TI Clang 4.0.x LTS。
- 运行环境：NoRTOS。

## 导入

1. 在 CCS 选择 `File -> Import Project(s)`。
2. 指向 `MSPM0G3507_EContest_BSP` 根目录。
3. 确认工程识别到 `.project`、`.cproject`、`.ccsproject`。
4. 在 Project Properties 的 Products 页面确认 MSPM0 SDK 与 SysConfig 可解析。
5. 打开 `empty.syscfg`，保存一次，检查生成目录中出现 `ti_msp_dl_config.c/.h`。
6. 执行 Clean Project，再 Build Project。

## 源码目录添加规则

`.cproject` 已添加 BSP、Components、Services、App 和 Examples 的头文件路径。CCS Managed Build 会递归发现工程内 `.c` 文件；如果手动复制模块到其他工程，需要同时添加相应 include path，并确认源文件未被 `Exclude from Build`。

## 编译入口

- `empty.c`：唯一 `main()`。
- `App/Src/app_main.c`：综合 Demo 调度。
- `Debug/syscfg`：SysConfig 自动生成文件，禁止手改。
- `Tests/Build/verify_build.ps1`：独立 SysConfig + TI Clang + 链接验证脚本。

## 常见问题

- `DeviceFamily_XYZ undefined`：编译命令缺少 SysConfig 生成的 `device.opt`。
- `interruptVectors undefined`：自建 Makefile 未加入 SDK 的 `startup_mspm0g350x_ticlang.c`；CCS/SysConfig 正常工程会处理引用文件。
- 找不到模块头文件：检查 `.cproject` include path 或 Project Properties。
- SysConfig pin conflict：不要修改生成文件；回到 `.syscfg` 重新分配外设或引脚。

