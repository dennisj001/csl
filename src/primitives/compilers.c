#include "../include/csl.h"

void
CSL_Here ( )
{
    DataStack_Push ( ( int64 ) Here ) ;
}

void
CSL_Code ( )
{
    DataStack_Push ( ( int64 ) _O_CodeByteArray ) ;
}

void
CompileCall ( )
{
    Compile_Call_TestRSP ( ( byte* ) DataStack_Pop ( ) ) ;
}

void
Compile_A_CSLWord ( )
{
    _Word_Compile ( ( Word* ) DataStack_Pop ( ) ) ;
}

void
CompileInt64 ( )
{
    _Compile_Int64 ( DataStack_Pop ( ) ) ;
}

void
Compile_Int32 ( )
{
    int64 l = DataStack_Pop ( ) ;
    _Compile_Int32 ( l ) ;
}

void
Compile_Int16 ( )
{
    int64 w = DataStack_Pop ( ) ;
    _Compile_Int16 ( ( short ) w ) ;
}

void
Compile_Int8 ( )
{
    int64 b = DataStack_Pop ( ) ;
    _Compile_Int8 ( b ) ;
}

void
Compile_N_Bytes ( )
{
    int64 size = DataStack_Pop ( ) ;
    byte * data = ( byte* ) DataStack_Pop ( ) ;
    _CompileN ( data, size ) ;
}

Word *
_CSL_Literal ( )
{
    int64 value = DataStack_Pop ( ) ;
    Word * word = DataObject_New ( LITERAL, 0, ( byte* ) value, 0, LITERAL | CONSTANT, 0, 0, value, 0, STRING_MEMORY, - 1, - 1 ) ;
    word->S_Value = ( int64 ) word->Name ;
    return word ;
}

void
CSL_Literal ( )
{
    Word * word = _CSL_Literal ( ) ;
    Interpreter_DoWord ( _Context_->Interpreter0, word, - 1, - 1 ) ;
    //DataStack_Push ( (int64) word ) ;
}

void
CSL_Constant ( )
{
    Word *tword = 0, *cword ;
    int64 value = DataStack_Pop ( ) ;
    tword = CSL_TypeStack_Pop ( ) ;
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    cword = DataObject_New ( CONSTANT, 0, name, 0, CONSTANT, 0, 0, value, 0, 0, - 1, - 1 ) ;
    if ( tword ) cword->W_ObjectAttributes |= tword->W_ObjectAttributes ;
    CSL_Finish_WordSourceCode ( _CSL_, cword, 0 ) ;
}

void
CSL_Variable ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    if ( name )
    {
        Word * word = DataObject_New ( NAMESPACE_VARIABLE, 0, name, 0, NAMESPACE_VARIABLE, 0, 0, 0, 0, 0, - 1, - 1 ) ;
        if ( ! Compiling ) CSL_Finish_WordSourceCode ( _CSL_, word, 0 ) ;
    }
}

// "{|" - exit the Compiler start interpreting
// named after the forth word '[' 

void
CSL_LeftBracket ( )
{
    Compiler * compiler = _Compiler_ ;
    SetState ( compiler, COMPILE_MODE, false ) ;
    if ( compiler->SaveOptimizeState ) CSL_OptimizeOn ( ) ;
}

// "|}" - enter the Compiler
// named following the forth word ']'

void
_CSL_RightBracket ( )
{
    Compiler * compiler = _Compiler_ ;
    SetState ( compiler, COMPILE_MODE, true ) ;
    compiler->SaveOptimizeState = GetState ( _CSL_, OPTIMIZE_ON ) ;
}

void
CSL_RightBracket ( )
{
    if ( ! Compiling ) Compiler_Init ( _Compiler_, 0 ) ;
    _CSL_RightBracket ( ) ;
}

void
CSL_AsmModeOn ( )
{
    SetState ( _Context_->Compiler0, ASM_MODE, true ) ;
    SetState ( _Context_, ASM_SYNTAX, true ) ;
}

void
CSL_AsmModeOff ( )
{
    SetState ( _Context_->Compiler0, ASM_MODE, false ) ;
    SetState ( _Context_, ASM_SYNTAX, false ) ;
}

void
CSL_CompileMode ( )
{
    DataStack_Push ( GetState ( _Context_->Compiler0, COMPILE_MODE ) ) ;
}

void
CSL_FinishWordDebugInfo ( )
{
    _CSL_FinishWordDebugInfo ( 0 ) ;
}
