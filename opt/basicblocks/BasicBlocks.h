#pragma once
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "SymbolTable.h"
#include "Value.h"

using namespace std;
class BasicBlock {
public:
    //构造方法
    BasicBlock()
    {}

    // 有参构造
    BasicBlock(SymbolTable & symtab)
    {}

    // 有参构造
    BasicBlock(std::string funcName, std::string entryName);

    // 有参构造
    BasicBlock(std::string funcName, std::string entryName, std::vector<std::string> exitName);

    // 有参构造
    BasicBlock(std::string funcName,
               std::string entryName,
               std::string exitName,
               std::string stateMents,
               IRInst * irInsts);

    // 有参构造
    BasicBlock(std::string funcName,
               std::string entryName,
               std::vector<std::string> exitName,
               std::vector<std::string> stateMents,
               std::vector<IRInst *> irInsts);

    //析构方法
    ~BasicBlock()
    {}

    // get、set方法
    std::string const & getFuncName()
    {
        return this->funcname;
    }
    std::string const & getEntryName()
    {
        return this->labelname_entry;
    }
    std::vector<std::string> const & getExitName()
    {
        return this->labelname_exit;
    }
    std::vector<std::string> const & getStatements()
    {
        return this->statements;
    }

    std::vector<IRInst *> getIRInsts()
    {
        return this->IRInsts;
    }

    std::vector<IRInst *> getOptIRInsts()
    {
        return this->optIRInsts;
    }

    void insertOptIRInst(IRInst * optIRInst)
    {
        this->optIRInsts.emplace_back(optIRInst);
    }

    void setFuncName(std::string const & funcName)
    {
        this->funcname = funcName;
    }
    void setEntryName(std::string const & entryName)
    {
        this->labelname_entry = entryName;
    }
    void insertStatements(std::string statements)
    {
        this->statements.push_back(statements);
    }
    void insertExitName(std::string const & exitName)
    {
        this->labelname_exit.push_back(exitName);
    }

    void clearExitName()
    {
        this->labelname_exit.clear();
    }
    void insertIRInsts(IRInst * irinst)
    {
        this->IRInsts.push_back(irinst);
    }

    //得到基本块的语句内容——> 基本块内所有指令序列tostring为字符串并拼接
    std::string getString()
    {
        std::string all_str;
        for (auto const & inst_str: BasicBlock::getStatements()) {
            all_str += inst_str;
        }
        return all_str;
    }

    void setAllStates(BasicBlock basicblock)
    {
        this->all_statements = getString();
    }
    std::string getAllStates()
    {
        return this->all_statements;
    }

    bool getCanMoveTo()
    {
        return this->canMoveTo;
    }

    void setCanMoveTo(bool canMoveTo)
    {
        this->canMoveTo = canMoveTo;
    }

    bool getCanDelete()
    {
        return this->canDelete;
    }
    void setCanDelete(bool canDelete)
    {
        this->canDelete = canDelete;
    }
    //初始化源操作数和目的操作数集合
    void setSrcDstSet()
    {
        for (auto & IRInst: this->getIRInsts()) {
            if (IRInst->getDst() != nullptr) {
                this->dstSet.insert(IRInst->getDst());
            }
            if (IRInst->getSrc().size() != 0) {
                for (auto & src: IRInst->getSrc()) {
                    this->srcSet.insert(src);
                }
            }
        }
    }
    std::set<Value *> getDstSet()
    {
        return this->dstSet;
    }
    std::set<Value *> getSrcSet()
    {
        return this->srcSet;
    }

    void clearDomFrontierSet()
    {
        this->domFrontierSet.clear();
    }
    void insertDomFrontierSet(BasicBlock * block)
    {
        this->domFrontierSet[this->labelname_entry].insert(block);
    }

    std::set<BasicBlock *> getDomFrontierSet()
    {
        return this->domFrontierSet[this->labelname_entry];
    }

    void claerIdomSet()
    {
        this->idomSet.clear();
    }
    void insertIdomSet(BasicBlock * block)
    {
        this->idomSet[this->labelname_entry].insert(block);
    }

    std::set<BasicBlock *> getIdomSet()
    {
        return this->idomSet[this->labelname_entry];
    }

    void clearReverseDomFrontierSet()
    {
        this->reverseDomFrontierSet.clear();
    }
    void insertReverseDomFrontierSet(BasicBlock * block)
    {
        this->reverseDomFrontierSet[this->labelname_entry].insert(block);
    }

    std::set<BasicBlock *> getReverseDomFrontierSet()
    {
        return this->reverseDomFrontierSet[this->labelname_entry];
    }

    void claerReverseIdomSet()
    {
        this->reverseIdomSet.clear();
    }
    void insertReverseIdomSet(BasicBlock * block)
    {
        this->reverseIdomSet[this->labelname_entry].insert(block);
    }

    std::set<BasicBlock *> getReverseIdomSet()
    {
        return this->reverseIdomSet[this->labelname_entry];
    }

private:
    //函数名
    std::string funcname;
    //入口
    std::string labelname_entry;
    //出口
    std::vector<std::string> labelname_exit;
    // tostring后的指令序列
    std::vector<std::string> statements;
    //得到要输出的IR序列
    std::vector<IRInst *> IRInsts;
    // SSA优化后的IR序列
    std::vector<IRInst *> optIRInsts;
    // 基本块内所有指令序列tostring为字符串并拼接
    std::string all_statements;
    // 基本块是否可达
    bool canMoveTo = false;
    // 基本块是否可以删除
    bool canDelete = false;
    // 基本块的目标操作数组成的集合
    std::set<Value *> dstSet;
    // 基本块的源操作数组成的集合
    std::set<Value *> srcSet;
    // 支配树需要idomSet
    std::unordered_map<std::string, std::set<BasicBlock *>> idomSet;
    // 支配树需要DFset
    std::unordered_map<std::string, std::set<BasicBlock *>> domFrontierSet;
    // 支配树需要逆向idomSet
    std::unordered_map<std::string, std::set<BasicBlock *>> reverseIdomSet;
    // 支配树需要逆向DFset
    std::unordered_map<std::string, std::set<BasicBlock *>> reverseDomFrontierSet;
};
class BasicBlocks {
public:
    BasicBlocks()
    {}
    ~BasicBlocks()
    {}
    // basicblocks用来管理基本块 数据结构为key；函数名 value：基本块vector
    BasicBlocks(SymbolTable & symtab)
    { // tostring指令用的
        std::string instStr;
        // label_active表示当前指令的 label_block表示当前基本块的label
        std::string label_block, label_active;
        // flag表示是不是进入基本块
        bool flag = false;

        for (auto func: symtab.getFunctionList()) {
            // basicblock是基本块
            BasicBlock * basicblock = new BasicBlock();
            for (auto & inst: func->getInterCode().getInsts()) {
                label_active = inst->getLabelName();
                if (label_active[1] == 'L' && label_active != "") {
                    if (flag) {
                        //这里先假定是顺序流 后面针对出口Label会进行修改
                        basicblock->insertExitName(label_active);
                        //将所有的指令tostring为基本块的语句
                        basicblock->setAllStates(*basicblock);
                        // 将指令的源操作数、目的操作数单独存下来
                        basicblock->setSrcDstSet();
                        //将这个基本块的信息插入basicblocks
                        BasicBlocks::insertBasicBlocks(func->getName(), *basicblock);
                        //释放空间
                        delete basicblock;
                        //重新new一个基本块
                        basicblock = new BasicBlock();
                    }
                    flag = true;
                    label_block = label_active;
                    basicblock->setFuncName(func->getName());
                    basicblock->setEntryName(label_block);
                }
                inst->toString(instStr);
                if (!instStr.empty()) {
                    // Label指令不加Tab键
                    if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
                        instStr = instStr + "\n";
                    } else {
                        instStr = "\t" + instStr + "\n";
                    }
                }
                basicblock->insertIRInsts(inst);
                //先暂定opt的IR是原始IR的副本，后期SSA需要修改
                basicblock->insertOptIRInst(inst);
                basicblock->insertStatements(instStr);
            }
            // 边界条件处理——>没有上一个label 则直接加一个从entry到exit的块
            if (label_block == "") {
                basicblock->setEntryName("entry " + func->getName());
            }
            // exit block追加
            basicblock->insertExitName("exit " + func->getName());
            //将所有的指令tostring为基本块的语句
            basicblock->setAllStates(*basicblock);
            //将这个基本块的信息插入basicblocks
            BasicBlocks::insertBasicBlocks(func->getName(), *basicblock);
            //释放空间
            delete basicblock;
            flag = false;
        }
        //初始化变量列表，SSA需要
        this->initLocalValueList();
    }
    // 插入EntryBlock
    void insertEntryBlock(std::string funcName, std::string entryName, std::vector<std::string> exitName)
    {
        BasicBlock * block = new BasicBlock(funcName, entryName, exitName);
        this->basicblocks[funcName].push_back(*block);
    }
    // 插入ExitBlock
    void insertExitBlock(std::string funcName, std::string entryName, std::vector<std::string> exitName)
    {
        BasicBlock * block = new BasicBlock(funcName, entryName, exitName);
        this->basicblocks[funcName].push_back(*block);
    }
    //设置Block的出口
    bool setBlockExit(std::string funcName, std::string entryName, std::vector<std::string> exitName)
    {
        for (auto & basicblock: this->basicblocks) {
            if (basicblock.first == funcName) {
                //注意 一定要用引用类型 不然是值传递 新建了一个对象而不是原对象
                for (auto & block: basicblock.second) {
                    if (block.getEntryName() == entryName) {
                        //一定要记得清除原有的出口信息
                        block.clearExitName();
                        //把控制流表中的出口信息传递到基本块中
                        for (auto const & exit_unit: exitName) {
                            block.insertExitName(exit_unit);
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }
    // get、set方法
    //直接得到所有函数的所有基本块
    unordered_map<std::string, std::vector<BasicBlock>> getBasicBlocks()
    {
        return this->basicblocks;
    }
    //根据函数名找到一组基本块——>必须是引用传递或者指针传递
    std::vector<BasicBlock> * getBasicBlocksByFuncName(const std::string funcName)
    {
        return &this->basicblocks[funcName];
    }
    //根据入口名找到基本块——>最好指针传递？引用传递其实也可以 主要有点concern
    BasicBlock * getBasicBlockByFuncEntry(const std::string funcName, const std::string entryName)
    {
        for (auto & basicblock: basicblocks[funcName]) {
            if (entryName == basicblock.getEntryName()) {
                return &basicblock;
            }
        }
        return nullptr;
    }
    //根据出口名找到基本块——>最好指针传递？引用传递其实也可以 主要有点concern
    BasicBlock * getBasicBlockByFuncExit(const std::string funcName, const std::string exitName)
    {
        for (auto & basicblock: basicblocks[funcName]) {
            if (basicblock.getExitName().size() == 2) {
                if (exitName == basicblock.getExitName()[0] || exitName == basicblock.getExitName()[1]) {
                    return &basicblock;
                }
            }
            if (basicblock.getExitName().size() == 1) {
                if (exitName == basicblock.getExitName()[0]) {
                    return &basicblock;
                }
            }
        }
        return nullptr;
    }
    void insertBasicBlocks(const std::string funcName, BasicBlock const & basicblock)
    {
        this->basicblocks[funcName].push_back(basicblock);
    }

    //得到这个基本块的前驱基本块
    std::vector<BasicBlock> * getPreBlock(BasicBlock & block, std::string funcName);

    //得到这个基本块的后继基本块
    std::vector<BasicBlock> * getPostBlock(BasicBlock & block, std::string funcName);

    void setBackEdge(std::string funcName, std::string entry, std::string exit)
    {
        this->backedge[funcName][entry] = exit;
    }
    unordered_map<std::string, std::string> getBackEdge(std::string funcName)
    {
        return this->backedge[funcName];
    }

    void setDfSet(std::string funcName, std::string dfValue)
    {
        this->dfSet[funcName].insert(dfValue);
    }

    std::set<std::string> getDfSet(std::string funcName)
    {
        return this->dfSet[funcName];
    }

    void initLocalValueList()
    {
        // 为内部的unordered_map设置默认插入函数
        for (auto & pair: this->localValueList) {
            pair.second = std::unordered_map<Value *, int>({{nullptr, 0}}); // 设置默认插入值为0
        }
    }

    unordered_map<Value *, int> & getLocalValueList(std::string funcName)
    {
        return this->localValueList[funcName];
    }
    void insertLocalValueList(std::string funcName, Value * value)
    {
        if (!findLocalValue(funcName, value)) {
            this->localValueList[funcName][value] = 1;
        } else {
            this->setTag(funcName, value);
        }
    }
    bool findLocalValue(std::string funcName, Value * value)
    {
        //已经在表中，则返回true
        if (this->localValueList[funcName][value] != 0) {
            return true;
        }
        return false;
    }
    int getTag(std::string funcName, Value * value)
    {
        return this->localValueList[funcName][value];
    }

    void setTag(std::string funcName, Value * value)
    {
        this->localValueList[funcName][value]++;
    }

private:
    //管理BasicBlock的向量 key：函数名 value：基本块vector
    unordered_map<std::string, std::vector<BasicBlock>> basicblocks;

    // 回边记录
    unordered_map<std::string, unordered_map<std::string, std::string>> backedge;

    // DF集合记录——>SSA用于插入Phi函数
    unordered_map<std::string, std::set<std::string>> dfSet;

    // Value记录——>目前只记录局部变量
    unordered_map<std::string, unordered_map<Value *, int>> localValueList{
        {}, // 空的外部unordered_map，键为字符串
    };
    ;
};

void getControlFlow(
    SymbolTable & symtab,
    std::unordered_map<std::string, std::unordered_map<std::string, vector<std::string>>> & controlflow);

void setControlFlow(
    BasicBlocks * basicblocks,
    std::unordered_map<std::string, std::unordered_map<std::string, vector<std::string>>> & controlflow);