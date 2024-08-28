/**
 * @file IRGenerator.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief AST遍历产生线性IR
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "AST.h"
#include "IRCode.h"
#include "IRGenerator.h"
#include "IRInst.h"
#include "SymbolTable.h"
#include "Value.h"
#include "ValueType.h"

extern IRInst *TrueEntry;
extern IRInst *FalseEntry;
extern IRInst *EntryEntry;
extern IRInst *ExitEntry;

extern IRInst *Mode_1_Entry;
extern IRInst *Mode_2_Entry;
extern IRInst *Break_Exit;
extern IRInst *Continue_Entry;

// TODO:全局变量用于记录数组地址
_numpy_ *temp_np;
/// @brief 构造函数
/// @param _root AST的根
/// @param _symtab 符号表
IRGenerator::IRGenerator(ast_node *_root, SymbolTable *_symtab)
    : root(_root), symtab(_symtab) {

  // astnode->OriginalType NodeKind
  ast2ir_handlers[LValK] = &IRGenerator::ir_LValK;

  /*LValK是作为叶子节点的，是运算或条件语句中的变量,若是数组的话，存在子节点表示维度即A
  = a[1][1] + b;
  也有可能是传进函数的实参，不过实参如果是数组类型，不存在子节点*/

  ast2ir_handlers[CompUnitK] = &IRGenerator::ir_CompUnitK;

  // CompUnitK是根节点

  ast2ir_handlers[ConstDeclK] = &IRGenerator::ir_ConstDeclK;

  ast2ir_handlers[ConstDefK] = &IRGenerator::ir_ConstDefK;

  ast2ir_handlers[ConstInitValK] = &IRGenerator::ir_ConstInitValK;

  /*ConstDefK是const定义语句其存储了所定义的名字，其子节点是一个赋值语句节点，其子节点的子节点为所定义的数值和数组维度，包含int和float类型
  ConstInitValK属于是赋值语句，其子节点的数值为父节点--ConstDefK的赋值数值,若是数组的话，ConstInitValK的子节点是ConstInitValK，子节点的子节点才是数值
  ConstExpGroupK若其有子节点且子节点有数值，则其父亲节点是数组变量，仅限于定义const和非const，不包含函数的形参数组
*/
  ast2ir_handlers[VarDeclK] = &IRGenerator::ir_VarDeclK;

  ast2ir_handlers[VarDefK] = &IRGenerator::ir_VarDefK;

  /*VarDefK是非const定义语句其存储了所定义的名字，其子节点是一个赋值语句节点，其子节点的子节点为所定义的数值和数组维度，包含int和float类型
  InitValK属于是赋值语句，其子节点的数值为父节点--VarDefK的赋值数值,若是数组的话，InitValK的子节点是InitValK，子节点的子节点才是数值
  若VarDefK的子节点没有InitValK，则说明该定义只声明了变量并没有给其赋值
  ConstExpGroupK若其有子节点且子节点有数值，则其父亲节点是数组变量，仅限于定义const和非const，不包含函数的形参数组
*/
  ast2ir_handlers[FuncDeclK] = &IRGenerator::ir_FuncDeclK;

  // FuncDeclK只是声明了函数，并没有给出函数的具体操作即Block，该节点包含了函数名和返回类型，其子节点为fomalpara，子节点的子节点是形参

  ast2ir_handlers[FuncDefK] = &IRGenerator::ir_FuncDefK;

  // FuncDefK是函数定义，子节点为Block和foamlpara，这两个的子节点分别是函数操作和形参

  ast2ir_handlers[FuncFParamsK] = &IRGenerator::ir_FuncFParamsK;

  /*FuncFParamsK是函数形参表，其子节点为FuncFParamK包含了形参的类型(int,float)和名字，若无形参其后面没有子节点
  若形参为数组，FuncFParamK子节点存在一个ExpK类型的节点，其他n个子节点为数值，该形参维度为n+1
*/
  ast2ir_handlers[BlockK] = &IRGenerator::ir_BlockK;

  /*BlockK是语句块，只要有{}的都算语句块，其中语句块大部分用于函数操作，if，while语句
  ps：注意作用域；if，while可以没有BlockK，即没有{}，eg：if（a） a = 1；


  ！！！！！！！！！！！！！！！！！！！！
  NumberK是数字，包含形参数组的维度，运算式子的数字、表示数组维度的数字，return的数字，条件式子的数字，定义变量，变量数组的数字,形参实参的数字
  ！！！！！！！！！！！！！！！！！！！！

*/
  ast2ir_handlers[StmtK] = &IRGenerator::ir_StmtK;

  ast2ir_handlers[NumberK] = &IRGenerator::ir_NumberK;

  ast2ir_handlers[InitValK] = &IRGenerator::ir_InitValK;

  ast2ir_handlers[UnaryExpK] = &IRGenerator::ir_UnaryExpK;

  // UnaryExpK为单目运算，包含函数调用，+-！的单目运算，存储着函数名或+-!;eg：putin(A);!A;

  ast2ir_handlers[MulExpK] = &IRGenerator::ir_MulExpK;

  // MulExpK包含了/%*这三个运算，子节点为MulExpK、AddExpK或者为运算对象,是双目运算符

  ast2ir_handlers[AddExpK] = &IRGenerator::ir_AddExpK;

  // AddExpK是双目运算，包含了+-这两个运算，子节点为MulExpK、AddExpK或者为运算对象

  ast2ir_handlers[RelExpK] = &IRGenerator::ir_RelExpK;

  // RelExpK是双目运算，包含了< <= > >=这四个运算,子节点为比较对象

  ast2ir_handlers[EqExpK] = &IRGenerator::ir_EqExpK;

  // EqExpK是双目运算，包含了== !=这两个运算,子节点为比较对象

  ast2ir_handlers[LAndExpK] = &IRGenerator::ir_LAndExpK;

  // LAndExpK是双目运算&&是and操作，子节点的and的对象

  ast2ir_handlers[LOrExpK] = &IRGenerator::ir_LOrExpK;

  // LOrExpK是双目运算||是or操作，子节点的or的对象
}

///@brief 存储数组
///@param node 当前节点
///@param is_int 是否为整形数组
///@param deep 数组维度，当前深度
///@param save_place 当前应存入的位置
void IRGenerator::dfs_save(Value *val, ast_node *node, _numpy_ *np, int is_int,
                           int deep, int save_place, int is_Loc) {
  //如果是a[1][2][3]那么sizes顺序为3-2-1
  int inner = 0; // 方便计算save_place
  // a[3][4][5] = { { 1,2,3,{5}},{},{}};
  int outer = 0;
  int save_ = 1;
  int is_done = 0;
  int _save_place = save_place;
  Value *temp = nullptr;

  // int k =  np->np_sizes.size();
  // if(deep > 1)
  // {
  for (int k = node->sons.size() - 1; k >= 0; k--) {
    auto son = node->sons[k];
    if (son->sons.size() == 0) {
      outer = outer + 1;
      inner = 0;
      continue;
    }
    if (son->sons[0]->OriginalType == NumberK ||
        son->sons[0]->OriginalType == LValK) {
      //赋值
      if (outer != 0) {
        for (int d = np->np_sizes.size(); d >= deep; d--) {
          inner = inner + (outer)*np->np_sizes[d - deep];
        }
        outer = 0;
      }
      ast_node *son_node = ir_visit_ast_node(son->sons[0]);

      // son->sons[0]->val->is_numpy = true;
      // if (son->sons[0]->val->is_numpy) {
      //     son->sons[0]->val->setNumpyIndex(Index++);
      // }

      if (!son_node) {
        break;
      }
      if (is_int == 1) {
        std::string src = std::to_string(son_node->val->intVal);
        np->numpy_int[save_place + inner] = son_node->val->intVal;
        if (is_Loc == 1) {
          temp = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
          symtab->currentFunc->currentBlock->insertValue(temp);
        } else if (is_Loc != 1) {
          temp = symtab->newTempValue(BasicType::TYPE_INT);
        }
        // temp->setNumpyIndex(Index - 1);
        temp->set_is_savenp();
        temp->intVal = son->sons[0]->val->intVal;
        temp->realVal = son->sons[0]->val->realVal;
        temp->np = temp_np;
        temp->np->cnt++;
        temp->indexLinear = save_place + inner;
        temp->insertNumpyIndex(save_place + inner);
        temp->setNumpyIndex();
        node->blockInsts.addInst(
            new BinaryIRInst(IRInstOperator::IRINST_OP_ADD_I, temp, val,
                             4 * (save_place + inner), 3));
        node->blockInsts.addInst(new AssignIRInst(temp, src, 6));
        // std::cout << save_place + inner << " :";
        // std::cout << np->numpy_int[save_place + inner] << std::endl;
      } else if (is_int == 0) {
        np->numpy_float[save_place + inner] = son_node->val->realVal;
        std::string src = std::to_string(son_node->val->realVal);
        np->numpy_int[save_place + inner] = son_node->val->intVal;
        if (is_Loc == 1) {
          temp = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
          symtab->currentFunc->currentBlock->insertValue(temp);
        } else if (is_Loc != 1) {
          temp = symtab->newTempValue(BasicType::TYPE_INT);
        }
        // temp->setNumpyIndex(Index - 1);
        temp->set_is_savenp();
        temp->intVal = son->sons[0]->val->intVal;
        temp->realVal = son->sons[0]->val->realVal;
        temp->np = temp_np;
        temp->np->cnt++;
        temp->indexLinear = save_place + inner;
        temp->insertNumpyIndex(save_place + inner);
        temp->setNumpyIndex();
        node->blockInsts.addInst(
            new BinaryIRInst(IRInstOperator::IRINST_OP_ADD_I, temp, val,
                             4 * (save_place + inner), 3));
        node->blockInsts.addInst(new AssignIRInst(temp, src, 6));
      }
      inner = inner + 1;
      // continue;
    } else if (son->sons[0]->OriginalType == InitValK ||
               son->sons[0]->OriginalType == ConstInitValK) {
      if (inner != 0) {
        outer = outer + 1;
        inner = 0;
      }
      //修改save_place
      // TODO有点问题
      int _deep = deep;
      for (int j = 0; j != (int)np->np_sizes.size(); j++) {
        if (is_done != 0)
          break;
        if (_deep == 1) {
          is_done = 1;
          break;
        }
        save_ = save_ * (int)np->np_sizes[j];
        _deep--;
      }
      _save_place = save_place + save_ * outer;
      dfs_save(val, son, np, is_int, deep - 1, _save_place, is_Loc);
    }
    if (inner == 0) {
      outer = outer + 1;
    }
    node->blockInsts.addInst(son->blockInsts);
  }
  // }
}

/// @brief 未知节点类型的节点处理
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_default(ast_node *node) {
  // 未知的节点
  printf("Unkown node(%d)\n", (int)node->node_type);
  return true;
}

/// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
/// @param node AST节点
/// @return 成功返回node节点，否则返回nullptr
ast_node *IRGenerator::ir_visit_ast_node(ast_node *node) {
  // 空节点
  if (nullptr == node) {
    return nullptr;
  }

  bool result;
  // astnode->OriginalType NodeKind
  std::unordered_map<NodeKind, ast2ir_handler_t>::const_iterator pIter;
  pIter = ast2ir_handlers.find(node->OriginalType);
  // std::unordered_map<ast_operator_type, ast2ir_handler_t>::const_iterator
  // pIter; pIter = ast2ir_handlers.find(node->node_type);
  if (pIter == ast2ir_handlers.end()) {
    // 没有找到，则说明当前不支持
    result = (this->ir_default)(node);
  } else {
    result = (this->*(pIter->second))(node);
  }

  if (!result) {
    // 语义解析错误，则出错返回
    node = nullptr;
  }

  return node;
}

/// @brief 遍历抽象语法树产生线性IR，保存到IRCode中
/// @param root 抽象语法树
/// @param IRCode 线性IR
/// @return true: 成功 false: 失败
bool IRGenerator::run() {
  ast_node *node;

  // 从根节点进行遍历
  node = ir_visit_ast_node(root);

  return node != nullptr;
}

/// @brief
/// LValK是作为叶子节点的，是运算或条件语句中的变量,若是数组的话，存在子节点表示维度即A
/// = a[1][1] + b;
//也有可能是传进函数的实参，不过实参如果是数组类型，不存在子节点
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_LValK(ast_node *node) {

  //寻找字母，如果没有就会报错，即使用了但没声明
  Value *val = nullptr;
  int flag_Loc = 0;
  // int flag_np = 1;
  /*switch(node->BasicType)
  {
      case Void: node->type.type = BasicType::TYPE_VOID; break;
      case Integer: node->type.type = BasicType::TYPE_INT; break;
      case Float: node->type.type = BasicType::TYPE_FLOAT; break;
      case Error: node->type.type = BasicType::TYPE_NONE; break;
  }*/
  //寻找局部变量
  if (symtab->currentFunc != nullptr &&
      symtab->currentFunc->currentBlock != nullptr) {
    flag_Loc = 1;
    val = symtab->currentFunc->currentBlock->findValue(node->name);
    if (!val) {
      // if(val == nullptr)
      // {
      // 	//当前Block没找到
      Var_Block *tempBlock = nullptr;
      ast_node *son = node;
      ast_node *parent = node->parent;
      // std::vector<Var_Block *> Block_vector =
      // symtab->currentFunc->Blocks_vector;
      while (parent->OriginalType != FuncDefK) {
        if (parent->OriginalType == BlockK && parent->is_saveBlock == 1) {
          int i = parent->Block_number;
          tempBlock = symtab->currentFunc->Blocks_vector[i];
          val = tempBlock->findValue(node->name);
          if (val != nullptr) {
            break;
          }
        }
        son = parent;
        parent = son->parent;
      }
      // int i = symtab->currentFunc->currentBlock->number;
      // for(;i>=0;i--)
      // {
      // 	tempBlock = symtab->currentFunc->Blocks_vector[i];
      // 	val = tempBlock->findValue(node->name);
      // 	if(val != nullptr)
      // 	{
      // 		break;
      // 	}
      // }

      if (val == nullptr) {
        // block里面没有声明，有可能是形参
        val = symtab->currentFunc->findValue(node->name, false);
        if (val != nullptr && val->is_saveFParam == 0) {
          val = nullptr;
        }
      }

      // }
      // val = symtab->currentFunc->newVarValue(node->name,node->type.type);
      //寻找全局全局变量
      if (val == nullptr)
        val = symtab->findValue(node->name, false);
      // flag_Loc = 0;

      // 使用了但没声明
      if (!val) {
        printf("line:%d %s undeclare ", node->line_no, node->name.c_str());
        return false;
      }
    }
  } else if (symtab->currentFunc == nullptr) {
    flag_Loc = 0;
    val = symtab->findValue(node->name, false);
    if (!val) {
      printf("line:%d %s undeclare ", node->line_no, node->name.c_str());
      return false;
    }
  }

  // node->val = val;
  // node->type.type = val->type.type;
  // if (node->sons.size() == 0) {
  //     //不是数组
  //     return true;
  // }

  std::vector<Value *> dim;
  // if(flag == 1)
  // {
  // 	InterCode & irCode = symtab->currentFunc->getInterCode();
  // 	irCode.addInst(Entry);//条件
  // 	irCode.addInst(node->blockInsts);
  // 	irCode.addInst(Exit);//出口
  // }
  // if(flag == 0)
  // {
  // node->blockInsts.addInst(Exit);
  // }

  // for (auto son: node->sons) {
  //     //文法显示数组维度使用的时候只能是数组
  //     ast_node * son_node = ir_visit_ast_node(son);
  //     if (!son_node) {
  //         return false;
  //     }
  // }

  node->val = val;
  node->type.type = val->type.type;
  if (node->sons.size() == 0) {
    //不是数组
    return true;
  }

  // if(flag == 1)
  // {
  // 	InterCode & irCode = symtab->currentFunc->getInterCode();
  // 	irCode.addInst(Entry);//条件
  // 	irCode.addInst(node->blockInsts);
  // 	irCode.addInst(Exit);//出口
  // }
  // if(flag == 0)
  // {
  // node->blockInsts.addInst(Exit);
  // }
  // int son_is_np = 0;
  Value *son_val = nullptr;
  int sizes = 0;
  int lenght = 1;
  if (node->val->np->np_sizes.size() != node->sons.size()) {
    sizes = node->val->np->np_sizes.size() - node->sons.size();
    for (int i = 0; i <= sizes - 1; i++) {
      lenght = lenght * node->val->np->np_sizes[i];
    }
  }
  for (auto son : node->sons) {
    //文法显示数组维度使用的时候只能是数组
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {
      return false;
    }
    node->blockInsts.addInst(son->blockInsts);
    if (nullptr != son->val->np) {
      // son_is_np = 1;
      if (flag_Loc == 1) {
        son_val = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
      } else if (flag_Loc == 0) {
        son_val = symtab->newTempValue(BasicType::TYPE_INT);
      }
      node->blockInsts.addInst(new AssignIRInst(son_val, son_node->np_val, 2));
      dim.push_back(son_val);
      continue;
    }
    dim.push_back(son_node->val);
  }
  // dim从低维到高维
  int get_place = dim[0]->intVal;
  int mul = 1;
  int cnt = 0;
  // if(flag_Loc) cnt = 0;
  for (int i = 1; i < (int)dim.size(); i++) {
    cnt = i - 1;
    mul = 1;
    while (cnt >= 0) {
      mul = mul * node->val->np->np_sizes[cnt];
      cnt--;
    }
    get_place = get_place + dim[i]->intVal * mul;
  }
  if (node->type.type == BasicType::TYPE_INT && node->val->is_saveFParam == 0 &&
      get_place >= 0 && get_place <= node->val->np->len) {
    node->val->intVal = node->val->np->numpy_int[get_place];
  } else if (node->type.type == BasicType::TYPE_FLOAT &&
             node->val->is_saveFParam == 0 && get_place >= 0 &&
             get_place <= node->val->np->len) {
    node->val->realVal = node->val->np->numpy_float[get_place];
  }
  Value *pre = nullptr;
  Value *result1 = nullptr;
  Value *result2 = nullptr;
  int is_i = 0;
  // TODO:线性距离
  // int dimLinear = 0;
  for (int i = dim.size() - 1; i >= 0; i--) {
    // TODO:全局数组则直接寻址不需要间接寻址
    // // 找到全局数组
    // if (node->val != nullptr && node->val->isConst() &&
    // !node->val->isliteral() && node->val->is_numpy &&
    //     symtab->findValue(node->val->getName()) != nullptr) {
    //     if (i != 0) {
    //         dimLinear += dim[i]->intVal * node->val->np->np_sizes[i];
    //     } else {
    //         dimLinear += dim[0]->intVal;
    //     }
    //     continue;
    // }
    if (i != 0) {
      is_i = 1;
      if (flag_Loc == 1) {
        result1 = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
        result2 = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
      } else if (flag_Loc == 0) {
        result1 = symtab->newTempValue(BasicType::TYPE_INT);
        result2 = symtab->newTempValue(BasicType::TYPE_INT);
      }
      // mul = mul * node->val->np->np_sizes[i-1];
      // cnt = i-1;
      // while(cnt!=0)
      // {
      // 	mul = mul * node->val->np->np_sizes[cnt];
      // 	cnt--;
      // }
      if (pre == nullptr) {
        node->blockInsts.addInst(
            new BinaryIRInst(IRInstOperator::IRINST_OP_MULT_I, result1, dim[i],
                             node->val->np->np_sizes[i - 1], 2));
        result1->intVal = dim[i]->intVal * node->val->np->np_sizes[i - 1];
        node->blockInsts.addInst(new BinaryIRInst(
            IRInstOperator::IRINST_OP_ADD_I, result2, result1, dim[i - 1], 1));
        result2->intVal = result1->intVal + dim[i - 1]->intVal;
      } else if (pre != nullptr) {
        node->blockInsts.addInst(
            new BinaryIRInst(IRInstOperator::IRINST_OP_MULT_I, result1, pre,
                             node->val->np->np_sizes[i - 1], 3));
        result1->intVal = dim[i]->intVal * node->val->np->np_sizes[i - 1];
        node->blockInsts.addInst(new BinaryIRInst(
            IRInstOperator::IRINST_OP_ADD_I, result2, result1, dim[i - 1], 1));
        result2->intVal = result1->intVal + dim[i - 1]->intVal;
      }
      pre = result2;

    } else if (i == 0 && is_i == 0) {

      //一维数组情况
      if (flag_Loc == 1) {
        // result1= symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
        result2 = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
      } else if (flag_Loc == 0) {
        // result1= symtab->newTempValue(BasicType::TYPE_INT);
        result2 = symtab->newTempValue(BasicType::TYPE_INT);
      }
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MULT_I, result2, dim[i], 1, 2));
      result2->intVal = dim[i]->intVal;
    }
  }
  // // TODO:全局数组就直接提前返回
  // if (node->val != nullptr && node->val->isConst() && !node->val->isliteral()
  // && node->val->is_numpy &&
  //     symtab->findValue(node->val->getName()) != nullptr) {
  //     //
  //     测试全局数组的IR是不是对的——>node->val是数组首地址，dim[i]表示第i维度要取的数
  //     Value * valueTemp = nullptr;
  //     for (auto value: symtab->getValueVector()) {
  //         if (value != nullptr && value->isTemp() && value->is_issavenp() &&
  //             value->np->np_name == node->val->getName()) {
  //             if (value->indexLinear == dimLinear) {
  //                 valueTemp = value;
  //                 break;
  //             }
  //         }
  //     }
  //     //说明没有初始化，则默认初始化为0,并new一个临时变量插入表里
  //     if (valueTemp == nullptr) {
  //         valueTemp = symtab->newTempValue(BasicType::TYPE_INT);
  //         valueTemp->indexLinear = dimLinear;
  //         valueTemp->intVal = 0;
  //         valueTemp->np = node->val->np;
  //     }
  //     IRInst * irInst = new AssignIRInst(node->val, valueTemp);
  //     irInst->isGlobal = true;
  //     node->blockInsts.addInst(irInst);
  //     return true;
  // }
  Value *result = nullptr;
  Value *another_result = nullptr;
  if (flag_Loc == 1) {
    result = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
    if (node->type.type == BasicType::TYPE_INT ||
        node->type.type == BasicType::TYPE_FNP_I) {
      another_result = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
    }
    if (node->type.type == BasicType::TYPE_FLOAT ||
        node->type.type == BasicType::TYPE_FNP_F) {
      another_result = symtab->currentFunc->newTempValue(BasicType::TYPE_FLOAT);
    }
  } else if (flag_Loc == 0) {
    result = symtab->newTempValue(BasicType::TYPE_INT);
    if (node->type.type == BasicType::TYPE_INT ||
        node->type.type == BasicType::TYPE_FNP_I) {
      another_result = symtab->newTempValue(BasicType::TYPE_INT);
    }

    if (node->type.type == BasicType::TYPE_FLOAT ||
        node->type.type == BasicType::TYPE_FNP_F) {
      another_result = symtab->newTempValue(BasicType::TYPE_FLOAT);
    }
  }
  node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_MULT_I,
                                            result, result2, 4, 3));
  node->blockInsts.addInst(new BinaryIRInst(
      IRInstOperator::IRINST_OP_ADD_I, another_result, node->val, result, 1));
  another_result->intVal = result2->intVal * 4;
  another_result->set_is_savenp();
  if (sizes != 0) {
    another_result->creat_np(lenght, another_result->type.type);
    for (int i = 0; i <= sizes - 1; i++) {
      another_result->np->np_sizes.push_back(node->val->np->np_sizes[i]);
    }
  }
  node->np_val = another_result;
  return true;
}

/// @brief CompUnitK是根节点
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_CompUnitK(ast_node *node) {
  // 新建main函数并默认设置当前函数为main函数

  for (auto son : node->sons) {

    // 遍历编译单元，要么是函数定义，要么是语句
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {
      return false;
    }
  }

  return true;
}

/// @brief ConstDeclK
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_ConstDeclK(ast_node *node) {
  Value *val = nullptr;
  int is_Loc = 0;
  switch (node->BasicType) {
  case Void:
    node->type.type = BasicType::TYPE_VOID;
    break;
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    break;
  case Error:
    node->type.type = BasicType::TYPE_NONE;
    return false;
  }
  for (int i = node->sons.size() - 1; i >= 0; i--) {
    auto son = node->sons[i];
    if (symtab->currentFunc != nullptr &&
        symtab->currentFunc->currentBlock != nullptr) {
      is_Loc = 1;
      //局部变量
      val = symtab->currentFunc->currentBlock->findValue(son->name);
      if (val) {
        printf("redefine");
        return false;
      }

      if (val == nullptr) {
        val = symtab->currentFunc->newVarValue(son->name, node->type.type);
        symtab->currentFunc->currentBlock->insertValue(val);
      }
      if (node->type.type == BasicType::TYPE_NONE) {
        val->SetLiteral();
      } else {
        val->SetConst();
      }
      // 局部变量计数器
      // TODO:全局变量和局部变量的区分都是isLocalVar()方法，但是不同的是全局变量在symtab中的varvecter，而局部变量在symtab->current下的varvecter中
      if (val->isLocalVar() && !val->isliteral()) {
        Value::createLocalVarName();
      }
      if (val->isTemp()) {
        Value::createTempVarName();
      }
    } else if (symtab->currentFunc == nullptr) {
      is_Loc = 0;
      //全局变量
      val = symtab->findValue(son->name, false);
      if (val) {
        printf("redefine");
        // printf(val->name);
        return false;
      }
      if (val == nullptr) {
        val = symtab->newVarValue(son->name, node->type.type);
        // symtab->currentFunc->currentBlock->insertValue(val);
      }
      if (node->type.type == BasicType::TYPE_NONE) {
        val->SetLiteral();
      } else {
        val->SetConst();
      }
    }
    node->val = val;
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {
      return false;
    }
    node->blockInsts.addInst(son_node->blockInsts);
  }
  if (is_Loc == 0) {
    symtab->code.addInst(node->blockInsts);
  }
  return true;
}

/// @brief ConstInitValK
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_ConstInitValK(ast_node *node) {
  int is_R_np = 0;
  // Value * L_np = nullptr;
  //不存在int a[5] = 6这种情况
  Value *R_np = nullptr;

  ast_node *son = node->sons[0];
  ast_node *son_node = ir_visit_ast_node(son);
  if (!son_node) {
    return false;
  }
  if (son_node->np_val != nullptr && son_node->np_val->is_issavenp()) {
    is_R_np = 1;
    R_np = son_node->np_val;
  }
  if (is_R_np == 0) {
    node->val = son_node->val;
    node->type.type = son_node->type.type;
    node->integer_val = son_node->integer_val;
    node->float_val = son_node->float_val;

    //赋值语句
    ast_node *left = node->parent;   // LValK
    ast_node *right = node->sons[0]; //表达式

    left->val->name = left->name;
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(new AssignIRInst(left->val, right->val));
    // if (symtab->currentFunc != nullptr) {
    //     InterCode & irCode = symtab->currentFunc->getInterCode();
    //     irCode.addInst(right->blockInsts);
    //     irCode.addInst(left->blockInsts);
    //     irCode.addInst(new AssignIRInst(left->val, right->val));
    // } else if (symtab->currentFunc == nullptr) {
    //     InterCode & irCode = symtab->getInterCode();
    //     irCode.addInst(right->blockInsts);
    //     irCode.addInst(left->blockInsts);
    //     irCode.addInst(new AssignIRInst(left->val, right->val));
    // }
  } else if (is_R_np == 1) {
    node->val = son_node->val;
    node->integer_val = son_node->integer_val;
    node->float_val = son_node->float_val;
    node->type.type = son_node->type.type;
    ast_node *left = node->parent;   // LValK
    ast_node *right = node->sons[0]; //表达式
    left->val->name = left->name;
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(new AssignIRInst(left->val, R_np, 2));
  }

  // ast_node * son = node->sons[0];
  // ast_node * son_node = ir_visit_ast_node(son);
  // if (!son_node) {
  //     return false;
  // }
  // node->val = son_node->val;
  // node->type.type = son_node->type.type;
  // node->integer_val = son_node->integer_val;
  // node->float_val = son_node->float_val;

  // ast_node * left = node->parent;   // LValK
  // ast_node * right = node->sons[0]; //表达式

  // left->val->name = left->name;

  // // 创建临时变量保存IR的值，以及线性IR指令
  // if (symtab->currentFunc == nullptr) {
  //     //全局
  //     InterCode & irCode = symtab->getInterCode();
  //     irCode.addInst(right->blockInsts);
  //     irCode.addInst(left->blockInsts);
  //     irCode.addInst(new AssignIRInst(left->val, right->val, 1));
  // } else {
  //     //局部
  //     InterCode & irCode = symtab->currentFunc->getInterCode();
  //     irCode.addInst(right->blockInsts);
  //     irCode.addInst(left->blockInsts);
  //     irCode.addInst(new AssignIRInst(left->val, right->val, 1));
  // }

  return true;
}

/// @brief
/// ConstDefK是const定义语句其存储了所定义的名字，其子节点是一个赋值语句节点，其子节点的子节点为所定义的数值和数组维度，包含int和float类型
// ConstInitValK属于是赋值语句，其子节点的数值为父节点--ConstDefK的赋值数值,若是数组的话，ConstInitValK的子节点是ConstInitValK，子节点的子节点才是数值
// ConstExpGroupK若其有子节点且子节点有数值，则其父亲节点是数组变量，仅限于定义const和非const，不包含函数的形参数组
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_ConstDefK(ast_node *node) {
  //可能有问题等会再看
  Value *val = nullptr;
  ast_node *parent = node->parent;
  int is_np = 0;
  int flag_f = 0;
  int len = 0;
  int is_all = 0;
  int is_Loc = 0;
  // int temp_int;
  // float temp_float;
  switch (parent->BasicType) {
  case Void:
    node->type.type = BasicType::TYPE_VOID;
    break;
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    flag_f = 1;
    break;
  case Error:
    node->type.type = BasicType::TYPE_NONE;
    return false;
  }
  // for(auto son:node->sons)
  // {
  // 	if(son->OriginalType == ConstInitValK)
  // 	{
  // 		ast_node * son_node = ir_visit_ast_node(son);
  //     	if (!son_node) {
  //         	return false;
  //     	}
  // 		temp_int = son_node->integer_val;
  // 		temp_float = son_node->float_val;
  // 	}
  // }
  if (symtab->currentFunc != nullptr &&
      symtab->currentFunc->currentBlock != nullptr) {
    is_Loc = 1;
    val = symtab->currentFunc->currentBlock->findValue(node->name);
    if (!val) {
      // 变量不存在，则创建一个变量
      val = symtab->currentFunc->newVarValue(node->name, node->type.type);
      symtab->currentFunc->currentBlock->insertValue(val);
      if (node->type.type == BasicType::TYPE_NONE) {
        val->SetLiteral();
      } else {
        val->SetConst();
      }
    }
  } else if (symtab->currentFunc == nullptr) {
    is_all = 1;
    val = symtab->findValue(node->name, false);
    if (!val) {
      val = symtab->newVarValue(node->name, node->type.type);
    }
    if (node->type.type == BasicType::TYPE_NONE) {
      val->SetLiteral();
    } else {
      val->SetConst();
    }
  }
  node->val = val;

  for (auto son : node->sons) {
    if (son->OriginalType == ConstExpGroupK && son->sons.size() != 0) {
      is_np = 1;
      break;
    }
    if (son->OriginalType == ConstInitValK) {
      ast_node *son_node = ir_visit_ast_node(son);
      if (!son_node) {
        return false;
      }
      if (is_all == 1)
        val->is_allassign = 1;
      if (flag_f)
        val->realVal = son_node->val->realVal + son_node->val->intVal;
      else if (0 == flag_f)
        val->intVal = (int)son_node->val->realVal + son_node->val->intVal;
      node->blockInsts.addInst(son_node->blockInsts);
    }
  }
  if (is_np) {
    //是数组
    val->is_numpy = 1;
    len = 1;
    std::vector<int> sizes;
    for (auto son : node->sons) {
      if (son->OriginalType == ConstExpGroupK) {
        // size = son->sons.size();
        for (auto _son : son->sons) {
          ast_node *son_node = ir_visit_ast_node(_son);
          if (!son_node) {
            return false;
          }
          if (son_node->OriginalType == AddExpK ||
              son_node->OriginalType == MulExpK) {
            len = len * son_node->val->intVal;
            sizes.push_back(son_node->val->intVal);
            continue;
          }
          len = len * son_node->integer_val;
          sizes.push_back(son_node->integer_val);
        }
        val->creat_np(len, node->type.type);
        val->np->np_sizes = sizes;
        temp_np = val->np;
        temp_np->np_name = val->name;
      }

      // 开始赋值
      if (son->OriginalType == ConstInitValK) {
        // for(auto _son:son->sons)
        // {
        if (node->type.type == BasicType::TYPE_INT)
          dfs_save(val, son, val->np, 1, sizes.size(), 0, is_Loc);
        else if (node->type.type == BasicType::TYPE_FLOAT)
          dfs_save(val, son, val->np, 0, sizes.size(), 0, is_Loc);
        // dfs_save(ast_node * node,_numpy_ *np,int is_int,deep,int save_place)
        //如果是a[1][2][3]那么sizes顺序为3-2-1
        //  }
      }
      node->blockInsts.addInst(son->blockInsts);
    }
  }

  // return true;

  // if (symtab->currentFunc != nullptr) {
  //     val = symtab->currentFunc->findValue(node->name, false);
  //     if (!val) {

  //         // 变量不存在，则创建一个变量
  //         val = symtab->currentFunc->newVarValue(node->name,
  //         node->type.type); if (node->type.type == BasicType::TYPE_NONE) {
  //             val->SetLiteral();
  //         } else {
  //             val->SetConst();
  //         }
  //     }
  // } else if (symtab->currentFunc == nullptr) {
  //     val = symtab->findValue(node->name, false);
  //     if (!val) {
  //         val = symtab->newVarValue(node->name, node->type.type);
  //     }
  // }
  // node->val = val;
  // for (auto son: node->sons) {
  //     if (son->OriginalType == ConstInitValK) {
  //         ast_node * son_node = ir_visit_ast_node(son);
  //         if (!son_node) {
  //             return false;
  //         }
  //         val->intVal = son_node->val->intVal;
  //         if (flag_f)
  //             val->realVal = son_node->val->realVal + son_node->val->intVal;
  //         else if (0 == flag_f)
  //             val->realVal = (int) son_node->val->realVal +
  //             son_node->val->intVal;
  //     }
  // }

  // if(symtab->currentFunc == nullptr)
  // {
  // 	//全局变量
  // 	val = symtab->findValue(node->name, false);
  // 	if(val)
  // 	{
  // 		printf("redefine");
  // 		return false;
  // 	}
  // 	switch(parent->BasicType)
  // 	{
  //         case Integer:
  // 		val = symtab->newConstValue((int)temp_int);
  // 		val->name = node->name;
  // 		break;
  //         case Float:
  // 		val = symtab->newConstValue(temp_float);
  // 		val->name = node->name;
  // 		break;
  // 		default:
  // 		return false;
  // 		break;
  // 	}

  // }
  // else
  // {
  // 	//局部变量
  // 	val = symtab->currentFunc->findValue(node->name, false);
  // 	if(val)
  // 	{
  // 		printf("redefine");
  // 		return false;
  // 	}
  // 	switch(parent->BasicType)
  // 	{
  //         case Integer:
  // 		//int temp = node->sons[1]->sons[0]->integer_val;
  // 		val =
  // symtab->currentFunc->newVarValue(node->name,node->type.type); 		val->intVal =
  // temp_int; 		break;
  //         case Float:
  // 		//float temp = node->sons[1]->sons[0]->float_val;
  // 		val =
  // symtab->currentFunc->newVarValue(node->name,node->type.type); 		val->realVal
  // = temp_float; 		break; 		default: 		return false; 		break;
  // 	}
  // 	val->SetConst();
  // }
  // node->val = val;
  return true;
}
/// @brief
/// VarDefK是非const定义语句其存储了所定义的名字，其子节点是一个赋值语句节点，其子节点的子节点为所定义的数值和数组维度，包含int和float类型
// InitValK属于是赋值语句，其子节点的数值为父节点--VarDefK的赋值数值,若是数组的话，InitValK的子节点是InitValK，子节点的子节点才是数值
//若VarDefK的子节点没有InitValK，则说明该定义只声明了变量并没有给其赋值
// ConstExpGroupK若其有子节点且子节点有数值，则其父亲节点是数组变量，仅限于定义const和非const，不包含函数的形参数组
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_VarDeclK(ast_node *node) {

  // ast_node * name_son = node->sons[0];
  // ast_node * val_son;
  // int temp_int = 0;
  // float temp_float = 0;
  // Value Temp;
  // int flag = 0;
  int is_Loc = 0;
  Value *val = nullptr;
  switch (node->BasicType) {
  case Void:
    node->type.type = BasicType::TYPE_VOID;
    break;
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    break;
  case Error:
    node->type.type = BasicType::TYPE_NONE;
    return false;
  }
  for (int i = node->sons.size() - 1; i >= 0; i--) {
    auto son = node->sons[i];
    if (symtab->currentFunc != nullptr &&
        symtab->currentFunc->currentBlock != nullptr) {
      is_Loc = 1;
      //局部变量
      val = symtab->currentFunc->currentBlock->findValue(son->name);
      if (val) {
        printf("redefine");
        return false;
      }

      if (val == nullptr) {
        val = symtab->currentFunc->newVarValue(son->name, node->type.type);
        symtab->currentFunc->currentBlock->insertValue(val);
      }

      // 局部变量计数器
      // TODO:全局变量和局部变量的区分都是isLocalVar()方法，但是不同的是全局变量在symtab中的varvecter，而局部变量在symtab->current下的varvecter中
      if (val->isLocalVar() && !val->isliteral()) {
        Value::createLocalVarName();
      }
      if (val->isTemp()) {
        Value::createTempVarName();
      }
    } else if (symtab->currentFunc == nullptr) {
      is_Loc = 0;
      //全局变量
      val = symtab->findValue(son->name, false);
      if (val) {
        printf("redefine");
        // printf(val->name);
        return false;
      }
    }
    node->val = val;
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {
      return false;
    }
    node->blockInsts.addInst(son_node->blockInsts);
  }
  if (is_Loc == 0) {
    symtab->code.addInst(node->blockInsts);
  }
  return true;
}

/// @brief VarDefK
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_VarDefK(ast_node *node) {
  Value *val = nullptr;
  ast_node *parent = node->parent;
  int is_np = 0;
  int flag_f = 0;
  int len = 0;
  int is_all = 0;
  int is_Loc = 0;
  // int size = 0;
  switch (parent->BasicType) {
  case Void:
    node->type.type = BasicType::TYPE_VOID;
    break;
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    flag_f = 1;
    break;
  case Error:
    node->type.type = BasicType::TYPE_NONE;
    return false;
  }
  if (symtab->currentFunc != nullptr &&
      symtab->currentFunc->currentBlock != nullptr) {
    val = symtab->currentFunc->currentBlock->findValue(node->name);
    is_Loc = 1;
    if (!val) {

      // 变量不存在，则创建一个变量
      val = symtab->currentFunc->newVarValue(node->name, node->type.type);
      symtab->currentFunc->currentBlock->insertValue(val);
    }
  } else if (symtab->currentFunc == nullptr) {
    is_all = 1;
    val = symtab->findValue(node->name, false);
    if (!val) {
      val = symtab->newVarValue(node->name, node->type.type);
    }
  }
  node->val = val;

  for (auto son : node->sons) {
    if (son->OriginalType == ConstExpGroupK && son->sons.size() != 0) {
      is_np = 1;
      break;
    }
    if (son->OriginalType == InitValK) {
      ast_node *son_node = ir_visit_ast_node(son);
      if (!son_node) {
        return false;
      }
      if (is_all == 1)
        val->is_allassign = 1;
      if (flag_f)
        val->realVal = son_node->val->realVal + son_node->val->intVal;
      else if (0 == flag_f)
        val->intVal = (int)son_node->val->realVal + son_node->val->intVal;
      node->blockInsts.addInst(son_node->blockInsts);
    }
  }
  if (is_np) {
    //是数组

    val->is_numpy = 1;
    len = 1;
    std::vector<int> sizes;
    for (auto son : node->sons) {
      if (son->OriginalType == ConstExpGroupK) {
        // size = son->sons.size();
        for (auto _son : son->sons) {
          ast_node *son_node = ir_visit_ast_node(_son);
          if (!son_node) {
            return false;
          }
          if (son_node->OriginalType == AddExpK ||
              son_node->OriginalType == MulExpK ||
              son_node->OriginalType == LValK) {
            len = len * son_node->val->intVal;
            sizes.push_back(son_node->val->intVal);
            continue;
          }
          len = len * son_node->integer_val;
          sizes.push_back(son_node->integer_val);
        }
        val->creat_np(len, node->type.type);
        val->np->np_sizes = sizes;
        temp_np = val->np;
        temp_np->np_name = val->name;
      }
      // 开始赋值
      if (son->OriginalType == InitValK) {
        // for(auto _son:son->sons)
        // {
        if (node->type.type == BasicType::TYPE_INT)
          dfs_save(val, son, val->np, 1, sizes.size(), 0, is_Loc);
        else if (node->type.type == BasicType::TYPE_FLOAT)
          dfs_save(val, son, val->np, 0, sizes.size(), 0, is_Loc);
        // dfs_save(ast_node * node,_numpy_ *np,int is_int,deep,int save_place)
        //如果是a[1][2][3]那么sizes顺序为3-2-1
        //  }
      }
      node->blockInsts.addInst(son->blockInsts);
    }
  }

  return true;
}

/// @brief
/// FuncDeclK只是声明了函数，并没有给出函数的具体操作即Block，该节点包含了函数名和返回类型，其子节点为fomalpara，子节点的子节点是形参
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_FuncDeclK(ast_node *node) {
  //文法定义不允许嵌套函数
  // 创建一个函数，用于当前函数处理
  std::vector<FuncFormalParam> Params;
  ast_node *FParam_son = node->sons[0];
  int flag = 0;
  for (auto son : FParam_son->sons) {
    if (!flag)
      flag = 1;
    switch (node->BasicType) {
    case Void:
      son->type.type = BasicType::TYPE_VOID;
      break;
    case Integer:
      son->type.type = BasicType::TYPE_INT;
      break;
    case Float:
      son->type.type = BasicType::TYPE_FLOAT;
      break;
    case Error:
      son->type.type = BasicType::TYPE_NONE;
      return false;
    }
    FuncFormalParam *temp_para = new FuncFormalParam(son->name, son->type.type);

    Params.push_back(*temp_para);
    delete temp_para;
  }
  if (symtab->currentFunc != symtab->mainFunc) {
    // 函数中嵌套定义函数，这是不允许的，错误退出
    // TODO 自行追加语义错误处理
    return false;
  }
  switch (node->BasicType) {
  case Void:
    node->type.type = BasicType::TYPE_VOID;
    break;
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    break;
  case Error:
    node->type.type = BasicType::TYPE_NONE;
    return false;
  }
  symtab->currentFunc = new Function(node->name, node->type.type);
  symtab->currentFunc->getParams().assign(Params.begin(), Params.end());
  bool result = symtab->insertFunction(symtab->currentFunc);
  // Function * Temp = symtab->findFunction(node->name);
  if (!result) {
    // 清理资源
    delete symtab->currentFunc;

    // 恢复当前函数指向main函数
    symtab->currentFunc = symtab->mainFunc;

    Function *Temp_F = symtab->findFunction(node->name);
    if (Temp_F->isBuiltin() == true) {
      // 恢复成指向main函数
      symtab->currentFunc = symtab->mainFunc;
      return true;
    }

    // 函数已经定义过了，不能重复定义，语义错误：出错返回。
    // TODO 自行追加语义错误处理

    return false;
  }
  //因为是函数声明，因此此时没有语句块

  // 恢复成指向main函数
  symtab->currentFunc = symtab->mainFunc;

  return true;
}

/// @brief
/// FuncDefK是函数定义，子节点为Block和foamlpara，这两个的子节点分别是函数操作和形参
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_FuncDefK(ast_node *node) {
  //文法定义不允许嵌套函数
  // 创建一个函数，用于当前函数处理
  std::vector<FuncFormalParam> Params;
  /*
  ast_node * FParam_son = node->sons[0];
  int flag = 0;*/
  if (symtab->currentFunc != symtab->mainFunc) {
    // 函数中嵌套定义函数，这是不允许的，错误退出
    // TODO 自行追加语义错误处理
    return false;
  }
  /*
      for(auto son: FParam_son->sons)
      {
          if(!flag)flag = 1;
          switch(node->BasicType)
          {
              case Void: son->type.type = BasicType::TYPE_VOID; break;
              case Integer: son->type.type = BasicType::TYPE_INT; break;
              case Float: son->type.type = BasicType::TYPE_FLOAT; break;
              case Error: son->type.type = BasicType::TYPE_NONE; break;
          }
          FuncFormalParam * temp_para = new
     FuncFormalParam(son->name,son->type.type); Params.push_back(*temp_para);
          delete temp_para;
      }
      */
  switch (node->BasicType) {
  case Void:
    node->type.type = BasicType::TYPE_VOID;
    break;
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    break;
  case Error:
    node->type.type = BasicType::TYPE_NONE;
    return false;
  }
  if (!symtab->findFunction(node->name)) { //找不到，即前面没有Decl函数声明
    /*if(flag)
    {
        symtab->currentFunc = new Function(node->name,node->type.type);
        symtab->currentFunc->getParams().assign(Params.begin(), Params.end());
    }
    else*/
    symtab->currentFunc = new Function(node->name, node->type.type);
    bool result = symtab->insertFunction(symtab->currentFunc);
    if (!result) {
      // 清理资源
      delete symtab->currentFunc;

      // 恢复当前函数指向main函数
      symtab->currentFunc = symtab->mainFunc;

      // 函数已经定义过了，不能重复定义，语义错误：出错返回。
      // TODO 自行追加语义错误处理

      return false;
    }
  } else
    symtab->currentFunc = symtab->findFunction(node->name);

  if (node->name == "main") {
    symtab->mainFunc = symtab->currentFunc;
  }
  // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
  InterCode &irCode = symtab->currentFunc->getInterCode();

  // 这里也可增加一个函数入口Label指令，便于后续基本块划分
  IRInst *entryLabelInst = new LabelIRInst();

  irCode.addInst(entryLabelInst);

  // 创建并加入Entry入口指令
  irCode.addInst(new EntryIRInst());

  // // TODO:全局初始化信息追加到这里 方便后端代码生成
  // irCode.addInst(symtab->code);

  // 创建出口指令并不加入出口指令，等函数内的指令处理完毕后加入出口指令
  IRInst *exitLabelInst = new LabelIRInst();

  // 函数出口指令保存到函数信息中，因为在语义分析函数体时return语句需要跳转到函数尾部，需要这个label指令
  symtab->currentFunc->setExitLabel(exitLabelInst);

  // 新建一个Value，用于保存函数的返回值，如果没有返回值可不用申请，
  // 目前未知，先创建一个，不用后续可释放
  Value *retValue = symtab->currentFunc->newVarValue(node->type.type);

  // 保存函数返回值变量到函数信息中，在return语句翻译时需要设置值到这个变量中
  symtab->currentFunc->setReturnValue(retValue);

  // 遍历函数体内的每个语句

  //

  for (auto son : node->sons) {

    // 遍历函数定义，孩子要么是形式参数，要么是block
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {

      // 对函数体内的语句进行语义分析时出现错误
      return false;
    }

    // IR指令追加到当前的节点中
    if (son == node->sons[0] && son->OriginalType == FuncFParamsK) {
      // symtab->currentFunc->set_useParams();
      for (auto &param : symtab->currentFunc->params) {
        Value *Temp = symtab->currentFunc->newTempValue(param.type.type);
        Temp->set_FParam();
        param.save_val = Temp;
        // param.val->is_saveFParam = 1;
        node->blockInsts.addInst(new AssignIRInst(param.val, param.save_val));
      }
    }
    node->blockInsts.addInst(son_node->blockInsts);
  }

  // 此时，所有指令都加入到当前函数中，也就是node->blockInsts

  // node节点的指令移动到函数的IR指令列表中
  irCode.addInst(node->blockInsts);

  // 添加函数出口Label指令，主要用于return语句跳转到这里进行函数的退出
  irCode.addInst(exitLabelInst);

  // 检查函数是否有返回值类型，则需要设置返回值，否则不设置
  if (symtab->currentFunc->getReturnType().type != BasicType::TYPE_VOID) {
    // 函数出口指令
    irCode.addInst(new ExitIRInst(retValue));
  } else {
    // 清理资源恢复原状
    symtab->currentFunc->deleteVarValue(retValue);
    symtab->currentFunc->setReturnValue(nullptr);
    delete retValue;

    // 函数出口指令
    irCode.addInst(new ExitIRInst());
  }

  // 恢复成指向main函数
  symtab->currentFunc = symtab->mainFunc;
  return true;
}

/// @brief
/// FuncFParamsK是函数形参表，其子节点为FuncFParamK包含了形参的类型(int,float)和名字，若无形参其后面没有子节点
//若形参为数组，FuncFParamK子节点存在一个ExpK类型的节点，其他n个子节点为数值，该形参维度为n+1
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_FuncFParamsK(ast_node *node) {
  // 获取当前要保存函数的形式参数清单
  auto &params = symtab->currentFunc->getParams();

  // 遍历形式参数列表，孩子是叶子节点
  int i = node->sons.size() - 1;
  for (; i >= 0; i--) {
    auto son = node->sons[i];
    // 创建变量，默认整型
    switch (son->BasicType) {
    case Void:
      son->type.type = BasicType::TYPE_VOID;
      break;
    case Integer:
      son->type.type = BasicType::TYPE_INT;
      break;
    case Float:
      son->type.type = BasicType::TYPE_FLOAT;
      break;
    case Error:
      son->type.type = BasicType::TYPE_NONE;
      break;
    }
    if (son->sons.size() == 0) {
      Value *var = symtab->currentFunc->newVarValue(son->name, son->type.type);
      var->is_saveFParam = 1;

      // 默认是整数类型
      params.emplace_back(son->name, son->type.type, var);
    } else if (son->sons.size() != 0) {
      Value *var = nullptr;
      if (son->type.type == BasicType::TYPE_INT) {
        var =
            symtab->currentFunc->newVarValue(son->name, BasicType::TYPE_FNP_I);
      } else if (son->type.type == BasicType::TYPE_FLOAT) {
        var =
            symtab->currentFunc->newVarValue(son->name, BasicType::TYPE_FNP_F);
      }
      var->is_saveFParam = 1;
      var->creat_np(100, son->type.type);
      var->np->_flag = 1;
      var->np->np_sizes.push_back(0);
      params.emplace_back(son->name, son->type.type, var);
    }
  }

  return true;
}

/// @brief
/// BlockK是语句块，只要有{}的都算语句块，其中语句块大部分用于函数操作，if，while语句
// ps：注意作用域；if，while可以没有BlockK，即没有{}，eg：if（a） a = 1；
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_BlockK(ast_node *node) {
  //处理if、while、{}
  // std::vector<ast_node *>::iterator pIter,end_pIter;
  // pIter = node->sons.end();
  // end_pIter = node->sons.begin();
  // pIter--;
  // end_pIter--;
  int is_Loc = 0;
  int is_null = 1;
  Var_Block *tempBlock = nullptr;
  if (symtab->currentFunc != nullptr) {
    is_Loc = 1;
    if (symtab->currentFunc->currentBlock != nullptr) {
      tempBlock = symtab->currentFunc->currentBlock;
      is_null = 0;
    }
    symtab->currentFunc->currentBlock = symtab->currentFunc->newBlock();
    node->is_saveBlock = 1;
    node->Block_number = symtab->currentFunc->currentBlock->number;
  }
  int i = node->sons.size() - 1;
  for (; i >= 0; i--) {

    // 遍历Block的每个语句，进行显示或者运算
    auto son = node->sons[i];
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {
      return false;
    }

    node->blockInsts.addInst(son_node->blockInsts);
  }
  if (is_Loc == 1 && is_null == 0 && tempBlock != nullptr) {
    symtab->currentFunc->currentBlock = tempBlock;
  }
  return true;
}

/// @brief StmtK包含AT_AssignStmt、AT_IfStmt、
// AT_WhileStmt、AT_BreakStmt、AT_ContinueStmt、
// AT_ConstInitVal
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_StmtK(ast_node *node) {
  if (node->ast_node_Type == AT_AssignStmt) {
    //赋值语句
    ast_node *son1_node = node->sons[0]; // LValK
    ast_node *son2_node = node->sons[1]; //表达式
    int is_L_np = 0;
    int is_R_np = 0;
    int is_Loc = 0;

    if (symtab->currentFunc != nullptr) {
      is_Loc = 1;
    } else if (symtab->currentFunc == nullptr) {
      is_Loc = 0;
    }

    Value *L_np = nullptr;
    Value *R_np = nullptr;
    Value *temp = nullptr;

    // 赋值节点，自右往左运算

    // 赋值运算符的左侧操作数
    ast_node *left = ir_visit_ast_node(son1_node);
    if (!left) {
      // 某个变量没有定值
      // 这里缺省设置变量不存在则创建，因此这里不会错误
      return false;
    }
    if (left->np_val != nullptr && left->np_val->is_issavenp()) {
      is_L_np = 1;
      L_np = left->np_val;
    }

    // 赋值运算符的右侧操作数
    ast_node *right = ir_visit_ast_node(son2_node);
    if (!right) {
      // 某个变量没有定值
      return false;
    }
    if (right->np_val != nullptr && right->np_val->is_issavenp()) {
      is_R_np = 1;
      R_np = right->np_val;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    // TODO real number add

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    if (is_L_np == 0 && is_R_np == 0) {
      node->blockInsts.addInst(new AssignIRInst(left->val, right->val));
    } else if (is_L_np == 0 && is_R_np == 1) {
      node->blockInsts.addInst(new AssignIRInst(left->val, R_np, 2));
    } else if (is_L_np == 1 && is_R_np == 0) {
      node->blockInsts.addInst(new AssignIRInst(L_np, right->val, 3));
    } else if (is_L_np == 1 && is_R_np == 1) {
      if (is_Loc == 1) {
        temp = symtab->currentFunc->newTempValue(R_np->type.type);
      } else {
        temp = symtab->newTempValue(R_np->type.type);
      }
      node->blockInsts.addInst(new AssignIRInst(temp, R_np, 2));
      node->blockInsts.addInst(new AssignIRInst(L_np, temp, 3));
    }

    // 这里假定赋值的类型是一致的
    // left->val->type = right->val->type;
    node->val = left->val;
  } else if (node->ast_node_Type == AT_WhileStmt) {
    // int flag = 0;
    int flag = 0;
    ast_node *son2_node = node->sons[1]; //循环体入口

    IRInst *tmptrue = TrueEntry;   //循环体入口
    IRInst *tmpfalse = FalseEntry; //循环体出口
    IRInst *tmpentry = EntryEntry; //条件入口
    IRInst *tmpexit = ExitEntry;   //循环体出口
    IRInst *tmpBreak = Break_Exit;
    IRInst *tmpContinue = Continue_Entry;

    IRInst *Entry1 = new LabelIRInst(); //条件入口
    IRInst *Entry2 = new LabelIRInst(); //循环体入口
    IRInst *Exit = new LabelIRInst();   //循环体出口

    TrueEntry = Entry2;
    FalseEntry = Exit;
    EntryEntry = Entry1;
    ExitEntry = Exit;
    Break_Exit = Exit;
    Continue_Entry = Entry1;

    BrIRInst *BrCondi = new BrIRInst(Entry1);

    // BcIRInst * BcT_F = new BcIRInst();

    // ast_node * parent = node->parent;
    // ast_node * parent_parent = parent->parent;

    // if(parent_parent->ast_node_Type == AT_IfStmt ||
    // parent_parent->ast_node_Type == AT_WhileStmt)
    // {
    // 	flag = 0;
    // }
    // if(flag == 0)
    // {
    if (node->sons[0]->OriginalType == LAndExpK ||
        node->sons[0]->OriginalType == LOrExpK) {
      flag = 1;
    }
    node->blockInsts.addInst(Entry1);
    // }

    for (auto son : node->sons) {

      //先遍历条件，再遍历循环体
      ast_node *son_node = ir_visit_ast_node(son);
      if (!son_node) {
        return false;
      }
      if (son_node == son2_node) //循环体入口节点
      {
        if (flag == 0) {
          BcIRInst *BcT_F = new BcIRInst(3, node->sons[0]->val);
          node->blockInsts.addInst(BcT_F); // br
        }
        // node->blockInsts.addInst(BcT_F);                // bc
        node->blockInsts.addInst(Entry2);               //
        node->blockInsts.addInst(son_node->blockInsts); //循环体
        node->blockInsts.addInst(BrCondi);              //回到条件入口
        continue;
      }
      node->blockInsts.addInst(son_node->blockInsts);
    }
    // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
    // if(flag == 3)
    // {
    // 	InterCode & irCode = symtab->currentFunc->getInterCode();
    // 	irCode.addInst(Entry1);
    // 	irCode.addInst(node->blockInsts);
    // 	irCode.addInst(Exit);
    // }
    // if(flag == 0)
    // {
    node->blockInsts.addInst(Exit);
    // }

    TrueEntry = tmptrue;
    FalseEntry = tmpfalse;
    EntryEntry = tmpentry;
    ExitEntry = tmpexit;
    // WhileStmt创建循环体入口、出口以及条件判断入口
    Break_Exit = tmpBreak;
    Continue_Entry = tmpContinue;

  } else if (node->ast_node_Type == AT_IfStmt) {
    // if
    int flag = 0;

    // TODO int Loc_flag = 0;
    //  int Unary_flag = 0;
    //  if(symtab->currentFunc != nullptr)
    //  {
    //  	Loc_flag = 1;
    //  }
    //  Value * val =nullptr;
    IRInst *tmptrue = TrueEntry;   // True入口
    IRInst *tmpfalse = FalseEntry; // False入口
    IRInst *tmpentry = EntryEntry; //条件入口
    IRInst *tmpexit = ExitEntry;   //出口

    IRInst *Entry = new LabelIRInst();  //条件入口
    IRInst *trueE = new LabelIRInst();  // True入口
    IRInst *falseE = new LabelIRInst(); // False入口
    IRInst *Exit = new LabelIRInst();   //出口

    TrueEntry = trueE;   // True入口
    FalseEntry = falseE; // False入口
    EntryEntry = Entry;  //条件入口
    ExitEntry = Exit;    //出口

    // BcIRInst * BcT_F = new BcIRInst();
    BrIRInst *BrExit = new BrIRInst(Exit);
    // ast_node * parent = node->parent;
    // ast_node * parent_parent = parent->parent;

    // if(parent_parent->ast_node_Type == AT_IfStmt ||
    // parent_parent->ast_node_Type == AT_WhileStmt)
    // {
    // 	flag = 0;
    // }
    int size = node->sons.size();

    if (node->sons[0]->OriginalType == LAndExpK ||
        node->sons[0]->OriginalType == LOrExpK) {
      flag = 1;
    }

    // TODO if(node->sons[0]->OriginalType != RelExpK ||
    // node->sons[0]->OriginalType != EqExpK)
    //  {
    //  	Unary_flag = 1;
    //  }

    // if(node->sons[0]->val->type.type != BasicType::TYPE_BOOL)
    // {
    // 	Unary_flag = 1;
    // }

    // if (node->OriginalType == LAndExpK || node->OriginalType == LOrExpK) {
    //     flag = 1;
    // }

    // if(flag == 0)
    // {
    node->blockInsts.addInst(Entry);
    // }
    if (size == 2) //没有else方便后续优化吧
    {
      for (auto son : node->sons) {

        //先遍历条件，再遍历循环体
        ast_node *son_node = ir_visit_ast_node(son);
        if (!son_node) {
          return false;
        }
        // if(son_node == node->sons[0])
        // {
        // 	node->blockInsts.addInst(son_node->blockInsts);
        // 	if(Unary_flag == 1)
        // 	{
        // 		//TODO
        // 		if(Loc_flag == 1) val =
        // symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL); 		else val =
        // symtab->newTempValue(BasicType::TYPE_BOOL);
        // 		// TODO
        // 		node->sons[0]->val = val;
        // 		node->blockInsts.addInst(new
        // BinaryIRInst(IRInstOperator::IRINST_OP_NQ,val, son_node->val,0,3));
        // 	}

        // 	continue;
        // }
        if (son_node == node->sons[1]) // True
        {
          // BcIRInst * BcT_F = new BcIRInst();
          if (flag == 0) {
            BcIRInst *BcT_F = new BcIRInst(3, node->sons[0]->val);
            node->blockInsts.addInst(BcT_F); // br
          }
          node->blockInsts.addInst(trueE);                //
          node->blockInsts.addInst(son_node->blockInsts); //循环体
          node->blockInsts.addInst(BrExit);
          node->blockInsts.addInst(falseE); //回到条件入口
          continue;
        }
        node->blockInsts.addInst(son_node->blockInsts);
      }
    } else if (size == 3) //存在else
    {
      for (auto son : node->sons) {

        //先遍历条件，再遍历循环体
        ast_node *son_node = ir_visit_ast_node(son);
        if (!son_node) {
          return false;
        }

        // if(son_node == node->sons[0])
        // {
        // 	node->blockInsts.addInst(son_node->blockInsts);
        // 	if(Unary_flag == 1)
        // 	{
        // 		//TODO
        // 		if(Loc_flag == 1) val =
        // symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL); 		else val =
        // symtab->newTempValue(BasicType::TYPE_BOOL);
        // 		// TODO
        // 		node->sons[0]->val = val;
        // 		node->blockInsts.addInst(new
        // BinaryIRInst(IRInstOperator::IRINST_OP_NQ,val, son_node->val,0,3));
        // 	}

        // 	continue;
        // }

        if (son_node == node->sons[1]) // True
        {
          // BcIRInst * BcT_F = new BcIRInst();
          if (flag == 0) {
            BcIRInst *BcT_F = new BcIRInst(3, node->sons[0]->val);
            node->blockInsts.addInst(BcT_F); // br
          }
          node->blockInsts.addInst(trueE);                //
          node->blockInsts.addInst(son_node->blockInsts); //循环体
          node->blockInsts.addInst(BrExit);
          node->blockInsts.addInst(falseE); //
          continue;
        }
        node->blockInsts.addInst(son_node->blockInsts);
      }
    }
    // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
    // if(flag == 1)
    // {
    // 	InterCode & irCode = symtab->currentFunc->getInterCode();
    // 	irCode.addInst(Entry);//条件
    // 	irCode.addInst(node->blockInsts);
    // 	irCode.addInst(Exit);//出口
    // }
    // if(flag == 0)
    // {
    node->blockInsts.addInst(Exit);
    // }

    TrueEntry = tmptrue;
    FalseEntry = tmpfalse;
    EntryEntry = tmpentry;
    ExitEntry = tmpexit;
  } else if (node->ast_node_Type == AT_BreakStmt) {
    // BreakStmt
    if (ExitEntry == nullptr) { // TODO 报错显示
      return false;             //没有while直接break
    }
    // Break_Exit = tmpBreak;
    // Continue_Entry = tmpContinue;
    BrIRInst *BrExit = new BrIRInst(Break_Exit);
    node->blockInsts.addInst(BrExit);
  } else if (node->ast_node_Type == AT_ContinueStmt) {
    // ContinueStmt
    if (EntryEntry == nullptr) { // TODO 报错显示
      return false;              //没有while直接continue
    }
    BrIRInst *BrEntry = new BrIRInst(Continue_Entry);
    node->blockInsts.addInst(BrEntry);
  } else if (node->ast_node_Type == AT_ReturnStmt) {
    ast_node *right = nullptr;
    int is_null = 1;
    // return语句可能没有没有表达式，也可能有，因此这里必须进行区分判断
    if (!node->sons.empty()) {

      ast_node *son_node = node->sons[0];

      // 返回的表达式的指令保存在right节点中
      right = ir_visit_ast_node(son_node);
      if (!right) {

        // 某个变量没有定值
        return false;
      }
      is_null = 0;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    // 创建临时变量保存IR的值，以及线性IR指令
    // TODO
    if (is_null == 0) {
      node->blockInsts.addInst(right->blockInsts);

      if (right->np_val != nullptr && right->np_val->is_issavenp()) {
        // 返回值赋值到函数返回值变量上，然后跳转到函数的尾部
        Value *temp =
            symtab->currentFunc->newTempValue(right->np_val->type.type);

        node->blockInsts.addInst(new AssignIRInst(temp, right->np_val, 2));
        node->blockInsts.addInst(
            new AssignIRInst(symtab->currentFunc->getReturnValue(), temp));

        // 跳转到函数的尾部出口指令上
        node->blockInsts.addInst(
            new BrIRInst(symtab->currentFunc->getExitLabel()));

        node->val = right->np_val;
      } else {
        // 返回值赋值到函数返回值变量上，然后跳转到函数的尾部
        node->blockInsts.addInst(new AssignIRInst(
            symtab->currentFunc->getReturnValue(), right->val));

        // 跳转到函数的尾部出口指令上
        node->blockInsts.addInst(
            new BrIRInst(symtab->currentFunc->getExitLabel()));

        node->val = right->val;
      }

      // 这里设置返回值类型
      ValueType &returnType = symtab->currentFunc->getReturnType();
      if (returnType.type == BasicType::TYPE_VOID) {
        // 设置类型
        returnType.type = right->val->type.type;
      } else if (returnType.type != right->val->type.type) {
        // 两者类型不一致，要出错显示
        // 或者隐式转换成更高的类型
        // TODO 这里目前什么都不做
      }
    } else if (is_null == 1) {
      node->blockInsts.addInst(
          new BrIRInst(symtab->currentFunc->getExitLabel()));
    }

  } else if (node->ast_node_Type == AT_EmptyStmt) {
    ;
  } else {
    return false;
  }
  return true;
}

/*
    ！！！！！！！！！！！！！！！！！！！！
    NumberK是数字，包含形参数组的维度，运算式子的数字、表示数组维度的数字，return的数字，条件式子的数字，定义变量，变量数组的数字,形参实参的数字
    ！！！！！！！！！！！！！！！！！！！！
*/
/// @brief NumberK建立value
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_NumberK(ast_node *node) {
  Value *val = nullptr;
  int err = 0;
  switch (node->BasicType) {
  case Integer:
    node->type.type = BasicType::TYPE_INT;
    val = symtab->newConstValue((int32_t)node->integer_val);
    // 新建一个整数常量Value
    break;
  case Float:
    node->type.type = BasicType::TYPE_FLOAT;
    val = symtab->newConstValue(node->float_val);
    // 新建一个浮点常量Value
    break;
  default:
    // TODO显示错误
    err = 1;
    break;
  }
  if (err)
    return false;
  node->val = val;
  return true;
}

/// @brief InitValK计算所要赋值的数值
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_InitValK(ast_node *node) {
  // int is_L_np = 0;
  int is_R_np = 0;
  // Value * L_np = nullptr;
  //不存在int a[5] = 6这种情况
  Value *R_np = nullptr;

  ast_node *son = node->sons[0];
  ast_node *son_node = ir_visit_ast_node(son);
  if (!son_node) {
    return false;
  }
  if (son_node->np_val != nullptr && son_node->np_val->is_issavenp()) {
    is_R_np = 1;
    R_np = son_node->np_val;
  }
  if (is_R_np == 0) {
    node->val = son_node->val;
    node->type.type = son_node->type.type;
    node->integer_val = son_node->integer_val;
    node->float_val = son_node->float_val;

    //赋值语句
    ast_node *left = node->parent;   // LValK
    ast_node *right = node->sons[0]; //表达式

    left->val->name = left->name;
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(new AssignIRInst(left->val, right->val));
    // if (symtab->currentFunc != nullptr) {
    //     InterCode & irCode = symtab->currentFunc->getInterCode();
    //     irCode.addInst(right->blockInsts);
    //     irCode.addInst(left->blockInsts);
    //     irCode.addInst(new AssignIRInst(left->val, right->val));
    // } else if (symtab->currentFunc == nullptr) {
    //     InterCode & irCode = symtab->getInterCode();
    //     irCode.addInst(right->blockInsts);
    //     irCode.addInst(left->blockInsts);
    //     irCode.addInst(new AssignIRInst(left->val, right->val));
    // }
  } else if (is_R_np == 1) {
    node->val = son_node->val;
    node->integer_val = son_node->integer_val;
    node->float_val = son_node->float_val;
    node->type.type = son_node->type.type;
    ast_node *left = node->parent;   // LValK
    ast_node *right = node->sons[0]; //表达式
    left->val->name = left->name;
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(new AssignIRInst(left->val, R_np, 2));
    // if (symtab->currentFunc != nullptr) {
    //     InterCode & irCode = symtab->currentFunc->getInterCode();
    //     irCode.addInst(right->blockInsts);
    //     irCode.addInst(left->blockInsts);
    //     irCode.addInst(new AssignIRInst(left->val, R_np, 2));
    // } else if (symtab->currentFunc == nullptr) {
    //     InterCode & irCode = symtab->getInterCode();
    //     irCode.addInst(right->blockInsts);
    //     irCode.addInst(left->blockInsts);
    //     irCode.addInst(new AssignIRInst(left->val, R_np, 2));
    // }
  }

  // 赋值节点，自右往左运算

  // // 赋值运算符的左侧操作数
  // ast_node * left = ir_visit_ast_node(son1_node);
  // if (!left) {
  // // 某个变量没有定值
  // // 这里缺省设置变量不存在则创建，因此这里不会错误
  // 	return false;
  // }

  // 赋值运算符的右侧操作数
  // ast_node * right = ir_visit_ast_node(son2_node);
  // if (!right) {
  // // 某个变量没有定值
  // 	return false;
  // }

  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add

  // // 创建临时变量保存IR的值，以及线性IR指令
  // if (symtab->currentFunc != nullptr) {
  //     InterCode & irCode = symtab->currentFunc->getInterCode();
  //     irCode.addInst(right->blockInsts);
  //     irCode.addInst(left->blockInsts);
  //     irCode.addInst(new AssignIRInst(left->val, right->val));
  // } else if (symtab->currentFunc == nullptr) {
  //     InterCode & irCode = symtab->getInterCode();
  //     irCode.addInst(right->blockInsts);
  //     irCode.addInst(left->blockInsts);
  //     irCode.addInst(new AssignIRInst(left->val, right->val));
  // }

  // 这里假定赋值的类型是一致的
  // left->val->type = right->val->type;
  // node->val = left->val;

  return true;
}

/// @brief
/// UnaryExpK为单目运算，包含函数调用，+-！的单目运算，存储着函数名或+-!;eg：putin(A);!A;
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_UnaryExpK(ast_node *node) {
  if (node->flag == 123) {
    //函数调用
    Value *save_temp = nullptr;
    std::vector<Value *> realParams;

    // 根据函数名查找函数，看是否存在。若不存在则出错
    // 这里约定函数必须先定义后使用
    auto pFunction = symtab->findFunction(node->name);
    if (nullptr == pFunction) {
      // TODO 这里输出错误信息
      return false;
    }

    // 设置存在函数调用
    symtab->currentFunc->setExistFuncCall(true);

    // 如果没有孩子，也认为是没有参数
    if (!node->sons.empty()) {

      // 只有一个节点，实际参数列表
      // auto paramsNode = node->sons[0];

      int argsCount = (int)node->sons.size();

      // 设置最大函数调用参数个数
      if (argsCount > symtab->currentFunc->getMaxFuncCallArgCnt()) {
        symtab->currentFunc->setMaxFuncCallArgCnt(argsCount);
      }

      // 遍历参数列表，孩子是表达式
      // 这里自左往右计算表达式
      for (int i = (int)node->sons.size() - 1; i >= 0; i--) {
        FuncFormalParam param = pFunction->params[i];
        // 遍历Block的每个语句，进行显示或者运算
        auto son = node->sons[i];
        ast_node *temp = ir_visit_ast_node(son);
        if (!temp) {
          return false;
        }

        node->blockInsts.addInst(temp->blockInsts);

        if (temp->val->is_numpy == 1 && temp->np_val != nullptr &&
            (param.type.type != BasicType::TYPE_FNP_I &&
             param.type.type != BasicType::TYPE_FNP_F)) {
          // TODO 有点问题的这里，可能要新增一个BasicType表示数组类型
          //不需要传进去一个数组，而是传进去一个数组的一个int或者float的
          save_temp = symtab->currentFunc->newTempValue(temp->val->type.type);
          node->blockInsts.addInst(
              new AssignIRInst(save_temp, temp->np_val, 2));
          realParams.push_back(save_temp);
          continue;
        } else if ((param.type.type == BasicType::TYPE_FNP_I ||
                    param.type.type == BasicType::TYPE_FNP_F) &&
                   temp->np_val != nullptr && temp->np_val->is_issavenp()) {
          realParams.push_back(temp->np_val);
          continue;
        }
        realParams.push_back(temp->val);
      }
    }

    // 创建临时变量，用于保存函数调用的结果
    Value *resultVal = nullptr;
    // int f_void = 0;

    // 返回调用有返回值，则需要分配临时变量
    if (pFunction->getReturnType().type == BasicType::TYPE_INT) {
      resultVal = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
    } else if (pFunction->getReturnType().type == BasicType::TYPE_FLOAT) {
      resultVal = symtab->currentFunc->newTempValue(BasicType::TYPE_FLOAT);
    } else if (pFunction->getReturnType().type == BasicType::TYPE_VOID) {
      resultVal = symtab->currentFunc->newTempValue(BasicType::TYPE_VOID);
      // f_void = 1;
    }

    // 创建函数调用指令
    node->blockInsts.addInst(
        new FuncCallIRInst(node->name, realParams, resultVal));

    // 设置存在函数调用，后面要根据是否函数调用进行调整栈分配策略
    symtab->currentFunc->setExistFuncCall(true);

    // 函数调用结果保存到node中，用于外部使用
    node->val = resultVal;
  }

  else {
    //单目运算
    // int is_LValK = 0;
    ast_node *son = node->sons[0];
    ast_node *parent = node->parent;
    int not_flag = 0;
    int min_flag = 0;
    int Loc_flag = 0;
    if (symtab->currentFunc != nullptr) {
      Loc_flag = 1;
    }
    if (parent->node_type == ast_operator_type::AST_UNARYOP_NOT) {
      not_flag = 1;
    }
    if (parent->node_type == ast_operator_type::AST_UNARYOP_MINUS) {
      min_flag = 1;
    }
    // if (node->sons[0]->OriginalType == LValK) {
    //     is_LValK = 1;
    // }
    ast_node *son_node = ir_visit_ast_node(son);
    if (!son_node) {
      return false;
    }
    node->blockInsts.addInst(son_node->blockInsts);
    Value *val = nullptr;
    // Value * val = new TempValue(son_node->val->type.type);
    // val->type.type = son_node->val->type.type;
    // val->regId = son_node->val->regId;
    // val->offset = son_node->val->offset;
    // val->baseRegNo = son_node->val->baseRegNo;
    switch (node->node_type) {
    case ast_operator_type::AST_UNARYOP_MINUS: //"-"
      if (son_node->node_type !=
          ast_operator_type::AST_UNARYOP_MINUS) { //孩子不是

        if (min_flag == 0) { // parent不是“-”，孩子也不是
          // TODO
          if (son_node->node_type == ast_operator_type::AST_UNARYOP_NOT) {
            if (Loc_flag == 1)
              val = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
            else
              val = symtab->newTempValue(BasicType::TYPE_INT);
          } else if (son_node->node_type !=
                     ast_operator_type::AST_UNARYOP_NOT) {
            if (Loc_flag == 1)
              val = symtab->currentFunc->newTempValue(son_node->val->type.type);
            else
              val = symtab->newTempValue(son_node->val->type.type);
          }
          val->intVal = -(son_node->val->intVal);
          val->realVal = -(son_node->val->realVal);
          node->blockInsts.addInst(new AssignIRInst(val, son_node->val, 5));
          node->val = val;
          node->val->MIN_is_three = 1;
          break;
        } else if (min_flag == 1) { // parent是“-”，孩子不是
          node->val = son_node->val;
          node->val->MIN_is_three = 1;
          break;
        }
      } else if (son_node->node_type ==
                 ast_operator_type::AST_UNARYOP_MINUS) { //孩子是
        if (min_flag == 1) { //孩子和父母都是“-”
          node->val = son_node->val;
          node->val->MIN_is_three = !(son_node->val->MIN_is_three);
          // val->intVal = (son_node->val->intVal);
          // val->realVal = (son_node->val->realVal);
          break;
        } else if (min_flag == 0) { //孩子是，父母不是
          if (Loc_flag == 1)
            val = symtab->currentFunc->newTempValue(son_node->val->type.type);
          else
            val = symtab->newTempValue(son_node->val->type.type);
          if (son_node->val->MIN_is_three == 1) {
            node->blockInsts.addInst(new AssignIRInst(val, son_node->val));
            val->intVal = (son_node->val->intVal);
            val->realVal = (son_node->val->realVal);
            node->val = val;
          } else if (son_node->val->MIN_is_three == 0) {
            node->blockInsts.addInst(new AssignIRInst(val, son_node->val, 5));
            val->intVal = -(son_node->val->intVal);
            val->realVal = -(son_node->val->realVal);
            node->val = val;
          }

          break;
        }
      }
      break;
    case ast_operator_type::AST_UNARYOP_PLUS: //"+"
      node->val = son_node->val;
      // val->name += son_node->val->name;
      break;
    case ast_operator_type::AST_UNARYOP_NOT: //"!"
      if (son_node->node_type !=
          ast_operator_type::AST_UNARYOP_NOT) { //孩子不是
        if (not_flag == 0) { // parent不是“-”，孩子也不是
          // TODO
          if (Loc_flag == 1)
            val = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);
          else
            val = symtab->newTempValue(BasicType::TYPE_BOOL);
          val->intVal = !(son_node->val->intVal);
          val->realVal = !(son_node->val->realVal);
          node->blockInsts.addInst(new BinaryIRInst(
              IRInstOperator::IRINST_OP_EQ, val, son_node->val, 0, 3));
          node->val = val;
          node->val->NOT_is_three = 1;
          break;
        } else if (not_flag == 1) { // parent是“-”，孩子不是
          node->val = (son_node->val);
          node->val->NOT_is_three = 1;
          break;
        }
      } else if (son_node->node_type ==
                 ast_operator_type::AST_UNARYOP_NOT) { //孩子是
        if (not_flag == 1) { //孩子和父母都是“-”
          node->val = (son_node->val);
          node->val->NOT_is_three = !(son_node->val->NOT_is_three);
          // val->intVal = (son_node->val->intVal);
          // val->realVal = (son_node->val->realVal);
          break;
        } else if (not_flag == 0) { //孩子是，父母不是
          if (son_node->val->NOT_is_three == 1) {
            if (Loc_flag == 1)
              val = symtab->currentFunc->newTempValue(son_node->val->type.type);
            else
              val = symtab->newTempValue(son_node->val->type.type);
            node->blockInsts.addInst(new AssignIRInst(val, son_node->val));
            val->intVal = (son_node->val->intVal);
            val->realVal = (son_node->val->realVal);
            node->val = val;
          } else if (son_node->val->NOT_is_three == 0) {
            if (Loc_flag == 1)
              val = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);
            else
              val = symtab->newTempValue(BasicType::TYPE_BOOL);
            node->blockInsts.addInst(new BinaryIRInst(
                IRInstOperator::IRINST_OP_EQ, val, son_node->val, 0, 3));
            val->intVal = !(son_node->val->intVal);
            val->realVal = !(son_node->val->realVal);
            node->val = val;
          }

          break;
        }
      }
    default:
      return false;
    }
    // node->val = val;
  }
  return true;
}

/// @brief
/// MulExpK包含了/%*这三个运算，子节点为MulExpK、AddExpK或者为运算对象,是双目运算符
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_MulExpK(ast_node *node) {
  ast_node *src1_node = node->sons[0];
  ast_node *src2_node = node->sons[1];
  int flag_f = 0;
  int result_int;
  float result_f;

  int is_Loc = 0;
  Value *left_np = nullptr;
  Value *right_np = nullptr;
  int L_flag = 0;
  int R_flag = 0;
  if (symtab->currentFunc != nullptr) {
    is_Loc = 1;
  } else if (symtab->currentFunc == nullptr) {
    is_Loc = 0;
  }
  // 加法节点，左结合，先计算左节点，后计算右节点

  // 加法的左边操作数
  ast_node *left = ir_visit_ast_node(src1_node);
  if (!left) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(left->blockInsts);

  if (left->np_val != nullptr && left->np_val->is_issavenp()) {
    if (is_Loc == 1)
      left_np = symtab->currentFunc->newTempValue(left->np_val->type.type);
    else
      left_np = symtab->newTempValue(left->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(left_np, left->np_val, 2));
    L_flag = 1;
  }
  // 加法的右边操作数
  ast_node *right = ir_visit_ast_node(src2_node);
  if (!right) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(right->blockInsts);

  if (right->np_val != nullptr && right->np_val->is_issavenp()) {
    if (is_Loc == 1)
      right_np = symtab->currentFunc->newTempValue(right->np_val->type.type);
    else
      right_np = symtab->newTempValue(right->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(right_np, right->np_val, 2));
    R_flag = 1;
  }
  if (left->val->type.type == BasicType::TYPE_FLOAT ||
      right->val->type.type == BasicType::TYPE_FLOAT) {
    flag_f = 1;
  }
  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add
  switch (node->node_type) {
  case ast_operator_type::AST_OP_MULT: // 乘号*
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->realVal * right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->intVal * right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_f = left->val->realVal * right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal * right->val->intVal;
    }
    break;
  case ast_operator_type::AST_OP_MOD: // %
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      // result_f = left->val->realVal % right->val->realVal;
      // TODO报错
      return false;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      // result_f = left->val->intVal % right->val->realVal;
      // TODO报错
      return false;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      // result_f = left->val->realVal % right->val->intVal;
      // TODO报错
      return false;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      // result_int = left->val->intVal % right->val->intVal;
    }
    break;
  case ast_operator_type::AST_OP_DIV: // 除号/
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->realVal / right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->intVal / right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_f = left->val->realVal / right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      // if (0 == right->val->intVal) {
      //     // TODO报错
      //     printf("integer divide by zero");
      //     return false;
      // }
      if (right->val->intVal == 0)
        right->val->intVal = 1;
      result_int = left->val->intVal / right->val->intVal;
    }
    break;
  default:
    return false;
  }
  Value *resultValue = nullptr;
  if (flag_f) {
    if (is_Loc == 1)
      resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_FLOAT);
    else if (is_Loc == 0)
      resultValue = symtab->newTempValue(BasicType::TYPE_FLOAT);
    resultValue->realVal = result_f;
  } else {
    if (is_Loc == 1)
      resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
    else if (is_Loc == 0)
      resultValue = symtab->newTempValue(BasicType::TYPE_INT);
    resultValue->intVal = result_int;
  }

  // 创建临时变量保存IR的值，以及线性IR指令
  // node->blockInsts.addInst(left->blockInsts);
  // node->blockInsts.addInst(right->blockInsts);
  switch (node->node_type) {
  case ast_operator_type::AST_OP_MULT:
    if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MULT_I, resultValue, left_np, right_np));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MULT_I, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MULT_I, resultValue, left_np, right->val));
    } else if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(
          new BinaryIRInst(IRInstOperator::IRINST_OP_MULT_I, resultValue,
                           left->val, right->val));
    }
    break;
    // node->blockInsts.addInst(new
    // BinaryIRInst(IRInstOperator::IRINST_OP_MULT_I, resultValue, left->val,
    // right->val)); break;
  case ast_operator_type::AST_OP_MOD:
    if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MOD_I, resultValue, left_np, right_np));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MOD_I, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MOD_I, resultValue, left_np, right->val));
    } else if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_MOD_I, resultValue, left->val, right->val));
    }
    break;
    // node->blockInsts.addInst(new
    // BinaryIRInst(IRInstOperator::IRINST_OP_MOD_I, resultValue, left->val,
    // right->val)); break;
  case ast_operator_type::AST_OP_DIV:
    if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_DIV_I, resultValue, left_np, right_np));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_DIV_I, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_DIV_I, resultValue, left_np, right->val));
    } else if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_DIV_I, resultValue, left->val, right->val));
    }
    break;
    // node->blockInsts.addInst(new
    // BinaryIRInst(IRInstOperator::IRINST_OP_DIV_I, resultValue, left->val,
    // right->val)); break;
  default:
    return false;
  }
  node->val = resultValue;
  return true;
}

/// @brief
/// AddExpK是双目运算，包含了+-这两个运算，子节点为MulExpK、AddExpK或者为运算对象
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_AddExpK(ast_node *node) {
  ast_node *src1_node = node->sons[0];
  ast_node *src2_node = node->sons[1];
  int flag_f = 0;
  float result_f;
  int result_int;

  int is_Loc = 0;
  Value *left_np = nullptr;
  Value *right_np = nullptr;
  int L_flag = 0;
  int R_flag = 0;
  if (symtab->currentFunc != nullptr) {
    is_Loc = 1;
  } else if (symtab->currentFunc == nullptr) {
    is_Loc = 0;
  }
  // 加法节点，左结合，先计算左节点，后计算右节点

  // 加法的左边操作数
  ast_node *left = ir_visit_ast_node(src1_node);
  if (!left) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(left->blockInsts);

  if (left->np_val != nullptr && left->np_val->is_issavenp()) {
    if (is_Loc == 1)
      left_np = symtab->currentFunc->newTempValue(left->np_val->type.type);
    else
      left_np = symtab->newTempValue(left->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(left_np, left->np_val, 2));
    L_flag = 1;
  }
  // 加法的右边操作数
  ast_node *right = ir_visit_ast_node(src2_node);
  if (!right) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(right->blockInsts);

  if (right->np_val != nullptr && right->np_val->is_issavenp()) {
    if (is_Loc == 1)
      right_np = symtab->currentFunc->newTempValue(right->np_val->type.type);
    else
      right_np = symtab->newTempValue(right->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(right_np, right->np_val, 2));
    R_flag = 1;
  }
  if (left->val->type.type == BasicType::TYPE_FLOAT ||
      right->val->type.type == BasicType::TYPE_FLOAT) {
    flag_f = 1;
  }
  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add
  switch (node->node_type) {
  case ast_operator_type::AST_OP_ADD: // 乘号+
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->realVal + right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->intVal + right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_f = left->val->realVal + right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal + right->val->intVal;
    }
    break;
  case ast_operator_type::AST_OP_SUB: // 除号-
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->realVal - right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_f = left->val->intVal - right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_f = left->val->realVal - right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal - right->val->intVal;
    }
    break;
  default:
    return false;
  }
  Value *resultValue = nullptr;
  if (flag_f) {
    if (is_Loc == 1)
      resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_FLOAT);
    else if (is_Loc == 0)
      resultValue = symtab->newTempValue(BasicType::TYPE_FLOAT);
    resultValue->realVal = result_f;
  } else {
    if (is_Loc == 1)
      resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
    else if (is_Loc == 0)
      resultValue = symtab->newTempValue(BasicType::TYPE_INT);
    resultValue->intVal = result_int;
  }

  // 创建临时变量保存IR的值，以及线性IR指令

  switch (node->node_type) {
  case ast_operator_type::AST_OP_ADD:
    if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_ADD_I, resultValue, left_np, right_np));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_ADD_I, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_ADD_I, resultValue, left_np, right->val));
    } else if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_ADD_I, resultValue, left->val, right->val));
    }
    break;
  case ast_operator_type::AST_OP_SUB:
    if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_SUB_I, resultValue, left_np, right_np));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_SUB_I, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_SUB_I, resultValue, left_np, right->val));
    } else if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_SUB_I, resultValue, left->val, right->val));
    }
    break;
  default:
    return false;
  }
  node->val = resultValue;
  return true;
}

/// @brief RelExpK是双目运算，包含了< <= > >=这四个运算,子节点为比较对象
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_RelExpK(ast_node *node) {
  ast_node *src1_node = node->sons[0];
  ast_node *src2_node = node->sons[1];
  // int flag_f = 0;
  int result_int = 0;
  // float result_f;
  // 加法节点，左结合，先计算左节点，后计算右节点

  int is_Loc = 0;
  Value *left_np = nullptr;
  Value *right_np = nullptr;
  int L_flag = 0;
  int R_flag = 0;
  if (symtab->currentFunc != nullptr) {
    is_Loc = 1;
  } else if (symtab->currentFunc == nullptr) {
    is_Loc = 0;
  }

  ast_node *left = ir_visit_ast_node(src1_node);
  if (!left) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(left->blockInsts);

  if (left->np_val != nullptr && left->np_val->is_issavenp()) {
    if (is_Loc == 1)
      left_np = symtab->currentFunc->newTempValue(left->np_val->type.type);
    else
      left_np = symtab->newTempValue(left->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(left_np, left->np_val, 2));
    L_flag = 1;
  }
  // 加法的右边操作数
  ast_node *right = ir_visit_ast_node(src2_node);
  if (!right) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(right->blockInsts);

  if (right->np_val != nullptr && right->np_val->is_issavenp()) {
    if (is_Loc == 1)
      right_np = symtab->currentFunc->newTempValue(right->np_val->type.type);
    else
      right_np = symtab->newTempValue(right->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(right_np, right->np_val, 2));
    R_flag = 1;
  }

  // if(left->val->type.type == BasicType::TYPE_FLOAT || right->val->type.type
  // == BasicType::TYPE_FLOAT)
  // {
  // 	flag_f = 1;
  // }
  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add
  switch (node->node_type) {
  case ast_operator_type::AST_OP_LT: // 乘号<
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->realVal < right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->intVal < right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->realVal < right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal < right->val->intVal;
    }
    break;
  case ast_operator_type::AST_OP_BT: // 除号>
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->realVal > right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->intVal > right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->realVal > right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal > right->val->intVal;
    }
    break;
  case ast_operator_type::AST_OP_LE: // 除号<=
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->realVal <= right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->intVal <= right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->realVal <= right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal <= right->val->intVal;
    }
    break;
  case ast_operator_type::AST_OP_BE: // 除号>=
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->realVal >= right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = left->val->intVal >= right->val->realVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->realVal >= right->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = left->val->intVal >= right->val->intVal;
    }
    break;
  default:
    return false;
  }
  Value *resultValue = nullptr;
  if (is_Loc == 1) {
    resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);
  } else if (is_Loc == 0) {
    resultValue = symtab->newTempValue(BasicType::TYPE_BOOL);
  }
  resultValue->intVal = result_int;

  // 创建临时变量保存IR的值，以及线性IR指令
  // node->blockInsts.addInst(left->blockInsts);
  // node->blockInsts.addInst(right->blockInsts);
  switch (node->node_type) {
  case ast_operator_type::AST_OP_LT:
    if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LT, resultValue, left->val, right->val));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LT, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LT, resultValue, left_np, right->val));
    } else if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LT, resultValue, left_np, right_np));
    }
    break;
  case ast_operator_type::AST_OP_BT:
    if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BT, resultValue, left->val, right->val));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BT, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BT, resultValue, left_np, right->val));
    } else if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BT, resultValue, left_np, right_np));
    }
    break;
  case ast_operator_type::AST_OP_LE:
    if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LE, resultValue, left->val, right->val));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LE, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LE, resultValue, left_np, right->val));
    } else if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_LE, resultValue, left_np, right_np));
    }
    break;
  case ast_operator_type::AST_OP_BE:
    if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BE, resultValue, left->val, right->val));
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BE, resultValue, left->val, right_np));
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BE, resultValue, left_np, right->val));
    } else if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_BE, resultValue, left_np, right_np));
    }
    break;
  default:
    return false;
  }
  node->val = resultValue;
  return true;
}

/// @brief EqExpK是双目运算，包含了== !=这两个运算,子节点为比较对象
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_EqExpK(ast_node *node) {
  ast_node *src1_node = node->sons[0];
  ast_node *src2_node = node->sons[1];
  // int flag_f = 0;
  int result_int = 0;
  // float result_f;
  // 加法节点，左结合，先计算左节点，后计算右节点

  // 加法的左边操作数

  int is_Loc = 0;
  Value *left_np = nullptr;
  Value *right_np = nullptr;
  int L_flag = 0;
  int R_flag = 0;
  if (symtab->currentFunc != nullptr) {
    is_Loc = 1;
  } else if (symtab->currentFunc == nullptr) {
    is_Loc = 0;
  }

  ast_node *left = ir_visit_ast_node(src1_node);
  if (!left) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(left->blockInsts);

  if (left->np_val != nullptr && left->np_val->is_issavenp()) {
    if (is_Loc == 1)
      left_np = symtab->currentFunc->newTempValue(left->np_val->type.type);
    else
      left_np = symtab->newTempValue(left->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(left_np, left->np_val, 2));
    L_flag = 1;
  }
  // 加法的右边操作数
  ast_node *right = ir_visit_ast_node(src2_node);
  if (!right) {
    // 某个变量没有定值
    return false;
  }

  node->blockInsts.addInst(right->blockInsts);

  if (right->np_val != nullptr && right->np_val->is_issavenp()) {
    if (is_Loc == 1)
      right_np = symtab->currentFunc->newTempValue(right->np_val->type.type);
    else
      right_np = symtab->newTempValue(right->np_val->type.type);
    node->blockInsts.addInst(new AssignIRInst(right_np, right->np_val, 2));
    R_flag = 1;
  }
  // if(left->val->type.type == BasicType::TYPE_FLOAT || right->val->type.type
  // == BasicType::TYPE_FLOAT)
  // {
  // 	flag_f = 1;
  // }
  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add
  switch (node->node_type) {
  case ast_operator_type::AST_OP_EQ: // 乘号==
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->realVal == right->val->realVal);
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->intVal == right->val->realVal);
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->realVal == right->val->intVal);
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->intVal == right->val->intVal);
    }
    break;
  case ast_operator_type::AST_OP_NQ: // 除号!=
    if (left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->realVal != right->val->realVal);
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->intVal != right->val->realVal);
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->realVal != right->val->intVal);
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->intVal != right->val->intVal);
    }
    break;
  default:
    return false;
  }
  Value *resultValue = nullptr;
  if (is_Loc == 1) {
    resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);
  } else if (is_Loc == 0) {
    resultValue = symtab->newTempValue(BasicType::TYPE_BOOL);
  }
  resultValue->intVal = result_int;

  // 创建临时变量保存IR的值，以及线性IR指令
  // node->blockInsts.addInst(left->blockInsts);
  // node->blockInsts.addInst(right->blockInsts);
  switch (node->node_type) {
  case ast_operator_type::AST_OP_EQ:
    if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_EQ, resultValue, left->val, right->val));
      break;
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_EQ, resultValue, left->val, right_np));
      break;
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_EQ, resultValue, left_np, right->val));
      break;
    } else if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_EQ, resultValue, left_np, right_np));
      break;
    }
  case ast_operator_type::AST_OP_NQ:
    if (L_flag == 0 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_NQ, resultValue, left->val, right->val));
      break;
    } else if (L_flag == 0 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_NQ, resultValue, left->val, right_np));
      break;
    } else if (L_flag == 1 && R_flag == 0) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_NQ, resultValue, left_np, right->val));
      break;
    } else if (L_flag == 1 && R_flag == 1) {
      node->blockInsts.addInst(new BinaryIRInst(
          IRInstOperator::IRINST_OP_NQ, resultValue, left_np, right_np));
      break;
    }
  default:
    return false;
  }
  node->val = resultValue;
  return true;
}

/// @brief LAndExpK是双目运算&&是and操作，子节点的and的对象
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_LAndExpK(ast_node *node) {
  ast_node *src1_node = node->sons[0];
  ast_node *src2_node = node->sons[1];
  ast_node *parent = node->parent;
  node->type.type = BasicType::TYPE_BOOL;
  BcIRInst *BcT_F_1 = nullptr;
  BcIRInst *BcT_F_2 = nullptr;
  int not_use = 0;
  // int flag_f = 0;
  int result_int = 0;
  // float result_f;
  // 加法节点，左结合，先计算左节点，后计算右节点
  int L_and = 0;

  IRInst *Entry = new LabelIRInst(); //第二个条件入口
  Mode_2_Entry = Entry;

  if (parent->OriginalType == LOrExpK || parent->ast_node_Type == AT_IfStmt ||
      parent->OriginalType == LAndExpK) {
    not_use = 1;
  }
  // 加法的左边操作数
  IRInst *tmptrue = TrueEntry;
  if (src1_node->OriginalType == LAndExpK) {
    TrueEntry = Entry;
    L_and = 1;
  }
  ast_node *left = ir_visit_ast_node(src1_node);
  if (!left) {
    // 某个变量没有定值
    return false;
  }
  if (src1_node->OriginalType == LAndExpK) {
    TrueEntry = tmptrue;
  }
  if (L_and == 0)
    BcT_F_1 = new BcIRInst(5, left->val);
  // 加法的右边操作数
  ast_node *right = ir_visit_ast_node(src2_node);
  if (!right) {
    // 某个变量没有定值
    return false;
  }
  BcT_F_2 = new BcIRInst(3, right->val);
  // if(left->val->type.type == BasicType::TYPE_FLOAT || right->val->type.type
  // == BasicType::TYPE_FLOAT)
  // {
  // 	flag_f = 1;
  // }
  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add
  switch (node->node_type) {
  case ast_operator_type::AST_OP_AND: // 乘号==
    if (L_and == 0 && left->val->type.type == BasicType::TYPE_FLOAT &&
        right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->realVal && right->val->realVal);
    } else if (L_and == 0 && left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->intVal && right->val->realVal);
    } else if (L_and == 0 && left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->realVal && right->val->intVal);
    } else if (L_and == 0 && left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->intVal && right->val->intVal);
    }
    break;
  default:
    return false;
  }
  Value *resultValue = nullptr;
  if (not_use == 0) {
    resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);
    resultValue->intVal = result_int;
  }

  // 创建临时变量保存IR的值，以及线性IR指令
  node->blockInsts.addInst(left->blockInsts);
  if (L_and == 0)
    node->blockInsts.addInst(BcT_F_1);
  node->blockInsts.addInst(Entry);
  node->blockInsts.addInst(right->blockInsts);
  node->blockInsts.addInst(BcT_F_2);
  // switch (node->node_type) {
  //     case ast_operator_type::AST_OP_AND:
  //         node->blockInsts.addInst(
  //             new BinaryIRInst(IRInstOperator::IRINST_OP_AND, resultValue,
  //             left->val, right->val));
  //         break;
  //     default:
  //         return false;
  // }
  if (not_use == 0)
    node->val = resultValue;
  return true;
}

/// @brief LOrExpK是双目运算||是or操作，子节点的or的对象
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_LOrExpK(ast_node *node) {
  ast_node *src1_node = node->sons[0];
  ast_node *src2_node = node->sons[1];
  ast_node *parent = node->parent;
  // int flag_f = 0;
  int not_use = 0;
  int result_int = 0;
  node->type.type = BasicType::TYPE_BOOL;
  if (parent->OriginalType == LOrExpK || parent->ast_node_Type == AT_IfStmt) {
    not_use = 1;
  }
  IRInst *Entry = new LabelIRInst(); //第二个条件入口
  Mode_1_Entry = Entry;

  int L_OR = 0;
  int R_OR = 0;

  BcIRInst *BcT_F_1 = nullptr;
  BcIRInst *BcT_F_2 = nullptr;
  // 加法的左边操作数
  IRInst *tmpfalse = FalseEntry;
  if (src1_node->OriginalType == LOrExpK ||
      src1_node->OriginalType == LAndExpK) {
    FalseEntry = Entry;
  }

  ast_node *left = ir_visit_ast_node(src1_node);
  if (!left) {
    // 某个变量没有定值
    return false;
  }
  if (src1_node->OriginalType == LOrExpK ||
      src1_node->OriginalType == LAndExpK) {
    FalseEntry = tmpfalse;
  }
  if (left->OriginalType != LOrExpK && left->OriginalType != LAndExpK) {
    BcT_F_1 = new BcIRInst(4, left->val);
  } else {
    L_OR = 1;
  }

  // TODO
  //  加法的右边操作数
  ast_node *right = ir_visit_ast_node(src2_node);
  if (!right) {
    // 某个变量没有定值
    return false;
  }
  if (right->OriginalType != LOrExpK && right->OriginalType != LAndExpK) {
    BcT_F_2 = new BcIRInst(3, right->val);
  } else {
    R_OR = 1;
  }
  // if(left->val->type.type == BasicType::TYPE_FLOAT || right->val->type.type
  // == BasicType::TYPE_FLOAT)
  // {
  // 	flag_f = 1;
  // }
  // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
  // TODO real number add
  switch (node->node_type) {
  case ast_operator_type::AST_OP_OR: // ||
    if (left->type.type == BasicType::TYPE_BOOL &&
        right->type.type == BasicType::TYPE_BOOL) {
      result_int = left->integer_val || right->integer_val;
    } else if (left->type.type == BasicType::TYPE_BOOL &&
               right->val != nullptr) {
      result_int =
          left->integer_val || right->val->realVal || right->val->intVal;
    } else if (right->type.type == BasicType::TYPE_BOOL &&
               left->val != nullptr) {
      result_int =
          right->integer_val || left->val->realVal || left->val->intVal;
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->realVal || right->val->realVal);
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_FLOAT) {
      result_int = (left->val->intVal || right->val->realVal);
    } else if (left->val->type.type == BasicType::TYPE_FLOAT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->realVal || right->val->intVal);
    } else if (left->val->type.type == BasicType::TYPE_INT &&
               right->val->type.type == BasicType::TYPE_INT) {
      result_int = (left->val->intVal || right->val->intVal);
    }
    break;
  default:
    return false;
  }
  Value *resultValue = nullptr;
  if (not_use == 0) {
    resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    resultValue->intVal = result_int;
  }

  // 创建临时变量保存IR的值，以及线性IR指令
  node->blockInsts.addInst(left->blockInsts);
  if (L_OR == 0 && BcT_F_1 != nullptr) {
    node->blockInsts.addInst(BcT_F_1);
  }
  node->blockInsts.addInst(Entry);
  node->blockInsts.addInst(right->blockInsts);
  if (R_OR == 0 && BcT_F_2 != nullptr) {
    node->blockInsts.addInst(BcT_F_2);
  }
  // switch (node->node_type) {
  //     case ast_operator_type::AST_OP_OR:
  //         node->blockInsts.addInst(
  //             new BinaryIRInst(IRInstOperator::IRINST_OP_OR, resultValue,
  //             left->val, right->val));
  //         break;
  //     default:
  //         return false;
  // }
  node->val = resultValue;
  node->integer_val = result_int;
  return true;
}
