#include "../../include/csl.h"
#define OP_EQ_STORE 0

// ( x n -- )
// C : "x += n" :: csl : "x n +="
// X variable op compile for group 1 opCodes - ia32 

void
Compile_TosRmToTOS ( )
{
    Compile_Move_Rm_To_Reg ( ACC, DSP, 0, 0 ) ;
    Compile_Move_Rm_To_Reg ( ACC, ACC, 0, 0 ) ;
    Compile_Move_Reg_To_Rm ( DSP, ACC, 0, 0 ) ;
}

void
_Compile_Set_C_LValue_WithImm_ThruReg ( byte * address, int64 value, Boolean rm, byte operandSize )
{
    Compile_MoveImm_To_Reg ( rm, ( int64 ) address, CELL ) ;
    Compile_MoveImm_To_Mem ( rm, value, operandSize ) ;
}

void
_Compile_Get_C_Value_ToReg ( Boolean reg, int64 value )
{
    Compile_MoveImm_To_Reg ( reg, ( int64 ) value, CELL_SIZE ) ;
}

// nb. necessary for esp/ebp

void
_Compile_GetRValue_FromLValue_ToReg ( Boolean reg, byte * address )
{
    Compile_MoveMemValue_To_Reg ( reg, ( byte* ) address, CELL_SIZE ) ;
}

void
_Compile_Get_FromCAddress_ToReg_ThruReg ( Boolean reg, byte * address, Boolean thruReg )
{
    Compile_MoveMemValue_ToReg_ThruReg ( reg, address, CELL_SIZE, thruReg ) ;
}

// thru reg is a 'scratch' reg

void
_Compile_Set_CAddress_WithRegValue_ThruReg ( byte * address, Boolean reg, Boolean thruReg )
{
    Compile_MoveReg_ToAddress_ThruReg ( reg, address, thruReg ) ;
}

#define ARRAY_MODE_CHECK 1

void
Compile_Peek (Compiler * compiler, Boolean stackReg , int64 size) // @
{
    int64 optSetupFlag = Compiler_CheckOptimize ( compiler, 0 ) ;
    if ( optSetupFlag & OPTIMIZE_DONE ) return ;
    else if ( ! optSetupFlag )
    {
        Word * one = CSL_WordList ( 1 ) ;
        //if ( one->StackPushRegisterCode ) // for now an object may have an array offset that needs to be considered
        //if ( ( ! ( one->W_ObjectAttributes & OBJECT ) ) && one->StackPushRegisterCode ) // for now an object may have an array offset that needs to be considered
        if ( one && ( ! ( one->W_ObjectAttributes & OBJECT ) ) && one->StackPushRegisterCode ) // for now an object may have an array offset that needs to be considered
        {
            SetHere (one->StackPushRegisterCode) ;
            Compile_Move_Rm_To_Reg ( ACC, ACC, 0, size ) ;
            Compile_ADDI ( REG, DSP, 0, sizeof (int64 ), size ) ;
            Compile_Move_Reg_To_Rm ( stackReg, ACC, 0, size ) ;
            return ;
        }
        else Compile_Move_Rm_To_Reg ( ACC, stackReg, 0, size ) ;
    }
    Compile_Move_Rm_To_Reg ( ACC, ACC, 0, size ) ;
    Compile_Move_Reg_To_Rm ( stackReg, ACC, 0, size ) ;
}

void
Compiler_ShowOptimizeArgs ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ; // nb. after _Compiler_GetOptimizeState
    DWL_ShowWord ( optInfo->wordArg1Node, 0, 1, ( int64 ) "optInfo->wordArg1", 0 ) ;
    DWL_ShowWord ( optInfo->wordArg2Node, 0, 1, ( int64 ) "optInfo->wordArg2", 0 ) ;
    //Pause () ;
}

void
Compile_Store ( Compiler * compiler, int lvalueSize ) // !
{
    if ( GetState ( _CSL_, OPTIMIZE_ON ) ) Compile_X_Equal ( compiler, STORE, lvalueSize ) ;
    else // when optimize is off, eg with arrays
    {
        //DBI_ON ;
        int64 stackReg = DSP ;
        Word * word ;
        d0 ( if ( Is_DebugModeOn ) _CSL_SC_WordList_Show ( "\nCompile_Store : not optimized", 0, 0 ) ) ;
        if ( ( word = ( Word* ) CSL_WordList ( 1 ) ) && word->StackPushRegisterCode ) SetHere (word->StackPushRegisterCode) ;
        else Compile_Move_Rm_To_Reg ( ACC, stackReg, 0, 0 ) ;
        Compile_Move_Rm_To_Reg ( OREG, stackReg, ( word && word->StackPushRegisterCode ) ? 0 : ( - CELL_SIZE ), 0 ) ;
        Compile_Move_Reg_To_Rm ( ( ( word && word->StackPushRegisterCode ) ? word->RegToUse : ACC, 0 ), OREG, 0, 0 ) ;
        Compile_SUBI ( REG, stackReg, 0, ( ( word && word->StackPushRegisterCode ) ? 1 : 2 ) * CELL_SIZE, 0 ) ;
        //DBI_OFF ;
    }
#if ARRAY_MODE_CHECK   
    CSL_ArrayModeOff_OptimizeOn ( ) ;
#endif    
}

void
Compile_Poke ( Compiler * compiler, int lvalueSize ) // =
{
#if 0    
    if ( ! _Context_StringEqual_PeekNextToken ( _Context_, ( byte* ) "[", 0 ) )
    {
        CSL_ArrayModeOff_OptimizeOn ( ) ;
    }
#endif    
    if ( GetState ( _CSL_, OPTIMIZE_ON ) ) Compile_X_Equal ( compiler, EQUAL, lvalueSize ) ;
    else // when optimize is off, eg with arrays
    {
        int64 stackReg = DSP ;
        Compile_Move_Rm_To_Reg ( OREG, stackReg, 0, 0 ) ;
        Compile_Move_Rm_To_Reg ( ACC, stackReg, - CELL_SIZE, 0 ) ;
        Compile_Move_Reg_To_Rm ( ACC, OREG, 0, 0 ) ;
        //if ( ! GetState ( _Context_, C_SYNTAX ) ) 
        Compile_SUBI ( REG, stackReg, 0, 2 * CELL_SIZE, BYTE ) ;
    }
#if ARRAY_MODE_CHECK   
    CSL_ArrayModeOff_OptimizeOn ( ) ;
#endif    
}

// n = *m
// ( n m -- )
// 
void
Compile_AtEqual ( Boolean stackReg ) // !
{
    Compile_Move_Rm_To_Reg ( ACC, stackReg, 0, 0 ) ;
    Compile_Move_Rm_To_Reg ( ACC, ACC, 0, 0 ) ;
    Compile_Move_Rm_To_Reg ( OREG, stackReg, - CELL_SIZE, 0 ) ;
    Compile_Move_Reg_To_Rm ( OREG, ACC, 0, 0 ) ;
    Compile_SUBI ( REG, stackReg, 0, CELL_SIZE * 2, BYTE ) ;
#if ARRAY_MODE_CHECK    
    CSL_ArrayModeOff_OptimizeOn ( ) ;
#endif    
}

#if OP_EQ_STORE

// '!' ( value address  -- ) // store value at address - dpans94 - address is on top - value was pushed first, leftmost in rpn, then address

void
Compile_Store ( Compiler * compiler, 0 ) // !
{
    int64 optSetupFlag = Compiler_CheckOptimize ( compiler, 0 ), stackReg = DSP ;
    if ( optSetupFlag & OPTIMIZE_DONE ) return ;
    else if ( optSetupFlag )
    {
        // _Compile_MoveImm ( cell direction, cell rm, cell disp, cell imm, cell operandSize )
        if ( compiler->OptInfo->OptimizeFlag & OPTIMIZE_IMM ) Compile_MoveImm ( compiler->OptInfo->Optimize_Dest_RegOrMem,
            compiler->OptInfo->Optimize_Rm, compiler->OptInfo->Optimize_Disp, compiler->OptInfo->Optimize_Imm, CELL_SIZE ) ;
        else if ( compiler->OptInfo->OptimizeFlag & OPTIMIZE_REGISTER )
        {
            // allow for one of these to be R8 which is 0
            //if ( compiler->OptInfo->Optimize_SrcReg || compiler->OptInfo->Optimize_DstReg ) _Compile_Move_Reg_To_Reg ( compiler->OptInfo->Optimize_DstReg, compiler->OptInfo->Optimize_SrcReg ) ;
            //_Compile_Move ( int64 direction, int8 reg, int64 rm, int64 sib, int64 disp )
            //_Compile_Move ( compiler->OptInfo->Optimize_Dest_RegOrMem, compiler->OptInfo->Optimize_Reg, compiler->OptInfo->Optimize_Rm, 0, 0 ) ;
            //Compile_Move ( uint8 direction, uint8 mod, uint8 reg, uint8 rm, uint8 operandSize, uint8 sib, int64 disp, uint8 dispSize, int64 imm, uint8 immSize )
            Compile_Move ( compiler->OptInfo->Optimize_Dest_RegOrMem, 0, compiler->OptInfo->Optimize_Reg, compiler->OptInfo->Optimize_Rm, 0, 0, 0, 0, 0, 0 ) ;
        }
        else Compile_Move_Reg_To_Rm ( compiler->OptInfo->Optimize_Rm, compiler->OptInfo->Optimize_Reg, compiler->OptInfo->Optimize_Disp, 0 ) ;
    }
    else
    {
        //DBI_ON ;
        Word * word ;
        d0 ( if ( Is_DebugModeOn ) _CSL_SC_WordList_Show ( "\nCompile_Store : not optimized", 0, 0 ) ) ;
        if ( ( word = ( Word* ) CSL_WordList ( 1 ) ) && word->StackPushRegisterCode ) SetHere (word->StackPushRegisterCode) ;
        else Compile_Move_Rm_To_Reg ( ACC, stackReg, 0, 0 ) ;
        Compile_Move_Rm_To_Reg ( OREG, stackReg, ( word && word->StackPushRegisterCode ) ? 0 : ( - CELL_SIZE ), 0 ) ;
        Compile_Move_Reg_To_Rm ( ( ( word && word->StackPushRegisterCode ) ? word->RegToUse : ACC, 0 ), OREG, 0 ) ;
        Compile_SUBI ( REG, stackReg, 0, ( ( word && word->StackPushRegisterCode ) ? 1 : 2 ) * CELL_SIZE, 0 ) ;
        //DBI_OFF ;
    }
#if ARRAY_MODE_CHECK   
    CSL_ArrayModeOff_OptimizeOn ( ) ;
#endif    
}

// ( address value -- ) store value at address - reverse order of parameters from '!'
// ( addr n -- ) // (*addr) = n

void
Compile_Poke ( Compiler * compiler, 0 ) // =
{
    int64 optSetupFlag = Compiler_CheckOptimize ( compiler, 0 ), stackReg = DSP ;
    if ( optSetupFlag & OPTIMIZE_DONE ) return ;
    else if ( optSetupFlag )
    {
        //Compiler_Word_SCHCPUSCA ( compiler->OptInfo->opWord, 0 ) ;
        if ( compiler->OptInfo->OptimizeFlag & OPTIMIZE_IMM )
        {
            // _Compile_MoveImm ( cell direction, cell rm, cell disp, cell imm, cell operandSize )
            Compile_MoveImm ( compiler->OptInfo->Optimize_Dest_RegOrMem,
                compiler->OptInfo->Optimize_Rm, compiler->OptInfo->Optimize_Disp, compiler->OptInfo->Optimize_Imm, 0 ) ;
        }
        else if ( compiler->OptInfo->OptimizeFlag & OPTIMIZE_REGISTER )
        {
            // allow for one of these to be R8 which is 0
            //if ( compiler->OptInfo->Optimize_SrcReg || compiler->OptInfo->Optimize_DstReg ) _Compile_Move_Reg_To_Reg ( compiler->OptInfo->Optimize_DstReg, compiler->OptInfo->Optimize_SrcReg ) ;
            //_Compile_Move ( int64 direction, int8 reg, int64 rm, int64 sib, int64 disp )
            //else 
            //_Compile_Move ( compiler->OptInfo->Optimize_Dest_RegOrMem, compiler->OptInfo->Optimize_Reg, compiler->OptInfo->Optimize_Rm, 0, 0 ) ;
            //Compile_Move ( uint8 direction, uint8 mod, uint8 reg, uint8 rm, uint8 operandSize, uint8 sib, int64 disp, uint8 dispSize, int64 imm, uint8 immSize )
            Compile_Move ( compiler->OptInfo->Optimize_Dest_RegOrMem, 0, compiler->OptInfo->Optimize_Reg, compiler->OptInfo->Optimize_Rm, 0, 0, 0, 0, 0, 0 ) ;
        }
        else Compile_Move_Reg_To_Rm ( compiler->OptInfo->Optimize_Rm, compiler->OptInfo->Optimize_Reg, compiler->OptInfo->Optimize_Disp, 0 ) ;
    }
    else // when optimize is off, eg with arrays
    {
        Compile_Move_Rm_To_Reg ( OREG, stackReg, 0, 0 ) ;
        Compile_Move_Rm_To_Reg ( ACC, stackReg, - CELL_SIZE, 0 ) ;
        Compile_Move_Reg_To_Rm ( ACC, OREG, 0, 0 ) ;
        //if ( ! GetState ( _Context_, C_SYNTAX ) ) 
        Compile_SUBI ( REG, stackReg, 0, 2 * CELL_SIZE, BYTE ) ;
    }
#if ARRAY_MODE_CHECK  
    CSL_ArrayModeOff_OptimizeOn ( ) ;
#endif    
}
#endif



