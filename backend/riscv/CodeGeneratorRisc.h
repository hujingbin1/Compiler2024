#pragma once
#include <string>
#include "IRInst.h"
#include "Value.h"
#include "Function.h"
#include "SymbolTable.h"
#include "CodeGenerator.h"
#include "CodeGeneratorAsm.h"
#include "RiscCode.h"

class CodeGeneratorRisc : public CodeGeneratorAsm {
public:
    CodeGeneratorRisc(SymbolTable & tab);
    ~CodeGeneratorRisc();
    void generateCode(std::vector<IRInst *> & inst_seq, Function * func);
    void genCodeSection(Function * func) override;
    void genHeader() override;
    void genDataSection() override;
    void registerAllocation(Function * fun) override;
    //字节对齐
    int byteAlign(Value * var);
    void translate_entry(IRInst * inst);
    void translate_exit(IRInst * inst, Function * fun);
    void translate_label(IRInst * inst);
    void translate_br(IRInst * inst);
    void translate_bc(IRInst * inst);
    void translate_addi(IRInst * inst);
    void translate_add(IRInst * inst);
    void translate_fadd(IRInst * inst);
    void translate_subi(IRInst * inst);
    void translate_sub(IRInst * inst);
    void translate_fsub(IRInst * inst);
    void translate_muli(IRInst * inst);
    void translate_mul(IRInst * inst);
    void translate_fmul(IRInst * inst);
    void translate_divi(IRInst * inst);
    void translate_div(IRInst * inst);
    void translate_fdiv(IRInst * inst);
    void translate_remi(IRInst * inst);
    void translate_assign(IRInst * inst);
    void translate_funcall(IRInst * inst);
    void translate_fundef(IRInst * inst);
    void translate_store(IRInst * inst);
    void translate_load(IRInst * inst);
    void translate_alloca(IRInst * inst);
    void translate_getptr(IRInst * inst);
    void translate_cmp_eq(IRInst * inst);
    void translate_cmp_neq(IRInst * inst);
    void translate_cmp_gt(IRInst * inst);
    void translate_cmp_ge(IRInst * inst);
    void translate_cmp_lt(IRInst * inst);
    void translate_cmp_le(IRInst * inst);
    void translate_zext(IRInst * inst);
    void translate_sext(IRInst * inst);
    void translate_max(IRInst * inst);

protected:
    bool isGlobal(Value * var);
    bool isGlobalTemp(Value * var);
    void load_var(Value * var, int32_t reg);
    void store_var(Value * var, int32_t reg);
    int32_t getReg(Value * var, int32_t reg);
    std::vector<RiscInst *> code_seq;
    std::vector<float> real_const;
};

// 8f64b9b7