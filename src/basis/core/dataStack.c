
#include "../../include/csl.h"

// these functions are part of the C vm and can't be compiled
// ! they are should only be called in C functions !

#if 0 // macros
uint64
_DataStack_GetTop ( )
{
    return _DspReg_[0] ;
}

void
_DataStack_SetTop ( int64 value )
{
    _DspReg_[0] = value ;
}
#endif

uint64
DataStack_Pop ( )
{
#if 0   
    int64 value = _DspReg_ [ 0 ] ; //
#else
    _DSP_C_PTR_ = _DspReg_ ;
    int64 value = Stack_Pop_WithExceptionFlag ( _DataStack_, 1 ) ;
#endif    
    //CSL_TypeStack_Drop ( ) ;
    _DspReg_ -- ;
    return value ;
}

void
DataStack_Push (int64 value)
{
    //word = word ? word : WordStack (0) ;
    //if ( word ) SetHere ( word->StackPushRegisterCode, 1 ) ;
    _DspReg_ ++ ;
    _DspReg_ [0] = value ;
}

void
DataStack_Dup ( )
{
    _DspReg_ [ 1 ] = _DspReg_[0] ;
    _DspReg_ ++ ;
    //CSL_TypeStack_Dup ( ) ;
}

void
DataStack_DropN ( int64 n )
{
    _DspReg_ -= n ;
}

void
DataStack_Drop ( )
{
    //_Dsp_ -- ;
    _DataStack_Drop ;
    CSL_TypeStack_Drop ( ) ;
    //_DSP_ = _Dsp_ ;
}

inline int64
DataStack_Overflow ( )
{
    return ( _DspReg_ > _DataStack_->StackMax ) ;
}

inline int64
DataStack_Underflow ( )
{
    return ( _DspReg_ < _DataStack_->InitialTosPointer ) ;
}

void
DataStack_Check ( )
{
    if ( DataStack_Underflow ( ) || DataStack_Overflow ( ) )
    {
        CSL_Exception ( STACK_OVERFLOW, 0, QUIT ) ;
    }
}

int64
DataStack_Depth ( )
{
    if ( _O_ && _CSL_ && _DataStack_ )
    {
        //_DataStackPointer_ = _Dsp_ ;
        _DSP_C_PTR_ = _DspReg_ ;
        return Stack_Depth ( _DataStack_ ) ;
    }
    return 0 ;
}

// safe form with stack checking

void
_CSL_PrintDataStack ( )
{
    Stack_Print ( _DataStack_, ( byte* ) "DataStack", 0 ) ;
}

void
CSL_PrintDataStack ( )
{
    Set_DataStackPointer_FromDspReg ( ) ;
    _CSL_PrintDataStack ( ) ;
}

void
CSL_PrintStackDepth ( )
{
    iPrintf ( "\nDataStack : depth = %d", Stack_Depth ( _DataStack_ ) ) ;
}

void
Set_DebuggerDspReg_FromDspReg ( )
{
    _Debugger_->cs_Cpu->R14d = _DspReg_ ;
}

void
Set_DspReg ( uint64 * ptr )
{
    _DspReg_ = ptr ;
}

void
Set_DspReg_FromDebuggerDspReg ( )
{
    Set_DspReg ( _Debugger_->cs_Cpu->R14d ) ;
}

void
_Set_DataStackPointers ( uint64 * ptr )
{
    Set_DspReg ( ptr ) ;
    _CSL_->DataStack->StackPointer = ptr ;
}

void
Set_DataStackPointers_FromDebuggerDspReg ( )
{
    _Set_DataStackPointers ( _Debugger_->cs_Cpu->R14d ) ;
}

void
Set_DataStackPointer_FromDspReg ( )
{
    _CSL_->DataStack->StackPointer = _DspReg_ ;
}

void
Rpn_SetDspReg ()
{
    int64 * newDsp = (int64*) DataStack_Pop () ;    
    Set_DspReg ( newDsp - 1 ) ;
}

void
Set_DspReg_FromDataStackPointer ( )
{
    Set_DspReg ( _CSL_->DataStack->StackPointer ) ;
}

void
SetDataStackPointer ( uint64 *sp )
{
    _CSL_->DataStack->StackPointer = sp ;
    Set_DspReg ( sp ) ;
}

uint64 *
GetDataStackPointer ( )
{
    //_CSL_->DataStack->StackPointer = _Dsp_ ;
    return _DspReg_ ;
}

void
CSL_CheckInitDataStack ( )
{
    if ( _O_->OVT_Exception->RestartCondition >= QUIT ) _CSL_DataStack_Init ( _CSL_ ) ;
    else if ( Stack_Depth ( _DataStack_ ) < 0 )
    {
        _Stack_PrintHeader ( _DataStack_, ( byte* ) "DataStack" ) ;
        iPrintf ( c_ad ( "\n\nError : %s : %s : Stack Underflow!" ), _Context_->CurrentlyRunningWord ? _Context_->CurrentlyRunningWord->Name : ( byte * ) "", _Context_Location ( _Context_ ) ) ;
        iPrintf ( c_gd ( "\nReseting DataStack.\n" ) ) ;
        _Stack_Init ( _CSL_->DataStack, _O_->DataStackSize ) ;
        _CSL_DataStack_Init ( _CSL_ ) ;
        _Stack_PrintHeader ( _DataStack_, ( byte* ) "DataStack" ) ;
    }
}

int64
OVT_StackSize ()
{
    return (_O_->DataStackSize / sizeof ( int64)) ;
}

void
CSL_DataStack_Size ( )
{
    DataStack_Push ( DataStack_Depth ( ) ) ;
}

