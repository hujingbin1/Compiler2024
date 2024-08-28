/**
 * @file value.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 变量以及常量等Value管理
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "Value.h"

// 这里暂时不用，需要时Value相关类实现在这里编写

uint64_t Value::VarCount = 0;    // 临时变量计数，默认从0开始
uint64_t Value::ConstCount = 0;  // 常量计数，默认从0开始
uint64_t Value::GlobalCount = 0; // 全局变量计数，默认从0开始