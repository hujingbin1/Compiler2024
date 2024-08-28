/**
 * @file ast.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 抽象语法树AST管理的源文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "AttrType.h"
#include "IRCode.h"
#include "Value.h"

extern int nCount; // 规约次数

/// @brief AST节点的类型
enum class ast_operator_type : int {

    /* 以下为AST的叶子节点 */

    /// @brief 无符号整数字面量叶子节点
    AST_OP_LEAF_LITERAL_UINT,

    /// @brief  无符号整数字面量叶子节点
    AST_OP_LEAF_LITERAL_FLOAT,

    /// @brief 变量ID叶子节点
    AST_OP_LEAF_VAR_ID,

    /// @brief 复杂类型的节点
    AST_OP_LEAF_TYPE,

    /* 以下为AST的内部节点，含根节点 */
    /// @brief 二元运算符+
    AST_OP_ADD,

    /// @brief 二元运算符-
    AST_OP_SUB,

	/// @brief 二元运算符*
	AST_OP_MULT,

	/// @brief 二元运算符/
	AST_OP_DIV,

	/// @brief 二元运算符%
	AST_OP_MOD,

	///@brief 二元运算符<
	AST_OP_LT,

	///@brief 二元运算符>
	AST_OP_BT,

	///@brief 二元运算符<=
	AST_OP_LE,

	///@brief 二元运算符>=
	AST_OP_BE,

	///@brief 二元运算符==
	AST_OP_EQ,

	///@brief 二元运算符!=
	AST_OP_NQ,

	///@brief 二元运算符&&
	AST_OP_AND,

	///@brief 二元运算符||
	AST_OP_OR,




    /// @brief 多个语句组成的块运算符
    AST_OP_BLOCK,

    /// @brief 赋值语句运算符
    AST_OP_ASSIGN,

    /// @brief 表达式语句运算符，不显示表达式的值
    AST_OP_EXPR,

    /// @brief 表达式显示语句运算符，需要显示表达式的值
    AST_OP_EXPR_SHOW,

    /// @brief return语句运算符
    AST_OP_RETURN_STATEMENT,

    /// @brief 函数定义运算符，函数名和返回值类型作为节点的属性，自左到右孩子：AST_OP_FUNC_FORMAL_PARAMS、AST_OP_BLOCK
    AST_OP_FUNC_DEF,

    /// @brief 形式参数列表运算符，可包含多个孩子：AST_OP_FUNC_FORMAL_PARAM
    AST_OP_FUNC_FORMAL_PARAMS,

    /// @brief 形参运算符，属性包含名字与类型，复杂类型时可能要包含孩子
    AST_OP_FUNC_FORMAL_PARAM,

    /// @brief 函数调用运算符，函数名作为节点属性，孩子包含AST_OP_FUNC_REAL_PARAMS
    AST_OP_FUNC_CALL,

    /// @brief 实际参数列表运算符，可包含多个表达式AST_OP_EXPR
    AST_OP_FUNC_REAL_PARAMS,

    /// @brief 文件编译单元运算符，可包含函数定义、语句块等孩子
    AST_OP_COMPILE_UNIT,

    // TODO 抽象语法树其它内部节点运算符追加
	/// @brief 单目运算符+
	AST_UNARYOP_PLUS,

	/// @brief 单目运算符-
    AST_UNARYOP_MINUS,

	/// @brief 单目运算符!
    AST_UNARYOP_NOT,

    /// @brief 最大标识符，表示非法运算符
    AST_OP_MAX,
};

/// @brief 记录最初节点的类型
typedef enum {
	CompUnitK = 1,
    DeclK = 2,
    ConstDeclK = 3,
    ConstDefGroupK = 4,
    ConstDefK = 5,
    ConstExpGroupK = 6,
    ConstInitValK = 7,
    ConstInitValGroupK = 8,
    dummy1K = 9,
    VarDeclK = 10,
    VarDefGroupK = 11,
    VarDefK = 12,
    InitValK =13,
    InitValGroupK = 14,
    FuncDefK = 15,
    FuncFParamsK = 16,
    FuncFParamGroupK = 17,
    FuncFParamK = 18,
    ExpGroupK = 19,
    BlockK = 20,
    BlockItemGroupK = 21,
    BlockItemK = 22,
    StmtK = 23,//先放在一边
    ExpK = 24, //形参数组
    CondK = 25,
    LValK = 26,
    NumberK = 27,
    PrimaryExpK = 28,
    UnaryExpK = 29,
    FuncRParamsK = 30,
    FuncRParamsGroupK = 31,
    MulExpK = 32,
    AddExpK = 33,
    RelExpK = 34,
    EqExpK = 35,
    LAndExpK = 36,
    LOrExpK = 37,
    ConstExpK = 38,
    IdentK = 39,

    FuncDeclK = 40,
}NodeKind;

typedef enum{
	Error    = 0,
    Void    = 1,
    Integer    = 2,
    Float    = 3,
}ExpType;

typedef enum 
    {
    AT_Expression,
    
    AT_CompUnit, 
    
    AT_ConstDecl,
    AT_VarDecl,
    AT_ConstInitVal,
    AT_ConstDef,
    AT_ConstExpGroup,
    AT_VarDef,
    AT_InitVal,
    AT_FuncDef, 
    AT_FuncFParam,//
    AT_Block,

    AT_IfStmt,
    AT_WhileStmt,
    AT_AssignStmt,
    AT_BreakStmt,
    AT_ContinueStmt,
    AT_ReturnStmt,
    AT_EmptyStmt,
    
    AT_FuncCall,
    AT_FuncRParams,//

    AT_Condition, // ：条件表达式
    AT_FuncFParams, // 函数形参表
    AT_FuncDecl, // ：函数声明

    AT_Error
    

    } ASTTYPE;



/// @brief 抽象语法树AST的节点描述类
class ast_node {
public:
    /// @brief 父节点
    ast_node * parent;

    /// @brief 孩子节点
    std::vector<ast_node *> sons;

    /// @brief 节点操作类型
    ast_operator_type node_type;

	/// @brief 节点的类型
	NodeKind OriginalType;

	/// @brief AST节点的类型
	ASTTYPE ast_node_Type;//新增

	/// @brief 声明类型
	ExpType BasicType; // 'void' | 'int' | 'float'//新增

    /// @brief 行号信息，主要针对叶子节点有用
    uint32_t line_no;

    /// @brief 节点值的类型，可用于函数返回值类型
    ValueType type;

	int flag;

	/// @brief 是不是字面量
	int IsLiteral;

    /// @brief 无符号整数字面量值
    uint32_t integer_val;

    /// @brief float类型字面量值
    float float_val;

    /// @brief 变量名，或者函数名
    std::string name;

    /// @brief 线性IR指令块，可包含多条IR指令，用于线性IR指令产生用
    InterCode blockInsts;

    /// @brief 线性IR指令或者运行产生的Value，用于线性IR指令产生用
    Value * val;

	Value * np_val = nullptr;

	int Block_number = 0;
	
	int is_saveBlock = 0;

    /// @brief 构造函数
    /// @param _type 节点值的类型
    /// @param line_no 行号
    ast_node(ValueType _type, int32_t _line_no);

    /// @brief 针对无符号整数字面量的构造函数
    /// @param attr 无符号整数字面量
    ast_node(digit_int_attr attr);

    /// @brief 针对float字面量的构造函数
    /// @param attr float型实数字面量
    ast_node(digit_real_attr attr);

    /// @brief 针对标识符ID的叶子构造函数
    /// @param attr 字符型标识符
    ast_node(var_id_attr attr);

    /// @brief 创建指定节点类型的节点
    /// @param _OriginalType 节点类型
    /// @param _line_no 行号
    ast_node(NodeKind _OriginalType, int32_t _line_no = -1);

    /// @brief 创建指定节点类型的节点
    /// @param _ast_node_Type 节点类型
    /// @param _line_no 行号
    ast_node(ast_operator_type _node_type, int32_t _line_no = -1);

	/// @brief 创建空节点
    ast_node();
};

/// @brief 判断是否是叶子节点
/// @param type 节点类型
/// @return true：是叶子节点 false：内部节点
bool isLeafNode(ast_operator_type type);

/// @brief 创建指定节点类型的节点
/// @param type 节点类型
/// @param  可变参数，最后一个孩子节点必须指定为nullptr。如果没有孩子，则指定为nullptr
/// @return 创建的节点
ast_node * new_ast_node(ast_operator_type type, ...);

/// @brief 向父节点插入一个节点
/// @param parent 父节点
/// @param node 节点
ast_node * insert_ast_node(ast_node * parent, ast_node * node);

/// @brief 创建无符号整数的叶子节点
/// @param val 词法值
/// @param line_no 行号
ast_node * new_ast_leaf_node(digit_int_attr attr);

/// @brief 创建实数的叶子节点
/// @param val 词法值
/// @param line_no 行号
ast_node * new_ast_leaf_node(digit_real_attr attr);

/// @brief 创建标识符的叶子节点
/// @param val 词法值
/// @param line_no 行号
ast_node * new_ast_leaf_node(var_id_attr attr);

/// @brief 创建具备指定类型的节点
/// @param type 节点值类型
/// @param line_no 行号
/// @return 创建的节点
ast_node * new_ast_leaf_node(BasicType type, int32_t line_no);

/// @brief 递归清理抽象语法树
/// @param node AST的节点
void free_ast_node(ast_node * node);

/// @brief AST资源清理
void free_ast();

/// @brief抽象语法树的根节点指针
extern ast_node * ast_root;

/// @brief 创建函数定义类型的内部AST节点
/// @param line_no 行号
/// @param func_name 函数名
/// @param block 函数体语句块
/// @param params 函数形参，可以没有参数
/// @return 创建的节点
ast_node * create_func_def(uint32_t line_no, const char * func_name, ast_node * block, ast_node * params = nullptr);

/// @brief 创建函数形式参数的节点
/// @param line_no 行号
/// @param param_name 参数名
/// @return 创建的节点
ast_node * create_func_formal_param(uint32_t line_no, const char * param_name);

/// @brief 创建AST的内部节点，并插入节点first_param
/// @param node_type 节点类型
/// @param Original_Type 节点de类型
/// @param first_param 第一个孩子节点
/// @return 创建的节点
ast_node * create_contain_node(ast_operator_type node_type,NodeKind Original_Type, ast_node * first_param = nullptr);

/// @brief 创建函数调用的节点
/// @param line_no 行号
/// @param func_name 被调用的函数名
/// @param params 实参节点
/// @return 创建的节点
ast_node * create_func_call(uint32_t line_no, const char * func_name, ast_node * params = nullptr);

/// @brief 设置ExpType
/// @param Node 设置的对象节点
/// @param type 所赋值的参数
void SetBasicType(ast_node * Node,ExpType type);

/// @brief 设置ASTTYPE
/// @param Node 设置的对象节点
/// @param type 所赋值的参数
void SetNode_ASTType(ast_node * Node,ASTTYPE type);

/// @brief 设置Nodekind
/// @param Node 设置的对象节点
/// @param type 所赋值的参数
void SetOriginalType(ast_node * Node,NodeKind type);

/// @brief 返回节点的line_no
/// @param Node 对象节点
uint32_t GetLine_no(ast_node * Node);

/// @brief 设置line_no
/// @param Node 设置的对象节点
/// @param lineno 所赋值的参数
void SetLine_no(ast_node * Node,uint32_t lineno);

/// @brief 返回节点的变量名，或者函数名 Name
/// @param Node 对象节点
std::string GetVarName(ast_node * Node);

/// @brief 设置line_no
/// @param Node 设置的对象节点
/// @param lineno 所赋值的参数
void SetVarName(ast_node * Node,std::string _name);

/// @brief 设置Node的字面量
/// @param Node 设置的对象节点
/// @param pvBuffer 所赋值的参数
/// @param Basic_Type 字面量类型
void SetLiteralContent (ast_node * Node, void * pvBuffer, ExpType Basic_Type);

/// @brief 设置Node的flag
/// @param Node 设置的对象节点
/// @param flag 所赋值的参数
void Ast_SetFlag(ast_node * Node,int _flag);