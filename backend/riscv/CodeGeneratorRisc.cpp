#include "CodeGeneratorRisc.h"
#include "IRInst.h"
#include "RiscCode.h"
#include "SymbolTable.h"
#include "Value.h"
#include "ValueType.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <set>
#include <string>
#include <vector>
//超过10的数组存在全局
#define MaxSize 100
//全局变量（不包括const）
bool CodeGeneratorRisc::isGlobal(Value *var) {
  return (var->isLocalVar() && symtab.findSymbolValue(var));
}

//全局临时变量
bool CodeGeneratorRisc::isGlobalTemp(Value *var) {
  return (var->isTemp() && symtab.findSymbolValue(var));
}

void CodeGeneratorRisc::genHeader() {
  // std::string header = ".align 4\n";
  // fprintf(fp, "%s", header.c_str());
  // return header;
}

void CodeGeneratorRisc::genDataSection() {
  // fprintf(fp, "\t.text\n");
  Value *item_last;
  int interval = 0;
  bool flag = false;

  //变量计数器，用于判定是否进行最后一个zero追加
  int cnt = 0;
  for (auto item : symtab.getValueVector()) {
    // const全局数组
    if (item->is_numpy && item->isConst() && !item->isliteral()) {
      fprintf(fp, "\t.globl\t%s\n", item->getName().c_str());
      fprintf(fp, "\t.section\t.rodata\n");
      fprintf(fp, "\t.align\t4\n");
      fprintf(fp, "\t.type\t%s, @object\n", item->getName().c_str());
      fprintf(fp, "\t.size\t%s, %d\n", item->getName().c_str(),
              item->np->len * 4);
      fprintf(fp, "%s:\n", item->getName().c_str());
    }
    //全局数组
    if (item->is_numpy && !item->isConst() && !item->isliteral()) {
      fprintf(fp, "\t.globl\t%s\n", item->getName().c_str());
      fprintf(fp, "\t.data\n");
      fprintf(fp, "\t.align\t4\n");
      fprintf(fp, "\t.type\t%s, @object\n", item->getName().c_str());
      fprintf(fp, "\t.size\t%s, %d\n", item->getName().c_str(),
              item->np->len * 4);
      fprintf(fp, "%s:\n", item->getName().c_str());
      if (item->np->cnt == 0) {
        fprintf(fp, "\t.zero\t%d\n", item->np->len * 4);
      }
    }
    //初始化，找到第一个数组元素
    if (item->isTemp() && item->is_issavenp() && !flag) {
      flag = true;
      item_last = item;
    }
    //遍历数组中的元素
    if (item->isTemp() && item->is_issavenp()) {
      cnt++;
      // interval为两个元素之间的间隔（线性的）
      interval = item->indexLinear - item_last->indexLinear - 1;
      // TODO;字节对齐
      if (interval > 0) {
        fprintf(fp, "\t.zero\t%d\n", interval * 4);
      }
      fprintf(fp, "\t.word");
      fprintf(fp, "\t%d\n", item->intVal);
      item_last = item;
      // enter = false;
    }
    if (item->isTemp() && item->is_issavenp() && cnt == item->np->cnt) {
      flag = false;
      cnt = 0;
      fprintf(fp, "\t.zero\t%d\n", (item->np->len - item->indexLinear - 1) * 4);
    }
    // Const全局变量
    if (item->isConst() && !item->isliteral() && !item->is_numpy) {
      fprintf(fp, "\t.globl\t%s\n", item->getName().c_str());
      fprintf(fp, "\t.section\t.rodata\n");
      fprintf(fp, "\t.align\t2\n");
      fprintf(fp, "\t.type\t%s, @object\n", item->getName().c_str());
      fprintf(fp, "\t.size\t%s, %d\n", item->getName().c_str(),
              item->getSize());
      fprintf(fp, "%s:\n", item->getName().c_str());
      if (item->type.type == BasicType::TYPE_INT) {
        fprintf(fp, "\t.word\t%d\n", item->intVal);
      }
    }
    //全局变量
    if (item->isLocalVar() && !item->is_numpy) {
      fprintf(fp, "\t.globl\t%s\n", item->getName().c_str());
      fprintf(fp, "\t.data\n");
      fprintf(fp, "\t.align\t2\n");
      fprintf(fp, "\t.type\t%s, @object\n", item->getName().c_str());
      fprintf(fp, "\t.size\t%s, %d\n", item->getName().c_str(),
              item->getSize());
      fprintf(fp, "%s:\n", item->getName().c_str());
      if (item->type.type == BasicType::TYPE_INT) {
        fprintf(fp, "\t.word\t%d\n", item->intVal);
      }
    }
  }
  for (auto func : symtab.getFunctionList()) {
    for (auto var : func->getVarValues()) {
      //找到局部数组——>大数组优化到全局
      if (var->is_numpy && !var->is_issavenp() && var->np != nullptr &&
          !var->np->_flag) {
        //超过MaxSize的优化到全局
        if (var->np != nullptr && var->np->len > MaxSize) {
          fprintf(fp, "\t.globl\t%s\n", var->np->np_name.c_str());
          fprintf(fp, "\t.data\n");
          fprintf(fp, "\t.align\t4\n");
          fprintf(fp, "\t.type\t%s, @object\n", var->np->np_name.c_str());
          fprintf(fp, "\t.size\t%s, %d\n", var->np->np_name.c_str(),
                  var->np->len * 4);
          fprintf(fp, "%s:\n", var->np->np_name.c_str());
          if (var->np->cnt == 0) {
            fprintf(fp, "\t.zero\t%d\n", var->np->len * 4);
          }
        }
      }
    }
  }
}
// TODO:字节对齐封装到这个方法
int CodeGeneratorRisc::byteAlign(Value *var) {
  switch (var->type.type) {
  case (BasicType::TYPE_INT):
    return (var->getSize() + 3) / 4 * 4;
  case (BasicType::TYPE_FLOAT):
    return (var->getSize() + 7) / 8 * 8;
  case (BasicType::TYPE_FNP_I):
    return (var->getSize() + 7) / 8 * 8;
  case (BasicType::TYPE_FNP_F):
    return (var->getSize() + 7) / 8 * 8;
  default:
    return (var->getSize() + 7) / 8 * 8;
  }
}

CodeGeneratorRisc::CodeGeneratorRisc(SymbolTable &tab)
    : CodeGeneratorAsm(tab) {}

CodeGeneratorRisc::~CodeGeneratorRisc() {}

void CodeGeneratorRisc::genCodeSection(Function *fun) {
  registerAllocation(fun);

  generateCode(fun->getInterCode().getInsts(), fun);

  std::string name = fun->getName();
  std::string asmName = name[0] == '@' ? name.substr(1) : name;
  fprintf(fp, "\t.align\t1\n");
  fprintf(fp, "\t.globl\t%s\n", asmName.c_str());
  fprintf(fp, "\t.type\t%s, @function\n", asmName.c_str());
  fprintf(fp, "%s:\n", asmName.c_str());

  for (auto item : code_seq) {
    fprintf(fp, "%s\n", item->toString().c_str());
  }

  for (size_t i = 0; i < real_const.size(); i++) {
    fprintf(fp, ".LC%d:\n", (int)i);
    fprintf(fp, "\t.word  %d", (int32_t)real_const[i]);
  }

  code_seq.clear();
  real_const.clear();
  for (auto item : fun->getVarValues())
    item->baseRegNo = item->regId = -1;
}

int32_t CodeGeneratorRisc::getReg(Value *var, int32_t reg) {
  if (var->regId == -1 || var->regId >= 28) {
    var->regId = reg;
    return reg;
  } else
    return var->regId;
}

void CodeGeneratorRisc::load_var(Value *var, int32_t reg) {
  bool is_float = var->type.type == BasicType::TYPE_FLOAT;
  std::string *regs = is_float ? RiscInst::f_regname : RiscInst::regname;
  InstType load = var->getSize() == 8 ? InstType::ld : InstType::lw;
  if (var->regId >= 28) {
    // TODO:除了全局数组部分，其他的const全局变量和非const的全局变量都需要修改到.data段
    //  const和字面量
    if ((var->isConst() || var->isliteral())) {
      //全局Const变量
      if (var->isConst() && !var->isliteral() && symtab.findSymbolValue(var)) {
        if (var->is_numpy) {
          code_seq.push_back(new RiscInst(InstType::lla, regs[var->regId],
                                          var->getName(), ""));
        } else {
          // TODO
          // 全局的const变量
          // code_seq.push_back(
          //     new RiscInst(load, regs[var->regId], regs[REG_FP],
          //     std::to_string(-var->getOffset())));
          code_seq.push_back(new RiscInst(InstType::lui, regs[31], "",
                                          "%hi(" + var->getName() + ")"));
          code_seq.push_back(new RiscInst(InstType::lw, regs[var->regId],
                                          regs[31],
                                          "%lo(" + var->getName() + ")"));
          // code_seq.push_back(new RiscInst(InstType::lla, regs[31],
          // var->getName(), "")); code_seq.push_back(new RiscInst(InstType::ld,
          // regs[var->regId], regs[31], std::to_string(0)));
        }
      }
      // 局部const变量
      else if (var->isConst() && !var->isliteral() &&
               !symtab.findSymbolValue(var)) {
        if (var->is_numpy) {
          // 局部的const数组
          // 优化到全局
          if (!var->is_issavenp() && var->np != nullptr && !var->np->_flag &&
              (var->np->len > MaxSize)) {
            code_seq.push_back(new RiscInst(InstType::lla, regs[var->regId],
                                            var->np->np_name, ""));
          } else {
            if (var->getOffset() != 0) {
              if (var->getOffset() < 2048) {
                code_seq.push_back(
                    new RiscInst(load, regs[var->regId], regs[REG_FP],
                                 std::to_string(-var->getOffset())));
              } else {
                code_seq.push_back(
                    new RiscInst(InstType::li, regs[5], "",
                                 std::to_string(-var->getOffset())));
                code_seq.push_back(new RiscInst(InstType::add, regs[5],
                                                regs[REG_FP], regs[5]));
                code_seq.push_back(new RiscInst(load, regs[var->regId], regs[5],
                                                std::to_string(0)));
              }
            } else {
              code_seq.push_back(new RiscInst(InstType::li, regs[var->regId],
                                              "", std::to_string(var->intVal)));
            }
          }
        } else {
          //局部的const变量
          if (var->getOffset() < 2048) {
            code_seq.push_back(new RiscInst(load, regs[var->regId],
                                            regs[REG_FP],
                                            std::to_string(-var->getOffset())));
          } else {
            code_seq.push_back(new RiscInst(InstType::li, regs[5], "",
                                            std::to_string(-var->getOffset())));
            code_seq.push_back(
                new RiscInst(InstType::add, regs[5], regs[REG_FP], regs[5]));
            code_seq.push_back(new RiscInst(load, regs[var->regId], regs[5],
                                            std::to_string(0)));
          }
        }
      } else {
        // 全局、局部的字面量——>立即数
        code_seq.push_back(new RiscInst(InstType::li, regs[var->regId], "",
                                        std::to_string(var->intVal)));
      }
    } else if (isGlobal(var)) {
      //  TODO:
      // 全局数组（非const的）
      if (var->is_numpy) {
        code_seq.push_back(
            new RiscInst(InstType::lla, regs[var->regId], var->getName(), ""));
      } else {
        //  TODO:
        // 全局变量（非const的）
        // code_seq.push_back(
        //     new RiscInst(load, regs[var->regId], regs[REG_FP],
        //     std::to_string(-var->getOffset())));
        code_seq.push_back(new RiscInst(InstType::lui, regs[31], "",
                                        "%hi(" + var->getName() + ")"));
        code_seq.push_back(new RiscInst(InstType::lw, regs[var->regId],
                                        regs[31],
                                        "%lo(" + var->getName() + ")"));
        // code_seq.push_back(new RiscInst(InstType::lla, regs[31],
        // var->getName(), "")); code_seq.push_back(new RiscInst(InstType::ld,
        // regs[var->regId], regs[31], std::to_string(0)));
      }
    } else {
      //局部数组（非const的）
      if (var->is_numpy) {
        //优化到全局
        if (!var->is_issavenp() && var->np != nullptr && !var->np->_flag &&
            (var->np->len > MaxSize)) {
          code_seq.push_back(new RiscInst(InstType::lla, regs[var->regId],
                                          var->np->np_name, ""));
        } else {
          if (var->getOffset() != 0) {
            if (var->getOffset() < 2048) {
              code_seq.push_back(
                  new RiscInst(load, regs[var->regId], regs[REG_FP],
                               std::to_string(-var->getOffset())));
            } else {
              code_seq.push_back(
                  new RiscInst(InstType::li, regs[5], "",
                               std::to_string(-var->getOffset())));
              code_seq.push_back(
                  new RiscInst(InstType::add, regs[5], regs[REG_FP], regs[5]));
              code_seq.push_back(new RiscInst(load, regs[var->regId], regs[5],
                                              std::to_string(0)));
            }
          } else {
            code_seq.push_back(new RiscInst(InstType::li, regs[var->regId], "",
                                            std::to_string(var->intVal)));
          }
        }

      } else {
        //局部变量（非const）
        if (var->getOffset() < 2048) {
          code_seq.push_back(new RiscInst(load, regs[var->regId], regs[REG_FP],
                                          std::to_string(-var->getOffset())));
        } else {
          code_seq.push_back(new RiscInst(InstType::li, regs[5], "",
                                          std::to_string(-var->getOffset())));
          code_seq.push_back(
              new RiscInst(InstType::add, regs[5], regs[REG_FP], regs[5]));
          code_seq.push_back(
              new RiscInst(load, regs[var->regId], regs[5], std::to_string(0)));
        }
      }
    }
  }

  if (var->regId != reg) {
    code_seq.push_back(
        new RiscInst(InstType::add, regs[reg], regs[0], regs[var->regId]));
  }
}

void CodeGeneratorRisc::store_var(Value *var, int32_t reg) {
  bool is_float = var->type.type == BasicType::TYPE_FLOAT;
  std::string *regs = is_float ? RiscInst::f_regname : RiscInst::regname;
  InstType store = var->getSize() == 8 ? InstType::sd : InstType::sw;
  // TODO:全局变量存储优化——>data段？
  if (var->regId >= 28) {
    if (isGlobal(var)) {
      code_seq.push_back(new RiscInst(InstType::lui, regs[31], "",
                                      "%hi(" + var->getName() + ")"));
      code_seq.push_back(new RiscInst(InstType::sw, regs[var->regId], regs[31],
                                      "%lo(" + var->getName() + ")"));
      // code_seq.push_back(new RiscInst(InstType::lla, regs[31],
      // var->getName(), "")); code_seq.push_back(new RiscInst(InstType::sd,
      // regs[var->regId], regs[31], std::to_string(0)));
    } else {
      // 大于MaxSize的局部数组用全局存
      if (var->is_numpy && !var->is_issavenp() && var->np != nullptr &&
          !var->np->_flag && var->np->len > MaxSize) {
        code_seq.push_back(new RiscInst(InstType::lui, regs[31], "",
                                        "%hi(" + var->np->np_name + ")"));
        code_seq.push_back(new RiscInst(InstType::sw, regs[var->regId],
                                        regs[31],
                                        "%lo(" + var->np->np_name + ")"));
      } else {
        if (var->getOffset() < 2048) {
          code_seq.push_back(new RiscInst(store, regs[reg], regs[REG_FP],
                                          std::to_string(-var->getOffset())));
        } else {
          code_seq.push_back(new RiscInst(InstType::li, regs[5], "",
                                          std::to_string(-var->getOffset())));
          code_seq.push_back(
              new RiscInst(InstType::add, regs[5], regs[REG_FP], regs[5]));
          code_seq.push_back(
              new RiscInst(store, regs[reg], regs[5], std::to_string(0)));
        }
      }
    }

    return;
  }
  if (var->regId != reg)
    code_seq.push_back(
        new RiscInst(InstType::mv, regs[reg], regs[var->regId], ""));
}

// TODO:寄存器分配中，局部变量和函数形参变量存在栈中，Const变量（全局、局部,包括const修饰的数组）存在.rodata段，【非const】数组变量（全局，局部）优化到.data段
// TODO:全局变量存在.data段，临时变量不存在栈里，直接用寄存器代替，立即数也不存在栈中，直接li进来
void CodeGeneratorRisc::registerAllocation(Function *fun) {
  std::vector<IRInst *> interinst = fun->getInterCode().getInsts();
  std::set<std::string> const_name;
  // simple liveness analysis
  int32_t cnt = 0, fcnt = 0;

  // formal params pass
  int32_t sp_offset = 0;
  std::vector<FuncFormalParam> params = fun->getParams();
  //先处理存在栈里的形参
  for (int i = 4; i < (int32_t)params.size(); i++) {
    // TODO:现在只考虑了int
    Value *var = fun->findValue(
        "%t" + std::to_string(std::stoi(params[i].val->getName().substr(2)) -
                              params.size()));
    var->baseRegNo = REG_SP;
    int32_t temp_size = (params[i].val->getSize() + 3) / 4 * 4;
    sp_offset += temp_size;
    var->setOffset(sp_offset);
  }

  // special for a0
  if (params.size()) {
    Value *var = fun->findValue(
        "%t" + std::to_string(std::stoi(params[0].val->getName().substr(2)) -
                              params.size()));
    int32_t size = params[0].val->getSize();
    if (size <= 0)
      size = 1;
    sp_offset += (size + 7) / 8 * 8;
    // sp_offset = byteAlign(var);
    var->baseRegNo = REG_SP;
    var->setOffset(sp_offset);
    if (sp_offset < 2048) {
      code_seq.push_back(
          new RiscInst(var->getOffset() == 8 ? InstType::sd : InstType::sw,
                       RiscInst::regname[10], RiscInst::regname[REG_SP],
                       std::to_string(-sp_offset)));
    } else {
      code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5],
                                      RiscInst::regname[5],
                                      std::to_string(-sp_offset)));
      code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                      RiscInst::regname[REG_SP],
                                      RiscInst::regname[5]));
      code_seq.push_back(new RiscInst(
          var->getOffset() == 8 ? InstType::sd : InstType::sw,
          RiscInst::regname[10], RiscInst::regname[5], std::to_string(0)));
    }
  }

  for (int i = 1; i < (int32_t)params.size() && i < 4; i++) {
    Value *var = fun->findValue(
        "%t" + std::to_string(std::stoi(params[i].val->getName().substr(2)) -
                              params.size()));
    var->baseRegNo = REG_SP;
    if (var->type.type == BasicType::TYPE_FLOAT) {
      var->regId = 11 + fcnt;
      fcnt++;
    } else {
      var->regId = 11 + cnt;
      cnt++;
    }
  }
  // for (auto var: symtab.getValueVector()) { // Get all the variables in the
  // function.
  //     if (var == nullptr || var->getOffset() != 0 || var->regId != -1)
  //         continue;
  //     bool is_float = var->type.type == BasicType::TYPE_FLOAT;
  //     if (is_float && var->isConst())
  //         real_const.push_back(var->realVal);
  //     // alloc space in stack
  //     // //
  //     全局变量、const修饰的全局变量、临时变量、指针变量（最后一个判定条件，取到数组地址）
  //     // if ((var->isLocalVar() && !var->is_numpy && var->regId == -1 &&
  //     var->baseRegNo == -1) ||
  //     //     (var->isConst() && !var->isliteral() && !var->is_numpy &&
  //     var->regId == -1 && var->baseRegNo
  //     //     == -1) || (var->isTemp() && !var->is_issavenp() && var->regId ==
  //     -1 && var->baseRegNo == -1)
  //     //     || (var->isTemp() && var->is_issavenp() && var->is_numpy &&
  //     var->np == nullptr && var->regId
  //     //     == -1 &&
  //     //      var->baseRegNo == -1)) {
  //     //     int32_t size = var->getSize();
  //     //     if (size <= 0)
  //     //         size = 1;
  //     //     size = (size + 7) / 8 * 8; // 8字节对齐
  //     //     sp_offset += size;
  //     //     var->setOffset(sp_offset);
  //     //     var->baseRegNo = REG_SP;
  //     //     //全局变量、全局Const变量初始化为0
  //     //     if (var->isLocalVar() || (var->isConst() && !var->isliteral()))
  //     {
  //     //         code_seq.push_back(new RiscInst(InstType::li,
  //     RiscInst::regname[28], " ",
  //     //         std::to_string(0))); code_seq.push_back(new
  //     RiscInst(InstType::sd,
  //     //                                         RiscInst::regname[28],
  //     //                                         RiscInst::regname[REG_SP],
  //     // std::to_string(-sp_offset)));
  //     //     }
  //     // }

  //     // // TODO:临时变量后续优化到寄存器
  //     // if (var->isTemp() && !var->is_numpy && !var->is_issavenp()) {
  //     //     int32_t size = var->getSize();
  //     //     if (size <= 0)
  //     //         size = 1;
  //     //     size = (size + 3) / 4 * 4; // 4字节对齐
  //     //     sp_offset += size;
  //     //     var->setOffset(sp_offset);
  //     //     var->baseRegNo = REG_SP;
  //     // }

  //     // TODO:取负需要修改

  //     //
  //     TODO:全局数组优化到.data段;const全局数组、const局部数组、const全局变量、const局部变量优化到.rodata段
  //     // TODO:目前只是支持单个函数内部调用
  //     // 处理数组变量名
  //     if (var->is_numpy && !var->is_issavenp() && var->np != nullptr &&
  //     !var->np->_flag && var->regId == -1 &&
  //         var->baseRegNo == -1) {
  //         int32_t size = var->getSize();
  //         if (size <= 0)
  //             size = 1;
  //         size = (size + 3) / 4 * 4; // 4字节对齐
  //         sp_offset += size;
  //         var->setOffset(sp_offset);
  //         var->baseRegNo = REG_SP;
  //         int32_t arr_size = 1;
  //         for (auto item: var->np->np_sizes)
  //             arr_size *= item;
  //         // TODO:现在只考虑了int
  //         sp_offset += arr_size * 4;
  //         sp_offset -= 4;
  //         var->setOffset(sp_offset);
  //         // 数组初始化为0
  //         for (int offset = arr_size; offset > 0; offset--) {
  //             code_seq.push_back(new RiscInst(InstType::li,
  //             RiscInst::regname[28], "", std::to_string(0)));
  //             code_seq.push_back(new RiscInst(InstType::sw,
  //                                             RiscInst::regname[28],
  //                                             RiscInst::regname[REG_SP],
  //                                             std::to_string(-(sp_offset - 4
  //                                             * offset))));
  //         }
  //         code_seq.push_back(new RiscInst(InstType::addi,
  //                                         RiscInst::regname[28],
  //                                         RiscInst::regname[REG_SP],
  //                                         std::to_string(-(sp_offset - 4))));
  //         sp_offset += 4;
  //         var->setOffset(sp_offset);
  //         //存数组首元素地址
  //         code_seq.push_back(new RiscInst(InstType::sd,
  //                                         RiscInst::regname[28],
  //                                         RiscInst::regname[REG_SP],
  //                                         std::to_string(-(sp_offset))));
  //     }
  //     //处理数组元素
  //     if (var->np != nullptr && var->isTemp() && var->is_issavenp() &&
  //     !var->np->_flag && var->regId == -1 &&
  //         var->baseRegNo == -1) {
  //         int32_t size = var->getSize();
  //         if (size <= 0)
  //             size = 1;
  //         size = (size + 7) / 8 * 8; // 8字节对齐
  //         sp_offset += size;
  //         var->setOffset(sp_offset);
  //         var->baseRegNo = REG_SP;
  //         if (sp_offset < 2048) {
  //             code_seq.push_back(new RiscInst(InstType::addi,
  //                                             RiscInst::regname[28],
  //                                             RiscInst::regname[REG_SP],
  //                                             std::to_string(-sp_offset)));
  //         } else {
  //             code_seq.push_back(new RiscInst(InstType::li,
  //             RiscInst::regname[5], "", std::to_string(-sp_offset)));
  //             code_seq.push_back(new RiscInst(InstType::add,
  //                                             RiscInst::regname[28],
  //                                             RiscInst::regname[REG_SP],
  //                                             RiscInst::regname[5]));
  //         }

  //         if (var->getOffset() < 2048) {
  //             code_seq.push_back(new RiscInst(InstType::sd,
  //                                             RiscInst::regname[28],
  //                                             RiscInst::regname[REG_SP],
  //                                             std::to_string(-var->getOffset())));
  //         } else {
  //             code_seq.push_back(
  //                 new RiscInst(InstType::li, RiscInst::regname[5], "",
  //                 std::to_string(-var->getOffset())));
  //             code_seq.push_back(
  //                 new RiscInst(InstType::add, RiscInst::regname[5],
  //                 RiscInst::regname[REG_SP], RiscInst::regname[5]));
  //             code_seq.push_back(
  //                 new RiscInst(InstType::sd, RiscInst::regname[28],
  //                 RiscInst::regname[5], std::to_string(0)));
  //         }
  //     }
  //     // std::cout << var->getName() << " " << var->getOffset() << std::endl;
  // }
  for (auto var :
       fun->getVarValues()) { // Get all the variables in the function.
    if (var == nullptr || var->getOffset() != 0 || var->regId != -1)
      continue;
    bool is_float = var->type.type == BasicType::TYPE_FLOAT;
    if (is_float && var->isConst())
      real_const.push_back(var->realVal);
    // alloc space in stack
    // 局部变量、const修饰的局部变量、临时变量、指针变量（最后一个判定条件，取到数组地址）、函数形参
    if ((var->isLocalVar() && !var->is_numpy && var->regId == -1 &&
         var->baseRegNo == -1) ||
        (var->isConst() && !var->isliteral() && !var->is_numpy &&
         var->regId == -1 && var->baseRegNo == -1) ||
        (var->isTemp() && !var->is_issavenp() && var->regId == -1 &&
         var->baseRegNo == -1) ||
        (var->isTemp() && var->is_issavenp() && var->is_numpy &&
         var->np == nullptr && var->regId == -1 && var->baseRegNo == -1) ||
        (var->is_saveFParam && var->regId == -1 && var->baseRegNo == -1)) {
      int32_t size = var->getSize();
      if (size <= 0)
        size = 1;
      size = (size + 3) / 4 * 4; // 4字节对齐
      sp_offset += size;
      var->setOffset(sp_offset);
      var->baseRegNo = REG_SP;
    }
    // 处理数组变量名，包含const和一般的
    // TODO:局部数组优化存储
    if (var->is_numpy && !var->is_issavenp() && var->np != nullptr &&
        !var->np->_flag && var->regId == -1 && var->baseRegNo == -1) {
      //大于MaxSize的用全局存
      if (var->np->len > MaxSize) {
        continue;
      }
      int32_t size = var->getSize();
      if (size <= 0)
        size = 1;
      size = (size + 3) / 4 * 4; // 4字节对齐
      sp_offset += size;
      var->setOffset(sp_offset);
      var->baseRegNo = REG_SP;
      int32_t arr_size = 1;
      for (auto item : var->np->np_sizes)
        arr_size *= item;
      sp_offset += arr_size * 4;
      sp_offset -= 4;
      var->setOffset(sp_offset);
      // // 数组初始化为0
      // for (int offset = arr_size; offset > 0; offset--) {
      //     code_seq.push_back(new RiscInst(InstType::li,
      //     RiscInst::regname[28], "", std::to_string(0)));
      //     code_seq.push_back(new RiscInst(InstType::sw,
      //                                     RiscInst::regname[28],
      //                                     RiscInst::regname[REG_SP],
      //                                     std::to_string(-(sp_offset - 4 *
      //                                     offset))));
      // }
      // code_seq.push_back(new RiscInst(InstType::addi,
      //                                 RiscInst::regname[28],
      //                                 RiscInst::regname[REG_SP],
      //                                 std::to_string(-(sp_offset - 4))));
      sp_offset += 4;
      var->setOffset(sp_offset);
      // //存数组首元素地址
      // code_seq.push_back(new RiscInst(InstType::sd,
      //                                 RiscInst::regname[28],
      //                                 RiscInst::regname[REG_SP],
      //                                 std::to_string(-(sp_offset))));
    }
    //处理数组元素
    if (var->np != nullptr && var->isTemp() && var->is_issavenp() &&
        !var->np->_flag && var->regId == -1 && var->baseRegNo == -1) {
      //大于MaxSize的用全局存
      if (var->np->len > MaxSize) {
        continue;
      }
      if (var->np->offset == 0) {
        int32_t size = var->getSize();
        if (size <= 0)
          size = 1;
        size = (size + 7) / 8 * 8; // 8字节对齐
        sp_offset += size;
        var->np->offset = sp_offset;
        var->setOffset(sp_offset);
        var->baseRegNo = REG_SP;
      } else {
        var->setOffset(var->np->offset);
        var->baseRegNo = REG_SP;
      }

      // if (sp_offset < 2048) {
      //     code_seq.push_back(new RiscInst(InstType::addi,
      //                                     RiscInst::regname[28],
      //                                     RiscInst::regname[REG_SP],
      //                                     std::to_string(-sp_offset)));
      // } else {
      //     code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5],
      //     "", std::to_string(-sp_offset))); code_seq.push_back(new
      //     RiscInst(InstType::add,
      //                                     RiscInst::regname[28],
      //                                     RiscInst::regname[REG_SP],
      //                                     RiscInst::regname[5]));
      // }

      // if (var->getOffset() < 2048) {
      //     code_seq.push_back(new RiscInst(InstType::sd,
      //                                     RiscInst::regname[28],
      //                                     RiscInst::regname[REG_SP],
      //                                     std::to_string(-var->getOffset())));
      // } else {
      //     code_seq.push_back(
      //         new RiscInst(InstType::li, RiscInst::regname[5], "",
      //         std::to_string(-var->getOffset())));
      //     code_seq.push_back(
      //         new RiscInst(InstType::add, RiscInst::regname[5],
      //         RiscInst::regname[REG_SP], RiscInst::regname[5]));
      //     code_seq.push_back(
      //         new RiscInst(InstType::sd, RiscInst::regname[28],
      //         RiscInst::regname[5], std::to_string(0)));
      // }
    }
    // std::cout << var->getName() << " " << var->getOffset() << std::endl;
  }

  sp_offset += 16 + cnt * 8;
  sp_offset += fcnt * 8;

  // protect registers
  // 栈帧修改为16字节对齐
  sp_offset = (sp_offset + 15) / 16 * 16;
  fun->setMaxDep(sp_offset);
  if (sp_offset < 2048) {
    code_seq.push_back(new RiscInst(InstType::addi, RiscInst::regname[REG_SP],
                                    RiscInst::regname[REG_SP],
                                    std::to_string(-sp_offset)));
  } else {
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5], "",
                                    std::to_string(-sp_offset)));
    code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[REG_SP],
                                    RiscInst::regname[REG_SP],
                                    RiscInst::regname[5]));
  }
  code_seq.push_back(new RiscInst(InstType::sd, RiscInst::regname[REG_RA],
                                  RiscInst::regname[REG_SP],
                                  std::to_string(cnt * 8 + fcnt * 8 + 8)));
  code_seq.push_back(new RiscInst(InstType::sd, RiscInst::regname[REG_FP],
                                  RiscInst::regname[REG_SP],
                                  std::to_string(cnt * 8 + fcnt * 8)));
  for (int i = 0; i < cnt; i++)
    code_seq.push_back(new RiscInst(InstType::sd, RiscInst::regname[11 + i],
                                    RiscInst::regname[REG_SP],
                                    std::to_string(8 * (i + fcnt))));
  // set fp
  for (int i = 0; i < fcnt; i++)
    code_seq.push_back(new RiscInst(InstType::fsd, RiscInst::f_regname[11 + i],
                                    RiscInst::regname[REG_SP],
                                    std::to_string(8 * i)));

  if (sp_offset < 2048) {
    code_seq.push_back(new RiscInst(InstType::addi, RiscInst::regname[REG_FP],
                                    RiscInst::regname[REG_SP],
                                    std::to_string(sp_offset)));
  } else {
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5], "",
                                    std::to_string(sp_offset)));
    code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[REG_FP],
                                    RiscInst::regname[REG_SP],
                                    RiscInst::regname[5]));
  }

  for (auto var :
       fun->getVarValues()) { // Get all the variables in the function.
    if (var->is_numpy && !var->is_issavenp() && var->np != nullptr &&
        !var->np->_flag) {
      //大于MaxSize的用全局存
      if (var->np->len > MaxSize) {
        continue;
      }
      int32_t arr_size = 1;
      for (auto item : var->np->np_sizes)
        arr_size *= item;
      var->setOffset(var->getOffset() - 4);
      // 数组初始化为0
      for (int offset = arr_size; offset > 0; offset--) {
        code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[28], "",
                                        std::to_string(0)));
        code_seq.push_back(new RiscInst(
            InstType::sw, RiscInst::regname[28], RiscInst::regname[REG_FP],
            std::to_string(-(var->getOffset() - 4 * offset))));
      }
      code_seq.push_back(new RiscInst(InstType::addi, RiscInst::regname[28],
                                      RiscInst::regname[REG_FP],
                                      std::to_string(-(var->getOffset() - 4))));
      var->setOffset(var->getOffset() + 4);
      //存数组首元素地址
      code_seq.push_back(new RiscInst(InstType::sd, RiscInst::regname[28],
                                      RiscInst::regname[REG_FP],
                                      std::to_string(-(var->getOffset()))));
    }
    //处理数组元素
    if (var->np != nullptr && var->isTemp() && var->is_issavenp() &&
        !var->np->_flag) {
      //大于MaxSize的用全局存
      if (var->np->len > MaxSize) {
        continue;
      }
      if (!var->np->is_Store) {
        var->np->is_Store = true;
        if (var->getOffset() < 2048) {
          code_seq.push_back(new RiscInst(InstType::addi, RiscInst::regname[28],
                                          RiscInst::regname[REG_FP],
                                          std::to_string(-var->getOffset())));
        } else {
          code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5],
                                          "",
                                          std::to_string(-var->getOffset())));
          code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[28],
                                          RiscInst::regname[REG_FP],
                                          RiscInst::regname[5]));
        }

        if (var->getOffset() < 2048) {
          code_seq.push_back(new RiscInst(InstType::sd, RiscInst::regname[28],
                                          RiscInst::regname[REG_FP],
                                          std::to_string(-var->getOffset())));
        } else {
          code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5],
                                          "",
                                          std::to_string(-var->getOffset())));
          code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                          RiscInst::regname[REG_FP],
                                          RiscInst::regname[5]));
          code_seq.push_back(new RiscInst(InstType::sd, RiscInst::regname[28],
                                          RiscInst::regname[5],
                                          std::to_string(0)));
        }
      }
    }
    // std::cout << var->getName() << " " << var->getOffset() << std::endl;
  }
}

void CodeGeneratorRisc::generateCode(std::vector<IRInst *> &inst_seq,
                                     Function *fun) {
  // for (auto inst: symtab.getInterCode().getInsts()) {
  //     //这部分是为了全局数组的初始化——>TODO:需要后续修改，不在栈上加载，直接进行初始化
  //     std::string temp;
  //     inst->toString(temp);
  //     switch (inst->getOp()) {
  //         case IRInstOperator::IRINST_OP_ENTRY:
  //             translate_entry(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_EXIT:
  //             translate_exit(inst, fun);
  //             break;
  //         case IRInstOperator::IRINST_OP_LABEL:
  //             translate_label(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_BR:
  //             translate_br(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_BC:
  //             translate_bc(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_ADD_I:
  //             translate_addi(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_SUB_I:
  //             translate_subi(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_MULT_I:
  //             translate_muli(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_DIV_I:
  //             translate_divi(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_MOD_I:
  //             translate_remi(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_ASSIGN:
  //             translate_assign(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_FUNC_CALL:
  //             translate_funcall(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_EQ:
  //             translate_cmp_eq(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_LE:
  //             translate_cmp_le(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_LT:
  //             translate_cmp_lt(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_BT:
  //             translate_cmp_gt(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_BE:
  //             translate_cmp_ge(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_NQ:
  //             translate_cmp_neq(inst);
  //             break;
  //         case IRInstOperator::IRINST_OP_MAX:
  //             translate_max(inst);
  //             break;
  //         default:
  //             break;
  //     }
  // }
  for (auto inst : inst_seq) {
    std::string temp;
    inst->toString(temp);
    switch (inst->getOp()) {
    case IRInstOperator::IRINST_OP_ENTRY:
      translate_entry(inst);
      break;
    case IRInstOperator::IRINST_OP_EXIT:
      translate_exit(inst, fun);
      break;
    case IRInstOperator::IRINST_OP_LABEL:
      translate_label(inst);
      break;
    case IRInstOperator::IRINST_OP_BR:
      translate_br(inst);
      break;
    case IRInstOperator::IRINST_OP_BC:
      translate_bc(inst);
      break;
    case IRInstOperator::IRINST_OP_ADD_I:
      translate_addi(inst);
      break;
    case IRInstOperator::IRINST_OP_SUB_I:
      translate_subi(inst);
      break;
    case IRInstOperator::IRINST_OP_MULT_I:
      translate_muli(inst);
      break;
    case IRInstOperator::IRINST_OP_DIV_I:
      translate_divi(inst);
      break;
    case IRInstOperator::IRINST_OP_MOD_I:
      translate_remi(inst);
      break;
    case IRInstOperator::IRINST_OP_ASSIGN:
      translate_assign(inst);
      break;
    case IRInstOperator::IRINST_OP_FUNC_CALL:
      translate_funcall(inst);
      break;
    case IRInstOperator::IRINST_OP_EQ:
      translate_cmp_eq(inst);
      break;
    case IRInstOperator::IRINST_OP_LE:
      translate_cmp_le(inst);
      break;
    case IRInstOperator::IRINST_OP_LT:
      translate_cmp_lt(inst);
      break;
    case IRInstOperator::IRINST_OP_BT:
      translate_cmp_gt(inst);
      break;
    case IRInstOperator::IRINST_OP_BE:
      translate_cmp_ge(inst);
      break;
    case IRInstOperator::IRINST_OP_NQ:
      translate_cmp_neq(inst);
      break;
    case IRInstOperator::IRINST_OP_MAX:
      translate_max(inst);
      break;
    default:
      break;
    }
  }
  return;
}

void CodeGeneratorRisc::translate_addi(IRInst *inst) {
  Value *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  if (src1->type.type == BasicType::TYPE_FLOAT ||
      src2->type.type == BasicType::TYPE_FLOAT)
    translate_fadd(inst);
  else
    translate_add(inst);
  return;
}

void CodeGeneratorRisc::translate_add(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_fadd(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (src1->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[29],
                                    RiscInst::regname[reg1], ""));
    reg1 = 29;
  }
  if (src2->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[30],
                                    RiscInst::regname[reg1], ""));
    reg2 = 30;
  }
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::fadd_d, RiscInst::f_regname[dreg],
                                  RiscInst::f_regname[reg1],
                                  RiscInst::f_regname[reg2]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_subi(IRInst *inst) {
  Value *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  if (src1->type.type == BasicType::TYPE_FLOAT ||
      src2->type.type == BasicType::TYPE_FLOAT)
    translate_fsub(inst);
  else
    translate_sub(inst);
  return;
}

void CodeGeneratorRisc::translate_sub(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::sub, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  store_var(dst, dreg);
  return;
}

void CodeGeneratorRisc::translate_fsub(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (src1->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[29],
                                    RiscInst::regname[reg1], ""));
    reg1 = 29;
  }
  if (src2->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[30],
                                    RiscInst::regname[reg1], ""));
    reg2 = 30;
  }
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::fsub_d, RiscInst::f_regname[dreg],
                                  RiscInst::f_regname[reg1],
                                  RiscInst::f_regname[reg2]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_muli(IRInst *inst) {
  Value *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    Value *dst = inst->getDst();
    int32_t reg1 = getReg(src1, 29), dreg = getReg(dst, 28);
    load_var(src1, reg1);
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[30], "",
                                    std::to_string(b_inst->src)));
    code_seq.push_back(new RiscInst(InstType::mul, RiscInst::regname[dreg],
                                    RiscInst::regname[reg1],
                                    RiscInst::regname[30]));
    store_var(dst, dreg);
    return;
  }
  if (src1->type.type == BasicType::TYPE_FLOAT ||
      src2->type.type == BasicType::TYPE_FLOAT)
    translate_fmul(inst);
  else
    translate_mul(inst);
}

void CodeGeneratorRisc::translate_mul(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::mul, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  store_var(dst, dreg);
  return;
}

void CodeGeneratorRisc::translate_fmul(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (src1->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[29],
                                    RiscInst::regname[reg1], ""));
    reg1 = 29;
  }
  if (src2->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[30],
                                    RiscInst::regname[reg1], ""));
    reg2 = 30;
  }
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::fmul_d, RiscInst::f_regname[dreg],
                                  RiscInst::f_regname[reg1],
                                  RiscInst::f_regname[reg2]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_divi(IRInst *inst) {
  Value *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  if (src1->type.type == BasicType::TYPE_FLOAT ||
      src2->type.type == BasicType::TYPE_FLOAT)
    translate_fdiv(inst);
  else
    translate_div(inst);
  return;
}

void CodeGeneratorRisc::translate_div(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::div, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  store_var(dst, dreg);
  return;
}

void CodeGeneratorRisc::translate_fdiv(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (src1->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[29],
                                    RiscInst::regname[reg1], ""));
    reg1 = 29;
  }
  if (src2->type.type != BasicType::TYPE_FLOAT) {
    code_seq.push_back(new RiscInst(InstType::fcvt_d_w, RiscInst::f_regname[30],
                                    RiscInst::regname[reg1], ""));
    reg2 = 30;
  }
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::fdiv_d, RiscInst::f_regname[dreg],
                                  RiscInst::f_regname[reg1],
                                  RiscInst::f_regname[reg2]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_remi(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc1(), *src2 = inst->getSrc2();
  int32_t reg1 = getReg(src1, 29), reg2 = getReg(src2, 30),
          dreg = getReg(dst, 28);
  load_var(src1, reg1);
  load_var(src2, reg2);
  code_seq.push_back(new RiscInst(InstType::rem, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  store_var(dst, dreg);
  return;
}

void CodeGeneratorRisc::translate_br(IRInst *inst) {
  code_seq.push_back(
      new RiscInst(InstType::jal, inst->getTrueInst()->getLabelName(), "", ""));
}

void CodeGeneratorRisc::translate_bc(IRInst *inst) {

  BcIRInst *Bc_inst = static_cast<BcIRInst *>(inst);
  Value *src = Bc_inst->temp;
  int32_t reg1 = getReg(src, 28);
  load_var(src, reg1);
  std::string true_label, false_label;
  if (Bc_inst->mode == 0 || Bc_inst->mode == 1 || Bc_inst->mode == 3 ||
      Bc_inst->mode == 4)
    true_label = inst->getTrueInst()->getLabelName();
  else
    true_label = Bc_inst->modeInst_2->getLabelName();
  if (Bc_inst->mode == 0 || Bc_inst->mode == 2 || Bc_inst->mode == 3 ||
      Bc_inst->mode == 5)
    false_label = inst->getFalseInst()->getLabelName();
  else
    false_label = Bc_inst->modeInst_1->getLabelName();
  code_seq.push_back(new RiscInst(InstType::bne, true_label,
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[0]));
  code_seq.push_back(new RiscInst(InstType::beq, false_label,
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[0]));
}

void CodeGeneratorRisc::translate_cmp_eq(IRInst *inst) {
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  Value *dst = inst->getDst(), *src1 = inst->getSrc1();
  int32_t reg1 = getReg(src1, 29), reg2, dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    reg2 = 30;
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[reg2], "",
                                    std::to_string(b_inst->src)));
  } else {
    Value *src2 = inst->getSrc2();
    reg2 = getReg(src2, 30);
    load_var(src2, reg2);
  }
  code_seq.push_back(new RiscInst(InstType::XOR, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  code_seq.push_back(new RiscInst(InstType::seqz, RiscInst::regname[dreg],
                                  RiscInst::regname[dreg], ""));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_cmp_neq(IRInst *inst) {
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  Value *dst = inst->getDst(), *src1 = inst->getSrc1();
  int32_t reg1 = getReg(src1, 29), reg2, dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    reg2 = 30;
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[reg2], "",
                                    std::to_string(b_inst->src)));
  } else {
    Value *src2 = inst->getSrc2();
    reg2 = getReg(src2, 30);
    load_var(src2, reg2);
  }
  code_seq.push_back(new RiscInst(InstType::XOR, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  code_seq.push_back(new RiscInst(InstType::snez, RiscInst::regname[dreg],
                                  RiscInst::regname[dreg], ""));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_cmp_lt(IRInst *inst) {
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  Value *dst = inst->getDst(), *src1 = inst->getSrc1();
  int32_t reg1 = getReg(src1, 29), reg2, dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    reg2 = 30;
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[reg2], "",
                                    std::to_string(b_inst->src)));
  } else {
    Value *src2 = inst->getSrc2();
    reg2 = getReg(src2, 30);
    load_var(src2, reg2);
  }
  code_seq.push_back(new RiscInst(InstType::slt, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_cmp_gt(IRInst *inst) {
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  Value *dst = inst->getDst(), *src1 = inst->getSrc1();
  int32_t reg1 = getReg(src1, 29), reg2, dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    reg2 = 30;
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[reg2], "",
                                    std::to_string(b_inst->src)));
  } else {
    Value *src2 = inst->getSrc2();
    reg2 = getReg(src2, 30);
    load_var(src2, reg2);
  }
  code_seq.push_back(new RiscInst(InstType::slt, RiscInst::regname[dreg],
                                  RiscInst::regname[reg2],
                                  RiscInst::regname[reg1]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_cmp_le(IRInst *inst) {
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  Value *dst = inst->getDst(), *src1 = inst->getSrc1();
  int32_t reg1 = getReg(src1, 29), reg2, dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    reg2 = 30;
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[reg2], "",
                                    std::to_string(b_inst->src)));
  } else {
    Value *src2 = inst->getSrc2();
    reg2 = getReg(src2, 30);
    load_var(src2, reg2);
  }
  code_seq.push_back(new RiscInst(InstType::slt, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1],
                                  RiscInst::regname[reg2]));
  code_seq.push_back(new RiscInst(InstType::XOR, RiscInst::regname[31],
                                  RiscInst::regname[reg2],
                                  RiscInst::regname[reg1]));
  code_seq.push_back(new RiscInst(InstType::seqz, RiscInst::regname[31],
                                  RiscInst::regname[31], ""));
  code_seq.push_back(new RiscInst(InstType::OR, RiscInst::regname[dreg],
                                  RiscInst::regname[31],
                                  RiscInst::regname[dreg]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_cmp_ge(IRInst *inst) {
  BinaryIRInst *b_inst = static_cast<BinaryIRInst *>(inst);
  Value *dst = inst->getDst(), *src1 = inst->getSrc1();
  int32_t reg1 = getReg(src1, 29), reg2, dreg = getReg(dst, 28);
  load_var(src1, reg1);
  if (b_inst->mode == 2 || b_inst->mode == 3) {
    reg2 = 30;
    code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[reg2], "",
                                    std::to_string(b_inst->src)));
  } else {
    Value *src2 = inst->getSrc2();
    reg2 = getReg(src2, 30);
    load_var(src2, reg2);
  }
  code_seq.push_back(new RiscInst(InstType::slt, RiscInst::regname[dreg],
                                  RiscInst::regname[reg2],
                                  RiscInst::regname[reg1]));
  code_seq.push_back(new RiscInst(InstType::XOR, RiscInst::regname[31],
                                  RiscInst::regname[reg2],
                                  RiscInst::regname[reg1]));
  code_seq.push_back(new RiscInst(InstType::seqz, RiscInst::regname[31],
                                  RiscInst::regname[31], ""));
  code_seq.push_back(new RiscInst(InstType::OR, RiscInst::regname[dreg],
                                  RiscInst::regname[31],
                                  RiscInst::regname[dreg]));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_load(IRInst *inst) {
  Value *dst = inst->getDst(), *src1 = inst->getSrc().front();
  int32_t reg1 = getReg(src1, 29), dreg = getReg(dst, 28);
  load_var(src1, reg1);
  InstType load = src1->getSize() == 8 ? InstType::ld : InstType::lw;
  code_seq.push_back(new RiscInst(load, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1], "0"));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_assign(IRInst *inst) {

  Value *dst = inst->getDst(), *src = inst->getSrc().front();
  int32_t flag = static_cast<AssignIRInst *>(inst)->_flag;
  bool neg = flag == 5;
  bool right_ptr = flag == 2 || flag == 4;
  bool left_ptr = flag == 3 || flag == 4 || inst->Assign_flag == 6;
  int32_t dreg = getReg(dst, 28), reg = getReg(src, 29);
  if (src->type.type == dst->type.type) {
    load_var(src, dreg);
    if (neg) {
      code_seq.push_back(new RiscInst(InstType::neg, RiscInst::regname[dreg],
                                      RiscInst::regname[dreg], ""));
    }
    if (right_ptr) {
      code_seq.push_back(new RiscInst(InstType::ld, RiscInst::regname[dreg],
                                      RiscInst::regname[reg], "0"));
    }

    if (left_ptr) {
      if (dst->getOffset() < 2048) {
        code_seq.push_back(new RiscInst(InstType::ld, RiscInst::regname[31],
                                        RiscInst::regname[REG_FP],
                                        std::to_string(-dst->getOffset())));
      } else {
        code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5], "",
                                        std::to_string(-dst->getOffset())));
        code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                        RiscInst::regname[5],
                                        RiscInst::regname[REG_FP]));
        code_seq.push_back(new RiscInst(InstType::ld, RiscInst::regname[31],
                                        RiscInst::regname[5],
                                        std::to_string(0)));
      }
      code_seq.push_back(new RiscInst(InstType::sw, RiscInst::regname[dreg],
                                      RiscInst::regname[31], "0"));
    } else
      store_var(dst, dreg);
  } else {
    load_var(src, dreg);
    if (src->type.type == BasicType::TYPE_FLOAT &&
        dst->type.type == BasicType::TYPE_INT)
      code_seq.push_back(new RiscInst(InstType::fcvt_w_d,
                                      RiscInst::regname[dreg],
                                      RiscInst::f_regname[reg], ""));
    else if (src->type.type == BasicType::TYPE_INT &&
             dst->type.type == BasicType::TYPE_FLOAT)
      code_seq.push_back(new RiscInst(InstType::fcvt_d_w,
                                      RiscInst::f_regname[dreg],
                                      RiscInst::regname[reg], ""));
    if (neg && dst->type.type == BasicType::TYPE_FLOAT) {
      code_seq.push_back(
          new RiscInst(InstType::fsub_d, RiscInst::f_regname[dreg],
                       RiscInst::f_regname[0], RiscInst::f_regname[dreg]));
    }
    if (neg && dst->type.type == BasicType::TYPE_INT) {
      code_seq.push_back(new RiscInst(InstType::neg, RiscInst::regname[dreg],
                                      RiscInst::regname[dreg], ""));
    }
    store_var(dst, dreg);
  }
}

void CodeGeneratorRisc::translate_alloca(IRInst *inst) {}

void CodeGeneratorRisc::translate_label(IRInst *inst) {
  code_seq.push_back(
      new RiscInst(InstType::label, inst->getLabelName(), "", ""));
}

void CodeGeneratorRisc::translate_funcall(IRInst *inst) {

  FuncCallIRInst *func_ptr = static_cast<FuncCallIRInst *>(inst);
  std::vector<Value *> params = func_ptr->getSrc();
  int32_t sp_size = 0;
  for (int i = 0; i < (int32_t)params.size() && i < 4; i++) {
    getReg(params[i], 28);
    load_var(params[i], 10 + i);
  }
  for (int i = 4; i < (int32_t)params.size(); i++) {
    int32_t temp_size = (params[i]->getSize() + 3) / 4 * 4;
    // int32_t temp_size = (params[i]->getSize() + 7) / 8 * 8;
    sp_size += temp_size;
    if (!params[i]->isliteral()) {
      if (params[i]->getOffset() < 2048) {
        code_seq.push_back(
            new RiscInst(temp_size == 8 ? InstType::ld : InstType::lw,
                         RiscInst::regname[28], RiscInst::regname[REG_FP],
                         std::to_string(-params[i]->getOffset())));
      } else {
        code_seq.push_back(
            new RiscInst(InstType::li, RiscInst::regname[5], "",
                         std::to_string(-params[i]->getOffset())));
        code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                        RiscInst::regname[REG_FP],
                                        RiscInst::regname[5]));
        code_seq.push_back(new RiscInst(
            temp_size == 8 ? InstType::ld : InstType::lw, RiscInst::regname[28],
            RiscInst::regname[5], std::to_string(0)));
      }
    } else
      code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[28], "",
                                      std::to_string(params[i]->intVal)));

    if (sp_size < 2048) {
      code_seq.push_back(new RiscInst(
          temp_size == 8 ? InstType::sd : InstType::sw, RiscInst::regname[28],
          RiscInst::regname[REG_SP], std::to_string(-sp_size)));
    } else {
      code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5], "",
                                      std::to_string(-sp_size)));
      code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                      RiscInst::regname[REG_SP],
                                      RiscInst::regname[5]));
      code_seq.push_back(new RiscInst(
          temp_size == 8 ? InstType::sd : InstType::sw, RiscInst::regname[28],
          RiscInst::regname[5], std::to_string(0)));
    }
  }
  code_seq.push_back(new RiscInst(InstType::call, func_ptr->name, "", ""));

  if (func_ptr->getDst()->type.type != BasicType::TYPE_VOID) {
    getReg(func_ptr->getDst(), 28);
    store_var(func_ptr->getDst(), 10);
  }
}

void CodeGeneratorRisc::translate_exit(IRInst *inst, Function *fun) {
  ExitIRInst *exit_ptr = static_cast<ExitIRInst *>(inst);

  //恢复被保存的寄存器
  int32_t cnt = 0, fcnt = 0;
  std::vector<FuncFormalParam> params = fun->getParams();
  // special for a0
  for (int i = 1; i < (int32_t)params.size() && i < 4; i++) {
    Value *var = fun->findValue(
        "%t" + std::to_string(std::stoi(params[i].val->getName().substr(2)) -
                              params.size()));
    var->baseRegNo = REG_SP;
    if (var->type.type == BasicType::TYPE_FLOAT) {
      var->regId = 11 + fcnt;
      fcnt++;
    } else {
      var->regId = 11 + cnt;
      cnt++;
    }
  }
  code_seq.push_back(new RiscInst(InstType::ld, RiscInst::regname[REG_RA],
                                  RiscInst::regname[REG_SP],
                                  std::to_string(8 * (cnt + fcnt) + 8)));
  for (int i = 0; i < fcnt; i++)
    code_seq.push_back(new RiscInst(InstType::fld, RiscInst::f_regname[11 + i],
                                    RiscInst::regname[REG_SP],
                                    std::to_string(8 * i)));
  for (int i = 0; i < cnt; i++)
    code_seq.push_back(new RiscInst(InstType::ld, RiscInst::regname[11 + i],
                                    RiscInst::regname[REG_SP],
                                    std::to_string(8 * fcnt + 8 * i)));
  //存储返回值
  if (fun->getReturnType().type != BasicType::TYPE_VOID) {
    Value *src = exit_ptr->getSrc().front();
    int32_t reg1 = getReg(src, 28);
    load_var(src, reg1);
    code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[10],
                                    RiscInst::regname[reg1],
                                    RiscInst::regname[0]));
  } else if (params.size()) {
    // TODO:参数size优化
    Value *var = fun->findValue(
        "%t" + std::to_string(std::stoi(params[0].val->getName().substr(2)) -
                              params.size()));
    int32_t size = params[0].val->getSize();
    if (size <= 0)
      size = 1;
    size = (size + 3) / 4 * 4;
    // int32_t size = 0;
    // size = byteAlign(var);
    if (var->getSize() < 2048) {
      code_seq.push_back(new RiscInst(
          size == 8 ? InstType::ld : InstType::lw, RiscInst::regname[10],
          RiscInst::regname[REG_SP], std::to_string(-var->getSize())));
    } else {
      code_seq.push_back(new RiscInst(InstType::li, RiscInst::regname[5], "",
                                      std::to_string(-var->getSize())));
      code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                      RiscInst::regname[REG_SP],
                                      RiscInst::regname[5]));
      code_seq.push_back(new RiscInst(size == 8 ? InstType::ld : InstType::lw,
                                      RiscInst::regname[10],
                                      RiscInst::regname[5], std::to_string(0)));
    }
  }
  code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[REG_SP],
                                  RiscInst::regname[REG_FP],
                                  RiscInst::regname[0]));
  if (fun->getMaxDep() - 8 * (cnt + fcnt) < 2048) {
    code_seq.push_back(new RiscInst(
        InstType::ld, RiscInst::regname[REG_FP], RiscInst::regname[REG_SP],
        std::to_string(8 * (cnt + fcnt) - fun->getMaxDep())));
  } else {
    code_seq.push_back(
        new RiscInst(InstType::li, RiscInst::regname[5], "",
                     std::to_string(8 * (cnt + fcnt) - fun->getMaxDep())));
    code_seq.push_back(new RiscInst(InstType::add, RiscInst::regname[5],
                                    RiscInst::regname[REG_SP],
                                    RiscInst::regname[5]));
    code_seq.push_back(new RiscInst(InstType::ld, RiscInst::regname[REG_FP],
                                    RiscInst::regname[5], std::to_string(0)));
  }

  code_seq.push_back(
      new RiscInst(InstType::jalr, RiscInst::regname[REG_RA], "x0", ""));
}

void CodeGeneratorRisc::translate_sext(IRInst *inst) {
  Value *src = inst->getSrc().front(), *dst = inst->getDst();
  int32_t reg1 = getReg(src, 29), dreg = getReg(dst, 28);
  load_var(src, reg1);
  code_seq.push_back(new RiscInst(InstType::sext_w, RiscInst::regname[dreg],
                                  RiscInst::regname[reg1], ""));
  store_var(dst, dreg);
}

void CodeGeneratorRisc::translate_zext(IRInst *inst) {}

void CodeGeneratorRisc::translate_max(IRInst *inst) {}

void CodeGeneratorRisc::translate_entry(IRInst *inst) {}

void CodeGeneratorRisc::translate_store(IRInst *inst) {}
