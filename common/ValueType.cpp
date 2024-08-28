/**
 * @file type.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 变量类型管理
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "ValueType.h"

/// @brief 构造函数，类型不存在
ValueType::ValueType() : type(BasicType::TYPE_NONE)
{}

/// @brief 构造函数
/// @param _type 指定类型
ValueType::ValueType(BasicType _type) : type(_type)
{}

/// @brief 转换字符串
/// @return 字符串
std::string ValueType::toString()
{
    std::string str;
    switch (type) {
        case BasicType::TYPE_INT:
            str = "i32";
            break;
        case BasicType::TYPE_FLOAT:
            str = "float";
            break;
		case BasicType::TYPE_FNP_I:
            str = "i32";
            break;
        case BasicType::TYPE_FNP_F:
            str = "float";
            break;
        case BasicType::TYPE_VOID:
            str = "void";
            break;
        case BasicType::TYPE_NONE:
            str = "None";
            break;
        default:
            str = "Unknown";
    }

    return str;
}