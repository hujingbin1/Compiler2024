#pragma once
#include <vector>
#include <map>
#include "BasicBlocks.h"
class DomainTree {
public:
    std::vector<BasicBlock *> reversePostTraverse;
    std::map<BasicBlock *, int> TraverseInd;
    std::vector<BasicBlock *> doms;
    std::vector<std::string> funcNameList;
    BasicBlocks * basicblocks;

public:
    DomainTree(BasicBlocks * basicblocks);
    ~DomainTree();
    void execute();
    std::vector<BasicBlock *> postTraverse(BasicBlock * entryBlock, std::string funcName);
    void getReversePostTraverse(std::string funcName);
    void getBlockDom(std::string funcName);
    void getBlockDomFront(std::string funcName);
    BasicBlock * intersect(BasicBlock * blcok1, BasicBlock * block2);
    bool isLoopEdge(BasicBlock * block1, BasicBlock * block2);
};

// class ReverseDomainTree {
//     std::vector<std::string> funcNameList;
//     BasicBlocks basicblocks;
//     std::map<BasicBlock *, int> reverseTraverseInd;
//     std::vector<BasicBlock *> reverseDomainBlock;
//     std::vector<BasicBlock *> reverseTraverse;
//     BasicBlock * exitBlock;

// public:
//     ReverseDomainTree(BasicBlocks & basicblocks);
//     ~ReverseDomainTree()
//     {}
//     BasicBlock * intersect(BasicBlock * block1, BasicBlock * block2);
//     void execute();
//     void getReversePostTraverse(std::string funcName);
//     void getPostTraverse(BasicBlock * block, std::string funcName, std::set<BasicBlock *> & visited);
//     void getBlockRdoms(std::string funcName);
//     void getBlockDomFrontR(std::string funcName);
//     void getPostTraverse(BasicBlock * block, std::set<BasicBlock *> & visited);
// };