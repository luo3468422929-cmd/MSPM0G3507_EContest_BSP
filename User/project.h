/**
 * @file project.h
 * @brief 汇总 main.c 需要的工程级公共头文件，保持主入口简洁。
 *
 * 所属层：User。普通模块不要依赖本头文件，以免形成隐式交叉依赖；
 * 每个 .c 仍应直接包含自己真正使用的接口。
 */
#ifndef USER_PROJECT_H
#define USER_PROJECT_H

#include "ti_msp_dl_config.h"

#include "car_control.h"
#include "lcd.h"
#include "task.h"
#include "test.h"
#include "user_config.h"

#endif
