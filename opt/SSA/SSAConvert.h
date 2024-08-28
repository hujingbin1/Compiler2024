#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include "BasicBlocks.h"
class ConvertSSA {
public:
    ConvertSSA()
    {}
    ConvertSSA(BasicBlocks * basicblocks)
    {
        this->basicblocks = basicblocks;
        for (auto & funcBlocks: this->basicblocks->getBasicBlocks()) {
            this->funcName.push_back(funcBlocks.first);
        }
    }
    ~ConvertSSA()
    {}
    // 插入phi函数
    void insertPhiFunc();
    // 重写所有变量定义语句
    void rwDefs();
    // 重写所有语句中变量定值和使用信息
    // void rwInst(BasicBlock * block);
    // void rwDefine(BasicBlock * block);
    // void reUse(BasicBlock * block);
    //是否为要转为SSA的类型
    bool isConvert(IRInst * inst);
    // void run();

private:
    BasicBlocks * basicblocks;
    vector<std::string> funcName;
    std::unordered_map<Value *, bool> varRwBlock;      //变量的定值信息，只考虑参数和局部变量
    std::unordered_map<Value *, std::string> varDFSet; //变量的支配边界
    //维护每个变量最新的SSA下标
    std::vector<int> varSSAIndex;
};
