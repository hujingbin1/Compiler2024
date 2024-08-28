# 1.主要功能
该项目实现了一个SysY2022语言的编译器，项目基于C++语言进行开发，可以实现基本的词法分析、语法分析、AST抽象语法树生成、语义分析、中间IR生成、后端RISCV汇编代码生成，并在功能实现的基础上做了部分性能优化，如图着色寄存器分配、控制流分析、数据流分析、SSA静态单赋值形态IR转换等。

注：该项目优化部分参考：https://gitlab.eduxiji.net/educg-group-18973-1895971/carrotcompiler
## 1.1.功能1
前端是基于flex+bison工具进行词法语法解析，并可生成AST抽象语法树，通过遍历抽象语法树，生成中间dragon IR。
## 1.2.功能2
中端基于IR进行了基本块划分、控制流图生成、数据流和控制流分析、SSA的IR转换等优化。
## 1.3.功能3
后端是针对RISCV平台进行了IR到汇编代码的生成，先将变量都存在栈中，然后通过图着色算法做寄存器分配，最终生成RISCV汇编代码。


# 2.软件总体结构
## 2.1.软件开发环境
操作系统：Ubuntu22.04
开发语言：C++
开发工具：VScode
## 2.2.软件运行环境
运行的操作系统：Ubuntu22.04
## 2.3.软件组成架构
fronted/ 目录下为前端代码，包括flex+bison进行词法语法分析，AST生成

DragonIR/ 目录下为中端IR生成的代码，IRGenerator.cpp是生成IR的主要代码文件。

common/ 目录下是通用的数据结构定义的文件、工具类等。

backend/ 目录下是后端代码，backend/riscv/ 目录下是构建RISCV汇编代码的代码文件

opt/ 目录下是进行中端优化的代码，包括SSA转换、控制流和数据流分析等。

## 2.4.源代码组织与构建
fronted/ 目录下为前端代码，包括flex+bison进行词法语法分析，AST生成。

DragonIR/ 目录下为中端IR生成的代码，IRGenerator.cpp是生成IR的主要代码文件。

common/ 目录下是通用的数据结构定义的文件、工具类等。

backend/ 目录下是后端代码，backend/riscv/ 目录下是构建RISCV汇编代码的代码文件

opt/ 目录下是进行中端优化的代码，包括SSA转换、控制流和数据流分析等。

可以通过CMakeLists.txt文件借助CMake工具构建可执行文件calculator，可执行文件可以添加不同的选项选择不同的输出文件，“./ calculator -S -a -o ./ast.png ./test.c”产生AST抽象语法树，“./ calculator -S -I -o ./ir.txt ./test.c”产生中间IR，“./ calculator -S -o ./risc.s ./test.c”产生后端RISCV的汇编代码。


# 3.详细设计
## 3.1.IR模块
主要涉及的数据结构有ast_node、IRInst、Symtab、Value、Funciton等，最重要的就是Symtab符号表的管理。

Symtab的属性说明：

1.///@brief变量名映射表，变量名-变量，只保存全局变量以及常量

2.std::unordered_map<std::string,Value*>varsMap;

3.///@brief只保存全局变量以及常量

4.std::vector<Value*>varsVector;

5.///@brief函数映射表，函数名-函数，便于检索

6.std::unordered_map<std::string,Function*>funcMap;

7.///@brief函数列表

8.std::vector<Function*>funcVector;

这里通过vector和unordered_map组合分别管理全局变量和函数。然后Symtab中主要都是一些方便增删改查的方法封装。其他数据结构也类似，都是一些比较基础封装，相关功能也在注释里写的比较清晰，不再赘述。
主要处理的代码在IRGenerator.cpp中，通过遍历AST，结合ast_node的属性，执行相应的翻译动作函数，将生成的IRCode插入函数中，并在翻译动作函数中根据变量的作用域，将Value插入对应的函数或者symtab中。

## 3.2.RISCV后端模块
这部分主要涉及的数据结构有code_seq、IRInst等，后端代码生成的主要是对code_seq的操作。通过symtab最重要的中间桥梁，遍历符号表中的每个函数，将局部变量和函数形参存在栈中，部分临时变量也存在栈中，尽可能让大部分临时变量存在寄存器中。

所以其实和AST到IR类似，需要执行IRInst到code_seq的转换，需要load_var、store_var、以及相应的指令操作翻译函数。

这部分算法主要用到的是图着色做寄存器分配算法。

## 3.3.中端优化模块
这部涉及的数据结构主要是BasicBlock基本块、Controlflow控制流、SSAConvert等，所有的优化都是基于基本块去做的。然后这部分算法是最多而且最复杂的，主要有SSA转换IR算法、死代码消除算法、支配树算法、回边检测算法、常量传播等。这些算法很多都是参考网上的资料做的，数据结构是自定义的，有的做的不是很理想还有bug。


TODO：文档完善，现在只写了一个框架，详细设计还需要完善。