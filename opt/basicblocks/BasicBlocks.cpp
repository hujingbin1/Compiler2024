#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "SymbolTable.h"
#include "BasicBlocks.h"
using namespace std;

// 有参构造
BasicBlock::BasicBlock(std::string funcName, std::string entryName)
{
    this->setFuncName(funcName);
    this->setEntryName(entryName);
    this->setSrcDstSet();
}

// 有参构造
BasicBlock::BasicBlock(std::string funcName, std::string entryName, std::vector<std::string> exitName)
{
    this->setFuncName(funcName);
    this->setEntryName(entryName);
    for (auto const & exit_unit: exitName) {
        this->insertExitName(exit_unit);
    }
    this->setSrcDstSet();
}

// 有参构造
BasicBlock::BasicBlock(std::string funcName,
                       std::string entryName,
                       std::string exitName,
                       std::string stateMents,
                       IRInst * irInsts)
{
    this->setFuncName(funcName);
    this->setEntryName(entryName);
    this->insertStatements(stateMents);
    this->insertExitName(exitName);
    this->insertIRInsts(irInsts);
    this->setSrcDstSet();
}

// 有参构造
BasicBlock::BasicBlock(std::string funcName,
                       std::string entryName,
                       std::vector<std::string> exitName,
                       std::vector<std::string> stateMents,
                       std::vector<IRInst *> irInsts)
{
    this->setFuncName(funcName);
    this->setEntryName(entryName);
    for (auto const & exit_unit: exitName) {
        this->insertExitName(exit_unit);
    }
    for (auto const & stateMent: stateMents) {
        this->insertStatements(stateMent);
    }
    for (auto & irInst: irInsts) {
        this->insertIRInsts(irInst);
    }
    this->setSrcDstSet();
}
//得到控制流
void getControlFlow(SymbolTable & symtab,
                    std::unordered_map<std::string, std::unordered_map<std::string, vector<std::string>>> & controlflow)
{
    //这里进行了基本块之间的节点到节点关系的映射——>边——>controlflow[func.getName()][{label}]中存的是下一跳的label，因为有bc指令，所以是一个vector
    std::string instStr;
    std::string str1, str2;
    // flag表示该函数进入基本块
    bool flag = false;
    for (auto func: symtab.getFunctionList()) {
        for (auto & inst: func->getInterCode().getInsts()) {
            str2 = inst->getLabelName();
            if (str2[1] == 'L' && str2 != "") {
                //边界情况处理
                if (flag) {
                    //注意 这里只是假设是顺序流，不考虑跳转，后续需要修改
                    controlflow[func->getName()][str1].emplace_back(str2);
                }
                str1 = str2;
                flag = true;
            }
        }
        flag = false;
    }
    //控制流修改，考虑跳转
    std::string label_active;
    std::string label_block;
    for (auto func: symtab.getFunctionList()) {
        for (auto & inst: func->getInterCode().getInsts()) {
            label_active = inst->getLabelName();
            if (label_active[1] == 'L' && label_active != "") {
                label_block = label_active;
            }
            switch (inst->getOp()) {
                case (IRInstOperator::IRINST_OP_ENTRY): {
                    controlflow[func->getName()]["entry " + func->getName()].emplace_back(label_block);
                    break;
                }
                case (IRInstOperator::IRINST_OP_BC): {
                    // 假定true分支label存在第0个 false分支label存在第1个
                    controlflow[func->getName()][label_block].clear();
                    controlflow[func->getName()][label_block].emplace_back(inst->getTrueLabelName());
                    controlflow[func->getName()][label_block].emplace_back(inst->getFalseLabelName());
                    break;
                }
                case (IRInstOperator::IRINST_OP_BR): {
                    controlflow[func->getName()][label_block].clear();
                    controlflow[func->getName()][label_block].emplace_back(inst->getTrueLabelName());
                    break;
                }
                case (IRInstOperator::IRINST_OP_EXIT): {
                    controlflow[func->getName()][label_block].clear();
                    controlflow[func->getName()][label_block].emplace_back("exit " + func->getName());
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
}
//设置基本块的控制流
void setControlFlow(BasicBlocks * basicblocks,
                    std::unordered_map<std::string, std::unordered_map<std::string, vector<std::string>>> & controlflow)
{
    std::vector<std::string> func_list;
    for (auto const & basicblock: basicblocks->getBasicBlocks()) {
        func_list.push_back(basicblock.first);
    }

    for (auto const & func: func_list) {
        for (auto const & controlflowunit: controlflow[func]) {
            if (controlflowunit.second.size() == 0) {
                break;
            }
            std::string entry_label = controlflowunit.first;
            std::vector<std::string> exit_label = controlflowunit.second;
            if (entry_label == "entry " + func) {
                basicblocks->insertEntryBlock(func, entry_label, exit_label);
                continue;
            }
            //之前已经处理过了
            // if (exit_label[0] == "exit " + func) {
            //     basicblocks->insertExitBlock(func, entry_label, exit_label);
            //     continue;
            // }
            basicblocks->setBlockExit(func, entry_label, exit_label);
        }
    }
}

//得到这个基本块的前驱基本块
std::vector<BasicBlock> * BasicBlocks::getPreBlock(BasicBlock & block, std::string funcName)
{
    //重新new一个vector管理block——>注意这里的block是新的block，重新分配内存的，而不是原来基本块的地址，只是构建支配树用
    std::vector<BasicBlock> * preBlock = new std::vector<BasicBlock>;
    //但是这里用了引用传值，unitBlock还是原来的基本块
    for (auto & unitBlock: *this->getBasicBlocksByFuncName(funcName)) {
        if (!unitBlock.getCanDelete()) {
            if (unitBlock.getExitName().size() == 1) {
                if (unitBlock.getExitName()[0] == block.getEntryName()) {
                    preBlock->emplace_back(unitBlock);
                }
            } else if (unitBlock.getExitName().size() == 2) {
                if (unitBlock.getExitName()[0] == block.getEntryName()) {
                    preBlock->emplace_back(unitBlock);
                }
                if (unitBlock.getExitName()[1] == block.getEntryName()) {
                    preBlock->emplace_back(unitBlock);
                }
            }
        }
    }
    return preBlock;
}

//得到这个基本块的后继基本块
std::vector<BasicBlock> * BasicBlocks::getPostBlock(BasicBlock & block, std::string funcName)
{
    //重新new一个vector管理block——>注意这里的block是新的block，重新分配内存的，而不是原来基本块的地址，只是构建支配树用
    std::vector<BasicBlock> * postBlock = new std::vector<BasicBlock>;
    //但是这里用了引用传值，unitBlock还是原来的基本块
    for (auto & unitBlock: *this->getBasicBlocksByFuncName(funcName)) {
        if (!unitBlock.getCanDelete()) {
            if (block.getExitName().size() == 1) {
                if (block.getExitName()[0] == unitBlock.getEntryName()) {
                    postBlock->emplace_back(unitBlock);
                }
            } else if (block.getExitName().size() == 2) {
                if (block.getExitName()[0] == unitBlock.getEntryName()) {
                    postBlock->emplace_back(unitBlock);
                }
                if (block.getExitName()[1] == unitBlock.getEntryName()) {
                    postBlock->emplace_back(unitBlock);
                }
            }
        }
    }
    return postBlock;
}