/**
 * @file IRCode.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief IR指令序列管理类
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <vector>

#include "IRCode.h"
#include "IRInst.h"

/// @brief 析构函数
InterCode::~InterCode()
{
    // 资源清理
    for (auto inst: code) {
        delete inst;
    }

    code.clear();
}

/// @brief 添加一个指令块，添加到尾部，并清除原来指令块的内容
/// @param block 指令块，请注意加入后会自动清空block的指令
void InterCode::addInst(InterCode & block, int flag)
{
    std::vector<IRInst *> & insert = block.getInsts();

    code.insert(code.end(), insert.begin(), insert.end());

    // InterCode析构会清理资源，因此移动指令到code中后必须清理，否则会释放多次导致程序例外
    // 当然，这里也可不清理，但InterCode的析构函数不能清理，需专门的函数清理即可。
    if (flag)
        insert.clear();
}

/// @brief 添加一条中间指令
/// @param inst IR指令
void InterCode::addInst(IRInst * inst)
{
    code.push_back(inst);
}

/// @brief 获取指令序列
/// @return 指令序列
std::vector<IRInst *> & InterCode::getInsts()
{
    return code;
}
