/**
 * @file graph.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief
 * 利用graphviz图形化显示AST，本文件采用C语言实现，没有采用C++的类实现，注意AST的遍历方式和其它的不同
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>
#include <vector>

#include "AST.h"
#include "Common.h"
extern int c;
using namespace std;

#ifdef USE_GRAPHVIZ
#include <gvc.h>

/// @brief 转换运算符成字符串
/// @param astnode AST节点
/// @return 运算符对应的字符串
string getNodeName(ast_node *astnode) {
  string nodeName = " ";
  string IDType = " ";
  string cond = " ";

  switch (astnode->ast_node_Type) {
  case AT_ConstExpGroup:
    nodeName = "Array";
    break;
  case AT_Expression:
    if (astnode->BasicType == 2)
      IDType = to_string((int32_t)astnode->integer_val);
    else if (astnode->BasicType == 3)
      IDType = to_string(astnode->float_val);
    if (astnode->OriginalType == AddExpK || astnode->OriginalType == MulExpK) {
      switch (astnode->node_type) {
      case ast_operator_type::AST_OP_ADD:
        nodeName = "+ ";
        break;
      case ast_operator_type::AST_OP_SUB:
        nodeName = "-";
        break;
      case ast_operator_type::AST_OP_MULT:
        nodeName = "*";
        break;
      case ast_operator_type::AST_OP_DIV:
        nodeName = "/";
        break;
      case ast_operator_type::AST_OP_MOD:
        nodeName = "%";
        break;
      default:
        nodeName = "unknow";
      }
    } else if (astnode->OriginalType == LValK)
      nodeName = astnode->name;
    else if (astnode->OriginalType == NumberK)
      nodeName = IDType;
    else if (astnode->OriginalType == ExpK)
      nodeName = "Array";
    else if (astnode->OriginalType == UnaryExpK) {
      switch (astnode->node_type) {
      case ast_operator_type::AST_UNARYOP_PLUS:
        nodeName = "+";
        break;
      case ast_operator_type::AST_UNARYOP_MINUS:
        nodeName = "-";
        break;
      case ast_operator_type::AST_UNARYOP_NOT:
        nodeName = "!";
        break;
      default:
        nodeName = "unknow";
        break;
      }
    }
    break;
  case AT_ConstDecl:
  case AT_ConstDef:
    if (astnode->BasicType == 1)
      IDType = "void ";
    else if (astnode->BasicType == 2)
      IDType = "int ";
    else if (astnode->BasicType == 3)
      IDType = "float ";
    nodeName = "const" + IDType + astnode->name;
    break;
  case AT_ConstInitVal:
    nodeName = "= AT_ConstInitVal";
    break;
  case AT_VarDef:
  case AT_VarDecl:
    if (astnode->BasicType == 1)
      IDType = "void ";
    else if (astnode->BasicType == 2)
      IDType = "int ";
    else if (astnode->BasicType == 3)
      IDType = "float ";
    nodeName = IDType + astnode->name;
    break;
  case AT_InitVal:
    nodeName = "= AT_InitVal";
    break;
  case AT_AssignStmt:
    nodeName = "= AT_AssignStmt";
    break;
  case AT_Block:
    nodeName = "block";
    break;
    /*
case ast_operator_type::AST_OP_EXPR:
nodeName = "expr";
break;
case ast_operator_type::AST_OP_EXPR_SHOW:
nodeName = "expr-show";
break;*/
  case AT_ReturnStmt:
    nodeName = "return";
    break;
  case AT_FuncDef:
  case AT_FuncDecl:
    if (astnode->BasicType == 1)
      IDType = "void ";
    else if (astnode->BasicType == 2)
      IDType = "int ";
    else
      IDType = "float ";
    nodeName = "func:" + IDType + astnode->name;
    break;
  case AT_FuncFParams:
    nodeName = "formal-params";
    break;
  case AT_FuncFParam:
    if (astnode->BasicType == 1)
      IDType = "void ";
    else if (astnode->BasicType == 2)
      IDType = "int ";
    else
      IDType = "float ";
    nodeName = IDType + astnode->name;
    break;
  case AT_CompUnit:
    nodeName = "Compile-Unit";
    break;
  case AT_FuncCall:
    if (astnode->flag == 123) {
      nodeName = astnode->name;
    }
    //{nodeName = "func-call:" + astnode->name;}
    // else如果不是
    {
      if (astnode->node_type == ast_operator_type::AST_UNARYOP_PLUS)
        nodeName = "+";
      else if (astnode->node_type == ast_operator_type::AST_UNARYOP_MINUS)
        nodeName = "-";
      else if (astnode->node_type == ast_operator_type::AST_UNARYOP_NOT)
        nodeName = "!";
      else if (astnode->node_type == ast_operator_type::AST_OP_MULT)
        nodeName = "*";
      else if (astnode->node_type == ast_operator_type::AST_OP_DIV)
        nodeName = "/";
      else if (astnode->node_type == ast_operator_type::AST_OP_MOD)
        nodeName = "%";
      else if (astnode->node_type == ast_operator_type::AST_OP_ADD)
        nodeName = "+";
      else if (astnode->node_type == ast_operator_type::AST_OP_SUB)
        nodeName = "-";
    }
    break;
  case AT_FuncRParams:
    nodeName = "real-params";
    break;

    // TODO 这里追加其它类型的结点，返回对应结点的字符串
  case AT_IfStmt:
    nodeName = "if-stmt";
    break;
  case AT_WhileStmt:
    nodeName = "while-stmt";
    break;
  case AT_BreakStmt:
    nodeName = "break";
    break;
  case AT_ContinueStmt:
    nodeName = "continue";
    break;
  case AT_Condition:
    nodeName = "condition ";
    if (astnode->node_type == ast_operator_type::AST_OP_OR)
      nodeName += " ||";
    else if (astnode->node_type == ast_operator_type::AST_OP_AND)
      nodeName += " &&";
    else if (astnode->node_type == ast_operator_type::AST_OP_LT)
      nodeName += " <";
    else if (astnode->node_type == ast_operator_type::AST_OP_BT)
      nodeName += " >";
    else if (astnode->node_type == ast_operator_type::AST_OP_LE)
      nodeName += " <=";
    else if (astnode->node_type == ast_operator_type::AST_OP_BE)
      nodeName += " >=";
    else if (astnode->node_type == ast_operator_type::AST_OP_EQ)
      nodeName += " ==";
    else if (astnode->node_type == ast_operator_type::AST_OP_NQ)
      nodeName += " !=";
    else if (astnode->node_type == ast_operator_type::AST_UNARYOP_NOT)
      nodeName += " !";
    else if (astnode->node_type == ast_operator_type::AST_UNARYOP_MINUS)
      nodeName += " -";
    else if (astnode->node_type == ast_operator_type::AST_UNARYOP_PLUS)
      nodeName += " +";
    break;
  default:
    // if(astnode->IsLiteral)
    // {
    // 	if(astnode->BasicType == 2) nodeName = to_string((int32_t)
    // astnode->integer_val); 	else 		nodeName =
    // to_string(astnode->float_val); 	printf("%d %d
    // \n",(int)astnode->ast_node_Type,(int)astnode->OriginalType); 	break;
    // }
    // nodeName = to_string(int(c)) + ":unknown " + astnode->name;
    // printf("%d:%d %d
    // \n",c,(int)astnode->ast_node_Type,(int)astnode->OriginalType); c++;
    break;
  }
  /*if(nodeName == " ")
                  printf("%d %d
     \n",(int)astnode->ast_node_Type,(int)astnode->OriginalType);
  */
  if (astnode->OriginalType == ConstInitValK)
    nodeName += " ConstInitValK";
  if (astnode->OriginalType == InitValK)
    nodeName += " InitValK";
  if (astnode->OriginalType == FuncDeclK)
    nodeName += " FuncDeclK";
  if (astnode->OriginalType == FuncDefK)
    nodeName += " FuncDefK";
  if (astnode->OriginalType == FuncFParamsK)
    nodeName += " FuncFParamsK";
  if (astnode->OriginalType == FuncFParamK)
    nodeName += " FuncFParamK";
  if (astnode->OriginalType == ExpK)
    nodeName += " ExpK";
  if (astnode->OriginalType == AddExpK)
    nodeName += " AddExpK";
  if (astnode->OriginalType == LValK)
    nodeName += " LValK";
  if (astnode->OriginalType == VarDefK)
    nodeName += " VarDefK";
  if (astnode->OriginalType == ConstDefK)
    nodeName += " ConstDefK";
  if (astnode->OriginalType == AddExpK)
    nodeName += " AddExpK";
  if (astnode->OriginalType == MulExpK)
    nodeName += " MulExpK";
  // if(astnode->ast_node_Type == AT_AssignStmt)
  // nodeName += " AT_AssignStmt";

  // printf("%d ",astnode->ast_node_Type);
  //  + to_string((int)astnode->line_no);
  return nodeName;
}

/// @brief AST遍历的函数类型声明
Agnode_t *graph_visit_ast_node(Agraph_t *, ast_node *);

/// @brief 叶子节点图形产生
/// @param g graphviz的Agraph_t
/// @param astnode 叶子节点
/// @return 创建的图形节点
Agnode_t *genLeafGraphNode(Agraph_t *g, ast_node *astnode) {
  // 新建结点，不指定名字
  // 第二个参数不指定名字则采用匿名，自动创建一个唯一的名字
  // 第三个参数若为1则g中没有找到则创建；若为0，则在g中根据第二个参数查找，找到返回有效值，否则返回NULL
  Agnode_t *node = agnode(g, (char *)nullptr, 1);
  if (node != nullptr) {

    // 获取叶子结点对应的名字
    string nodeName = getNodeName(astnode);

    // 设置文本的颜色与字体
    agsafeset(node, (char *)"fontcolor", (char *)"black", (char *)"");
    agsafeset(node, (char *)"fontname", (char *)"SimSun", (char *)"");

    // 设置节点的label，在节点内显示
    agsafeset(node, (char *)"label", (char *)nodeName.c_str(), "");

    // 设置节点的形状，矩形框
    agsafeset(node, (char *)"shape", (char *)"record", (char *)"");

    // 设置矩形框内的填充色，红色。必须线设置style，后设置fillcolor，否则fillcolor属性设置无效
    agsafeset(node, (char *)"style", (char *)"filled", (char *)"");
    agsafeset(node, (char *)"fillcolor", (char *)"yellow", (char *)"");
  }

  return node;
}

/// @brief 内部节点图形产生，包含父子关系的边等
/// @param g graphviz的Agraph_t
/// @param astnode 内部节点
/// @return 创建的图形节点
Agnode_t *genInternalGraphNode(Agraph_t *g, ast_node *astnode) {
  std::vector<Agnode_t *> son_nodes;

  // 遍历AST内部结点的孩子，获取创建孩子的图形结点，递归
  // 这里用到了C++向量的容器遍历方法之一，从头开始到尾部
  std::vector<ast_node *>::iterator pIter;
  for (pIter = astnode->sons.begin(); pIter != astnode->sons.end(); ++pIter) {

    Agnode_t *son_node = graph_visit_ast_node(g, *pIter);
    son_nodes.push_back(son_node);
  }

  // 创建一个父节点
  Agnode_t *node = agnode(g, (char *)nullptr, 1);
  if (node != nullptr) {

    // 内部结点对应的运算符名称
    string nodeName = getNodeName(astnode);

    // 设置图形节点的属性，节点内文本label和节点形状
    agsafeset(node, (char *)"label", (char *)nodeName.c_str(), (char *)"");
    agsafeset(node, (char *)"shape", (char *)"ellipse", (char *)"");

    // 本结点与孩子按照自左往右构造图形的边agedge
    // 这里演示C++的另外一种遍历方式，采用C++的新语法for
    // 如果指针类型可不用引用，否则请用引用，避免C++的复制操作带来的性能损失
    // 利用C++的auto关键字，让编译器自动推导类型
    for (auto son_node : son_nodes) {

      // 创建一条边，关联两个节点，假定A和B，边为AB，边没有指定名字，则由函数内部创建唯一名称
      // 第二个参数：边的第一个节点A
      // 第二个参数：边的第二个节点B
      // 第三个参数：指定边的名字，用于定位，这里不需要，指定空即可
      // 第四个参数：若为1，则指定名称的边不存在则创建；若为0，则指定的名称的边不创建
      agedge(g, node, son_node, (char *)nullptr, 1);
    }
  }

  return node;
}

/// @brief 遍历抽象语法树节点产生图片，递归
/// @param g graphviz的Agraph_t
/// @param astnode AST的节点
/// @return 创建的图形
Agnode_t *graph_visit_ast_node(Agraph_t *g, ast_node *astnode) {
  // 非法节点
  if (nullptr == astnode) {
    return nullptr;
  }

  Agnode_t *graph_node;
  graph_node = genInternalGraphNode(g, astnode);
  /*
      if (!isLeafNode(astnode->node_type)) {
          // 叶子节点
          graph_node = genLeafGraphNode(g, astnode);
      } else {
          // 内部运算符节点
          graph_node = genInternalGraphNode(g, astnode);
      }
  */
  return graph_node;
}

/// @brief 抽象语法树AST的图形化显示，这里用C语言实现
/// @param root 抽象语法树的根
/// @param filePath
/// 转换成图形的文件名，主要要通过文件名后缀来区分图片的类型，如png，svg，pdf等皆可
void OutputAST(ast_node *root, const std::string filePath) {
  // 创建GV的上下文
  GVC_t *gv = gvContext();

  // 创建一个图形，Agdirected指明有向图
  Agraph_t *g = agopen((char *)"ast", Agdirected, nullptr);

  // 设置graph的属性
  // agsafeset(g, (char *)"rankdir", (char *)"LR", (char *)"LR");

  // 指定输出的图像质量
  agsafeset(g, (char *)"dpi", (char *)"600", (char *)"");

  // 遍历AST产生图形的结点以及相关边
  (void)graph_visit_ast_node(g, root);

  // 设置图形的布局
  gvLayout(gv, g, "dot");

  // 解析文件名的后缀。由于gvRenderFilename要根据后缀来判断产生什么类型的图片，默认png
  string fileExtName;

  string::size_type pos = filePath.find_last_of('.');
  if (pos == string::npos) {
    fileExtName = "png";
  } else {
    fileExtName = filePath.substr(pos + 1);
  }

  // 输出到一个文件中，png格式
  gvRenderFilename(gv, g, fileExtName.c_str(), filePath.c_str());

  // 关闭图形上下文，并清理资源
  gvFreeLayout(gv, g);
  agclose(g);
  gvFreeContext(gv);
}
#else
void OutputAST(ast_node *root, const std::string filePath) {}
#endif