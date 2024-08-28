#pragma once
#include <string>
#include "Value.h"
#define MAXREG 32
#define REG_SP 2
#define REG_FP 8
#define REG_RA 1

enum class InstType {
    lw,
    flw,
    ld,
    fld,
    sw,
    fsw,
    sd,
    fsd,
    mv,
    neg,
    add,
    fadd_d,
    sub,
    fsub_d,
    mul,
    fmul_d,
    div,
    fdiv_d,
    rem,
    slt,
    XOR,
    AND,
    OR,
    NOT,
    addi,
    subi,
    lui,
    li,
    push,
    pop,
    jal,
    call,
    jalr,
    beq,
    bne,
    bge,
    ble,
    blt,
    bgt,
    label,
    seqz,
    snez,
    sext_w,
    fcvt_d_w,
    fcvt_w_d,
    lla
};

class RiscInst {
public:
    static std::string regname[MAXREG];

    static std::string f_regname[MAXREG];

    enum InstType opcode;

    std::string rst;

    std::string arg1;

    std::string arg2;

    RiscInst()
    {}

    RiscInst(InstType opcode, std::string rst, std::string arg1, std::string arg2)
        : opcode(opcode), rst(rst), arg1(arg1), arg2(arg2)
    {}

    std::string toString();
};