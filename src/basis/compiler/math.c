#include "../../include/csl.h"

// X variable op compile for group 5 opCodes - inc/dec - ia32 

inline int64
Abs ( int64 x )
{
    return (( int64 ) ( ( ( x ) >= 0 ) ? ( x ) : ( - ( x ) ) ) ) ;
}

void
Compile_Minus ( Compiler * compiler )
{
    Compile_X_Group1 (compiler, SUB) ;
}

void
Compile_Plus ( Compiler * compiler )
{
    Compile_X_Group1 (compiler, ADD) ;
}

void
Compile_Multiply ( Compiler * compiler )
{
    int64 optSetupFlag = CO_CheckOptimize ( compiler, 0 ) ;
    if ( optSetupFlag & CO_DONE ) return ;
    else if ( optSetupFlag )
    {
        CompileOptimizeInfo * optInfo = compiler->OptInfo ; //Compiler_CheckOptimize may change the optInfo
        //_Compile_IMUL ( int8 mod, int8 reg, int8 rm, int8 sib, int64 disp, uint64 imm )
        //optInfo->CO_Reg = ACC ; // emulate MUL
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
        _Compile_IMUL ( optInfo->CO_Mod, optInfo->CO_Reg, optInfo->CO_Rm, 0, optInfo->CO_Disp, 0 ) ;
        if ( optInfo->CO_Rm == DSP ) _Compile_Move_Reg_To_StackN (DSP, 0, optInfo->CO_Reg , 0) ;
        else _Word_CompileAndRecord_PushReg (CSL_WordList (0), optInfo->CO_Reg, true , 0) ;
    }
    else
    {
        Compile_Pop_To_Acc ( DSP ) ;
        //Compile_IMUL ( cell mod, cell reg, cell rm, sib, disp, imm, size )
        Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
        Compile_MUL ( MEM, DSP, REX_W | MODRM_B | DISP_B, 0, 0, 0, CELL_SIZE ) ;
        CSL_WordList (0)->StackPushRegisterCode = Here ;
        Compile_Move_ACC_To_TOS ( DSP ) ;
    }
    //DBI_OFF ;
}
// ( a b -- a / b ) dividend in edx:eax, quotient in eax, remainder in edx ; immediate divisor in ecx

void
_Compile_Divide ( Compiler * compiler, uint64 type )
{
    Boolean reg ;
    // dividend in edx:eax, quotient/divisor in eax, remainder in edx
    int64 optSetupFlag = CO_CheckOptimize ( compiler, 0 ) ;
    if ( optSetupFlag & CO_DONE ) return ;
    else if ( optSetupFlag )
    {
        CompileOptimizeInfo * optInfo = compiler->OptInfo ; //Compiler_CheckOptimize may change the optInfo
        Compile_MoveImm ( REG, RDX, 0, 0, CELL ) ;
        // Compile_IDIV( mod, rm, controlFlag, sib, disp, imm, size )
        Compile_IDIV ( optInfo->CO_Mod, optInfo->CO_Rm, ( ( optInfo->CO_Disp != 0 ) ? DISP_B : 0 ), 0, optInfo->CO_Disp, 0, 0 ) ;
        if ( type == MODULO ) reg = RDX ;
        else reg = ACC ;
        if ( reg != ACC ) Compile_Move_Reg_To_Reg ( ACC, reg, 0 ) ; // for consistency finally use RAX so optInfo can always count on rax as the pushed reg
        CSL_CompileAndRecord_Word0_PushReg ( ACC, true ) ;
    }
    else
    {
        // 64 bit dividend EDX:R8 / srcReg
        // EDX holds high order bits
        _Compile_Move_StackN_To_Reg (ACC, DSP, - 1 , 0) ;
        Compile_MoveImm ( REG, RDX, 0, 0, CELL ) ;
        Compile_IDIV ( MEM, DSP, 0, 0, 0, 0, 0 ) ;
        _Compile_Stack_DropN ( DSP, 1 ) ;
        if ( type == MODULO ) reg = RDX ;
        else reg = ACC ;
        _Compile_Move_Reg_To_StackN (DSP, 0, reg , 0) ;
        return ;
    }
    //if ( GetState ( _Context_, C_SYNTAX ) ) _Stack_DropN ( _Context_->Compiler0->WordStack, 2 ) ;
}

void
Compile_Divide ( Compiler * compiler )
{
    _Compile_Divide ( compiler, DIV ) ;
}

// ( a b -- a / b ) quotient in eax, divisor and remainder in edx

void
Compile_Mod ( Compiler * compiler )
{
    _Compile_Divide ( compiler, MODULO ) ;
}

void
_CSL_Do_IncDec ( int64 op )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    if ( CompileMode )
    {
        Compile_X_Group5 ( compiler, op ) ; // ? INC : DEC ) ; //, RVALUE ) ;
    }
    else
    {
        Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
        int64 sd = List_Depth ( _CSL_->CSL_N_M_Node_WordList ) ;
        Word *one = ( Word* ) CSL_WordList ( 1 ) ; // the operand
        if ( op == INC )
        {
            if ( ( sd > 1 ) && one->W_ObjectAttributes & ( PARAMETER_VARIABLE | LOCAL_VARIABLE | NAMESPACE_VARIABLE ) )
            {
                *( ( int64* ) ( TOS ) ) += 1 ;
                DataStack_Drop ( ) ;
            }
            else _DspReg_ [0] ++ ;
        }
        else
        {
            if ( ( sd > 1 ) && one->W_ObjectAttributes & ( PARAMETER_VARIABLE | LOCAL_VARIABLE | NAMESPACE_VARIABLE ) )
            {
                *( ( int64* ) ( TOS ) ) -= 1 ;
                DataStack_Drop ( ) ;
            }
            else _DspReg_ [0] -- ;
        }
        //CSL->set_DspReg_FromDataStackPointer ( ) ; // update DSP reg
    }
}

