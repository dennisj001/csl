
#include "../include/csl.h"

void
Word_Disassemble ( Word * word )
{
    byte * start ;
    if ( word && word->Definition )
    {
        byte endbr64 [] = { 0xf3, 0x0f, 0x1e, 0xfa } ;
        start = word->CodeStart ;
        if ( ( word->W_MorphismAttributes & CPRIMITIVE ) && ( ! memcmp ( endbr64, word->CodeStart, 4 ) ) ) start += 4 ; //4: account for new intel insn used by gcc : endbr64 f3 0f 1e fa
        _Context_->CurrentDisassemblyWord = word ;
        _Debugger_->LastSourceCodeWord = 0 ;
        int64 size = _Debugger_Disassemble (_Debugger_, word, start, word->S_CodeSize ? word->S_CodeSize : 128, ( word->W_MorphismAttributes & ( CPRIMITIVE | DLSYM_WORD | DEBUG_WORD ) ? 1 : 0 ) ) ;
        //_Debugger_->LastSourceCodeWord = word ;
        if ( ( ! word->S_CodeSize ) && ( size > 0 ) ) word->S_CodeSize = size ;
        iPrintf ( "\nWord_Disassemble : word - \'%s\' :: codeSize = %d", word->Name, size ) ; // ? 1 : return - 'ret' - ins
        _Context_->CurrentDisassemblyWord = 0 ;
    }
}

void
_CSL_Word_Disassemble ( Word * word )
{
    if ( word )
    {
        _CSL_SetSourceCodeWord ( word ) ;
        iPrintf ( "\nWord :: %s.%s : definition = 0x%016lx : disassembly at %s :", word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "", c_gd ( word->Name ), ( uint64 ) word->Definition, Context_Location ( ) ) ;
        Word_Disassemble ( word ) ;
        //_Printf ( "\n" ) ;
    }
    else
    {
        iPrintf ( "\nWordDisassemble : word = 0x%016lx", word ) ;
    }
}

void
CSL_Word_Disassemble ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    _CSL_Word_Disassemble ( word ) ;
}

void
Debugger_WDis ( Debugger * debugger )
{
    Word * word = debugger->w_Word ;
    if ( ! word ) word = _Interpreter_->w_Word ;
    _CSL_Word_Disassemble ( word ) ;
}

void
CSL_Disassemble ( )
{
    uint64 number = DataStack_Pop ( ) ;
    byte * address = ( byte* ) DataStack_Pop ( ) ;
    _Debugger_Disassemble (_Debugger_, 0, address, number, 0 ) ;
}

void
CSL_Disassemble_Block ( )
{
    //uint64 number = DataStack_Pop ( ) ;
    byte * address = ( byte* ) DataStack_Pop ( ) ;
    _Debugger_Disassemble (_Debugger_, 0, address, K, 1 ) ;
}


