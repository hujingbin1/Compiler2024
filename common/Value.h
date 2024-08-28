/**
 * @file value.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 变量以及常量等Value管理的头文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "Common.h"
#include "ValueType.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

extern int countTemp;

class _numpy_ {
public:
  std::vector<int> np_sizes;
  //如果是a[1][2][3]那么sizes顺序为3-2-1
  int *numpy_int = nullptr;
  float *numpy_float = nullptr;
  int len = 0;
  //记录数组实际插入了几个元素
  int cnt = 0;
  bool is_int = 0;

  bool _flag = 0;

  int32_t offset = 0;

  bool is_Store = false;
  //数组名
  std::string np_name = " ";
  // 1的时候代表着是数组形参

  _numpy_(int lenght, BasicType _type) {
    switch (_type) {
    case BasicType::TYPE_INT:
      numpy_int = new int[lenght];
      for (int i = 0; i < lenght; i++) {
        numpy_int[i] = 0;
      }
      is_int = 1;
      len = lenght;
      break;
    case BasicType::TYPE_FLOAT:
      numpy_float = new float[lenght];
      for (int i = 0; i < lenght; i++) {
        numpy_int[i] = 0;
      }
      is_int = 0;
      len = lenght;
      break;
    default:
      break;
    }
  }

  int if_isint() { return is_int; }
};

/// @brief 变量、常量等管理的基类
class Value {

public:
  static uint64_t VarCount; // 局部变量、临时变量、内存变量计数，默认从0开始
  static uint64_t ConstCount;  // 常量计数，默认从0开始
  static uint64_t GlobalCount; // 全局变量计数，默认从0开始
  /// @brief 新建一个常量名字，全局编号，没有考虑常量的重用
  /// \return 常量名字
  static std::string createConstVarName() {
    return "%c" + std::to_string(ConstCount++);
  }

  /// @brief 新建一个全局变量名字，全局编号
  /// \return 临时变量名
  static std::string createGlobalVarName() {
    return "%g" + std::to_string(GlobalCount++);
  }

  /// @brief 新建一个临时变量名，全局编号
  /// \return 临时变量名
  static std::string createTempVarName() {
    countTemp++;
    return "%t" + std::to_string(VarCount++);
  }

  /// @brief 新建一个局部变量名，全局编号，同临时变量一起编号
  /// \return 临时变量名
  static std::string createLocalVarName() {
    countTemp++;
    return "%l" + std::to_string(VarCount++);
  }

  /// @brief 新建一个内存变量名，全局变量，同临时变量一起编号
  /// @return 内存变量名
  static std::string createMemVarName() {
    countTemp++;
    return "%m" + std::to_string(VarCount++);
  }

protected:
  /// @brief 是否是常量
  bool _const = false;

  /// @brief 是否编译器内部产生的临时变量
  bool _temp = false;

  /// @brief 是否是用户定义的变量或标识符
  bool _var = false;

  /// @brief 是否是内存变量
  bool _mem = false;
  /// @brief 是否是字面量
  bool _literal = false;

  bool FParam = 0;
  //形参

  bool is_savenp = false;
  //如果该t是用于 *%t5 = i32 5；则其为1 或者用于取数的，则为1

public:
  /// @brief 变量名或内部标识的名字
  std::string name;

  /// @brief 类型
  ValueType type;

  int MIN_is_three = 0;

  int NOT_is_three = 0;

  _numpy_ *np = nullptr;

  bool is_allassign = 0;
  //全局变量初始化

  bool is_saveFParam = 0;

  bool is_numpy = 0;

  bool is_numpy_assign = false;

  ///@brief 是不是活跃变量
  bool is_active = false;

  ///@brief 记录数组元素的下标
  std::vector<int> indexs;

  ///@brief 记录数组元素的下标(线性)
  int32_t indexLinear;

  /// @brief 整型常量的值
  int32_t intVal = 0;

  /// @brief 实数常量的值
  float realVal = 0;

  /// @brief 寄存器编号，-1表示没有分配寄存器
  int32_t regId = -1;

  /// @brief 变量在栈内的偏移量，对于全局变量默认为0，临时变量没有意义
  int32_t offset = 0;

  /// @brief 栈内寻找时基址寄存器名字
  int32_t baseRegNo = -1;

  /// @brief
  /// SSA优化需要标号——>这里是得到它的原始标号，只要被定义，originTag就要加1
  int originTag;

  /// @brief SSA优化需要标号——>这里是生成Phi函数用 key:block的入口
  /// value:最后一个变量定义的tag
  std::unordered_map<std::string, int> blockTag;

  bool creat_np(int lenght, BasicType _type) {
    np = new _numpy_(lenght, _type);
    if (np != nullptr) {
      is_numpy = 1;
    }
    return is_numpy;
  }

protected:
  /// @brief 默认实数类型的构造函数，初始值为0
  Value(BasicType _type) : type(_type) {
    // 这里不需要代码
  }

  /// @brief 构造函数
  /// @param _name
  /// @param _type
  Value(std::string _name, BasicType _type) : name(_name), type(_type) {
    // 不需要增加代码
  }

public:
  /// @brief 析构函数
  virtual ~Value() {
    // 如有资源清理，请这里追加代码
  }

  /// @brief 获取名字
  /// @return 变量名
  virtual std::string getName() const { return name; }

  /// @brief Value变字符串显示
  /// @return 字符串
  virtual std::string toString() {
    //修改格式
    // return type.toString() + " " + getName();
    return getName();
  }

  /// @brief 获取函数栈内偏移
  /// @return 栈内偏移
  int32_t getOffset() { return offset; }

  /// @brief 设置栈内偏移，需要放在栈内的变量有效
  /// @param 栈内偏移
  void setOffset(int32_t _offset) { offset = _offset; }

  /// @brief 检查变量是否是常量
  /// @return true: 是 false：不是
  bool isConst() { return _const; }

  /// @brief 检查变量是否是临时变量
  /// @return true: 是 false：不是
  bool isTemp() { return _temp; }

  /// @brief 检查变量是否是变量
  /// @return true: 是 false：不是
  bool isLocalVar() { return _var; }

  /// @brief 检查变量是否是变量
  /// @return true: 是 false：不是
  bool isMemVar() { return _mem; }

  /// @brief 检查变量是否是字面量
  /// @return true: 是 false：不是
  bool isliteral() { return _literal; }
  bool is_issavenp() { return is_savenp; }

  bool is_FParam() { return FParam; }
  void setState(bool state) { is_active = state; }

  bool getState() { return is_active; }
  std::vector<int> getNumpyIndex() { return this->indexs; }

  void clearNumpyIndex() { this->indexs.clear(); }
  void insertNumpyIndex(int32_t index) { this->indexs.emplace_back(index); }
  //得到这个变量在这个基本块中最后一次定义的tag
  int getBlockTag(std::string entryName) { return this->blockTag[entryName]; }

  //设置这个变量在基本块中最后一次定义的tag——>用于Phi函数选择
  void setBlockTag(std::string entryName, int tag) {
    this->blockTag[entryName] = tag;
  }

  void setNumpyIndex() {
    //找到数组变量的各个维度索引
    if (this->is_issavenp() && this->isTemp() && this->np != nullptr) {
      int SumIndex = this->getNumpyIndex()[0];
      this->clearNumpyIndex();

      std::vector<int> indices(this->np->np_sizes.size(), 0); // 初始化多维索引

      for (int i = 0; i < this->np->np_sizes.size(); ++i) {
        int dimSize = this->np->np_sizes[i];
        indices[i] = SumIndex % dimSize;
        SumIndex /= dimSize;
      }

      for (int dim = this->np->np_sizes.size() - 1; dim >= 0; dim--) {
        this->insertNumpyIndex(indices[dim]);
      }
    }
  }
  // TODO:这里的方法需要修改
  //设置这个变量的原始tag
  int getOriginTag() { return this->originTag; }

  //得到这个变量的原始tag
  void setOriginTag(int tag) { this->originTag = tag; }

  /// @brief 根据变量类型获取所占空间的大小
  /// @return 空间大小，单位字节
  int32_t getSize() {
    // 这里假定为4字节
    // 实际需要根据ValueType获取大小
    if (is_numpy)
      return 8;

    if (type.type == BasicType::TYPE_INT) {
      // int 4字节
      return 4;
    } else if (type.type == BasicType::TYPE_FLOAT) {
      // float 4字节
      //修改为8字节
      return 8;
    }

    // 其它类型暂不支持，统一返回-1
    return -1;
  }
  void SetConst() {
    _const = true;

    _temp = false;

    _var = false;

    _mem = false;

    _literal = false;
  }

  void SetLiteral() {
    _const = true;

    _temp = false;

    _var = false;

    _mem = false;

    _literal = true;
  }
  void set_is_savenp() { is_savenp = 1; }

  void set_FParam() { FParam = 1; }
};

/// @brief 临时变量类
class TempValue : public Value {

public:
  /// @brief 创建临时Value，用于保存中间IR指令的值
  /// \param val
  TempValue(BasicType _type) : Value(_type) {
    _temp = true;
    name = createTempVarName();
  }

  /// @brief 创建临时Value，用于保存中间IR指令的值
  /// \param val
  TempValue() : Value(BasicType::TYPE_FLOAT) {
    _temp = true;
    name = createTempVarName();
  }

  /// @brief 析构函数
  ~TempValue() override {
    // 如有资源清理，请这里追加代码
  }
};

/// @brief 常量类
class ConstValue : public Value {

public:
  /// @brief 整数的临时变量值
  /// \param val
  ConstValue(int32_t val) : Value(BasicType::TYPE_INT) {
    _const = true;
    _literal = true;
    name = std::to_string(val);
    intVal = val;
  }

  /// @brief 实数的临时变量值
  /// \param val
  ConstValue(float val) : Value(BasicType::TYPE_FLOAT) {
    _const = true;
    _literal = true;
    name = std::to_string(val);
    realVal = val;
  }

  /// @brief 析构函数
  ~ConstValue() override {
    // 如有资源清理，请这里追加代码
  }

  /// @brief 获取名字
  /// @return
  std::string getName() const override {
    if (type.type == BasicType::TYPE_INT) {
      return int2str(this->intVal);
    } else {
      return double2str(this->realVal);
    }
  }
};

/// @brief 变量类
class VarValue : public Value {

public:
  /// @brief 创建临时Value，用于保存中间IR指令的值
  /// \param val
  VarValue(std::string _name, BasicType _type) : Value(_name, _type) {
    _var = true;
  }

  /// @brief 创建临时Value，用于保存中间IR指令的值
  /// \param val
  VarValue(BasicType type) : Value(type) {
    name = createLocalVarName();
    _var = true;
  }

  /// @brief 析构函数
  ~VarValue() override {
    // 如有资源清理，请这里追加代码
  }

  // /// @brief 重写变量的tostring方法——>感觉不好改，算了
  // std::string toString() override
  // {
  //     return getName() + "_" + "%%%";
  // }
};

/// @brief 变量类
class IntRegValue : public Value {

public:
  /// @brief 创建临时Value，用于保存中间IR指令的值
  /// \param val
  IntRegValue(std::string _name, int32_t _reg_no)
      : Value(_name, BasicType::TYPE_INT) {
    _var = true;
    regId = _reg_no;
  }

  /// @brief 析构函数
  ~IntRegValue() override {
    // 如有资源清理，请这里追加代码
  }
};

/// @brief 内存值，必须在内存中
class MemValue : public Value {

public:
  /// @brief 创建内存Value，用于保存中间IR指令的值
  /// \param val
  MemValue(BasicType type) : Value(type) {
    name = createMemVarName();
    _mem = true;
  }

  /// @brief 析构函数
  ~MemValue() override {
    // 如有资源清理，请这里追加代码
  }
};

/// @brief 寄存器Value，该值保存在寄存器中
class RegValue : public Value {

public:
  /// @brief 创建寄存器Value
  /// \param reg_no 寄存器编号
  RegValue(int reg_no, std::string reg_name) : Value(BasicType::TYPE_INT) {
    name = reg_name;
    regId = reg_no;
  }

  /// @brief 析构函数
  ~RegValue() override {
    // 如有资源清理，请这里追加代码
  }
};
