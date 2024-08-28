#include <unordered_map>
#include <string>
#include "BasicBlocks.h"
#include "IRInst.h"
#include "ControlFlowAnalysis.h"

ControlFlowAnalysis::ControlFlowAnalysis()
{}
//构造方法
ControlFlowAnalysis::ControlFlowAnalysis(BasicBlocks * basicblocks, std::string funcName)
{
    for (auto & basicblock: *basicblocks->getBasicBlocksByFuncName(funcName)) {
        this->labelVisit[basicblock.getEntryName()] = basicblock.getCanMoveTo();
    }
}
ControlFlowAnalysis::~ControlFlowAnalysis()
{}

bool canDeleteBlock(BasicBlock & block)
{
    //情况1：说明只有label标签
    if (block.getIRInsts().size() == 1 && block.getIRInsts()[0]->getOp() == IRInstOperator::IRINST_OP_LABEL) {
        // 删除空块——>迭代器内部不能删除空块，额外用一个标记位来标记可以被删除
        block.setCanDelete(true);
        return true;
    }
    // 情况2 label 标签 + 直接跳转的branch指令
    if (block.getIRInsts().size() == 2 && block.getIRInsts()[0]->getOp() == IRInstOperator::IRINST_OP_LABEL &&
        block.getIRInsts()[1]->getOp() == IRInstOperator::IRINST_OP_BR) {
        // 删除空块——>迭代器内部不能删除空块，额外用一个标记位来标记可以被删除
        block.setCanDelete(true);
        return true;
    }
    return false;
}

//删除空块、跳转块
void deleteNullBlock(BasicBlocks * basicblocks, std::string funcName)
{
    //注意，必须是地址传递，否则无法修改基本块；不用指针类型也不可以，还是会在栈上分配内存，即使返回值为*类型，取地址后还是进行了值传递
    std::vector<BasicBlock> * basicblock = basicblocks->getBasicBlocksByFuncName(funcName);
    for (auto & block: *basicblock) {
        if (canDeleteBlock(block)) {
            for (auto & blockchange: *basicblock) {
                // 找到需要修改的基本块
                // 现在先只考虑单出口的情况
                if (blockchange.getExitName()[0] == block.getEntryName() && blockchange.getExitName().size() == 1) {
                    //清除原来的出口
                    blockchange.clearExitName();
                    // 设置新的出口
                    blockchange.insertExitName(block.getExitName()[0]);
                }
                // 现在考虑双出口的情况
                if (blockchange.getExitName()[0] == block.getEntryName() && blockchange.getExitName().size() == 2) {
                    //暂存现在的出口2
                    std::string exitLabel = blockchange.getExitName()[1];
                    //清除原来的出口
                    blockchange.clearExitName();
                    // 设置新的出口1
                    blockchange.insertExitName(block.getExitName()[0]);
                    // 设置新的出口2
                    blockchange.insertExitName(exitLabel);
                }
                // 现在考虑双出口的情况
                if (blockchange.getExitName()[1] == block.getEntryName() && blockchange.getExitName().size() == 2) {
                    //暂存现在的出口1
                    std::string exitLabel = blockchange.getExitName()[0];
                    //清除原来的出口
                    blockchange.clearExitName();
                    // 设置新的出口1
                    blockchange.insertExitName(exitLabel);
                    // 设置新的出口2
                    blockchange.insertExitName(block.getExitName()[0]);
                }
            }
        }
    }
}

// TODO:不可达基本块消除
void deleteDeadBlock(BasicBlocks * basicblocks, std::string funcName)
{
    std::string entry_label = "entry " + funcName;
    std::string exit_label = "exit " + funcName;
    ControlFlowAnalysis * controlFlowAnalysis = new ControlFlowAnalysis(basicblocks, funcName);
    dfsBlock(funcName, entry_label, controlFlowAnalysis, basicblocks);
}

void dfsBlock(std::string funcName,
              std::string labelName,
              ControlFlowAnalysis * controlFlowAnalysis,
              BasicBlocks * basicblocks)
{
    const bool isVisited = true;
    //游标——>指向当前的基本块
    std::string label_active = labelName;
    //获得当前的基本块
    BasicBlock * block = basicblocks->getBasicBlockByFuncEntry(funcName, label_active);
    if (block != nullptr) {
        //说明不是死代码，可以到达
        block->setCanMoveTo(isVisited);
        // 分支指令，两路搜索
        // TODO：在这里可以找到回边
        if (block->getExitName().size() == 2) {
            label_active = block->getExitName()[0];
            if (controlFlowAnalysis->getLabelVisit(label_active) == false) {
                controlFlowAnalysis->setLabelVisit(label_active, isVisited);
                //从第一个出口搜索
                dfsBlock(funcName, label_active, controlFlowAnalysis, basicblocks);
            }
            label_active = block->getExitName()[1];
            if (controlFlowAnalysis->getLabelVisit(label_active) == false) {
                controlFlowAnalysis->setLabelVisit(label_active, isVisited);
                //从第二个出口搜索
                dfsBlock(funcName, label_active, controlFlowAnalysis, basicblocks);
            }
        } else if (block->getExitName().size() == 1) {
            label_active = block->getExitName()[0];
            if (controlFlowAnalysis->getLabelVisit(label_active) == false) {
                controlFlowAnalysis->setLabelVisit(label_active, isVisited);
                //从第一个出口搜索
                dfsBlock(funcName, label_active, controlFlowAnalysis, basicblocks);
            }
        } else {
            //没有出口——> 异常——>回溯
            return;
        }
    }
}