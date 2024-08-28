#include <functional>
#include <vector>
#include "BasicBlocks.h"
#include "DomainTree.h"
// 参考：https://gitlab.eduxiji.net/educg-group-18973-1895971/carrotcompiler

// TODO:注释掉的方法不知道还需不需要

//构造方法
DomainTree::DomainTree(BasicBlocks * basicblocks)
{
    this->basicblocks = basicblocks;
    for (auto & basicblock: this->basicblocks->getBasicBlocks()) {
        this->funcNameList.emplace_back(basicblock.first);
    }
}
//执行产生支配树
void DomainTree::execute()
{
    for (auto const & funcName: this->funcNameList) {
        if (this->basicblocks->getBasicBlocksByFuncName(funcName) != nullptr) {
            getBlockDom(funcName);
            getBlockDomFront(funcName);
        }
    }
}

//判断回边——>最后没有用
bool DomainTree::isLoopEdge(BasicBlock * block1, BasicBlock * block2)
{
    // block1的遍历id是不是比block2大，如果是，就不存在回边，反之存在回边
    return TraverseInd[block1] > TraverseInd[block2];
}

//拓扑排序——>该模块没有问题了
std::vector<BasicBlock *> DomainTree::postTraverse(BasicBlock * entryBlock, std::string funcName)
{
    std::set<BasicBlock *> vis;
    std::vector<BasicBlock *> ans;
    std::vector<BasicBlock *> pathStack; // 更改为vector，以便使用find

    // 定义DFS函数
    std::function<void(BasicBlock *)> dfs = [&](BasicBlock * place) {
        // 去除无效基本块
        if (!place->getCanDelete()) {
            // 标记已经访问过的基本块
            vis.insert(place);
            // 将当前节点压入pathStack中
            pathStack.push_back(place);

            // 得到下一个基本块——>单出口情况
            if (place->getExitName().size() == 1) {
                BasicBlock * nextBlock = this->basicblocks->getBasicBlockByFuncEntry(funcName, place->getExitName()[0]);
                while (nextBlock != nullptr) {
                    // 没有找到，则继续深度搜索；反之，结束搜索，跳出循环
                    if (vis.find(nextBlock) == vis.end()) {
                        dfs(nextBlock);
                    } else {
                        //  检测回边：如果nextBlock已经在pathStack中，说明存在回边
                        if (std::find(pathStack.begin(), pathStack.end(), nextBlock) != pathStack.end()) {
                            std::cout << "回边存在: 从基本块 " << place->getEntryName() << " 到 "
                                      << nextBlock->getEntryName() << std::endl;
                            basicblocks->setBackEdge(funcName, place->getEntryName(), nextBlock->getEntryName());
                        }
                        break;
                    }
                }
                // 在所有后继遍历完成后，将当前节点从pathStack中移除
                pathStack.pop_back();
                ans.push_back(place);
            }
            // 得到下一个基本块——>双出口情况
            if (place->getExitName().size() == 2) {
                BasicBlock * nextBlock2 =
                    this->basicblocks->getBasicBlockByFuncEntry(funcName, place->getExitName()[0]);
                BasicBlock * nextBlock1 =
                    this->basicblocks->getBasicBlockByFuncEntry(funcName, place->getExitName()[1]);

                if (nextBlock1 != nullptr) {
                    dfs(nextBlock1);
                }
                if (nextBlock2 != nullptr) {
                    dfs(nextBlock2);
                }
                // 在所有后继遍历完成后，将当前节点从pathStack中移除
                pathStack.pop_back();
                ans.push_back(place);
            }
        }
    };

    // 深度搜索
    dfs(entryBlock);
    return ans;
}
// dfs版本2
// std::vector<BasicBlock *> DomainTree::postTraverse(BasicBlock * entryBlock, std::string funcName)
// {
//     std::set<BasicBlock *> visited;
//     std::vector<BasicBlock *> result;

//     std::function<void(BasicBlock *)> dfs = [&](BasicBlock * block) {
//         if (visited.find(block) == visited.end() && !block->getCanDelete()) {
//             visited.insert(block);
//             for (const auto & exitName: block->getExitName()) {
//                 BasicBlock * nextBlock = this->basicblocks.getBasicBlockByFuncEntry(funcName, exitName);
//                 if (nextBlock) {
//                     dfs(nextBlock);
//                 }
//             }
//             result.push_back(block);
//         }
//     };

//     dfs(entryBlock);
//     return result;
// }

//逆后序遍历,用于计算支配关系
void DomainTree::getReversePostTraverse(std::string funcName)
{
    //初始化
    doms.clear();
    reversePostTraverse.clear();
    TraverseInd.clear();
    auto entryBlock = this->basicblocks->getBasicBlockByFuncEntry(funcName, "entry " + funcName);
    //通过dfs进行拓扑排序
    auto seq = postTraverse(entryBlock, funcName);
    std::reverse(reversePostTraverse.begin(), reversePostTraverse.end());
	int size = 0;
	size = seq.size();
    for (int i = 0; i < size; i++) {
        TraverseInd[seq[i]] = i;
    }
    reversePostTraverse = seq;
}

//找到支配的结点
void DomainTree::getBlockDom(std::string funcName)
{
    getReversePostTraverse(funcName);
    auto root = this->basicblocks->getBasicBlockByFuncEntry(funcName, "entry " + funcName);
    auto root_id = TraverseInd[root];
    doms.resize(root_id + 1, nullptr);
    doms.back() = root;
    bool change = true;
    while (change) {
        change = false;
        for (auto & block: reversePostTraverse) {
            if (block != root) {
                auto preBlock = this->basicblocks->getPreBlock(*block, funcName);
                BasicBlock * curDom = nullptr;
                for (auto & preUnitBlock: *preBlock) {
                    //由于preBlock不是地址传递，所以需要在基本块中重新找
                    BasicBlock * preUnitBlockReal =
                        this->basicblocks->getBasicBlockByFuncEntry(funcName, preUnitBlock.getEntryName());
                    if (doms[TraverseInd[preUnitBlockReal]] != nullptr) {
                        curDom = preUnitBlockReal;
                        break;
                    }
                }

                for (auto & preUnitBlock: *preBlock) {
                    //由于preBlock不是地址传递，所以需要在基本块中重新找
                    BasicBlock * preUnitBlockReal =
                        this->basicblocks->getBasicBlockByFuncEntry(funcName, preUnitBlock.getEntryName());
                    if (doms[TraverseInd[preUnitBlockReal]] != nullptr) {
                        //找交汇的基本块
                        curDom = intersect(preUnitBlockReal, curDom);
                    }
                }
                if (doms[TraverseInd[block]] != curDom) {
                    doms[TraverseInd[block]] = curDom;
                    change = true;
                }
            }
        }
    }
    for (auto & block: reversePostTraverse) {
        block->insertIdomSet(doms[TraverseInd[block]]);
    }
}

//找到DF集合
void DomainTree::getBlockDomFront(std::string funcName)
{
    std::vector<BasicBlock> * basicblock = this->basicblocks->getBasicBlocksByFuncName(funcName);
    for (auto & block: *basicblock) {
        //得到这个基本块的前驱基本块
        auto preBlock = this->basicblocks->getPreBlock(block, funcName);
        if (preBlock->size() >= 2) {
            //注意：auto一定都是引用传值
            for (auto & preUnitBlock: *preBlock) {
                //由于preBlock不是地址传递，所以需要在基本块中重新找
                BasicBlock * preUnitBlockReal =
                    this->basicblocks->getBasicBlockByFuncEntry(funcName, preUnitBlock.getEntryName());
                // TraverseInd中保存的是每个basicblock通过dfs得到的int类型的遍历顺序，doms是存放的支配的结点
                while (preUnitBlockReal != doms[TraverseInd[&block]]) {
                    //这里的算法是，从doms数组中找到支配的结点，递归的向前查找，直到找到block的doms
                    preUnitBlockReal->insertDomFrontierSet(&block);
                    preUnitBlockReal = doms[TraverseInd[preUnitBlockReal]];
                }
            }
        }
    }

    for (auto & block: reversePostTraverse) {
        for (auto & unit: block->getIdomSet()) {
            std::cout << "基本块:" << block->getEntryName() << " 支配集合:" << unit->getEntryName() << std::endl;
        }
    }
    for (auto & block: reversePostTraverse) {
        for (auto & unit: block->getDomFrontierSet()) {
            std::cout << "基本块:" << block->getEntryName() << " DF集合:" << unit->getEntryName() << std::endl;
            this->basicblocks->setDfSet(funcName, unit->getEntryName());
        }
    }
    // bool is_loop = false;
    // // 根据拓扑排序可以进行回边判断
    // for (size_t i = 0; i < reversePostTraverse.size() - 1; ++i) {
    //     // if (!isLoopEdge(reversePostTraverse[i], reversePostTraverse[i + 1])) {
    //     std::cout << "块号：" << reversePostTraverse[i]->getEntryName() << " 序号："
    //               << TraverseInd[reversePostTraverse[i]] << std::endl;
    //     // is_loop = true;
    //     // }
    // }

    // if (!is_loop) {
    //     std::cout << "没有循环" << std::endl;
    // } else {
    //     std::cout << "有循环" << std::endl;
    // }
}

//找到汇合的block——>逆向搜索递归，直到汇合
BasicBlock * DomainTree::intersect(BasicBlock * block1, BasicBlock * block2)
{
    auto head1 = block1;
    auto head2 = block2;
    //这里的算法是通过拓扑排序后，找两个基本块的上游支配结点，直到重合，找到汇合的基本块
    while (head1 != head2) {
        while (TraverseInd[head1] < TraverseInd[head2])
            head1 = doms[TraverseInd[head1]];
        while (TraverseInd[head2] < TraverseInd[head1])
            head2 = doms[TraverseInd[head2]];
    }
    return head1;
}
// //构造方法
// ReverseDomainTree::ReverseDomainTree(BasicBlocks & basicblocks)
// {
//     this->basicblocks = basicblocks;
//     for (auto & basicblock: this->basicblocks.getBasicBlocks()) {
//         this->funcNameList.emplace_back(basicblock.first);
//     }
// }
// void ReverseDomainTree::execute()
// {
//     for (auto const & funcName: this->funcNameList) {
//         if (this->basicblocks.getBasicBlocksByFuncName(funcName) != nullptr) {
//             for (auto & block: *this->basicblocks.getBasicBlocksByFuncName(funcName)) {
//                 block.claerReverseIdomSet();
//                 block.clearReverseDomFrontierSet();
//             }
//             getBlockDomR(funcName);
//             getBlockDomFrontR(funcName);
//             getBlockRdoms(funcName);
//         }
//     }
// }

// void ReverseDomainTree::getPostTraverse(BasicBlock * block, std::string funcName, std::set<BasicBlock *> & visited)
// {
//     visited.insert(block);
//     BasicBlock * lastBlock = basicblocks.getBasicBlockByFuncExit(funcName, block->getEntryName());
//     for (auto parent: bb->pre_bbs_)
//         if (visited.find(parent) == visited.end())
//             getPostTraverse(parent, visited);
//     reverseTraverseInd[bb] = reverseTraverse.size();
//     reverseTraverse.push_back(bb);
// }

// void ReverseDomainTree::getReversePostTraverse(std::string funcName)
// {
//     reverseDomainBlock.clear();
//     reverseTraverse.clear();
//     reverseTraverseInd.clear();
//     for (auto bb: f->basic_blocks_) {
//         auto terminate_instr = bb->get_terminator();
//         if (terminate_instr->op_id_ == Instruction::Ret) {
//             exitBlock = bb;
//             break;
//         }
//     }
//     assert(exitBlock != nullptr);
//     std::set<BasicBlock *> visited = {};
//     getPostTraverse(exitBlock, visited);
//     reverse(reverseTraverse.begin(), reverseTraverse.end());
// }

// void ReverseDomainTree::getBlockDomR(std::string funcName)
// {
//     getReversePostTraverse(f);
//     auto root = exitBlock;
//     auto root_id = reverseTraverseInd[root];
//     for (int i = 0; i < root_id; i++)
//         reverseDomainBlock.push_back(nullptr);
//     reverseDomainBlock.push_back(root);
//     bool change = true;
//     while (change) {
//         change = false;
//         for (auto bb: reverseTraverse) {
//             if (bb != root) {
//                 BasicBlock * new_irdom = nullptr;
//                 for (auto rpred_bb: bb->succ_bbs_)
//                     if (reverseDomainBlock[reverseTraverseInd[rpred_bb]] != nullptr) {
//                         new_irdom = rpred_bb;
//                         break;
//                     }
//                 for (auto rpred_bb: bb->succ_bbs_)
//                     if (reverseDomainBlock[reverseTraverseInd[rpred_bb]] != nullptr)
//                         new_irdom = intersect(rpred_bb, new_irdom);
//                 if (reverseDomainBlock[reverseTraverseInd[bb]] != new_irdom) {
//                     reverseDomainBlock[reverseTraverseInd[bb]] = new_irdom;
//                     change = true;
//                 }
//             }
//         }
//     }
// }
// void ReverseDomainTree::getBlockRdoms(std::string funcName)
// {
//     for (auto bb: f->basic_blocks_) {
//         if (bb == exitBlock)
//             continue;
//         auto current = bb;
//         while (current != exitBlock) {
//             bb->rdoms_.insert(current);
//             current = reverseDomainBlock[reverseTraverseInd[current]];
//         }
//     }
// }
// void ReverseDomainTree::getBlockDomFrontR(std::string funcName)
// {
//     for (auto bb_iter = f->basic_blocks_.rbegin(); bb_iter != f->basic_blocks_.rend(); bb_iter++) {
//         auto bb = *bb_iter;
//         if (bb->succ_bbs_.size() >= 2) {
//             for (auto rpred: bb->succ_bbs_) {
//                 auto runner = rpred;
//                 while (runner != reverseDomainBlock[reverseTraverseInd[bb]]) {
//                     runner->rdom_frontier_.insert(bb);
//                     runner = reverseDomainBlock[reverseTraverseInd[runner]];
//                 }
//             }
//         }
//     }
// }
// BasicBlock * ReverseDomainTree::intersect(BasicBlock * block1, BasicBlock * block2)
// {
//     auto head1 = block1;
//     auto head2 = block2;
//     while (head1 != head2) {
//         while (reverseTraverseInd[head1] < reverseTraverseInd[head2])
//             head1 = reverseDomainBlock[reverseTraverseInd[head1]];
//         while (reverseTraverseInd[head2] < reverseTraverseInd[head1])
//             head2 = reverseDomainBlock[reverseTraverseInd[head2]];
//     }
//     return head1;
// }
