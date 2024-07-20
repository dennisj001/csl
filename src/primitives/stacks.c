#include "../include/csl.h"

void
CSL_Dsp ( )
{
    DataStack_Push (( int64 ) _DspReg_ ) ;
}

void
CSL_Drop ( )
{
    if ( CompileMode )
    {
        //Word * one = Compiler_WordStack ( - 1 ) ;
        Word * one = CSL_WordList ( 1 ) ;
        if ( GetState ( _CSL_, CO_ON ) && one && ( one->StackPushRegisterCode ) ) SetHere (one->StackPushRegisterCode) ;
        else _Compile_Stack_Drop ( DSP ) ;
    }
    else
    {
        DataStack_Drop ( ) ;
    }
}

void
CSL_DropN ( )
{
    if ( CompileMode ) _Compile_Stack_DropN ( DSP, _DataStack_Top ( ) + 1 ) ;
    else DataStack_DropN ( TOS + 1 ) ;
}

void
CSL_Dup ( )
{
    if ( CompileMode ) _Compile_Stack_Dup ( DSP ) ;
    else DataStack_Dup ( ) ;
    CSL_TypeStack_Dup ( ) ;
}

void
CSL_Tos ( )
{
    //CSL_Dup ( ) ;
    SetState ( _Context_->Compiler0, RETURN_TOS, true ) ;
}

#if 0

void
CSL_Ndrop ( )
{
    if ( CompileMode )
    {
        //Compile_SUBI( mod, operandReg, offset, immediateData, size ) 
        Word * one = CSL_WordList ( 1 ) ;
        SetHere (one->Coding) ;
        Compile_SUBI ( REG, DSP, 0, one->W_Value * CELL_SIZE, BYTE ) ;
    }
    else
    {
        uint64 n = _DataStack_Top ( ) ;
        DataStack_DropN ( n + 1 ) ;
    }
}
#endif
// result is as if one did n dups in a row 

void
CSL_NDup ( )
{
    int64 n = TOS ;
    int64 value = * -- _DspReg_ ; // -1 : n now occupies 1 to be also used slot
    while ( n -- )
    {

        * ++ _DspReg_ = value ;
    }
    //CSL->Set_DspReg_FromDataStackPointer ( ) ; // update DSP reg
}

// pick is from stack below top index
// 0 pick is Dsp [ 0] - TOS 
// 1 pick is Dsp [-1]
// ..., etc.

void
CSL_Pick ( ) // pick
{
    if ( CompileMode )
    {
        _Compile_Stack_Pick ( DSP ) ;
    }
    else
    {
        //* Dsp = ( * ( Dsp - * ( Dsp ) - 1 ) ) ;
        //int64 top = Dsp [0] ;
        _DspReg_ [0] = _DspReg_ [ - ( _DspReg_ [0] + 1 ) ] ;
    }
}

int64 
CSL_StackDepth ()
{
    return DataStack_Depth ( ) ;
    //DataStack_Push (  ) ;
}

void
CSL_Swap ( )
{
    if ( CompileMode )
    {
        _Compile_Stack_Swap ( DSP ) ;
    }
    else
    {
        int64 a = TOS ;
        TOS = _DspReg_ [ - 1 ] ;
        _DspReg_ [ - 1 ] = a ;
    }
}

void
CSL_PrintNDataStack ( )
{
    // Intel SoftwareDevelopersManual-253665.pdf section 6.2 : a push decrements ESP, a pop increments ESP
    // therefore TOS is in lower mem addresses, bottom of stack is in higher memory addresses
    int64 size = DataStack_Pop ( ) ;
    _CSL_PrintNDataStack ( size ) ;
}

#if 0

void
CSL_PrintRspRegStack ( )
{
    // Intel SoftwareDevelopersManual-253665.pdf section 6.2 : a push decrements ESP, a pop increments ESP
    // therefore TOS is in lower mem addresses, bottom of stack is in higher memory addresses
    _CSL_PrintNReturnStack ( 8 ) ;
}
#endif

void
CSL_PrintReturnStack ( )
{
    _CSL_PrintNReturnStack ( 8, 1 ) ;
    CSL_NewLine ( ) ;

}

void
CSL_PrintNReturnStack ( )
{
    // Intel SoftwareDevelopersManual-253665.pdf section 6.2 : a push decrements ESP, a pop increments ESP
    // therefore TOS is in lower mem addresses, bottom of stack is in higher memory addresses
    int64 size = DataStack_Pop ( ) ;
    _CSL_PrintNReturnStack ( size, 1 ) ;
    CSL_NewLine ( ) ;
}

void
CSL_PrintNDataStack_8 ( )
{
    // Intel SoftwareDevelopersManual-253665.pdf section 6.2 : a push decrements ESP, a pop increments ESP
    // therefore TOS is in lower mem addresses, bottom of stack is in higher memory addresses
    _CSL_PrintNDataStack ( 8 ) ;
}

