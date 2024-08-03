
#include "../../include/csl.h"

void
_Compile_C_Call_1_Arg ( byte* function, int64 arg )
{
    //_Compile_RspReg_Push ( arg ) ;
    _Compile_PushReg ( RDI ) ;
    Compile_MoveImm_To_Reg ( RDI, arg, CELL ) ;
    Compile_Call ( function ) ;
    _Compile_PopToReg ( RDI ) ;
    //_Compile_RspReg_Drop ( ) ;
}

void
_CompileN ( byte * data, int64 size )
{
    byte * svHere = Here ;
    ByteArray_AppendCopy ( _O_CodeByteArray, size, data ) ; // size in bytes
    if ( _DBI || ( _O_->Dbi > 1 ) )
    {
        //d1 ( Debugger_UdisOneInstruction ( _Debugger_, 0, here, ( byte* ) "", ( byte* ) "" ) ; ) ;
        d1 ( _Debugger_Disassemble ( _Debugger_, 0, svHere, size, 1 ) ) ;
    }
}

// nb : can be used by an compiler on
// a code body that has only one RET point

void
_CompileFromUptoRET ( byte * data )
{
    ByteArray_AppendCopyUpToRET ( _O_CodeByteArray, data ) ; // size in bytes
}

// copy block from 'address' to Here

void
_Compile_WordInline ( Word * word ) // , byte * dstAddress )
{
    BI_Block_Copy (0, 0, ( byte* ) word->Definition, word->S_CodeSize ) ;
}

void
_CompileFromName ( byte * wordName )
{
    Word * word = Finder_Word_FindUsing (_Context_->Finder0, wordName, 0) ;
    // ?? Exception : error message here
    if ( ! word ) _Throw ( QUIT ) ;
    _Word_Compile ( word ) ;
}

void
_CompileFromName_Inline ( byte * wordName )
{
    Word * word = Finder_Word_FindUsing (_Context_->Finder0, wordName, 0) ;
    if ( ! word ) _Throw ( QUIT ) ;
    _Compile_WordInline ( word ) ;
}


