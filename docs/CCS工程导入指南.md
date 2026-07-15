# CCS 工程导入与编译

1. CCS 选择 `File > Import Projects`，选择工程根目录。
2. 确认工程使用 MSPM0 SDK 2.10.0.04、SysConfig 1.26.2、TI Clang 4.0.x LTS。
3. 双击 `empty.syscfg`，确认设备为 MSPM0G3507 LQFP-64 且没有红色冲突。
4. `Project > Clean` 后构建 Debug。
5. CCS 的 include path 应只有工程根、Debug、`Bsp`、`Hardware`、`Control`、`User` 和 SDK 路径。

`.project`、`.cproject`、`.ccsproject`、`.settings/` 是 CCS/Eclipse 工程元数据，每个可直接导入的 CCS 工程通常都会有，不是业务驱动。不要删除或手工重建；改工程属性时 CCS 会自动更新它们。

VS Code 只负责编辑和红线提示，真正烧录/调试仍由 CCS 完成。若 VS Code 标红，先运行一次 Debug 构建生成 `Debug/ti_msp_dl_config.h`，再检查 `.vscode/c_cpp_properties.json`。
