/**
 * @file main.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 主程序文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */

// TODOsymtab.h写的有问题，在于Func.cpp line161处
// TODO将Func.cpp中的关于全局变量和常数的输出挪到了symtab处
// TODOFunc.cpp line214和217有点怪
#include <getopt.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Function.h"
#include "IRInst.h"


#include "AST.h"
#include "CodeGenerator.h"
#include "CodeGeneratorRisc.h"

#ifdef USE_SIMULATION
#include "CodeSimulator.h"
#endif

#include "FlexBisonExecutor.h"
#include "FrontEndExecutor.h"
#include "Graph.h"
#include "IRGenerator.h"
#include "BasicBlocks.h"
#include "CfgGraph.h"
#include "ControlFlowAnalysis.h"
#include "DataFlowAnalysis.h"
#include "DomainTree.h"
#include "SSAConvert.h"
#include "SymbolTable.h"

using namespace std;
#define FUNCCALL_SIGNATURE 123

/// @brief 归约次数
int nCount;

int c;

/// @brief 是否显示帮助信息
bool gShowHelp = false;

/// @brief 显示抽象语法树，非线性IR
int gShowAST = 0;

/// @brief 产生线性IR，线性IR，默认输出
int gShowLineIR = 0;

/// @brief 输出CFG控制流图
int gShowCFG = 0;

/// @brief 是否进行控制流优化
int controlFlowOpt = 0;

///@brief 是否进行数据流优化
int dataFlowOpt = 0;

/// @brief 显示汇编
int gShowASM = 0;

/// @brief 直接运行，默认运行
int gDirectRun = 0;

/// @brief 输出中间IR，含汇编或者自定义IR等，默认输出线性IR
int gShowSymbol = 0;

/// @brief 前端分析器，默认选Flex和Bison
bool gFrontEndFlexBison = true;

/// @brief 前端分析器Antlr4，是否选中
bool gFrontEndAntlr4 = false;

/// @brief 前端分析器用递归下降分析法，是否选中
bool gFrontEndRecursiveDescentParsing = false;

/// @brief 输入源文件
std::string gInputFile;

/// @brief 输出文件，不同的选项输出的内容不同
std::string gOutputFile;

IRInst *TrueEntry = nullptr;
IRInst *FalseEntry = nullptr;
IRInst *EntryEntry = nullptr;
IRInst *ExitEntry = nullptr;
IRInst *Mode_1_Entry = nullptr;
IRInst *Mode_2_Entry = nullptr;
IRInst *Break_Exit = nullptr;
IRInst *Continue_Entry = nullptr;

int countTemp = 0;
// int countConst = 0;
SymbolTable symtab;
/// @brief 显示帮助
/// @param exeName
void showHelp(const std::string &exeName) {
  std::cout << exeName + " -S [-A | -D| -F] [-a | -I] [-o output] source\n";
  std::cout << exeName + " -R [-A | -D] source\n";
}

/// @brief 参数解析与有效性检查
/// @param argc
/// @param argv
/// @return
int ArgsAnalysis(int argc, char *argv[]) {
  int ch;

  // 指定参数解析的选项，可识别-h、-o、-S、-a、-I、-R、-A、-D、-F选项，并且-o要求必须要有附加参数
  const char options[] = "ho:SaIRADFO";

  opterr = 1;

lb_check:
  while ((ch = getopt(argc, argv, options)) != -1) {
    switch (ch) {
    case 'h':
      gShowHelp = true;
      break;
    case 'o':
      gOutputFile = optarg;
      break;
    case 'S':
      gShowSymbol = 1;
      break;
    case 'a':
      gShowAST = 1;
      break;
    case 'I':
      // 产生中间IR
      gShowLineIR = 1;
      break;
    case 'F':
      // 产生CFG控制流图
      gShowCFG = 1;
      break;
    case 'R':
      // 直接运行，默认运行
      gDirectRun = 1;
      break;
    case 'A':
      // 选用antlr4
      gFrontEndAntlr4 = true;
      gFrontEndFlexBison = false;
      gFrontEndRecursiveDescentParsing = false;
      break;
    case 'D':
      // 选用递归下降分析法与词法手动实现
      gFrontEndAntlr4 = false;
      gFrontEndFlexBison = false;
      gFrontEndRecursiveDescentParsing = true;
      break;
    case 'O':
      // 启动数据流和控制流优化
      gShowCFG = 1;
      controlFlowOpt = 1;
      dataFlowOpt = 1;
      break;
    default:
      return -1;
      break; /* no break */
    }
  }

  argc -= optind;
  argv += optind;

  if (argc >= 1) {

    // 第一次设置
    if (gInputFile.empty()) {

      gInputFile = argv[0];
    } else {
      // 重复设置则出错
      return -1;
    }

    if (argc > 1) {
      // 多余一个参数，则说明输入的源文件后仍然有参数要解析
      optind = 0;
      goto lb_check;
    }
  }

  // 必须指定输入文件和输出文件
  if (gInputFile.length() == 0) {
    return -1;
  }

  // 这三者只能指定一个
  int flag = gShowSymbol + gDirectRun;
  if (flag != 1) {
    // 运行与中间IR只能同时选择一个
    return -1;
  }

  flag = gShowLineIR + gShowAST;

  if (gShowSymbol) {

    if (flag == 0) {
      // 没有指定，则输出汇编指令
      gShowASM = 1;
    } else if (flag != 1) {
      // 线性中间IR、抽象语法树只能同时选择一个
      return -1;
    }
  } else {
    // 如果-S没有指定，但指定了-a等选项时，则失败
    if (flag != 0) {
      return -1;
    }
  }

  if (gOutputFile.empty()) {

    // 默认文件名
    if (gShowAST) {
      gOutputFile = "ast.png";
    } else if (gShowCFG) {
      gOutputFile = "cfg.png";
    } else if (gShowLineIR) {
      gOutputFile = "ir.txt";
    } else {
      gOutputFile = "asm.s";
    }
  } else {

    // 直接运行时，文件不能指定
    if (gDirectRun) {
      return -1;
    }
  }

  return 0;
}

/// @brief 主程序
/// @param argc
/// @param argv
/// @return
int main(int argc, char *argv[]) {
  // 函数返回值，默认-1
  int result = -1;

  // 内部函数调用返回值保存变量
  int subResult;

  // 这里采用do {} while(0)架构的目的是如果处理出错可通过break退出循环，出口唯一
  // 在编译器编译优化时会自动去除，因为while恒假的缘故
  do {

    // 参数解析
    subResult = ArgsAnalysis(argc, argv);
    if (subResult < 0) {

      // 在终端显示程序帮助信息
      showHelp(argv[0]);

      // 错误不用设置，因此result默认值为-1
      break;
    }

    // 显示帮助
    if (gShowHelp) {

      // 在终端显示程序帮助信息
      showHelp(argv[0]);

      // 这里必须设置返回值，因默认值为-1
      result = 0;

      break;
    }

    // 创建词法语法分析器
    FrontEndExecutor *frontEndExecutor;
    // 默认为Flex+Bison
    frontEndExecutor = new FlexBisonExecutor(gInputFile);

    // 前端执行：词法分析、语法分析后产生抽象语法树，其root为全局变量ast_root
    subResult = frontEndExecutor->run();
    if (!subResult) {

      std::cout << "FrontEnd's analysis failed\n";

      // 退出循环
      break;
    }

    // 清理前端资源
    delete frontEndExecutor;

    // 这里可进行非线性AST的优化

    if (gShowAST) {

      // 遍历抽象语法树，生成抽象语法树图片
      OutputAST(ast_root, gOutputFile);

      // 清理抽象语法树
      free_ast();

      // 设置返回结果：正常
      result = 0;

      break;
    }

    // 输出线性中间IR、计算器模拟解释执行、输出汇编指令
    // 都需要遍历AST转换成线性IR指令

    // // 符号表，保存所有的变量以及函数等信息
    // SymbolTable symtab;

    // 遍历抽象语法树产生线性IR，相关信息保存到符号表中
    IRGenerator ast2IR(ast_root, &symtab);
    subResult = ast2IR.run();
    if (!subResult) {

      // 输出错误信息
      std::cout << "GenIR failed\n";

      break;
    }

    // 清理抽象语法树
    free_ast();

    if (gShowLineIR) {

      // 输出IR
      symtab.outputIR(gOutputFile, symtab);

      // 设置返回结果：正常
      result = 0;

      break;
    }
//    // 这里可追加中间代码优化，体系结果无关的优化等
//
//    // dragon ir 格式化
   for (auto func : symtab.getFunctionList()) {
     std::string instStr;
     //这里的instStr保存了整个IR信息，但是我们要进行基本块划分和控制流生成，不需要这个，只是想着刷新符号表命名
     func->toString(instStr, symtab);
   }
//
//    // controlflow用来管理控制流图的边
//    std::unordered_map<std::string,
//                       std::unordered_map<std::string, vector<std::string>>>
//        controlflow;
//#ifdef USE_GRAPHVIZ
//    // node_list用来管理控制流图
//    std::unordered_map<std::string, std::unordered_map<std::string, Agnode_t *>>
//        node_list;
//#endif
//
//    // basicblocks用来管理基本块，管理控制流图的结点
//    //得到基本块
//    BasicBlocks *basicblocks = new BasicBlocks(symtab);
//
//    //得到控制流，具体实现方式是Label的map映射
//    getControlFlow(symtab, controlflow);
//
//    //更新基本块的出口设置,basicblocks中有了控制流信息
//    setControlFlow(basicblocks, controlflow);

    //显示控制流图
    if (gShowCFG) {

//      //输出控制流图——>没有优化的
//      // OutputCFG(gOutputFile, basicblocks, node_list);
//
//      //获得函数名，按道理来说，我在basicblocks中已经有函数名信息了，但是不知道为啥用first会报错，只能先这样
//      std::vector<std::string> func_list;
//      for (auto const &basicblock : basicblocks->getBasicBlocks()) {
//        func_list.push_back(basicblock.first);
//      }
//
//      // 进行控制流的优化_——>domaintree完善了剩下的控制流
//      if (controlFlowOpt) {
//        for (auto const &funcName : func_list) {
//          // 需要删除的基本块 canDelete 属性为true
//          deleteNullBlock(basicblocks, funcName);
//          // 无法到达的基本块 canCanMove
//          // 属性为false。这里通过dfs遍历图结构，找到不可达块。
//          deleteDeadBlock(basicblocks, funcName);
//        }
//      }
//
//      // 进行数据流的优化——>SSA完成后
//      if (dataFlowOpt) {
//        for (auto const &funcName : func_list) {
//          bool direct = true;
//          DataFlowAnalysis *dataflow =
//              new DataFlowAnalysis(basicblocks, direct, funcName);
//          dataflow->init();
//        }
//      }
//
//#ifdef USE_GRAPHVIZ
//      //输出控制流图——>优化后的
//      OutputCFG(gOutputFile, basicblocks, node_list);
//#endif
//
//      // TODO:SSA——>支配树、回边检测、无效块消除、变量重命名已完成
//      DomainTree *domainTree = new DomainTree(basicblocks);
//      domainTree->execute();
//
//      for (auto const &backedge : basicblocks->getBackEdge("main")) {
//        std::cout << "检测到回边：" << std::endl;
//        std::cout << backedge.first << " to " << backedge.second << std::endl;
//      }
//      std::cout << "DF集合：" << std::endl;
//      for (auto const &df : basicblocks->getDfSet("main")) {
//        std::cout << df << std::endl;
//      }
//
//      ConvertSSA *convertSSA = new ConvertSSA(basicblocks);
//      convertSSA->rwDefs();
//      // TODO:phi函数插入、控制流分析（后续右值用的哪个变量）、SSA的IR产生、USE/DEF集合、IN/OUT集合
//      //  设置返回结果：正常
//      result = 0;
//
//      break;
    }

    // 后端处理，体系结果相关的操作
    // 这里提供两种模式：第一种是解释执行器CodeSimulator；第二种为面向ARM32的汇编产生器CodeGeneratorArm32
    // 需要时可根据需要修改或追加新的目标体系架构

    CodeGenerator *generator;

    if (gShowASM) {

      // 输出面向Riscv的汇编指令
      generator = new CodeGeneratorRisc(symtab);
      generator->run(gOutputFile);
      delete generator;
    } else {

#ifdef USE_SIMULATION
      // 遍历中间代码指令，解释执行，得出运算结果
      generator = new CodeSimulator(symtab);
      generator->run(gOutputFile);
      delete generator;
#endif
    }

    // 清理符号表
    symtab.freeValues();

    result = 0;
  } while (false);

  return result;
}
