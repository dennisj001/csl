#include "../../include/csl.h"

void
_Compile_Call_Acc ( void )
{
    //_Compile_Group5 ( int64 code, int64 mod, int64 rm, int64 sib, int64 disp, int64 size )
    _Compile_Group5 ( CALL, 3, 0, 0, 0, 0 ) ;
}

void
Compile_DataStack_PopAndCall ( void )
{
    Compile_Pop_ToAcc_AndCall ( DSP ) ;
}

void
Compile_Call_From_C_Address ( uint64 bptr )
{
    _Compile_GetRValue_FromLValue_ToReg ( ACC, ( byte* ) bptr ) ;
    //_Compile_MoveImm_To_Reg ( ACC, ( uint64 ) bptr, CELL_SIZE ) ;
    _Compile_Call_Acc ( ) ;
}

void
Compile_CallCFunctionWithParameter_TestAlignRSP2 ( byte * cFunction, Word * word )
{
    //DBI_ON ;
    Compile_MoveImm_To_Reg ( RDI, ( int64 ) word, CELL_SIZE ) ; // RDI is x64 abi first parameter 
    Compile_MoveImm_To_Reg ( SREG, ( int64 ) cFunction, CELL_SIZE ) ;
    Compile_Call ( ( byte* ) _CSL_->Call_ToAddressThruSREG_TestAlignRSP ) ;
    //DBI_OFF ;
}

void
Compile_Call_CurrentBlock ( )
{
    Compile_Call_From_C_Address ( ( uint64 ) & _CSL_->CurrentBlock ) ;
}

// >R - Rsp to

void
_Compile_RspReg_To ( ) // data stack pop to rsp
{
    Compile_DspPop_RspPush ( ) ;
}

// rdrop

void
_Compile_RspReg_Drop ( )
{
    _Compile_DropN_Rsp ( 1 ) ;
}

// rsp

void
_Compile_RspReg_Get ( )
{
    _Compile_Stack_PushReg ( DSP, RSP, 0 ) ;
}

// rsp@

void
_Compile_RspReg_Fetch ( )
{
    Compile_Move_Reg_To_Reg ( ACC, RSP, 0 ) ;
    Compile_Move_Rm_To_Reg ( ACC, ACC, 0, 0 ) ;
    _Compile_Stack_PushReg ( DSP, ACC, 0 ) ;
}

// r< - r from

void
_Compile_RspReg_From ( )
{
    _Compile_RspReg_Fetch ( ) ;
    _Compile_RspReg_Drop ( ) ;
}

void
_Compile_GetVarLitObj_RValue_To_Reg ( Word * word, int64 reg, int size )
{
    if ( ! size ) size = word->CompiledDataFieldByteSize ;
    Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    if ( word->W_ObjectAttributes & REGISTER_VARIABLE ) return ; // already 
    else if ( word->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE | T_LISP_SYMBOL | LOCAL_OBJECT )
        || ( word->W_LispAttributes & T_LISP_SYMBOL ) )
    {
        _Compile_Move_StackN_To_Reg ( reg, FP, LocalOrParameterVar_Offset ( word ), size ) ;
    }
    else if ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE | THIS ) )
    {
        if ( _Context_->BaseObject ) SetHere ( _Context_->BaseObject->Coding ) ;
        _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) word->W_PtrToValue, 0 ) ;
        Compile_Move_Rm_To_Reg ( reg, reg, 0, size ) ; // not implemented
    }
    else if ( word->W_ObjectAttributes & ( OBJECT ) ) _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) word->W_Value, 0 ) ; //( int64 ) word->W_PtrToValue ) ;
    else if ( word->W_ObjectAttributes & ( LITERAL | CONSTANT ) ) _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) word->W_Value, 0 ) ;
    else if ( word->W_ObjectAttributes & DOBJECT )
    {
        _CSL_Do_DynamicObject_ToReg ( word, reg ) ;
        Compile_Move_Rm_To_Reg ( reg, reg, 0, size ) ; // not implemented
    }
    else if ( word->W_MorphismAttributes & ( CPRIMITIVE ) ) ; // do nothing here
    else SyntaxError ( QUIT ) ;
}

void
Do_ObjectOffset ( Word * word, int64 reg )
{
    //if ( ! ( word->W_ObjectAttributes & ( O_POINTER ) ) )
    {
        Compiler * compiler = _Context_->Compiler0 ;
        int64 offset = word->AccumulatedOffset ;
        if ( ( offset == 0 ) && GetState ( _CSL_, IN_OPTIMIZER ) ) return ;
        else
        {
            Compile_ADDI ( REG, reg, 0, offset, INT32_SIZE ) ; // only a 32 bit offset ??
            compiler->AccumulatedOffsetPointer = ( int32* ) ( Here - INT32_SIZE ) ; // offset will be calculated as we go along by ClassFields and Array accesses
        }
    }
}

void
Compile_GetVarLitObj_RValue_To_Reg ( Word * word, int64 reg, int size )
{
    _Compile_GetVarLitObj_RValue_To_Reg ( word, reg, size ) ;
    if ( word->W_ObjectAttributes & ( OBJECT | THIS ) )
    {
        Do_ObjectOffset ( word, reg ) ;
        Compile_Move_Rm_To_Reg ( reg, reg, 0, size ) ;
    }
}

// this is not tested for VARIABLE or REGISTER_VARIABLE

void
_Compile_SetVarLitObj_With_Reg ( Word * word, int64 reg, int64 thruReg )
{
    if ( word->W_ObjectAttributes & REGISTER_VARIABLE )
    {
        if ( word->RegToUse == reg ) return ;
        else Compile_Move_Reg_To_Reg ( word->RegToUse, reg, 0 ) ;
    }
    else if ( word->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
        _Compile_Move_Reg_To_StackN ( FP, LocalOrParameterVar_Offset ( word ), reg, 0 ) ;
    else if ( word->W_ObjectAttributes & NAMESPACE_VARIABLE ) _Compile_SetAtAddress_WithReg ( ( int64* ) word->W_PtrToValue, reg, thruReg ) ;
}

void
_Compile_GetVarLitObj_LValue_To_Reg ( Word * word, int64 reg, int size )
{
    if ( ! size ) size = word->CompiledDataFieldByteSize ;
    Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    if ( word->W_ObjectAttributes & REGISTER_VARIABLE )
    {
        if ( word->RegToUse == reg ) return ;
        else Compile_Move_Reg_To_Reg ( reg, word->RegToUse, 0 ) ;
    }
        //else if ( ( word->W_ObjectAttributes & ( O_POINTER ) ) ) _Compile_GetVarLitObj_RValue_To_Reg ( word, reg, 0 ) ; //_Compile_LEA ( reg, FP, 0, LocalOrParameterVar_Disp ( word ) ) ;
    else if ( ( word->W_ObjectAttributes & ( OBJECT | THIS ) ) ) _Compile_GetVarLitObj_RValue_To_Reg ( word, reg, 0 ) ;
    else if ( word->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) ) _Compile_LEA ( reg, FP, 0, LocalOrParameterVar_Disp ( word ) ) ;
    else if ( word->W_ObjectAttributes & ( LITERAL | CONSTANT ) ) _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) word->W_Value, size ) ;
    else if ( word->W_ObjectAttributes & DOBJECT ) _CSL_Do_DynamicObject_ToReg ( word, reg ) ;
    else if ( word->W_ObjectAttributes & NAMESPACE_VARIABLE )
    {
        int64 value ;
        if ( GetState ( _Context_->Compiler0, LC_ARG_PARSING ) || ( GetState ( _Context_, C_SYNTAX ) && ( ! Is_LValue ( _Context_, word ) ) ) )//GetState ( _Context_, C_RHS ) )
        {
            value = ( int64 ) word->W_Value ;
        }
        else value = ( int64 ) word->W_PtrToValue ;
        _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) value, size ) ;
    }
    else if ( word->W_MorphismAttributes & ( CPRIMITIVE ) ) ; // do nothing here
    else SyntaxError ( QUIT ) ;

    if ( word->W_ObjectAttributes & ( OBJECT | THIS ) ) Do_ObjectOffset ( word, reg ) ;
}

void
Do_C_Pointer_StackAccess ( byte * ptr )
{
    if ( Compiling )
    {
        Compile_MoveImm_To_Reg ( RAX, ( int64 ) ptr, CELL_SIZE ) ;
        Word_Set_StackPushRegisterCode_To_Here ( _Context_->CurrentEvalWord ) ;
        _Compile_Stack_PushReg ( DSP, RAX, 0 ) ;
    }
    else DataStack_Push ( ( int64 ) ptr ) ;
}

// c ffi : foreign function interface

