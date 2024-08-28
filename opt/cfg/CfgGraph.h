/**
 * @file graph.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 通过graphviz显示CGF控制流图的头文件
 * @version 0.1
 * @date 2023-09-24
 * @copyright Copyright (c) 2023
 *
 */
#ifdef USE_GRAPHVIZ

#pragma once
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cgraph.h>

#include "BasicBlocks.h"
#include "Function.h"
#include "SymbolTable.h"


using namespace std;

/// @brief CFG控制流的图形化显示，这里用C语言实现
/// @param filePath
/// 转换成图形的文件名，主要要通过文件名后缀来区分图片的类型，如png，svg，pdf等皆可
void OutputCFG(
    std::string filePath, BasicBlocks *basicblocks,
    std::unordered_map<std::string, std::unordered_map<std::string, Agnode_t *>>
        &node_list);

#endif
