
#pragma once
///@author 胡景斌
///@brief 数据流分析

#include <unordered_map>
#include <vector>
#include <list>
#include <set>
#include "CfgGraph.h"
#include "BasicBlocks.h"
#include "IRInst.h"
#include "Value.h"

using namespace std;
class DataFlowAnalysis {
public:
    DataFlowAnalysis(BasicBlocks * basicblocks, bool direct, std::string funcName);

    ~DataFlowAnalysis()
    {
        boundVals.clear();
        initVals.clear();
        optCode.clear();
    };

    // 数据流分析的in与out的初始化
    void init();

    // 数据流分析的in与out的交叉计算
    void join(BasicBlock *)
    {}

    // 数据流分析最后一步翻译
    bool translate(BasicBlock *)
    {
        return true;
    }

    // 根据控制流图进行数据流分析
    void analyse();

    // 根据分析获得数据流信息进行优化处理
    void handle()
    {}

    // 通过数据流进行分析与优化处理
    void optimize();

protected:
    void forwardAnalyse();

    void reverseAnalyse();

protected:
    /**
     * 控制流图，基本块的关联图
     */
    BasicBlocks * basicblocks;

    /**
     * 数据流分析方向 true:正向 false:逆向
     */
    bool direct;

    /**
     * 需要分析的函数名
     * */

    std::string funcName;

    /**
     * 需要分析的基本块（根据函数名取基本块）
     * */
    std::vector<BasicBlock> * basicblock;

    /**
     * 边界集合
     */
    vector<double> boundVals;

    /**
     * 初值集合
     */
    vector<double> initVals;

    /**
     * IN集合
     * */
    unordered_map<std::string, std::set<Value *>> inSet;

    /**
     * OUT集合
     * */
    unordered_map<std::string, std::set<Value *>> outSet;

    /**
     * def集合
     * */
    unordered_map<std::string, std::set<Value *>> defSet;

    /**
     * use集合
     * */
    unordered_map<std::string, std::set<Value *>> useSet;

public:
    //全集
    std::set<std::string> U;

    //空集
    std::set<std::string> E;

    // 记录前面阶段优化后的代码
    list<IRInst *> optCode;
};
