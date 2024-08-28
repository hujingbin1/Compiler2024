#include "RiscCode.h"

//这里修改了fp——>s0
std::string RiscInst::regname[MAXREG] = {"x0", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "fp", "s1", "a0",
                                         "a1", "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
                                         "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

std::string RiscInst::f_regname[MAXREG] = {
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
    "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
};

std::string RiscInst::toString()
{
    std::string ret;
    switch (opcode) {
        case InstType::lw:
            ret = "\tlw " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::ld:
            ret = "\tld " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::sw:
            ret = "\tsw " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::sd:
            ret = "\tsd " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::flw:
            ret = "\tflw " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::fld:
            ret = "\tfld " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::fsw:
            ret = "\tfsw " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::fsd:
            ret = "\tfsd " + rst + ", " + arg2 + "(" + arg1 + ")";
            break;
        case InstType::mv:
            ret = "\tmv " + rst + ", " + arg1;
            break;
        case InstType::lui:
            ret = "\tlui " + rst + ", " + arg2;
            break;
        case InstType::neg:
            ret = "\tneg " + rst + ", " + arg1;
            break;
        case InstType::add:
            ret = "\tadd " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::fadd_d:
            ret = "\tfadd.d " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::sub:
            ret = "\tsub " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::fsub_d:
            ret = "\tfsub.d " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::mul:
            ret = "\tmul " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::fmul_d:
            ret = "\tfmul.d " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::div:
            ret = "\tdiv " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::fdiv_d:
            ret = "\tfdiv.d " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::rem:
            ret = "\trem " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::slt:
            ret = "\tslt " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::OR:
            ret = "\tor " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::AND:
            ret = "\tand " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::XOR:
            ret = "\txor " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::NOT:
            ret = "\tnot " + rst + ", " + arg1;
            break;
        case InstType::addi:
            ret = "\taddi " + rst + ", " + arg1 + ", " + arg2;
            ;
            break;
        case InstType::subi:
            ret = "\tsubi " + rst + ", " + arg1 + ", " + arg2;
            break;
        case InstType::li:
            ret = "\tli " + rst + ", " + arg2;
            break;
        case InstType::push:
            ret = "\tpush " + rst;
            break;
        case InstType::pop:
            ret = "\tpop " + rst;
            break;
        case InstType::jal:
            ret = "\tjal " + rst;
            break;
        case InstType::call:
            ret = "\tcall " + rst;
            break;
        case InstType::jalr:
            ret = "\tjalr " + rst;
            break;
        case InstType::beq:
            ret = "\tbeq " + arg1 + ", " + arg2 + ", " + rst;
            break;
        case InstType::bne:
            ret = "\tbne " + arg1 + ", " + arg2 + ", " + rst;
            break;
        case InstType::blt:
            ret = "\tblt " + arg1 + ", " + arg2 + ", " + rst;
            break;
        case InstType::bgt:
            ret = "\tbgt " + arg1 + ", " + arg2 + ", " + rst;
            break;
        case InstType::ble:
            ret = "\tble " + arg1 + ", " + arg2 + ", " + rst;
            break;
        case InstType::bge:
            ret = "\tbge " + arg1 + ", " + arg2 + ", " + rst;
            break;
        case InstType::label:
            ret = rst + ":";
            break;
        case InstType::sext_w:
            ret = "\tsext.w " + rst + ", " + arg1;
            break;
        case InstType::seqz:
            ret = "\tseqz " + rst + ", " + arg1;
            break;
        case InstType::snez:
            ret = "\tsnez " + rst + ", " + arg1;
            break;
        case InstType::fcvt_d_w:
            ret = "\tfcvt_d_w " + rst + ", " + arg1;
            break;
        case InstType::fcvt_w_d:
            ret = "\tfcvt_w_d " + rst + ", " + arg1;
            break;
        case InstType::lla:
            ret = "\tlla " + rst + ", " + arg1;
            break;
        default:
            break;
    }
    return ret;
}