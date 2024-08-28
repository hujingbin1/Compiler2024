#include <vector>
#include "DataFlowAnalysis.h"
#include "CfgGraph.h"
#include "BasicBlocks.h"


DataFlowAnalysis::DataFlowAnalysis(BasicBlocks * basicblocks, bool direct, std::string funcName)
{
    this->basicblocks = basicblocks;
    this->direct = direct;
    this->funcName = funcName;
    this->basicblock = this->basicblocks->getBasicBlocksByFuncName(this->funcName);
}
void DataFlowAnalysis::init()
{
    // TODO:初始化use def集合
    for (auto & block: *this->basicblock) {
        if (!block.getCanDelete() && block.getCanMoveTo()) {
            std::string key = this->funcName + block.getEntryName();
            for (auto & srcValue: block.getSrcSet()) {
                if (srcValue->isLocalVar() || srcValue->isTemp()) {
                    useSet[key].insert(srcValue);
                }
            }
            for (auto & desValue: block.getDstSet()) {
                if (desValue->isLocalVar() || desValue->isTemp()) {
                    defSet[key].insert(desValue);
                }
            }

            // //集合不空
            // if (!defSet[key].empty() && !useSet[key].empty()) {
            //     for (auto unit: useSet[key]) {
            //         //指针不空
            //         if (unit == nullptr) {
            //             continue;
            //         }
            //         // 如果defSet集合中找到useSet的元素，则defSet中删除之
            //         if (defSet[key].find(unit) != defSet[key].end()) {
            //             defSet[key].erase(unit);
            //         }
            //     }
            // }

            // for (auto & inst: block.getIRInsts()) {
            //     if (inst.getSrc().size() == 1) {
            //         if (inst.getSrc1() != nullptr) {
            //             if (inst.getSrc1()->isTemp() || inst.getSrc1()->isLocalVar()) {
            //                 useSet[key].insert(inst.getSrc1());
            //             }
            //         }
            //     }
            //     if (inst.getSrc().size() == 2) {
            //         if (inst.getSrc1() != nullptr) {
            //             if (inst.getSrc1()->isTemp() || inst.getSrc1()->isLocalVar()) {
            //                 useSet[key].insert(inst.getSrc1());
            //             }
            //         }
            //         if (inst.getSrc2() != nullptr) {
            //             if (inst.getSrc2()->isTemp() || inst.getSrc2()->isLocalVar()) {
            //                 useSet[key].insert(inst.getSrc2());
            //             }
            //         }
            //     }
            //     // 如果useSet中找到dst操作数，就不把dst操作数加入def集合
            //     if (useSet[key].find(inst.getDst()) != useSet[key].end()) {
            //         continue;
            //     } else {
            //         if (inst.getDst() != nullptr) {
            //             if (inst.getDst()->isTemp() || inst.getDst()->isLocalVar()) {
            //                 defSet[key].insert(inst.getDst());
            //             }
            //         }
            //     }
            // }
        }
    }
    for (auto & block: *this->basicblock) {
        if (!block.getCanDelete() && block.getCanMoveTo()) {
            std::string key = this->funcName + block.getEntryName();
            std::cout << key << std::endl;
            if (!useSet[key].empty()) {
                std::cout << "useSet:" << std::endl;
                for (auto & unit: useSet[key]) {
                    if (unit == nullptr) {
                        continue;
                    }
                    std::cout << unit->getName() << std::endl;
                }
                std::cout << "——————————————" << std::endl;
            } else {
                std::cout << "useSet:" << std::endl;
                std::cout << "空集" << std::endl;
                std::cout << "——————————————" << std::endl;
            }
            if (!defSet[key].empty()) {
                std::cout << "defSet:" << std::endl;
                for (auto & unit: defSet[key]) {
                    if (unit == nullptr) {
                        continue;
                    }
                    std::cout << unit->getName() << std::endl;
                }
                std::cout << "——————————————" << std::endl;
            } else {
                std::cout << "defSet:" << std::endl;
                std::cout << "空集" << std::endl;
                std::cout << "——————————————" << std::endl;
            }
        }
    }
}
void DataFlowAnalysis::forwardAnalyse()
{
    //初始化边界集合Entry.out
    init();

    bool outChange = true;

    while ((outChange)) {

        //重新设定变化标记
        outChange = false;
        // basicblock.first是函数名，basicblock.second是基本块组成的向量
        for (auto & basicblock: basicblocks->getBasicBlocks()) {
            for (auto & block: basicblock.second) {
                // TODO:块不可达不处理——>不可达代码消除
                if (!block.getCanMoveTo())
                    continue;
                // 排除Entry基本块
                if (block.getEntryName() == "entry " + basicblock.first)
                    continue;
                // 交汇运算
                join(&block);
                // 传递函数执行时比较前后out集合是否有差别
                if (translate(&block)) {
                    outChange = true;
                }
            }
        }
    }
}
void DataFlowAnalysis::reverseAnalyse()
{
    init();

    // B.in集合发生变化则需要继续循环，否则退出
    bool change = true;
    while (change) {

        // 重新设定变化标记
        change = false;

        // basicblock.first是函数名，basicblock.second是基本块组成的向量
        for (auto & basicblock: basicblocks->getBasicBlocks()) {
            // for (auto & block: basicblock.second)
            int cnt = basicblock.second.size();
            cnt--;
            while (cnt >= 0) {
                // TODO：引用传递或者指针传递修改，这里目前是拷贝了一个新的，注意不能修改block，只能读里面的信息，不能写，写不能写到基本块里
                //这里不清楚为啥不能用指针，必须得重新拷贝一个新的——>值传递
                BasicBlock block = basicblock.second[cnt];
                {
                    // 块不可达不处理
                    if (!block.getCanMoveTo())
                        continue;
                    // 排除Exit基本块
                    if (block.getExitName()[0] == "exit " + basicblock.first)
                        continue;
                    //交汇运算
                    join(&block);
                    // 传递函数执行时比较前后out集合是否有差别
                    if (translate(&block)) {
                        change = true;
                    }
                }
                cnt--;
            }
        }
    }
}

void DataFlowAnalysis::analyse()
{
    if (direct) {
        forwardAnalyse();
    } else {
        reverseAnalyse();
    }
}

// 通过数据流进行分析与优化处理
void DataFlowAnalysis::optimize()
{
    // 数据流分析
    analyse();

    // 数据流处理
    handle();
}
