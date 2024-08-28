/**
 * @file IRInst.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief IR指令类
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>
#include <string>
#include <vector>

#include "IRInst.h"
#include "BasicBlocks.h"
#include "SymbolTable.h"
#include "Value.h"
#include "ValueType.h"
#include "Common.h"
extern IRInst * TrueEntry;
extern IRInst * FalseEntry;
extern IRInst * Mode_1_Entry;
extern IRInst * Mode_2_Entry;
extern IRInst * Mode_1_Entry;
extern IRInst * Mode_2_Entry;
extern IRInst * Break_Exit;
extern IRInst * Continue_Entry;
extern SymbolTable symtab;
/// @brief 构造函数
IRInst::IRInst()
{
    // 未知指令
    op = IRInstOperator::IRINST_OP_MAX;
}

/// @brief 构造函数
/// @param op
/// @param result
/// @param srcVal1
/// @param srcVal2
IRInst::IRInst(IRInstOperator _op, Value * _result) : op(_op), dstValue(_result)
{}

/// @brief 获取指令操作码
/// @return 指令操作码
IRInstOperator IRInst::getOp()
{
    return op;
}

/// @brief 获取源操作数列表
/// @return 源操作数列表
std::vector<Value *> & IRInst::getSrc()
{
    return srcValues;
}

/// @brief 获取目的操作数，或者结果操作数
/// @return 目的操作数，或者结果操作数
Value * IRInst::getDst()
{
    return dstValue;
}

/// @brief 取得源操作数1
/// @return
Value * IRInst::getSrc1()
{
    return srcValues[0];
}

/// @brief 取得源操作数1的寄存器号
/// @return 寄存器号，可能为-1，表示在内存或立即数
int IRInst::getSrc1RegId()
{
    return srcValues[0]->regId;
}

/// @brief 取得源操作数2
/// @return
Value * IRInst::getSrc2()
{
    return srcValues[1];
}

/// @brief 取得源操作数2的寄存器号
/// @return 寄存器号，可能为-1，表示在内存或立即数
int IRInst::getSrc2RegId()
{
    return srcValues[1]->regId;
}

/// @brief 转换成字符串
/// @param str 转换后的字符串
void IRInst::toString(std::string & str)
{
    // 未知指令
    str = "Unkown IR Instruction";
}

// optIRInst::optIRInst()
// {}

// optIRInst::~optIRInst()
// {}
/// @brief 构造函数
LabelIRInst::LabelIRInst() : IRInst(IRInstOperator::IRINST_OP_LABEL)
{
    // TODO 这里先设置为空字符串，实际上必须是唯一的Label名字
    // 处理方式：(1) 全局唯一 (2) 函数内唯一
    labelName = createLabelName();
}

/// @brief 构造函数
/// @param name Label名字，要确保函数内唯一
LabelIRInst::LabelIRInst(std::string name) : IRInst(IRInstOperator::IRINST_OP_LABEL)
{
    labelName = name;
}

/// @brief 析构函数
LabelIRInst::~LabelIRInst()
{}

/// @brief 转换成字符串
/// @param str 返回指令字符串
void LabelIRInst::toString(std::string & str)
{
    str = labelName + ":";
}

/// @brief 构造函数
/// @param _op 操作符
/// @param _result 结果操作数
/// @param _srcVal1 源操作数1
/// @param _srcVal2 源操作数2
BinaryIRInst::BinaryIRInst(IRInstOperator _op, Value * _result, Value * _srcVal1, Value * _srcVal2, int flag)
    : IRInst(_op, _result)
{
    mode = flag;
    srcValues.push_back(_srcVal1);
    srcValues.push_back(_srcVal2);
}

/// @brief 构造函数
/// @param _op 操作符
/// @param _result 结果操作数
/// @param _srcVal1 源操作数1
/// @param _srcVal2 源操作数2
BinaryIRInst::BinaryIRInst(IRInstOperator _op, Value * _result, Value * _srcVal1, int _srcVal2, int flag)
    : IRInst(_op, _result)
{
    mode = flag;
    Binary_mode = mode;
    srcValues.push_back(_srcVal1);
    // srcValues.push_back(_srcVal2);
    src = _srcVal2;
    Binary_src = src;
    //修改了，将数组初始化需要的立即数插入了
    Value * srcVal2 = new ConstValue(src);
    srcVal2->is_numpy = true;
    srcValues.push_back(srcVal2);
    if (_srcVal1->is_numpy && !_srcVal1->isliteral() && _srcVal1->isConst() &&
        symtab.findValue(_srcVal1->getName()) != nullptr) {
        //定位到全局数组
        this->isGlobal = true;
    }
}

/// @brief 析构函数
BinaryIRInst::~BinaryIRInst()
{}

/// @brief 转换成字符串
/// @param str 转换后的字符串
void BinaryIRInst::toString(std::string & str)
{

    Value *src1 = srcValues[0], *result = dstValue;
    std::string src_2;
    std::string src_1;
    std::string dst;
    // mode 0,1 正常情况；2，3 src2是传进来的是一个立即数数字如10，12；4 两边都是数组取数，5 src2是数组取数，6 src1
    // 是数组取数
    if (mode == 4) {
        Value * src2 = srcValues[1];
        src_1 = src_1 + "*";
        src_2 = src_2 + "*";
        src_2 = src_2 + src2->toString();
        src_1 = src_1 + src1->toString();
    }
    if (mode == 5) {
        Value * src2 = srcValues[1];
        // src_1 = src_1 + "*";
        src_2 = src_2 + "*";
        src_2 = src_2 + src2->toString();
        src_1 = src_1 + src1->toString();
    }
    if (mode == 6) {
        Value * src2 = srcValues[1];
        src_1 = src_1 + "*";
        // src_2 = src_2 + "*";
        src_2 = src_2 + src2->toString();
        src_1 = src_1 + src1->toString();
    }
    if (mode == 0 || mode == 1) {
        Value * src2 = srcValues[1];
        src_2 = src2->toString();
        src_1 = src1->toString();
    }
    // if(mode == 1)
    // {
    // 	Value*src2 = srcValues[1];
    // 	src_2 = src2->toString();
    // 	src_1 = src1->getName();
    // }
    if (mode == 2 || mode == 3) {
        src_1 = src1->toString();
        src_2 = std::to_string(src);
    }
    // if(mode == 3)
    // {
    // 	src_1 = src1->toString();
    // 	src_2 = std::to_string(src);
    // }

    dst = result->getName();
    // TODO:测试IR时请注释掉以下9行，这几行是用于SSA的IR转换测试使用
    // if (this->getSrc1() != nullptr && this->getSrc1()->isLocalVar()) {
    //     src_1 += "_" + int2str(this->getSrc1Tag());
    // }
    // if (this->getSrc2() != nullptr && this->getSrc2()->isLocalVar()) {
    //     src_2 += "_" + int2str(this->getSrc2Tag());
    // }
    // if (this->getDst() != nullptr && this->getDst()->isLocalVar()) {
    //     dst += "_" + int2str(this->getTag());
    // }
    switch (op) {
        case IRInstOperator::IRINST_OP_ADD_I:

            // 加法指令，二元运算
            str = dst + " = add " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_SUB_I:

            // 减法指令，二元运算
            str = dst + " = sub " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_MOD_I:

            // %指令，二元运算
            str = dst + " = mod " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_DIV_I:

            // /指令，二元运算
            str = dst + " = div " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_MULT_I:

            // 二元运算 *指令
            str = dst + " = mul " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_LT:

            // 二元运算 <指令
            str = dst + " = icmp lt " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_BT:

            // 二元运算 >指令
            str = dst + " = icmp gt " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_LE:

            // 二元运算 <=指令
            str = dst + " = icmp le " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_BE:

            // 二元运算 >=指令
            str = dst + " = icmp ge " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_EQ:

            // 二元运算 ==指令
            str = dst + " = icmp eq " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_NQ:

            // 二元运算 !=指令
            str = dst + " = icmp ne " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_AND:

            // 二元运算 &指令
            str = dst + " = and " + src_1 + ", " + src_2;
            break;
        case IRInstOperator::IRINST_OP_OR:

            // 二元运算 >指令
            str = dst + " = or " + src_1 + ", " + src_2;
            break;
        default:
            // 未知指令
            IRInst::toString(str);
            break;
    }
}

/// @brief 无参数的函数调用
/// @param name 函数名
/// @param result 保存返回值的Value
FuncCallIRInst::FuncCallIRInst(std::string _name) : IRInst(IRInstOperator::IRINST_OP_FUNC_CALL, nullptr), name(_name)
{}

/// @brief 含有参数的函数调用
/// @param _srcVal1 函数的实参Value
/// @param result 保存返回值的Value
FuncCallIRInst::FuncCallIRInst(std::string _name, Value * _srcVal1, Value * _result)
    : IRInst(IRInstOperator::IRINST_OP_FUNC_CALL, _result), name(_name)
{
    srcValues.push_back(_srcVal1);
}

/// @brief 含有参数的函数调用
/// @param srcVal 函数的实参Value
/// @param result 保存返回值的Value
FuncCallIRInst::FuncCallIRInst(std::string _name, std::vector<Value *> & _srcVal, Value * _result)
    : IRInst(IRInstOperator::IRINST_OP_FUNC_CALL, _result), name(_name)
{
    // 实参拷贝
    srcValues = _srcVal;
}

/// @brief 析构函数
FuncCallIRInst::~FuncCallIRInst()
{}

/// @brief 转换成字符串显示
/// @param str 转换后的字符串
void FuncCallIRInst::toString(std::string & str)
{
    Value * result = dstValue;

    // TODO 这里应该根据函数名查找函数定义或者声明获取函数的类型
    // 这里假定所有函数返回类型要么是i32，要么是void
    // 函数参数的类型是i32

    if (result->type.type == BasicType::TYPE_VOID) {

        // 函数没有返回值设置
        str = "call void @" + name + "(";
    } else {

        // 函数有返回值要设置到结果变量中
        str = result->getName() + " = call i32 @" + name + "(";
    }

    for (size_t k = 0; k < srcValues.size(); ++k) {

        if (srcValues[k]->type.type == BasicType::TYPE_INT || srcValues[k]->type.type == BasicType::TYPE_FNP_I) {
            str += "i32 ";
        } else if (srcValues[k]->type.type == BasicType::TYPE_FLOAT ||
                   srcValues[k]->type.type == BasicType::TYPE_FNP_F) {
            str += "float ";
        }
        str += srcValues[k]->toString();
        if (srcValues[k]->np != nullptr) {
            for (int i = 0; i <= (int) srcValues[k]->np->np_sizes.size() - 1; i++) {
                str += "[" + std::to_string(srcValues[k]->np->np_sizes[i]) + "]";
            }
        }
        if (k != (srcValues.size() - 1)) {
            str += ", ";
        }
    }

    str += ")";
}
/// @brief 赋值IR指令
/// @param _result
/// @param _srcVal1
AssignIRInst::AssignIRInst(Value * _result, Value * _srcVal1, int flag)
    : IRInst(IRInstOperator::IRINST_OP_ASSIGN, _result)
{
    _flag = flag;
    srcValues.push_back(_srcVal1);
}

AssignIRInst::AssignIRInst(Value * _result, std::string _srcVal1, int flag)
    : IRInst(IRInstOperator::IRINST_OP_ASSIGN, _result)
{
    _flag = flag;
    Assign_flag = flag;
    src = _srcVal1;
    Assign_src = src;
    //新增new一个新的Value插入
    Value * srcVal1 = new ConstValue(std::stoi(src));
    srcVal1->is_numpy = true;
    srcValues.push_back(srcVal1);
}

/// @brief 析构函数
AssignIRInst::~AssignIRInst()
{}

/// @brief 转换成字符串显示
/// @param str 转换后的字符串
void AssignIRInst::toString(std::string & str)
{
    Value * src1 = nullptr;
    Value * result = nullptr;
    if (_flag != 6) {
        src1 = srcValues[0];
        result = dstValue;
    } else if (_flag == 6) {
        result = dstValue;
        // std::string src1 = src;
    }

    std::string dst;
    std::string src_1;
    dst = result->getName();
    if (src1 != nullptr) {
        src_1 = src1->toString();
    }

    // TODO:测试IR时请注释掉以下6行，这几行是用于SSA的IR转换测试使用
    // if (this->getSrc1() != nullptr && this->getSrc1()->isLocalVar()) {
    //     src_1 += "_" + int2str(this->getSrc1Tag());
    // }
    // if (this->getDst() != nullptr && this->getDst()->isLocalVar()) {
    //     dst += "_" + int2str(this->getTag());
    // }
    if (_flag == 0) {
        str = dst + " = " + src_1;
    } else if (_flag == 1) {
        str = "Const " + dst + " = " + src_1;
    } else if (_flag == 2) {
        //右边是数组
        str = dst + " = " + "*" + src_1;
    } else if (_flag == 3) {
        //左边是数组
        str = "*" + dst + " = " + src_1;
    } else if (_flag == 4) {
        //两边是数组
        str = "*" + dst + " = " + "*" + src_1;
    } else if (_flag == 5) {
        str = dst + " = neg " + src_1;
    } else if (_flag == 6) {
        str = "*" + dst + " = " + src;
    }
}

/// @brief return语句指令
/// @param _result 返回结果值
ExitIRInst::ExitIRInst(Value * _result) : IRInst(IRInstOperator::IRINST_OP_EXIT, nullptr)
{
    if (_result != nullptr) {
        srcValues.push_back(_result);
    }
}

/// @brief 析构函数
ExitIRInst::~ExitIRInst()
{}

/// @brief 转换成字符串显示
/// @param str 转换后的字符串
void ExitIRInst::toString(std::string & str)
{
    if (srcValues.empty()) {
        str = "exit";
    } else {
        Value * src1 = srcValues[0];
        str = "exit " + src1->toString();
    }
}

/// @brief return语句指令
EntryIRInst::EntryIRInst() : IRInst(IRInstOperator::IRINST_OP_ENTRY, nullptr)
{}

/// @brief 析构函数
EntryIRInst::~EntryIRInst()
{}

/// @brief 转换成字符串
void EntryIRInst::toString(std::string & str)
{
    str = "entry";
}

// /// @brief return语句指令
// /// @param target 跳转目标
// GotoIRInst::GotoIRInst(IRInst * target) : IRInst(IRInstOperator::IRINST_OP_BR, nullptr)
// {
//     // 真假目标一样，则无条件跳转
//     trueInst = falseInst = target;
// }

// /// @brief 析构函数
// GotoIRInst::~GotoIRInst()
// {}

// /// @brief 转换成字符串
// void GotoIRInst::toString(std::string & str)
// {
//     // 修改为dragonir格式
//     // str = "goto " + trueInst->getLabelName();
//     str = "br label " + trueInst->getLabelName();
// }

/// @brief return语句指令
/// @param target 跳转目标
BrIRInst::BrIRInst(IRInst * target) : IRInst(IRInstOperator::IRINST_OP_BR, nullptr)
{
    // 真假目标一样，则无条件跳转
    trueInst = falseInst = target;
}

/// @brief 析构函数
BrIRInst::~BrIRInst()
{}

/// @brief 转换成字符串
void BrIRInst::toString(std::string & str)
{
    // 修改为dragonir格式
    str = "br label " + trueInst->getLabelName();
}

/// @brief return语句指令
/// @param target 跳转目标
BcIRInst::BcIRInst(int flag, Value * target) : IRInst(IRInstOperator::IRINST_OP_BC, nullptr)
{
    // 真假目标一样，则无条件跳转
    trueInst = TrueEntry;
    falseInst = FalseEntry;
    if (flag == 1 || flag == 4) {
        modeInst_1 = Mode_1_Entry;
        if (target != nullptr && flag == 4)
            temp = target;
    } else if (flag == 2 || flag == 5) {
        modeInst_2 = Mode_2_Entry;
        if (target != nullptr && flag == 5)
            temp = target;
    } else if (target != nullptr && flag == 3) {
        temp = target;
    }
    T = countTemp;
    mode = flag;
}

/// @brief 析构函数
BcIRInst::~BcIRInst()
{}

/// @brief 转换成字符串
void BcIRInst::toString(std::string & str)
{
    // 修改为dragonir格式
    if (mode == 0) {
        str = "bc %t" + std::to_string(T) + ", label " + trueInst->getLabelName() + ", label " +
              falseInst->getLabelName();
    } else if (mode == 1) { // or
        str = "bc %t" + std::to_string(T) + ", label " + trueInst->getLabelName() + ", label " +
              modeInst_1->getLabelName();
    } else if (mode == 2) { // and
        str = "bc %t" + std::to_string(T) + ", label " + modeInst_2->getLabelName() + ", label " +
              falseInst->getLabelName();
    } else if (mode == 3) {
        str = "bc " + temp->toString() + ", label " + trueInst->getLabelName() + ", label " + falseInst->getLabelName();
    } else if (mode == 4) { // or
        str =
            "bc " + temp->toString() + ", label " + trueInst->getLabelName() + ", label " + modeInst_1->getLabelName();
    } else if (mode == 5) { // and
        str =
            "bc " + temp->toString() + ", label " + modeInst_2->getLabelName() + ", label " + falseInst->getLabelName();
    }
}

PhiIRInst::PhiIRInst() : IRInst(IRInstOperator::IRINST_OP_PHI, nullptr)
{}
/// @brief Phi函数指令
/// @param 两个Value，flag是控制流分析得到，进行Value选择
PhiIRInst::PhiIRInst(Value * value, int tagSrc1, int tagSrc2, bool flag)
    : IRInst(IRInstOperator::IRINST_OP_PHI, nullptr)
{

    srcValues.push_back(value);
    srcValues.push_back(value);
    // 插入标号
    this->setSrc1Tag(tagSrc1);
    this->setSrc2Tag(tagSrc2);
    // flag是真，目的操作数选择src1;反之选择src2——>后续分析数据流可能会用到
    if (flag) {
        // this->localVarTag = tagSrc1;
    } else {
        // this->localVarTag = tagSrc2;
    }
    //重新定值
    int tagDst = tagSrc1 > tagSrc2 ? tagSrc1 : tagSrc2;
    this->localVarTag = tagDst + 1;
    dstValue = value;
}

/// @brief 析构函数
PhiIRInst::~PhiIRInst()
{}

/// @brief 转换成字符串
void PhiIRInst::toString(std::string & str)
{
    Value *src1 = srcValues[0], *src2 = srcValues[1], *result = dstValue;
    str = result->getName() + "_" + int2str(this->localVarTag) + "定值:" + result->getName() + "_" +
          int2str(this->localVarTag) + " = phi (" + src1->toString() + "_" + int2str(this->tagSrc1) + "," +
          src2->toString() + "_" + int2str(this->tagSrc2) + ")";
}