
// opCodes 80 to 83 : ADD OR ADC SBB AND_OPCODE SUB XOR CMP operating with either a direct register, or indirect memory operand on a immediate operand
#define Compile_ADDI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( ADD, mod, operandReg, offset, immediateData, size )
#define Compile_ORI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( OR, mod, operandReg, offset, immediateData, size )
#define Compile_ADCI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( ADC, mod, operandReg, offset, immediateData, size )
#define Compile_SBBI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( SBB, mod, operandReg, offset, immediateData, size )
#define Compile_ANDI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( AND, mod, operandReg, offset, immediateData, size )
#define Compile_SUBI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( SUB, mod, operandReg, offset, immediateData, size )
#define Compile_XORI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( XOR, mod, operandReg, offset, immediateData, size )
#define Compile_CMPI( mod, operandReg, offset, immediateData, size ) _Compile_X_Group1_Immediate ( CMP, mod, operandReg, offset, immediateData, size )

#define _Compile_ADD_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( ADD, dstReg, srcReg )
#define _Compile_SUB_Reg_From_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( SUB, dstReg, srcReg )
#define _Compile_OR_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( OR, dstReg, srcReg )
#define _Compile_AND_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( AND, dstReg, srcReg )
#define _Compile_CMP_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( CMP, dstReg, srcReg )
#define _Compile_XOR_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( XOR, dstReg, srcReg )
#define _Compile_SBB_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( SBB, dstReg, srcReg )
#define _Compile_ADC_Reg_To_Reg( dstReg, srcReg ) _Compile_X_Group1_Reg_To_Reg ( ADC, dstReg, srcReg )

//#define _Compile_TEST_Reg_To_Reg( dstReg, srcReg ) _Compile_Op_Special_Reg_To_Reg ( TEST_R_TO_R, dstReg, srcReg )
//#define _Compile_XCHG_Reg_To_Reg( dstReg, srcReg ) _Compile_Op_Special_Reg_To_Reg ( XCHG_R_TO_R, dstReg, srcReg )


#define Compile_ADD( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( ADD, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_OR( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( OR, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_ADC( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( ADC, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_SBB( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( SBB, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_AND( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( AND, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_SUB( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( SUB, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_XOR( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( XOR, toRegOrMem, mod, reg, rm, sib, disp, isize )
#define Compile_CMP( toRegOrMem, mod, reg, rm, sib, disp, isize ) _Compile_X_Group1 ( CMP, toRegOrMem, mod, reg, rm, sib, disp, isize )

#define Compile_AND_BYTE( toRegOrMem, mod, reg, rm, sib, disp ) _Compile_Group1_BYTE ( AND_OPCODE, toRegOrMem, mod, reg, rm, sib, disp )

#define CfrtTil_Compile_JG( jmpToAddr ) Compile_Jcc (GREATER, 0, jmpToAddr )
#define CfrtTil_Compile_JGE( jmpToAddr ) Compile_Jcc (TTT_LESS, NOT_ZERO, jmpToAddr )
#define CfrtTil_Compile_JEQ( jmpToAddr ) Compile_Jcc (TTT_EQUAL, 0, jmpToAddr )
#define CfrtTil_Compile_JNE( jmpToAddr ) Compile_Jcc (TTT_EQUAL, NOT_ZERO, jmpToAddr )
#define CfrtTil_Compile_JNG( jmpToAddr ) Compile_Jcc (GREATER, NOT_ZERO, jmpToAddr )
#define CfrtTil_Compile_JNB( jmpToAddr ) Compile_Jcc (TTT_BELOW, NOT_ZERO, jmpToAddr )
#define CfrtTil_Compile_JB( jmpToAddr ) Compile_Jcc (TTT_BELOW, 0, jmpToAddr )
#define CfrtTil_Compile_JNL( jmpToAddr ) Compile_Jcc (TTT_LESS, NOT_ZERO, jmpToAddr )
#define CfrtTil_Compile_JLE( jmpToAddr ) Compile_Jcc (GREATER, NOT_ZERO, jmpToAddr )
#define CfrtTil_Compile_JL( jmpToAddr ) Compile_Jcc (TTT_LESS, 0, jmpToAddr )

// group 2
#define Compile_ROL( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, ROL, rm, 0, sib, disp, imm )
#define Compile_ROR( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, ROR, rm, 0, sib, disp, imm )
#define Compile_RCL( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, RCL, rm, 0, sib, disp, imm )
#define Compile_RCR( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, RCR, rm, 0, sib, disp, imm )
#define Compile_SHL( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, SHL, rm, 0, sib, disp, imm )
#define Compile_SHR( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, SHR, rm, 0, sib, disp, imm )
#define Compile_SAL( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, SAL, rm, 0, sib, disp, imm )
#define Compile_SAR( mod, rm, sib, disp, imm ) _Compile_Group2 ( mod, SAR, rm, 0, sib, disp, imm )
#define Compile_ROL_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, ROL, rm, 0, sib, disp )
#define Compile_ROR_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, ROR, rm, 0, sib, disp )
#define Compile_RLC_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, RCL, rm, 0, sib, disp )
#define Compile_RCR_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, RCR, rm, 0, sib, disp )
#define Compile_SHL_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, SHL, rm, 0, sib, disp )
#define Compile_SHR_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, SHR, rm, 0, sib, disp )
#define Compile_SAL_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, SAL, rm, 0, sib, disp )
#define Compile_SAR_CL( mod, rm, sib, disp ) _Compile_Group2_CL ( mod, SAR, rm, 0, sib, disp )

//group 3
// div eax, src ::=> eax/(src - modRm) quotient in eax remainder in edx
#define Compile_DIV( mod, rm, controlFlag, sib, disp, imm, size ) _Compile_Group3 (DIV, mod, rm, REX_B | MODRM_B | controlFlag, sib, disp, imm, size )
#define Compile_IDIV( mod, rm, controlFlag, sib, disp, imm, size ) _Compile_Group3 (IDIV, mod, rm, REX_B | MODRM_B | controlFlag, sib, disp, imm, size )
// mul eax, src ::=> eax = eax * modRm src
#define Compile_MUL( mod, rm, controlFlag, sib, disp, imm, size ) _Compile_Group3 (MUL, mod, rm, REX_B | MODRM_B | controlFlag, sib, disp, imm, size )
// two's complement negation
#define Compile_NEG( mod, rm, controlFlag, sib, disp ) _Compile_Group3 (NEG, mod, rm, REX_B | MODRM_B | controlFlag, sib, disp, 0, 0 )
// one's complement negation
#define Compile_NOT( mod, rm, controlFlag, sib, disp ) _Compile_Group3 (NOT, mod, rm, REX_B | MODRM_B | controlFlag, sib, disp, 0, 0 )
// logical compare
#define Compile_TEST( mod, rm, controlFlag, sib, disp, imm, size ) _Compile_Group3 (TEST, mod, rm, REX_B | MODRM_B | controlFlag, sib, disp, imm, size )

// group 5
#define Compile_INC( mod, rm, sib, disp ) _Compile_Group5 ( INC, mod, rm, sib, disp, 0  )
#define Compile_DEC( mod, rm, sib, disp ) _Compile_Group5 ( DEC, mod, rm, sib, disp, 0  )
#define Compile_PUSH( mod, rm, sib, disp ) _Compile_Group5 ( PUSH, mod, rm, sib, disp, 0 )

#define Compile_POP( mod, rm, sib, disp ) _Compile_GroupPushPop ( POP, mod, rm, sib, disp, 0 )

#define _Compile_TestImm_0xffffffff( mod, reg, rm, sib, disp ) _Compile_Test ( mod, reg, rm, IMM_B, disp, 0xffffffff )
#if 0
#define _Compile_TestImm_0xffffffff_R8( ) _CompileTest ( REG, 0, R8, 0, 0xffffffff )
#define _Compile_TestImm_0_R8( ) _CompileTest ( REG, 0, R8, 0, 0 )
#define _Compile_TestImm_0_Reg( rmReg ) _CompileTest ( REG, 0, rmReg, 0, 0 )
#define _Compile_TestImm_0_Rm( rm, sib, disp ) _Compile_Test ( MEM, 0, rm, disp, 0 )
#endif
#define  PUSH_IMM    0x68      
#define _Compile_PushRspImm( data ) _Compile_Write_Instruction_X64 (0, 0, PUSH_IMM, 0, 0, 0, 0, 0, 0, CELL_SIZE )


