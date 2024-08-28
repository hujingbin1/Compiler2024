#pragma once
#include <unordered_map>
#include <string>
#include "BasicBlocks.h"

class ControlFlowAnalysis {

public: // 目前实现的只有不可达基本块消除
    ControlFlowAnalysis();
    ControlFlowAnalysis(BasicBlocks * basicblocks, std::string funcName);
    ~ControlFlowAnalysis();

    void setLabelVisit(std::string labelName, bool isVisit)
    {
        this->labelVisit[labelName] = isVisit;
    }

    bool getLabelVisit(std::string & labelName)
    {
        return this->labelVisit[labelName];
    }

private:
    // key:基本块编号 value:是否已经访问过
    std::unordered_map<std::string, bool> labelVisit;
};
// 删除空块
void deleteNullBlock(BasicBlocks * basicblocks, std::string funcName);
// 不可达基本块消除
void deleteDeadBlock(BasicBlocks * basicblocks, std::string funcName);
void dfsBlock(std::string funcname,
              std::string labelName,
              ControlFlowAnalysis * controlFlowAnalysis,
              BasicBlocks * basicblocks);
