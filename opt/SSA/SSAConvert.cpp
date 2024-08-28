#include <string>
#include <iostream>
#include <vector>
#include "SSAConvert.h"
#include "BasicBlocks.h"
#include "IRInst.h"
bool ConvertSSA::isConvert(IRInst * inst)
{
    if (inst->getDst() == nullptr) {
        return false;
    }
    return inst->getDst()->isLocalVar();
}
void ConvertSSA::insertPhiFunc()
{}
void ConvertSSA::rwDefs()
{
    for (auto & funcName: this->funcName) {
        std::vector<BasicBlock> * basicblock = this->basicblocks->getBasicBlocksByFuncName(funcName);
        for (auto & block: *basicblock) {
            std::vector<IRInst *> IRInst = block.getOptIRInsts();
            for (auto & inst: IRInst) {
                //目的操作数是局部变量
                if (this->isConvert(inst)) {
                    //插入表里，tag++
                    this->basicblocks->insertLocalValueList(funcName, inst->getDst());
                    //获取当前tag
                    int tag = this->basicblocks->getTag(funcName, inst->getDst());
                    inst->setTag(tag);
                    // 重命名局部变量，加上tag——>注意:只是指令格式发生变化,方便分析数据流,Value属性没变,Inst增加新属性
                    // 用于产生SSA的IR
                    std::cout << block.getEntryName() << std::endl;
                    std::cout << inst->getDst()->getName() + " " + int2str(inst->getTag()) << std::endl;
                    std::cout << "——————————————————————————————————————" << std::endl;
                }
            }
        }
    }
    // for (auto & funcName: this->funcName) {
    //     for (auto & item: this->basicblocks->getLocalValueList(funcName)) {
    //         std::cout << item.first->getName() << " " << item.second << std::endl;
    //     }
    // }
}
// void ConvertSSA::record_var_rewrite(IRUnit * unit)
// {
//     const int block_count = m_idfn.size();
//     m_var_rewrite_block.clear();
//     m_current_var_symbol.clear();
//     int var_count = 0;
//     for (const auto & instr: unit->get_definations_const()) {
//         if (is_ssa_target_type(instr.a())) {
//             m_var_rewrite_block.push_back(BitMap(block_count));
//             m_current_var_symbol.push_back({});
//             m_current_var_symbol.back().push_back(instr.a()); //根据tag索引当前符号
//             instr.a()->set_tag(var_count);
//             var_count++;
//         }
//     }
//     //遍历每个block的每条语句，找到定值语句并填写变量在哪些block中被定过值
//     for (int i = block_count - 1; i >= 0; --i) {
//         IRBlock * block = m_idfn[i];
//         // entry节点必须被添加进去
//         if (block->is_entry()) {
//             for (int j = 0; j < var_count; ++j) {
//                 m_var_rewrite_block[j].set(i);
//             }
//         }
//         for (const auto & instr: block->get_instr_list_const()) {
//             if (instr.type() == IRType::Assign || instr.type() == IRType::ArrayLoad) {
//                 if (is_ssa_target_type(instr.r())) {
//                     assert(instr.r()->get_tag() >= 0 && instr.r()->get_tag() < m_var_rewrite_block.size());
//                     m_var_rewrite_block[instr.r()->get_tag()].set(i);
//                 }
//             }
//         }
//     }
// }
// void ConvertSSA::compute_dfp_for_all_vars()
// {
//     const int var_count = m_var_rewrite_block.size();
//     m_var_dominance_frontiers_plus.clear();
//     for (int i = 0; i < var_count; ++i) {
//         m_var_dominance_frontiers_plus.push_back(compute_dominance_frontiers_plus(m_var_rewrite_block[i]));
//     }
// #ifdef DEBUG_INFO_OPTIMIZER
//     std::cout << "Dominance Frontiers for vars:" << std::endl;
//     for (int i = 0; i < var_count; ++i) {
//         std::cout << "l" << i << " " << m_var_dominance_frontiers_plus[i].get_string() << std::endl;
//     }
// #endif
// }
// void ConvertSSA::insertPhiFunc()
// {
//     const int block_count = m_idfn.size();
//     const int var_count = m_current_var_symbol.size();
//     //遍历每个变量，在需要的block中添加phi函数
//     for (int k = var_count - 1; k >= 0; --k) {
//         for (int i = block_count - 1; i >= 0; --i) {
//             if (m_var_dominance_frontiers_plus[k].test(i)) {
//                 IRBlock * block = m_idfn[i];
//                 IRSymbol * r = m_current_var_symbol[k].back();
//                 block->add_instr_to_front(
//                     IRInstr::create_phi_func(r, m_ir_sym_table->create_phi_func(r->basic_type())));
//             }
//         }
//     }
// }
// //重写变量定值
// IRSymbol * ConvertSSA::rewrite_define(IRSymbol * sym)
// {
//     int ssa_index = m_var_ssa_index[sym->get_tag()]++;
//     m_current_var_symbol[sym->get_tag()].push_back(m_ir_sym_table->create_ssa(sym, ssa_index));
//     auto res = m_current_var_symbol[sym->get_tag()].back();
//     res->set_tag(sym->get_tag());
//     return res;
// }
// //重写变量使用
// IRSymbol * ConvertSSA::rewrite_use(IRSymbol * sym)
// {
//     return m_current_var_symbol[sym->get_tag()].back();
// }
// void ConvertSSA::rewrite_instructions(IRBlock * block)
// {
//     //维护符号栈增量记录，方便回退到本函数处理前的状态
//     const int var_count = m_current_var_symbol.size();
//     std::vector<int> rewrite_def_count(var_count);
//     for (auto & instr: block->get_instr_list()) {
//         //重写使用
//         bool rewrite_a = false;
//         bool rewrite_b = false;
//         bool rewrite_r = false;
//         switch (instr.type()) {
//             case IRType::BinaryCalc:
//                 rewrite_a = true;
//                 rewrite_b = true;
//                 break;
//             case IRType::UnaryCalc:
//                 rewrite_a = true;
//                 break;
//             case IRType::Assign:
//                 rewrite_a = true;
//                 break;
//             case IRType::RParam:
//                 rewrite_a = true;
//                 break;
//             case IRType::ArrayStore:
//                 rewrite_r = true;
//                 rewrite_a = true;
//                 rewrite_b = true;
//                 break;
//             case IRType::ArrayLoad:
//                 rewrite_a = true;
//                 rewrite_b = true;
//                 break;
//             case IRType::Call:
//             case IRType::CallWithRet:
//             case IRType::Return:
//                 break;
//             case IRType::ValReturn:
//                 rewrite_a = true;
//                 break;
//             case IRType::BlockGoto:
//                 break;
//             case IRType::BlockCondGoto:
//                 rewrite_a = true;
//                 break;
//             case IRType::PhiFunc:
//                 break;
//             default:
//                 Optimizer::optimizer_error("Unexpected instruction!");
//                 break;
//         }
//         if (rewrite_a && is_ssa_target_type(instr.a()))
//             instr.rebind_a(rewrite_use(instr.a()));
//         if (rewrite_b && is_ssa_target_type(instr.b()))
//             instr.rebind_b(rewrite_use(instr.b()));
//         if (rewrite_r && is_ssa_target_type(instr.r()))
//             instr.rebind_r(rewrite_use(instr.r()));
//         //重写定值
//         if (instr.type() == IRType::Assign || instr.type() == IRType::ArrayLoad || instr.type() ==
//         IRType::PhiFunc) {
//             if (is_ssa_target_type(instr.r())) {
//                 instr.rebind_r(rewrite_define(instr.r()));
//                 rewrite_def_count[instr.r()->get_tag()]++;
//             }
//         }
//     }
//     //为phi函数添加参数
//     const int odegree = block->out_degree();
//     for (int i = 0; i < odegree; ++i) {
//         auto child = block->get_succ(i);
//         for (auto & instr: child->get_instr_list()) {
//             if (instr.type() == IRType::PhiFunc) {
//                 instr.a()->add_phi_param(m_current_var_symbol[instr.r()->get_tag()].back(), block);
//             }
//         }
//     }
//     for (auto child: block->get_idom_child()) {
//         //递归
//         rewrite_instructions(child);
//     }
//     //回退符号栈
//     for (int i = 0; i < var_count; ++i) {
//         while (rewrite_def_count[i] > 0) {
//             m_current_var_symbol[i].pop_back();
//             rewrite_def_count[i]--;
//         }
//     }
// }
// void ConvertSSA::rewrite_defs(IRUnit * unit)
// {
//     for (auto & instr: unit->get_definations()) {
//         if (is_ssa_target_type(instr.a()))
//             instr.rebind_a(rewrite_define(instr.a()));
//     }
// }
// void ConvertSSA::run()
// {
//     std::cout << "Running pass: Convert SSA" << std::endl;
//     if (m_cfg == nullptr) {
//         Optimizer::optimizer_error("No target specified");
//     }
//     if (m_ir_sym_table == nullptr) {
//         Optimizer::optimizer_error("No IR symbol table specified");
//     }
//     //对每个unit进行SSA化
//     for (auto & unit: *m_cfg) {
//         if (unit.get_type() == IRUnitType::FuncDef) {
//             // //计算支配树,O(nlogn),n为基本块数
//             // CFGManager::build_dominator_tree(&unit);
//             // //对支配树做dfs，求出每个节点的dfn，并填写idfn数组,O(n)
//             // m_idfn.clear();
//             // compute_idfn(unit.get_entry());
//             // //计算每个基本块的支配边界,O(n^2)
//             // compute_dominance_frontiers();
//             //记录每个变量的定值信息
//             record_var_rewrite(&unit);
//             //计算每个变量的迭代支配边界,O(k*n^2),k为变量数
//             compute_dfp_for_all_vars();
//             //插入phi函数,O(k*n)
//             insert_phi_func();
//             m_var_ssa_index.clear();
//             m_var_ssa_index.resize(m_var_rewrite_block.size());
//             //重写所有变量定义语句,第一次定义时下标为_0
//             rewrite_defs(&unit);
//             //重写定值、使用语句,并为phi函数确定参数,O(k*n + i),i表示指令数
//             rewrite_instructions(unit.get_entry());
//             //删除exit里没用的phi函数
//             unit.get_exit()->get_instr_list().clear();
//         }
//     }
// }