/**
 * @file IRInst.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief IR指令类的头文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <vector>
#include <iostream>

#include "Value.h"
using namespace std;
/// @brief IR指令操作码
enum class IRInstOperator {

    /// @brief 函数入口指令，对应函数的prologue，用户栈空间分配、寄存器保护等
    IRINST_OP_ENTRY,

    /// @brief 函数出口指令，对应函数的epilogue，用于栈空间的恢复与清理、寄存器恢复等
    IRINST_OP_EXIT,

    /// @brief Label指令，用于语句的跳转
    IRINST_OP_LABEL,

    /// @brief 无条件跳转指令
    // IRINST_OP_GOTO,
    IRINST_OP_BR,

    /// @brief 整数的加法指令，二元运算
    IRINST_OP_ADD_I,

    /// @brief 整数的减法指令，二元运算
    IRINST_OP_SUB_I,

    /// @brief 整数的%指令，二元运算
    IRINST_OP_MOD_I,

    /// @brief 整数的/指令，二元运算
    IRINST_OP_DIV_I,

    /// @brief 整数的*指令，二元运算
    IRINST_OP_MULT_I,

    /// @brief 比较<指令，二元运算
    IRINST_OP_LT,

    /// @brief 比较>指令，二元运算
    IRINST_OP_BT,

    /// @brief 比较<=指令，二元运算
    IRINST_OP_LE,

    /// @brief 比较>=指令，二元运算
    IRINST_OP_BE,

    /// @brief 比较==指令，二元运算
    IRINST_OP_EQ,

    /// @brief 比较!=指令，二元运算
    IRINST_OP_NQ,

    /// @brief 比较==指令，二元运算
    IRINST_OP_AND,

    /// @brief 比较!=指令，二元运算
    IRINST_OP_OR,

    /// @brief 赋值指令，一元运算
    IRINST_OP_ASSIGN,

    /// @brief 函数调用，多目运算，个数不限
    IRINST_OP_FUNC_CALL,

    /* 后续可追加其他的IR指令 */
    /// @brief 条件跳转
    // IRINST_OP_BR,
    IRINST_OP_BC,

    /// @brief 最大指令码，也是无效指令
    IRINST_OP_MAX,

    /// @brief 新增phi函数指令码
    IRINST_OP_PHI
};

/// @brief IR指令的基类
class IRInst {

public:
    int Binary_mode = 0;
    int Binary_src = 0;
    int Assign_flag = 0;
    //这里用来区分inst中，进行数组元素使用的时候，是全局数组还是局部数组
    bool isGlobal = false;
    std::string Assign_src;

    /// @brief 构造函数
    IRInst();

    /// @brief 构造函数
    /// @param op
    /// @param result
    IRInst(IRInstOperator op, Value * result = nullptr);

    /// @brief 析构函数
    virtual ~IRInst() = default;

    /// @brief 获取指令操作码
    /// @return 指令操作码
    IRInstOperator getOp();

    /// @brief 获取源操作数列表
    /// @return 源操作数列表
    std::vector<Value *> & getSrc();

    /// @brief 获取目的操作数，或者结果操作数
    /// @return 目的操作数，或者结果操作数
    Value * getDst();

    /// @brief 取得源操作数1
    /// @return 源操作数1
    Value * getSrc1();

    /// @brief 取得源操作数1的寄存器号
    /// @return 寄存器号，可能为-1，表示在内存或立即数
    int getSrc1RegId();

    /// @brief 取得源操作数2
    /// @return 源操作数2
    Value * getSrc2();

    /// @brief 取得源操作数2的寄存器号
    /// @return 寄存器号，可能为-1，表示在内存或立即数
    int getSrc2RegId();

    /// @brief 转换成字符串
    virtual void toString(std::string & str);

    /// @brief 是否是Dead指令
    bool isDead()
    {
        return dead;
    }

    /// @brief 设置指令是否是Dead指令
    /// @param _dead 是否是Dead指令，true：Dead, false: 非Dead
    void setDead(bool _dead = true)
    {
        dead = _dead;
    }

    /// @brief 获取Label指令的命令
    /// @return Label名字
    std::string getLabelName()
    {
        return labelName;
    }

    ///@brief 获取真出口的Label
    /// @return 真出口的Label名字
    std::string getTrueLabelName()
    {
        return trueInst->getLabelName();
    }

    ///@brief 获取假出口的Label
    /// @return 假出口的Label名字
    std::string getFalseLabelName()
    {

        return falseInst->getLabelName();
    }

    ///@brief 获取真出口的Label
    /// @return 真出口的Label名字
    IRInst * getTrueInst()
    {
        return trueInst;
    }

    ///@brief 获取假出口的Label
    /// @return 假出口的Label名字
    IRInst * getFalseInst()
    {

        return falseInst;
    }

    int getTag()
    {
        return this->localVarTag;
    }

    void setTag(int tag)
    {
        this->localVarTag = tag;
    }

    int getSrc1Tag()
    {
        return this->tagSrc1;
    }

    void setSrc1Tag(int src1Tag)
    {
        this->tagSrc1 = src1Tag;
    }

    int getSrc2Tag()
    {
        return this->tagSrc2;
    }

    void setSrc2Tag(int src2Tag)
    {
        this->tagSrc2 = src2Tag;
    }

    bool getDstWirte()
    {
        return this->tagDstWrite;
    }
    bool getSrc1Wirte()
    {
        return this->tagSrc1Write;
    }
    bool getSrc2Wirte()
    {
        return this->tagSrc2Write;
    }

    void setDstWirte(bool dst)
    {
        this->tagDstWrite = dst;
    }
    void setSrc1Wirte(bool src1)
    {
        this->tagSrc1Write = src1;
    }
    void setSrc2Wirte(bool src2)
    {
        this->tagSrc2Write = src2;
    }

protected:
    /// @brief IR指令操作码
    enum IRInstOperator op;

    /// @brief 源操作数
    std::vector<Value *> srcValues;

    /// @brief 目的操作数或结果或跳转指令的目标
    Value * dstValue;

    /// @brief 是否是Dead指令
    bool dead = false;

    /// @brief Label指令的名字
    std::string labelName;

    /// @brief 目标真出口指令，指向Label指令，主要用于有条件跳转
    IRInst * trueInst;

    /// @brief 目标假出口指令，指向Label指令，主要用于有条件跳转
    IRInst * falseInst;

    ///	@brief 重命名局部变量的目的操作数,SSA需要（即重写定值集合）
    int localVarTag;

    /// @brief 源操作数1使用重命名后的局部变量，SSA需要
    int tagSrc1;

    /// @brief 源操作数2使用重命名后的局部变量，SSA需要
    int tagSrc2;

    bool tagDstWrite = false;

    bool tagSrc1Write = false;

    bool tagSrc2Write = false;
};

// class optIRInst : public IRInst {
// public:
//     optIRInst();
//     ~optIRInst();

//     /// @brief 转换成字符串
//     /// @param str 返回指令字符串
//     void toString(std::string & str) override
//     {}

// private:
//     std::vector<Value *> valueSet;
//     // //标号
//     // int tagSrc1;
//     // int tagSrc2;
//     // int tagDst;
// };

/// @brief Label指令
class LabelIRInst : public IRInst {

public:
    /// @brief 构造函数
    LabelIRInst();

    /// @brief 构造函数
    /// @param name Label名字，要确保函数内唯一
    LabelIRInst(std::string name);

    /// @brief 析构函数
    virtual ~LabelIRInst();

    /// @brief 转换成字符串
    /// @param str 返回指令字符串
    void toString(std::string & str) override;

protected:
    /// @brief 新建Label名字，目前采用全局唯一编号，实际上函数内唯一即可
    /// \return Label名字
    static std::string createLabelName()
    {
        static uint64_t tempLabelCount = 0; // 常量计数，默认从0开始

        return ".L" + std::to_string(++tempLabelCount);
    }
};

/// @brief 二元运算指令
class BinaryIRInst : public IRInst {

public:
    /// @brief 构造函数
    /// @param _op 操作符
    /// @param _result 结果操作数
    /// @param _srcVal1 源操作数1
    /// @param _srcVal2 源操作数2
    BinaryIRInst(IRInstOperator _op, Value * _result, Value * _srcVal1, Value * _srcVal2, int flag = 0);

    /// @brief 构造函数
    /// @param _op 操作符
    /// @param _result 结果操作数
    /// @param _srcVal1 源操作数1
    /// @param _srcVal2 源操作数2
    BinaryIRInst(IRInstOperator _op, Value * _result, Value * _srcVal1, int _srcVal2, int flag = 2);

    /// @brief 析构函数
    virtual ~BinaryIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;

    int mode = 0;
    // 0三个都是value
    // 1三个都是value但是第二个输出名字
    // 2前两个是value，最后一个是int

    int src = 0;
};

/// @brief 函数调用指令
class FuncCallIRInst : public IRInst {

public:
    /// @brief 函数名
    std::string name;

public:
    /// @brief 无参数并且没有要设置的返回值的函数调用
    /// @param name 函数名
    FuncCallIRInst(std::string _name);

    /// @brief 含有参数的函数调用
    /// @param _srcVal1 函数的实参Value
    /// @param result 保存返回值的Value
    FuncCallIRInst(std::string _name, Value * _srcVal1, Value * _result = nullptr);

    /// @brief 含有参数的函数调用
    /// @param srcVal 函数的实参Value
    /// @param result 保存返回值的Value
    FuncCallIRInst(std::string _name, std::vector<Value *> & _srcVal, Value * _result = nullptr);

    /// @brief 析构函数
    virtual ~FuncCallIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

/// @brief 赋值指令或者说复制指令
class AssignIRInst : public IRInst {

public:
    int _flag;
    std::string src;

    /// @brief 构造函数
    /// @param result
    /// @param srcVal1
    AssignIRInst(Value * result, Value * srcVal1, int flag = 0);
    AssignIRInst(Value * result, std::string srcVal1, int flag = 0);

    /// @brief 析构函数
    virtual ~AssignIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

class EntryIRInst : public IRInst {

public:
    /// @brief return语句指令
    EntryIRInst();

    /// @brief 析构函数
    virtual ~EntryIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

/// @brief return语句指令
class ExitIRInst : public IRInst {

public:
    /// @brief return语句指令
    /// @param _result 返回结果值
    ExitIRInst(Value * result = nullptr);

    /// @brief 析构函数
    virtual ~ExitIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

class BrIRInst : public IRInst {

public:
    /// @brief return语句指令
    /// @param target 跳转目标
    BrIRInst(IRInst * target);

    /// @brief 析构函数
    virtual ~BrIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

class BcIRInst : public IRInst {

public:
    int T;
    int mode = 0;
    Value * temp = nullptr;
    /// @brief return语句指令
    /// @param target 跳转目标
    BcIRInst(int flag = 0, Value * target = nullptr);

    /// @brief 目标真出口指令，指向Label指令，主要用于有条件跳转
    IRInst * modeInst_1;

    /// @brief 目标假出口指令，指向Label指令，主要用于有条件跳转
    IRInst * modeInst_2;

    /// @brief 析构函数
    virtual ~BcIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

class PhiIRInst : public IRInst {

public:
    PhiIRInst();

    /// @brief 构造函数进行phi函数Value选择
    PhiIRInst(Value * value, int tagSrc1, int tagSrc2, bool flag);

    /// @brief 析构函数
    virtual ~PhiIRInst() override;

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};