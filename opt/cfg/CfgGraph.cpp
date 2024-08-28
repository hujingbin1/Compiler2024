/**
 * @file CfgGraph.cpp
 * @author 胡景斌
 * @brief 利用graphviz图形化显示CFG，本文件采用C语言实现，没有采用C++的类实现
 * @version 0.1
 * @date 2024-6-24
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifdef USE_GRAPHVIZ
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <gvc.h>

#include "BasicBlocks.h"
#include "CfgGraph.h"
#include "Function.h"
#include "SymbolTable.h"


using namespace std;

/// @author 胡景斌
/// @brief CFG控制流的图形化显示，这里用C语言实现
/// @param SymbolTable 符号表
/// @param filePath
/// 转换成图形的文件名，主要要通过文件名后缀来区分图片的类型，如png，svg，pdf等皆可

void OutputCFG(
    std::string filePath, BasicBlocks *basicblocks,
    std::unordered_map<std::string, std::unordered_map<std::string, Agnode_t *>>
        &node_list)
{

  // 创建GV的上下文
  GVC_t *gv = gvContext();

  // 创建一个图形，Agdirected指明有向图
  Agraph_t *g = agopen((char *)"cfg", Agdirected, nullptr);

  // 指定输出的图像质量
  agsafeset(g, (char *)"dpi", (char *)"600", (char *)"");

  std::string const &entry = "entry ";
  std::string const &exit = "exit ";

  // 获得函数名，按道理来说，我在basicblocks中已经有函数名信息了，但是不知道为啥用first会报错，只能先这样
  std::vector<std::string> func_list;
  for (auto const &basicblock : basicblocks->getBasicBlocks())
  {
    func_list.push_back(basicblock.first);
  }

  for (auto const &func : func_list)
  {
    // 新建结点，不指定名字
    // 第二个参数不指定名字则采用匿名，自动创建一个唯一的名字
    // 第三个参数若为1则g中没有找到则创建；若为0，则在g中根据第二个参数查找，找到返回有效值，否则返回NULL
    Agnode_t *entry_node = agnode(g, (char *)(entry + func).c_str(), 1);

    Agnode_t *exit_node = agnode(g, (char *)(exit + func).c_str(), 1);

    // entry_node风格设定
    //  设置文本的颜色与字体
    agsafeset(entry_node, (char *)"fontcolor", (char *)"black", (char *)"");
    agsafeset(entry_node, (char *)"fontname", (char *)"SimSun", (char *)"");

    // 设置节点的形状，矩形框
    agsafeset(entry_node, (char *)"shape", (char *)"record", (char *)"");

    // 设置矩形框内的填充色，红色。必须线设置style，后设置fillcolor，否则fillcolor属性设置无效
    agsafeset(entry_node, (char *)"style", (char *)"filled", (char *)"");
    agsafeset(entry_node, (char *)"fillcolor", (char *)"yellow", (char *)"");

    // exit_node风格设定
    // 设置文本的颜色与字体
    agsafeset(exit_node, (char *)"fontcolor", (char *)"black", (char *)"");
    agsafeset(exit_node, (char *)"fontname", (char *)"SimSun", (char *)"");

    // 设置节点的形状，矩形框
    agsafeset(exit_node, (char *)"shape", (char *)"record", (char *)"");

    // 设置矩形框内的填充色，红色。必须线设置style，后设置fillcolor，否则fillcolor属性设置无效
    agsafeset(exit_node, (char *)"style", (char *)"filled", (char *)"");
    agsafeset(exit_node, (char *)"fillcolor", (char *)"yellow", (char *)"");

    // 单独设置出入口

    agsafeset(entry_node, (char *)"label", (char *)(entry + func).c_str(),
              (char *)"");
    agsafeset(entry_node, (char *)"shape", (char *)"ellipse", (char *)"");
    agsafeset(exit_node, (char *)"label", (char *)(exit + func).c_str(),
              (char *)"");
    agsafeset(exit_node, (char *)"shape", (char *)"ellipse", (char *)"");

    node_list[func][entry + func] = entry_node;
    node_list[func][exit + func] = exit_node;
    if (entry_node != nullptr && exit_node != nullptr)
    {
      for (auto &basicblock : *basicblocks->getBasicBlocksByFuncName(func))
      {
        if (!basicblock.getCanDelete())
        {
          // entry_node已经添加 无需再添加
          if (basicblock.getEntryName() == entry + func)
          {
            continue;
          }
          Agnode_t *inter_node = agnode(g, (char *)nullptr, 1);
          if (inter_node != nullptr)
          {
            // 将basicblock的IR存在inter_node中
            agsafeset(inter_node, (char *)"label",
                      (char *)basicblock.getString().c_str(), (char *)"");
            agsafeset(inter_node, (char *)"shape", (char *)"ellipse",
                      (char *)"");
            node_list[func][basicblock.getEntryName()] = inter_node;
          }
        }
      }
    }
  }
  for (auto const &func : func_list)
  {
    // 加边
    for (auto &basicblock : *basicblocks->getBasicBlocksByFuncName(func))
    {
      // 创建一条边，关联两个节点，假定A和B，边为AB，边没有指定名字，则由函数内部创建唯一名称
      // 第二个参数：边的第一个节点A
      // 第二个参数：边的第二个节点B
      // 第三个参数：指定边的名字，用于定位，这里不需要，指定空即可
      // 第四个参数：若为1，则指定名称的边不存在则创建；若为0，则指定的名称的边不创建
      // size为2说明是bc指令
      if (!basicblock.getCanDelete())
      {
        if (basicblock.getExitName().size() == 2)
        {
          Agedge_t *edgeTrue =
              agedge(g, node_list[func][basicblock.getEntryName()],
                     node_list[func][basicblock.getExitName()[0]],
                     (char *)func.c_str(), 1);
          agsafeset(edgeTrue, (char *)"label", (char *)"true", (char *)"");

          Agedge_t *edgeFalse =
              agedge(g, node_list[func][basicblock.getEntryName()],
                     node_list[func][basicblock.getExitName()[1]],
                     (char *)func.c_str(), 1);
          agsafeset(edgeFalse, (char *)"label", (char *)"false", (char *)"");
        }
        else
        {
          Agedge_t *edge = agedge(g, node_list[func][basicblock.getEntryName()],
                                  node_list[func][basicblock.getExitName()[0]],
                                  (char *)nullptr, 1);
          agsafeset(edge, (char *)"label", (char *)nullptr, (char *)"");
        }
      }
    }
  }

  // 设置图形的布局
  gvLayout(gv, g, "dot");
  // 解析文件名的后缀。由于gvRenderFilename要根据后缀来判断产生什么类型的图片，默认png
  string fileExtName;

  string::size_type pos = filePath.find_last_of('.');
  if (pos == string::npos)
  {
    fileExtName = "png";
  }
  else
  {
    fileExtName = filePath.substr(pos + 1);
  }

  // 输出到一个文件中，png格式
  gvRenderFilename(gv, g, fileExtName.c_str(), filePath.c_str());

  // 关闭图形上下文，并清理资源
  gvFreeLayout(gv, g);
  agclose(g);
  gvFreeContext(gv);
}

#endif
