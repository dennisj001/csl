#include "../include/csl.h"

void
CSL_Peek ( ) // @
{
    if ( CompileMode ) Compile_Peek ( _Context_->Compiler0, DSP, 0 ) ;
    else TOS = * ( int64* ) TOS ;
    //cntx->BaseObject = 0 ;
}

void
CSL_8Peek ( ) // @
{
    if ( CompileMode ) Compile_Peek ( _Context_->Compiler0, DSP, 1 ) ;
    else TOS = * ( int8* ) TOS ;
    //cntx->BaseObject = 0 ;
}

void
CSL_16Peek ( ) // @
{
    if ( CompileMode ) Compile_Peek ( _Context_->Compiler0, DSP, 2 ) ;
    else TOS = * ( int16* ) TOS ;
    //cntx->BaseObject = 0 ;
}

void
CSL_32Peek ( ) // @
{
    if ( CompileMode ) Compile_Peek ( _Context_->Compiler0, DSP, 4 ) ;
    else TOS = * ( int32* ) TOS ;
    //cntx->BaseObject = 0 ;
}

// reg -- puts reg value TOS

// peekReg ( reg -- regValue ) //reg : reg number :: returns reg value on top of stack

void
CSL_PeekReg ( ) // @
{
    //DBI_ON ;
    ByteArray * svcs = _O_CodeByteArray, *ba = _CSL_->PeekPokeByteArray ;
    _ByteArray_DataClear ( ba ) ;
    Set_CompilerSpace ( ba ) ; // now compile to this space
    Compile_Move_Reg_To_Rm ( DSP, TOS & 0xf, 0, 0 ) ;
    _Compile_Return ( ) ;
    Set_CompilerSpace ( svcs ) ; // now compile to this space
    ( ( VoidFunction ) ba->BA_Data ) ( ) ;
    //DBI_OFF ;
}

// pokeRegWithValue ( lvalue reg -- puts value of reg at lvalue address )

void
CSL_PokeRegWithValue ( ) // @
{
    //DBI_ON ;
    uint64 reg = DataStack_Pop ( ) ;
    uint64 value = DataStack_Pop ( ) ;
    ByteArray * svcs = _O_CodeByteArray, *ba = _CSL_->PeekPokeByteArray ;
    _ByteArray_DataClear ( ba ) ;
    Set_CompilerSpace ( ba ) ; // now compile to this space
    Compile_MoveImm ( REG, reg, 0, ( uint64 ) value, CELL_SIZE ) ;
    _Compile_Return ( ) ;
    Set_CompilerSpace ( svcs ) ; // now compile to this space
    ( ( VoidFunction ) ba->BA_Data ) ( ) ;
    //DBI_OFF ;
}

// pokeRegAtAddress ( address, reg -- )

void
CSL_PokeRegAtAddress ( ) // @
{
    uint64 reg = DataStack_Pop ( ) ;
    uint64 address = DataStack_Pop ( ) ;
    ByteArray * svcs = _O_CodeByteArray, *ba = _CSL_->PeekPokeByteArray ;
    _ByteArray_Init ( ba ) ;
    Set_CompilerSpace ( ba ) ; // now compile to this space
    _Compile_PushReg ( ACC ) ;
    Compile_MoveImm ( REG, ACC, 0, ( uint64 ) address, CELL_SIZE ) ;
    Compile_Move_Reg_To_Rm ( ACC, reg, 0, 0 ) ;
    _Compile_PopToReg ( ACC ) ;
    _Compile_Return ( ) ;
    Set_CompilerSpace ( svcs ) ; // reset compiler space pointer
    ( ( VoidFunction ) ba->BA_Data ) ( ) ;
}

int64
Word_LvalueObjectByteSize ( Word * lvalueWord, Word * rValueWord ) // = 
{
    int64 size ;
    if ( ( rValueWord->CompiledDataFieldByteSize == 8 ) || ( lvalueWord->W_ObjectAttributes & ( BIGNUM | REGISTER_VARIABLE ) )
        || ( Namespace_IsUsing ( ( byte* ) "BigNum" ) ) )
    {
        size = ( sizeof (int64 ) ) ;
    }
    else if ( lvalueWord->CompiledDataFieldByteSize ) size = lvalueWord->CompiledDataFieldByteSize ;
    else size = ( sizeof (int64 ) ) ;
    return size >= ( sizeof (int64 ) ) ? ( sizeof (int64 ) ) : size ;
}

void
Do_MoveErrorReport ( int64 value, byte * one, byte * two )
{
    byte *buffer = Buffer_DataCleared ( _CSL_->DebugB3 ) ;
    snprintf ( buffer, 127, "\n_CSL_Move : Type Error : value == %ld : is greater than %s - max value for sizeof %s", value, one, two ) ;
    Error ( buffer, QUIT ) ;
}

void
_CSL_Move ( int64 * address, int64 value, int64 lvalueSize )
{
    switch ( lvalueSize )
    {
        case 1:
        {
            if ( value > 255 ) Do_MoveErrorReport ( value, "255", "(byte)" ) ;
            else * ( byte* ) address = ( byte ) value ;
            break ;
        }
        case 2:
        {
            if ( value > 65535 ) Do_MoveErrorReport ( value, "65535", "(int16)" ) ;
            else * ( int16* ) address = ( int16 ) value ;
            break ;
        }
        case 4:
        {
            if ( value > 2147483647 ) Do_MoveErrorReport ( value, "2147483647", "(int32)" ) ;
            else * ( int32* ) address = ( int32 ) value ;
            break ;
        }
        case 8:
        default:
        {
            * ( int64 * ) address = ( int64 ) value ;
            break ;
        }
    }
    SetState ( _Context_, ADDRESS_OF_MODE, false ) ;
}
// ( addr n -- ) // (*addr) = n

void
_CSL_Poke ( int64 lvalueSize ) // = 
{
    _CSL_Move ( ( int64 * ) NOS, TOS, lvalueSize ) ;
    _DspReg_ -= 2 ;
}

void
CSL_Poke ( ) // = 
{
    Word * lvalueWord, *rvalueWord ;
    lvalueWord = TWS ( - 1 ), rvalueWord = TWS ( 0 ) ;
    int64 lvalueSize = Word_LvalueObjectByteSize ( lvalueWord, rvalueWord ) ;
    if ( ! _TypeMismatch_CheckError_Print ( lvalueWord, rvalueWord, 1 ) )
    {
        if ( CompileMode ) Compile_Poke ( _Context_->Compiler0, lvalueSize ) ;
        else _CSL_Poke ( lvalueSize ) ;
    }
}

void
CSL_AtEqual ( ) // !
{
    Word * lvalueWord = TWS ( 0 ), *rvalueWord = TWS ( - 1 ) ;
    //Word * lvalueWord = TWS ( - 1 ), *rvalueWord = TWS ( 0 ) ;
    int64 lvalueSize = Word_LvalueObjectByteSize ( lvalueWord, rvalueWord ) ;
    //if ( ! _TypeMismatch_CheckError_Print ( lvalueWord, rvalueWord, 1 ) )
    {
        if ( CompileMode ) Compile_AtEqual ( DSP ) ;
        else
        {
            _CSL_Move ( ( int64 * ) NOS, * ( int64* ) TOS, lvalueSize ) ;
            _DspReg_ -= 2 ;
        }
    }
}

// ( n addr -- ) // (*addr) = n

void
CSL_Store ( ) // !
{
    Word * lvalueWord = TWS ( 0 ), *rvalueWord = TWS ( - 1 ) ;
    int64 lvalueSize = Word_LvalueObjectByteSize ( lvalueWord, rvalueWord ) ;
    if ( ! _TypeMismatch_CheckError_Print ( lvalueWord, rvalueWord, 1 ) )
    {
        if ( CompileMode ) Compile_Store ( _Context_->Compiler0, Word_LvalueObjectByteSize ( lvalueWord, rvalueWord ) ) ;
        else
        {
            _CSL_Move ( ( int64 * ) TOS, NOS, lvalueSize ) ;
            _DspReg_ -= 2 ;
        }
    }
}

void
CSL_Abs ( )
{
    //int64 value = DataStack_Pop () ;
    //DataStack_Push (( int64 ) abs (value) ) ;
    TOS = abs ( TOS ) ;
}


