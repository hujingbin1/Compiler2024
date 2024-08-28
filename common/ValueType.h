/**
 * @file type.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 变量类型管理的头文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <string>

/// @brief 基本类型枚举类
enum class BasicType : int {
    TYPE_NONE = 0,  // 节点不存在类型
    TYPE_VOID = 1,  // void型，仅用于函数返回值
    TYPE_INT = 2,   // 整型
    TYPE_FLOAT = 3, // Float类型
    TYPE_MAX = 4,   // 其它类型，未知类型
    TYPE_BOOL = 5,   //新增布尔型用于记录判断结果
	TYPE_FNP_I = 6,	//函数形参整数数组
	TYPE_FNP_F = 7	//函数形参浮点数组
};

/// @brief 目前只考虑基本类型，数组类型后面可定义
class ValueType {

public:
    /// @brief 值的类型
    BasicType type;

    /// @brief 构造函数，类型不存在
    ValueType();

    /// @brief 构造函数
    /// @param _type 指定类型
    ValueType(BasicType _type);

    /// @brief 转换字符串
    /// @return 字符串
    std::string toString();
};
