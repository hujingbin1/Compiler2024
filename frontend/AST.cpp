﻿/**
 * @file ast.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 抽象语法树管理
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <cstdarg>
#include <cstdint>
#include <string>
#include <string.h>
//string.h可以用么

#include "AST.h"
#include "AttrType.h"
#include "ValueType.h"

/* 整个AST的根节点 */
ast_node * ast_root = nullptr;

/// @brief 创建指定节点类型的节点
/// @param _node_type 节点类型
/// @param _line_no 行号
ast_node::ast_node(ast_operator_type _node_type, int32_t _line_no ) : node_type(_node_type), line_no(_line_no)
{
    type.type = BasicType::TYPE_MAX;
    parent = nullptr;
    val = nullptr;
}

/// @brief 创建指定节点类型的节点
/// @param _OriginalType 节点类型
/// @param _line_no 行号
ast_node::ast_node(NodeKind _OriginalType, int32_t _line_no ) : OriginalType(_OriginalType), line_no(_line_no)
{
    type.type = BasicType::TYPE_MAX;
    parent = nullptr;
    val = nullptr;
}



/// @brief 构造函数
/// @param _type 节点值的类型
/// @param line_no 行号
ast_node::ast_node(ValueType _type, int32_t _line_no) : line_no(_line_no), type(_type)
{
    node_type = ast_operator_type::AST_OP_LEAF_TYPE;
    parent = nullptr;
    val = nullptr;
}

/// @brief 针对无符号整数字面量的构造函数
/// @param attr 无符号整数字面量
ast_node::ast_node(digit_int_attr attr) : line_no(attr.lineno), integer_val(attr.val)
{
    node_type = ast_operator_type::AST_OP_LEAF_LITERAL_UINT;
    type.type = BasicType::TYPE_INT;
    parent = nullptr;
    val = nullptr;
}

/// @brief 针对float字面量的构造函数
/// @param attr 浮点数整数字面量
ast_node::ast_node(digit_real_attr attr) : line_no(attr.lineno), float_val(attr.val)
{
    node_type = ast_operator_type::AST_OP_LEAF_LITERAL_FLOAT;
    type.type = BasicType::TYPE_FLOAT;
    parent = nullptr;
    val = nullptr;
}

/// @brief 针对标识符ID的叶子构造函数
/// @param attr 字符型字面量
ast_node::ast_node(var_id_attr attr) : line_no(attr.lineno), name(attr.id)
{
    node_type = ast_operator_type::AST_OP_LEAF_VAR_ID;
	OriginalType = IdentK ;

    // 目前这里默认设置为int类型
    type.type = BasicType::TYPE_INT;
}

/// @brief 创建空节点
ast_node::ast_node()
{
    type.type = BasicType::TYPE_MAX;
    parent = nullptr;
    val = nullptr;
}

/// @brief 判断是否是叶子节点
/// @param type 节点类型
/// @return true：是叶子节点 false：内部节点
bool isLeafNode(ast_operator_type type)
{
    bool is_leaf;

    switch (type) {
        case ast_operator_type::AST_OP_LEAF_LITERAL_UINT:
        case ast_operator_type::AST_OP_LEAF_LITERAL_FLOAT:
        case ast_operator_type::AST_OP_LEAF_VAR_ID:
        case ast_operator_type::AST_OP_FUNC_FORMAL_PARAM:
            is_leaf = true;
            break;
        default:
            is_leaf = false;
            break;
    }

    return is_leaf;
}

/// @brief 创建指定节点类型的节点
/// @param type 节点类型
/// @param
/// 可变参数，可支持插入若干个孩子节点，自左往右的次序，最后一个孩子节点必须指定为nullptr。如果没有孩子，则指定为nullptr
/// @return 创建的节点
ast_node * new_ast_node(ast_operator_type type, ...)
{
    ast_node * parent_node = new ast_node((ast_operator_type) type, -1);

    va_list valist;

    /* valist指向传入的第一个可选参数 */
    va_start(valist, type);

    for (;;) {

        ast_node * node = va_arg(valist, ast_node *);
        if (nullptr == node) {
            break;
        }

        parent_node->sons.push_back(node);
        node->parent = parent_node;
    }

    /* 清理为 valist 保留的内存 */
    va_end(valist);

    return parent_node;
}

/// @brief 向父节点插入一个节点
/// @param parent 父节点
/// @param node 节点
ast_node * insert_ast_node(ast_node * parent, ast_node * node)
{
    node->parent = parent;
    parent->sons.push_back(node);
    return parent;
}

/// @brief 创建无符号整数的叶子节点
/// @param attr 无符号整数字面量
ast_node * new_ast_leaf_node(digit_int_attr attr)
{
    ast_node * node = new ast_node(attr);

    return node;
}

/// @brief 创建实数的叶子节点
/// @param attr 浮点数字面量
ast_node * new_ast_leaf_node(digit_real_attr attr)
{
    ast_node * node = new ast_node(attr);

    return node;
}

/// @brief 创建标识符的叶子节点
/// @param attr 字符型字面量
ast_node * new_ast_leaf_node(var_id_attr attr)
{
    ast_node * node = new ast_node(attr);

    return node;
}

/// @brief 创建具备指定类型的节点
/// @param type 节点值类型
/// @param line_no 行号
/// @return 创建的节点
ast_node * new_ast_leaf_node(BasicType type, int32_t line_no)
{
    ast_node * node = new ast_node(type, line_no);

    return node;
}

/// @brief 递归清理抽象语法树
/// @param node AST的节点
void free_ast_node(ast_node * node)
{
    if (node) {

        for (auto child: node->sons) {
            free_ast_node(child);
        }

        // 这里没有必要清理孩子，由于下面就要删除该节点
        // node->sons.clear();
    }

    // 清理node资源
    delete node;
}

/// @brief AST资源清理
void free_ast()
{
    free_ast_node(ast_root);
    ast_root = nullptr;
}

/// @brief 创建函数定义类型的内部AST节点
/// @param line_no 行号
/// @param func_name 函数名
/// @param block 函数体语句块
/// @param params 函数形参，可以没有参数
/// @return 创建的节点
ast_node * create_func_def(uint32_t line_no, const char * func_name, ast_node * block, ast_node * params)
{
    ast_node * node = new ast_node(ast_operator_type::AST_OP_FUNC_DEF, line_no);
    node->type.type = BasicType::TYPE_VOID;
    node->name = func_name;

    // 如果没有参数，则创建参数节点
    if (!params) {
        params = new ast_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS);
    }

    // 如果没有函数体，则创建函数体，也就是语句块
    if (!block) {
        block = new ast_node(ast_operator_type::AST_OP_BLOCK);
    }

    node->sons.push_back(params);
    params->parent = node;

    node->sons.push_back(block);
    block->parent = node;

    return node;
}

/// @brief 创建函数形式参数的节点
/// @param line_no 行号
/// @param param_name 参数名
/// @return 创建的节点
ast_node * create_func_formal_param(uint32_t line_no, const char * param_name)
{
    ast_node * node = new ast_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, line_no);

    // 这里假定整型
    node->type.type = BasicType::TYPE_INT;
    node->name = param_name;

    return node;
}

/// @brief 创建AST的内部节点
/// @param node_type 节点类型
/// @param first_param 第一个孩子节点
/// @return 创建的节点
ast_node * create_contain_node(ast_operator_type node_type,NodeKind _Original_Type, ast_node * first_param)
{
    ast_node * node = new ast_node(node_type);
	node->OriginalType = _Original_Type;

    if (first_param != nullptr) {
        first_param->parent = node;
        node->sons.push_back(first_param);
    }

    return node;
}

/// @brief 创建函数调用的节点
/// @param line_no 行号
/// @param func_name 被调用的函数名
/// @param params 实参节点
/// @return 创建的节点
ast_node * create_func_call(uint32_t line_no, const char * func_name, ast_node * params)
{
    ast_node * node = new ast_node(ast_operator_type::AST_OP_FUNC_CALL, line_no);
    node->type.type = BasicType::TYPE_MAX;
    node->name = func_name;

    // 如果没有参数，则创建参数节点
    if (!params) {
        params = new ast_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS);
    }

    node->sons.push_back(params);
    params->parent = node;

    return node;
}

/// @brief 设置ExpType
/// @param Node 设置的对象节点
/// @param type 所赋值的参数
void SetBasicType(ast_node * Node,ExpType type)
{
	Node->BasicType = type;
}

/// @brief 设置ASTTYPE
/// @param Node 设置的对象节点
/// @param type 所赋值的参数
void SetNode_ASTType(ast_node * Node,ASTTYPE type)
{
	Node->ast_node_Type = type;
}

/// @brief 设置Nodekind
/// @param Node 设置的对象节点
/// @param type 所赋值的参数
void SetOriginalType(ast_node * Node,NodeKind type)
{
	Node->OriginalType = type;
}

/// @brief 返回节点的line_no
/// @param Node 对象节点
uint32_t GetLine_no(ast_node * Node)
{
	return Node->line_no;
}

/// @brief 设置line_no
/// @param Node 设置的对象节点
/// @param lineno 所赋值的参数
void SetLine_no(ast_node * Node,uint32_t lineno)
{
	Node->line_no = lineno;
}

/// @brief 返回节点的变量名，或者函数名 Name
/// @param Node 对象节点
std::string GetVarName(ast_node * Node)
{
	return Node->name;
}

/// @brief 设置line_no
/// @param Node 设置的对象节点
/// @param lineno 所赋值的参数
void SetVarName(ast_node * Node,std::string _name)
{
	Node->name = _name;
}

/// @brief 设置Node的字面量
/// @param Node 设置的对象节点
/// @param pvBuffer 所赋值的参数
/// @param Basic_Type 字面量类型
void SetLiteralContent(ast_node * Node, void * pvBuffer, ExpType Basic_Type)
{
    if (Basic_Type == Integer)
    {
        memcpy( &Node->integer_val, pvBuffer, sizeof(int) );
        Node->IsLiteral = 1;
    }
    else if (Basic_Type == Float)
    {
        memcpy( &Node->float_val, pvBuffer, sizeof(float) );
        Node->IsLiteral = 1;
    }
    else if (Node->BasicType == Integer)
    {
        memcpy( &Node->integer_val, pvBuffer, sizeof(int) );
        Node->IsLiteral = 1;
    }
    else if (Node->BasicType == Float)
    {
        memcpy( &Node->float_val, pvBuffer, sizeof(float) );
        Node->IsLiteral = 1;
    }

}

/// @brief 设置Node的flag
/// @param Node 设置的对象节点
/// @param flag 所赋值的参数
void Ast_SetFlag(ast_node * Node,int _flag)
{
	Node->flag = _flag;
}
