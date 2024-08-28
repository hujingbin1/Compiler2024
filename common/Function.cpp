/**
 * @file function.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 函数形参与函数管理
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "Function.h"
#include "IRInst.h"
#include "SymbolTable.h"
#include "IRGenerator.h"
#include "ValueType.h"

/// @brief 默认整型参数
/// @param _name 形参的名字
FuncFormalParam::FuncFormalParam(std::string _name) : name(_name)
{
    type.type = BasicType::TYPE_INT;
}

/// @brief 基本类型的参数
/// @param _name 形参的名字
/// @param _type 基本类型
/// @param _val Value
FuncFormalParam::FuncFormalParam(std::string _name, BasicType _type, Value * _val) : name(_name), type(_type), val(_val)
{}

/// @brief 函数形参转字符串输出
/// @return
std::string FuncFormalParam::toString()
{
    std::string typeName;
    typeName = type.toString();

    // 类型名 空格 形参参数名
    return typeName + " " + name;
}

/// @brief 匿名构造函数
Function::Function()
{
    returnType.type = BasicType::TYPE_VOID;
}

/// @brief 指定有函数类型与名字的构造函数
/// @param _name
/// @param _type
/// @param _builtin
Function::Function(std::string _name, BasicType _type, bool _builtin) : name(_name), builtIn(_builtin)
{
    returnType.type = _type;
}

/// @brief 指定函数名字、函数返回类型以及函数形式参数的构造函数
/// @param _name
/// @param _type
/// @param _param
Function::Function(std::string _name, BasicType _type, FuncFormalParam _param, bool _builtin)
    : name(_name), builtIn(_builtin)
{
    returnType.type = _type;
    params.emplace_back(_param);
}

/// @brief 取得函数名字
/// @return 函数名字
std::string & Function::getName()
{
    return name;
}

// /// @brief 重写equals方法
// /// @return 是否相等
// bool & Function::equals(std::string const & name)
// {}

/// @brief 获取函数返回类型
/// @return 返回类型
ValueType & Function::getReturnType()
{
    return returnType;
}

/// @brief 获取函数的形参列表
/// @return 形参列表
std::vector<FuncFormalParam> & Function::getParams()
{
    return params;
}

/// @brief 获取函数内的IR指令代码
/// @return IR指令代码
InterCode & Function::getInterCode()
{
    return code;
}

/// @brief 判断该函数是否是内置函数
/// @return true: 内置函数，false：用户自定义
bool Function::isBuiltin()
{
    return builtIn;
}

/// @brief 设置函数是否是内置或用户自定义
/// @param _builtin true: 内置函数 false: 用户自定义函数
void Function::setBuiltin(bool _builtin)
{
    builtIn = _builtin;
}

/// @brief 函数指令信息输出
/// @param str 函数指令
void Function::toString(std::string & str, SymbolTable & symtab)
{
    if (builtIn) {
        // 内置函数则什么都不输出
        return;
    }

    if (getName() == "putint") {
        return;
    }

    // 得到全局变量和常量信息
    // TODO搬到outputIR那里

    // TODO搬到outputIR那里

    // 输出函数头
    // str += returnType.toString() + " " + name + "(";
    str += "define " + returnType.toString() + " @" + name + "(";
    int add = 0;
    int _add = 1;
    // if(returnType.type != BasicType::TYPE_VOID)
    // {
    // 	_add = 1;
    // }
    if (returnType.type == BasicType::TYPE_VOID) {
        _add = 0;
    }

    // int is_add = 0;
    bool firstParam = false;
    for (auto & param: params) {

        if (!firstParam) {
            firstParam = true;
        } else {
            str += ", ";
        }

        std::string param_str = param.save_val->type.toString() + " %t" + std::to_string(_add);
        param.save_val->name = "%t" + std::to_string(_add);
        if (param.val->np != nullptr && param.val->np->np_sizes.size() != 0) {
            for (int i = param.val->np->np_sizes.size() - 1; i >= 0; i--) {
                param_str += "[" + std::to_string(param.val->np->np_sizes[i]) + "]";
            }
            param.save_val->is_numpy = 1;
        }

        add = add + 1;
        _add = _add + 1;
        // is_add = 1;

        str += param_str;
    }

    str += ") ";

    str += "{\n";
    //得到局部变量和临时变量信息
    std::vector<Value *> localValueVector = varsVector;

    // 更新map
    std::unordered_map<std::string, Value *> * localVarsMap = &varsMap;

    //得到要输出的IR序列
    std::vector<IRInst> IRInsts;
    for (auto & inst: symtab.currentFunc->getInterCode().getInsts()) {
        switch (inst->getOp()) {
            case (IRInstOperator::IRINST_OP_BR):
                break;
            case (IRInstOperator::IRINST_OP_BC):
                break;
            default:
                break;
        }
        IRInsts.push_back(*inst);
    }

    //  遍历所有的线性IR指令，文本输出
    // 遍历临时变量
    // int _index = 0;
    for (int index = 0; index < (int) localValueVector.size(); ++index) {
        Value * value = localValueVector[index];
        // Value * mapValue = localVarsMap[value->getName()];
        // _index = index + add;
        if (value == nullptr) {
            continue;
        }
        if (index == 0) {
            if (returnType.type != BasicType::TYPE_VOID) {
                str += "\tdeclare " + returnType.toString() + " " + "%l0" + "\n";
                // localValueVector.front()->toString() + "\n";
                value->name = "%l0";
                // //修改命名
                // localVarsMap->erase(nameMap);
                // localVarsMap->emplace(value->name, value);
                continue;
            }
        }
        if (value->is_FParam())
            continue;
        //重新编号临时变量
        if (value->isTemp()) {
            // TODO:这样直接改命名可以吗？
            if (value->is_issavenp() == 1) {
                value->name = "%t" + std::to_string(index);
                if (value->np != nullptr && value->is_numpy == 1) {
                    switch (value->type.type) {
                        case BasicType::TYPE_INT:
                            // str += "\tdeclare i32 %t" + std::to_string(index) + "\n";
                            str += "\tdeclare i32 " + value->toString();
                            break;
                        case BasicType::TYPE_FNP_I:
                            // str += "\tdeclare i32 %t" + std::to_string(index) + "\n";
                            str += "\tdeclare i32 " + value->toString();
                            break;
                        case BasicType::TYPE_BOOL:
                            str += "\tdeclare i1 " + value->toString();
                            break;
                        case BasicType::TYPE_FLOAT:
                            // str += "\tdeclare f32 %t" + std::to_string(index) + "\n";
                            str += "\tdeclare float " + value->toString();
                            break;
                        case BasicType::TYPE_FNP_F:
                            // str += "\tdeclare i32 %t" + std::to_string(index) + "\n";
                            str += "\tdeclare float " + value->toString();
                            break;
                        default:
                            break;
                    }
                    for (int i = 0; i <= (int) value->np->np_sizes.size() - 1; i++) {
                        str += "[" + std::to_string(value->np->np_sizes[i]) + "]";
                    }
                    str += "\n";
                } else {
                    if (value->isConst() == 1) {
                        str += "\tdeclare const";
                    } else if (value->isConst() == 0) {
                        str += "\tdeclare";
                    }
                    value->is_numpy = 1;
                    switch (value->type.type) {
                        case BasicType::TYPE_INT:
                            // str += "\tdeclare i32 %t" + std::to_string(index) + "\n";
                            str += "i32* " + value->toString() + "\n";
                            break;
                        case BasicType::TYPE_BOOL:
                            str += "i1* " + value->toString() + "\n";
                            break;
                        case BasicType::TYPE_FLOAT:
                            // str += "\tdeclare f32 %t" + std::to_string(index) + "\n";
                            str += "float* " + value->toString() + "\n";
                            break;
                        default:
                            break;
                    }
                }

            } else if (value->is_issavenp() == 0) {
                value->name = "%t" + std::to_string(index);
                switch (value->type.type) {
                    case BasicType::TYPE_INT:
                        // str += "\tdeclare i32 %t" + std::to_string(index) + "\n";
                        str += "\tdeclare i32 " + value->toString() + "\n";
                        break;
                    case BasicType::TYPE_BOOL:
                        str += "\tdeclare i1 " + value->toString() + "\n";
                        break;
                    case BasicType::TYPE_FLOAT:
                        // str += "\tdeclare f32 %t" + std::to_string(index) + "\n";
                        str += "\tdeclare float " + value->toString() + "\n";
                        break;
                    default:
                        break;
                }
            }
        }
        // 重新命名局部变量

        if (value->isConst()) {
            str += "\tdeclare const ";
            if (value->is_saveFParam == 1) {
                int _index = index + add;
                switch (value->type.type) {
                    case BasicType::TYPE_INT:
                        str += "i32 %l" + std::to_string(_index);
                        break;
                    case BasicType::TYPE_FLOAT:
                        str += "float %l" + std::to_string(_index);
                        break;
                    case BasicType::TYPE_FNP_I:
                        str += "i32 %l" + std::to_string(_index);
                        break;
                    case BasicType::TYPE_FNP_F:
                        str += "float %l" + std::to_string(_index);
                        break;
                    default:
                        break;
                }
                if (value->np != nullptr && value->np->np_sizes.size() != 0) {
                    for (int i = value->np->np_sizes.size() - 1; i >= 0; i--) {
                        str += "[" + std::to_string(value->np->np_sizes[i]) + "]";
                    }
                }
                str += " ; variable: " + value->toString() + "\n";
                value->name = "%l" + std::to_string(_index);
            } else {
                switch (value->type.type) {
                    case BasicType::TYPE_INT:
                        str += "i32 %l" + std::to_string(index);
                        break;
                    case BasicType::TYPE_FLOAT:
                        str += "float %l" + std::to_string(index);
                        break;
                    default:
                        break;
                }
                if (value->np != nullptr && value->np->np_sizes.size() != 0) {
                    for (int i = value->np->np_sizes.size() - 1; i >= 0; i--) {
                        str += "[" + std::to_string(value->np->np_sizes[i]) + "]";
                    }
                }
                str += " ; variable: " + value->toString() + "\n";
                value->name = "%l" + std::to_string(index);
            }
            // TODO:这样直接改命名可以吗？
        }

        if (value->isLocalVar()) {
            if (value->isConst() == 1) {
                str += "\tdeclare const";
            } else if (value->isConst() == 0) {
                str += "\tdeclare ";
            }
            if (value->is_saveFParam == 1) {
                int _index = index + add;
                switch (value->type.type) {
                    case BasicType::TYPE_INT:
                        str += "i32 %l" + std::to_string(_index);
                        break;
                    case BasicType::TYPE_FLOAT:
                        str += "float %l" + std::to_string(_index);
                        break;
                    case BasicType::TYPE_FNP_I:
                        str += "i32 %l" + std::to_string(_index);
                        break;
                    case BasicType::TYPE_FNP_F:
                        str += "float %l" + std::to_string(_index);
                        break;
                    default:
                        break;
                }
                if (value->np != nullptr && value->np->np_sizes.size() != 0) {
                    for (int i = value->np->np_sizes.size() - 1; i >= 0; i--) {
                        str += "[" + std::to_string(value->np->np_sizes[i]) + "]";
                    }
                }
                str += " ; variable: " + value->toString() + "\n";
                value->name = "%l" + std::to_string(_index);
            } else {
                switch (value->type.type) {
                    case BasicType::TYPE_INT:
                        str += "i32 %l" + std::to_string(index);
                        break;
                    case BasicType::TYPE_FLOAT:
                        str += "float %l" + std::to_string(index);
                        break;
                    default:
                        break;
                }
                if (value->np != nullptr && value->np->np_sizes.size() != 0) {
                    for (int i = value->np->np_sizes.size() - 1; i >= 0; i--) {
                        str += "[" + std::to_string(value->np->np_sizes[i]) + "]";
                    }
                }
                str += " ; variable: " + value->toString() + "\n";
                value->name = "%l" + std::to_string(index);
            }
            // TODO:这样直接改命名可以吗？
        }
    }
    localVarsMap->clear();
    for (int index = 0; index < (int) localValueVector.size(); ++index) {
        Value * value = localValueVector[index];
        localVarsMap->emplace(value->getName(), value);
    }
    for (auto & inst: code.getInsts()) {

        std::string instStr;
        inst->toString(instStr);

        if (!instStr.empty()) {

            // Label指令不加Tab键
            if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
                str += instStr + "\n";
                instStr = instStr + "\n";
            } else {

                str += "\t" + instStr + "\n";
                instStr = "\t" + instStr + "\n";
            }
        }
        // std::cout << instStr;
    }

    // 输出函数尾部
    str += "}\n";
    // std::cout << "}\n";

    // for (auto & func: symtab.getFunctionList()) {
    //     for (auto & params: func->getParams()) {
    //         if (params.save_val != nullptr) {
    //             params.val = params.save_val;
    //         }
    //     }
    // }
}

/// @brief 获取下一个Label名字
/// @return 下一个Label名字
std::string Function::getNextLabelName()
{
    std::string name = std::to_string(nextLabelNo);
    nextLabelNo++;
    return name;
}

/// @brief 获取下一个临时变量名字
/// @return 下一个临时变量名字
std::string Function::getNextTempVarName()
{
    std::string name = std::to_string(nextTempVarNo);
    nextTempVarNo++;
    return name;
}

/// @brief 设置函数出口指令
/// @param inst 出口Label指令
void Function::setExitLabel(IRInst * inst)
{
    exitLabel = inst;
}

/// @brief 获取函数出口指令
/// @return 出口Label指令
IRInst * Function::getExitLabel()
{
    return exitLabel;
}

/// @brief 设置函数返回值变量
/// @param val 返回值变量，要求必须是局部变量，不能是临时变量
void Function::setReturnValue(Value * val)
{
    returnValue = val;
}

/// @brief 获取函数返回值变量
/// @return 返回值变量
Value * Function::getReturnValue()
{
    return returnValue;
}

/// @brief 从函数内的局部变量中删除
/// @param val 变量Value
void Function::deleteVarValue(Value * val)
{
    auto pIter = varsMap.find(val->getName());
    if (pIter != varsMap.end()) {
        varsMap.erase(pIter);
    }

    auto pIter2 = std::find(varsVector.begin(), varsVector.end(), val);
    varsVector.erase(pIter2);
}

/// @brief 获取最大栈帧深度
/// @return 栈帧深度
int Function::getMaxDep()
{
    return maxDepth;
}

/// @brief 设置最大栈帧深度
/// @param dep 栈帧深度
void Function::setMaxDep(int dep)
{
    maxDepth = dep;

    // 设置函数栈帧被重定位标记，用于生成不同的栈帧保护代码
    relocated = true;
}

/// @brief 获取本函数需要保护的寄存器
/// @return 要保护的寄存器
std::vector<int32_t> & Function::getProtectedReg()
{
    return protectedRegs;
}

/// @brief 获取本函数需要保护的寄存器字符串
/// @return 要保护的寄存器
std::string & Function::getProtectedRegStr()
{
    return protectedRegStr;
}

/// @brief 获取函数调用参数个数的最大值
/// @return 函数调用参数个数的最大值
int Function::getMaxFuncCallArgCnt()
{
    return maxFuncCallArgCnt;
}

/// @brief 设置函数调用参数个数的最大值
/// @param count 函数调用参数个数的最大值
void Function::setMaxFuncCallArgCnt(int count)
{
    maxFuncCallArgCnt = count;
}

/// @brief 函数内是否存在函数调用
/// @return 是否存在函调用
bool Function::getExistFuncCall()
{
    return funcCallExist;
}

/// @brief 设置函数是否存在函数调用
/// @param exist true: 存在 false: 不存在
void Function::setExistFuncCall(bool exist)
{
    funcCallExist = exist;
}

/// @brief 新建变量型Value
/// @param name 变量ID
/// @param type 变量类型
Value * Function::newVarValue(std::string name, BasicType type)
{
    Value * retVal;

    retVal = nullptr;
    if (retVal == nullptr) {
        retVal = new VarValue(name, type);
        insertValue(retVal);
    } else {
        // 已存在的Value，返回失败
        retVal = nullptr;
    }

    return retVal;
}

/// @brief Value插入到符号表中
/// @param name Value的名称
/// @param val Value信息
void Function::insertValue(Value * val)
{
    varsMap.emplace(val->name, val);
    varsVector.push_back(val);
}

/// @brief 新建一个匿名变量型的Value，并加入到符号表，用于后续释放空间
/// \param type 类型
/// \return 变量Value
Value * Function::newVarValue(BasicType type)
{
    // 创建匿名变量，肯定唯一，直接插入
    Value * var = new VarValue(type);

    insertValue(var);

    return var;
}

/// 新建一个临时型的Value，并加入到符号表，用于后续释放空间
/// \param intVal 整数值
/// \return 常量Value
Value * Function::newTempValue(BasicType type)
{
    // 肯定唯一存在，直接插入即可
    Value * temp = new TempValue(type);

    insertValue(temp);

    return temp;
}

/// 根据变量名取得当前符号的值。若变量不存在，则说明变量之前没有定值，则创建一个未知类型的值，初值为0
/// \param name 变量名
/// \param create true: 不存在返回nullptr；false：不存在则不创建
/// \return 变量对应的值
Value * Function::findValue(std::string name, bool create)
{
    Value * temp = nullptr;

    // 这里只是针对函数内的变量进行检查，如果要考虑全局变量，则需要继续检查symtab的符号表
    auto pIter = varsMap.find(name);
    if (pIter != varsMap.end()) {

        // 如果考虑作用域、存在重名的时候，需要从varsVector逆序检查到底用那个Value

        temp = pIter->second;
    }

    // 没有找到，并且指定了全局符号表，则继续查找
    if ((!temp) && symtab) {

        temp = symtab->findValue(name, false);
    }

    // 变量名没有找到
    if ((!temp) && create) {
        temp = newVarValue(name);
    }

    return temp;
}

/// @brief 设置符号表，以便全局符号查找
void Function::setSymtab(SymbolTable * _symtab)
{
    symtab = _symtab;
}

void Function::insertBlock(Var_Block * block)
{
    blockMap.emplace(block->number, block);
    Blocks_vector.push_back(block);
}

Var_Block * Function::newBlock()
{
    Var_Block * temp = new Var_Block(Count_Block);

    Count_Block = Count_Block + 1;

    insertBlock(temp);

    return temp;
}

Var_Block::Var_Block(int num)
{
    number = num;
}

void Var_Block::insertValue(Value * val)
{
    vars_map.emplace(val->name, val);
    vars_vector.push_back(val);
}

Value * Var_Block::findValue(std::string name)
{
    Value * temp = nullptr;

    // 这里只是针对函数内的变量进行检查，如果要考虑全局变量，则需要继续检查symtab的符号表
    auto pIter = vars_map.find(name);
    if (pIter != vars_map.end()) {

        // 如果考虑作用域、存在重名的时候，需要从varsVector逆序检查到底用那个Value

        temp = pIter->second;
    }

    return temp;
}

// void set_useParams()
// {
// 	use_params = params;
// }