
#include "../../include/csl.h"

/*
void
Compile_Test ( Compiler * compiler )
{
    if ( CheckOptimizeOperands ( compiler, 5 ) )
    {
        _Compile_Test ( compiler->OptInfo->CO_Mod, compiler->OptInfo->CO_Reg,
                compiler->OptInfo->CO_Rm, compiler->OptInfo->CO_Disp, compiler->OptInfo->CO_Imm ) ;
    }
    else
    {
        _Compile_Stack_PopToReg ( DSP, R8 ) ;
        _Compile_Stack_PopToReg ( DSP, ECX ) ;
        _Compile_Test ( REG, R8, ECX, 0, 0 ) ;
    }
   _Compiler_Setup_BI_tttn ( _Context->Compiler0, ZERO, N ) ;
}

void
Compile_BitWise_TEST ( Compiler * compiler )
{
    Compile_Test ( compiler ) ;
}
 
void
CSL_TEST ( )
{
    if ( GetState( _Context_->Compiler0, BLOCK_MODE ) )
    {
        Compile_BitWise_TEST ( _Context->Compiler0 ) ;
    }
    else
    {
        Error ( ( byte* ) "\n\"TEST\" can be used only in compile mode.", 1 ) ;
    }
}
 */

// opCodes TEST NOT NEG MUL DIV IMUL IDIV // group 3 - F6-F7

void
Compile_X_Group3 ( Compiler * compiler, int64 code ) //OP_1_ARG
{
    int64 optSetupFlag = CO_CheckOptimize (compiler, 0) ; //OP_1_ARG
    if ( optSetupFlag & CO_DONE ) return ;
    else if ( optSetupFlag )
    {
        //_Compile_Group3 ( cell code, cell mod, cell rm, cell sib, cell disp, cell imm, cell size )
        _Compile_Group3 ( code, compiler->OptInfo->CO_Mod,
            compiler->OptInfo->CO_Rm, REX_W | MODRM_B, 0, compiler->OptInfo->CO_Disp, compiler->OptInfo->CO_Imm, 0 ) ;
        if ( compiler->OptInfo->CO_Rm != DSP ) // if the result is not already tos
        {
            if ( compiler->OptInfo->CO_Rm != ACC ) Compile_Move_Rm_To_Reg (ACC, compiler->OptInfo->CO_Rm,
                compiler->OptInfo->CO_Disp , 0) ;
            CSL_CompileAndRecord_PushAccum () ;
        }
    }
    else
    {
        _Compile_Group3 ( code, MEM, DSP, REX_W | MODRM_B, 0, 0, 0, 0 ) ;
    }
}

void
Compile_X_Shift ( Compiler * compiler, int64 op, Boolean stackFlag, Boolean opEqualFlag )
{
    int64 optSetupFlag = CO_CheckOptimize (compiler, 0) ; //OP_1_ARG
    if ( optSetupFlag & CO_DONE ) return ;
    else if ( optSetupFlag )
    {
#if 1       
        // _Compile_Group2 ( int64 mod, int64 regOpCode, int64 rm, int64 sib, cell disp, cell imm )
        if ( compiler->OptInfo->OptimizeFlag & CO_IMM )
        {
            _Compile_Group2 ( compiler->OptInfo->CO_Mod,
                op, compiler->OptInfo->CO_Rm, IMM_B, 0, compiler->OptInfo->CO_Disp, compiler->OptInfo->CO_Imm ) ;
        }
        else //if ( ( compiler->OptInfo->CO_Imm == 0 ) && ( compiler->OptInfo->CO_Rm != ACC ) ) // this logic is prototype maybe not precise 
#endif            
        {
            //DBI_ON ;
            _Compile_Group2_CL ( compiler->OptInfo->CO_Mod, op, compiler->OptInfo->CO_Rm, 0, compiler->OptInfo->CO_Disp ) ;
            //_Compile_Group2 ( compiler->OptInfo->CO_Mod, op, compiler->OptInfo->CO_Rm, 
                //(( compiler->OptInfo->OptimizeFlag & CO_IMM ) ? IMM_B : 0), 0, compiler->OptInfo->CO_Disp, compiler->OptInfo->CO_Imm ) ;
        }
        if ( ( ! opEqualFlag ) && ( stackFlag && (( compiler->OptInfo->CO_Rm != DSP ) && ( compiler->OptInfo->CO_Rm != FP ) )) ) // if the result is not already tos
        {
            if ( compiler->OptInfo->CO_Rm != ACC ) 
            Compile_Move_Rm_To_Reg (ACC, compiler->OptInfo->CO_Rm, compiler->OptInfo->CO_Disp , 0) ;
            CSL_CompileAndRecord_PushAccum () ;
        }
        //DBI_OFF ;
    }
    else
    {
        Word *one = ( Word* ) CSL_WordList (1) ; // the operand
        if ( one->W_ObjectAttributes & LITERAL ) 
        {
            SetHere (one->Coding) ;
            //_Compile_MoveImm_To_Reg ( OREG, one->W_Value, 4 ) ;
            //_Compile_Group2 ( int64 mod, int8 regOpCode, int8 rm, int8 sib, int64 disp, int64 imm )
            _Compile_Group2 ( MEM, op, DSP, 0, 0, 0, one->W_Value ) ;
            return ;
        }
        else if ( one->StackPushRegisterCode )
        {
            SetHere (one->StackPushRegisterCode) ; // leave optInfo_0_two value in R8 we don't need to push it
            Compile_Move_Reg_To_Reg (OREG, one->RegToUse , 0) ;
        }
        else
        {
            _Compile_Move_StackN_To_Reg (OREG, DSP, 0 , 0) ;
            Compile_SUBI ( REG, DSP, 0, CELL_SIZE, BYTE ) ;
        }
        //_Compile_Group2_CL ( int64 mod, int64 regOpCode, int64 rm, int64 sib, cell disp )
        _Compile_Group2_CL ( MEM, op, DSP, 0, 0 ) ;
    }
}

void
Compile_BitWise_NOT ( Compiler * compiler )
{
    Compile_X_Group3 ( compiler, NOT ) ;
}

// two complement

void
Compile_BitWise_NEG ( Compiler * compiler )
{
    Compile_X_Group3 ( compiler, NEG ) ;
}

void
Compile_ShiftLeft ( )
{
    Compile_X_Shift ( _Context_->Compiler0, SHL, 1, 0 ) ;
}

void
Compile_ShiftRight ( )
{
    Compile_X_Shift ( _Context_->Compiler0, SHR, 1, 0 ) ;
}

