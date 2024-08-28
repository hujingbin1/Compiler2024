/**
 * @file IRGenerator.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief AST遍历产生线性IR的头文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <unordered_map>

#include "AST.h"
#include "SymbolTable.h"


/// @brief AST遍历产生线性IR类
class IRGenerator
{

public:
    /// @brief 构造函数
    /// @param root
    /// @param symtab
    IRGenerator(ast_node * root, SymbolTable * symtab);

    /// @brief 析构函数
    ~IRGenerator() = default;

    /// @brief 运行产生IR
    bool run();

protected:
/*
    /// @brief 编译单元AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_compile_unit(ast_node * node);

    /// @brief 函数定义AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_define(ast_node * node);

    /// @brief 形式参数AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_formal_params(ast_node * node);

    /// @brief 函数调用AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_call(ast_node * node);

    /// @brief 语句块（含函数体）AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_block(ast_node * node);

    /// @brief 表达式语句ST节点翻译成线性中间IR的共同函数
    /// @param node AST节点
    /// @param show 是否显示值，true：显示，false：不显示
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_show_internal(ast_node * node, bool show);

    /// @brief 不显示表达式AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_expr_noshow(ast_node * node);

    /// @brief 显示表达式AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_expr_show(ast_node * node);

    /// @brief 整数加法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_add(ast_node * node);

    /// @brief 整数减法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_sub(ast_node * node);

    /// @brief 赋值AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_assign(ast_node * node);

    /// @brief return节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_return(ast_node * node);

    /// @brief 标识符叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_var_id(ast_node * node);

    /// @brief 无符号整数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_uint(ast_node * node);

    /// @brief float数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_float(ast_node * node);
*/

	///@brief 存储数组
	///@param node 当前节点
	///@param is_int 是否为整形数组
	///@param deep 数组维度，当前深度
	///@param save_place 当前应存入的位置
	void dfs_save(Value * val, ast_node * node, _numpy_ * np, int is_int, int deep, int save_place,int is_Loc);
	//如果是a[1][2][3]那么sizes顺序为3-2-1


    /// @brief 未知节点类型的节点处理
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_default(ast_node * node);

	/// @brief LValK是作为叶子节点的，是运算或条件语句中的变量,若是数组的话，存在子节点表示维度即A = a[1][1] + b;
	//也有可能是传进函数的实参，不过实参如果是数组类型，不存在子节点
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_LValK(ast_node * node);

	/// @brief CompUnitK是根节点
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_CompUnitK(ast_node * node);

	/// @brief ConstInitValK
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_ConstInitValK(ast_node * node);

	/// @brief ConstDeclK
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_ConstDeclK(ast_node * node);

	/// @brief ConstDefK是const定义语句其存储了所定义的名字，其子节点是一个赋值语句节点，其子节点的子节点为所定义的数值和数组维度，包含int和float类型	
	//ConstInitValK属于是赋值语句，其子节点的数值为父节点--ConstDefK的赋值数值,若是数组的话，ConstInitValK的子节点是ConstInitValK，子节点的子节点才是数值
    //ConstExpGroupK若其有子节点且子节点有数值，则其父亲节点是数组变量，仅限于定义const和非const，不包含函数的形参数组
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_ConstDefK(ast_node * node);

	/// @brief VarDefK是非const定义语句其存储了所定义的名字，其子节点是一个赋值语句节点，其子节点的子节点为所定义的数值和数组维度，包含int和float类型	
	//InitValK属于是赋值语句，其子节点的数值为父节点--VarDefK的赋值数值,若是数组的话，InitValK的子节点是InitValK，子节点的子节点才是数值
	//若VarDefK的子节点没有InitValK，则说明该定义只声明了变量并没有给其赋值
    //ConstExpGroupK若其有子节点且子节点有数值，则其父亲节点是数组变量，仅限于定义const和非const，不包含函数的形参数组
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_VarDeclK(ast_node * node);

	/// @brief VarDefK
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_VarDefK(ast_node * node);

	/// @brief FuncDeclK只是声明了函数，并没有给出函数的具体操作即Block，该节点包含了函数名和返回类型，其子节点为fomalpara，子节点的子节点是形参
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_FuncDeclK(ast_node * node);

	/// @brief FuncDefK是函数定义，子节点为Block和foamlpara，这两个的子节点分别是函数操作和形参
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_FuncDefK(ast_node * node);

	/// @brief FuncFParamsK是函数形参表，其子节点为FuncFParamK包含了形参的类型(int,float)和名字，若无形参其后面没有子节点
	//若形参为数组，FuncFParamK子节点存在一个ExpK类型的节点，其他n个子节点为数值，该形参维度为n+1
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_FuncFParamsK(ast_node * node);

	/// @brief BlockK是语句块，只要有{}的都算语句块，其中语句块大部分用于函数操作，if，while语句
	//ps：注意作用域；if，while可以没有BlockK，即没有{}，eg：if（a） a = 1；
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_BlockK(ast_node * node);

	/// @brief StmtK包含AT_AssignStmt、AT_IfStmt、
	//AT_WhileStmt、AT_BreakStmt、AT_ContinueStmt、
	//AT_ContinueStmt、AT_ConstInitVal
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_StmtK(ast_node * node);

	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_NumberK(ast_node * node);

	/// @brief InitValK计算所要赋值的数值
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_InitValK(ast_node * node);


	/// @brief UnaryExpK为单目运算，包含函数调用，+-！的单目运算，存储着函数名或+-!;eg：putin(A);!A;
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_UnaryExpK(ast_node * node);

	/// @brief MulExpK包含了/%*这三个运算，子节点为MulExpK、AddExpK或者为运算对象,是双目运算符
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_MulExpK(ast_node * node);

	/// @brief AddExpK是双目运算，包含了+-这两个运算，子节点为MulExpK、AddExpK或者为运算对象
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_AddExpK(ast_node * node);

	/// @brief RelExpK是双目运算，包含了< <= > >=这四个运算,子节点为比较对象
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_RelExpK(ast_node * node);

	/// @brief EqExpK是双目运算，包含了== !=这两个运算,子节点为比较对象
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_EqExpK(ast_node * node);

	/// @brief LAndExpK是双目运算&&是and操作，子节点的and的对象
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_LAndExpK(ast_node * node);

	/// @brief LOrExpK是双目运算||是or操作，子节点的or的对象
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_LOrExpK(ast_node * node);


    /// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
    /// @param node AST节点
    /// @return 成功返回node节点，否则返回nullptr
    ast_node * ir_visit_ast_node(ast_node * node);

    /// @brief AST的节点操作函数
    typedef bool (IRGenerator::*ast2ir_handler_t)(ast_node *);

    /// @brief AST节点运算符与动作函数关联的映射表
    std::unordered_map<NodeKind, ast2ir_handler_t> ast2ir_handlers;

private:
    /// @brief 抽象语法树的根
    ast_node * root;

    /// @brief 符号表
    SymbolTable * symtab;
};
