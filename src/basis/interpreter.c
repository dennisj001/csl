
#include "../include/csl.h"

void
Interpreter_Init ( Interpreter * interp )
{
    _O_->OVT_Interpreter = interp ;
}

Interpreter *
Interpreter_New ( uint64 allocType )
{
    Interpreter * interp = ( Interpreter * ) Mem_Allocate ( sizeof (Interpreter ), allocType ) ;

    interp->Lexer0 = Lexer_New ( allocType ) ;
    interp->ReadLiner0 = interp->Lexer0->ReadLiner0 ;
    interp->Finder0 = Finder_New ( allocType ) ;
    interp->Compiler0 = Compiler_New ( allocType ) ;
    Interpreter_Init ( interp ) ;
    return interp ;
}

void
_Interpreter_Copy ( Interpreter * interp, Interpreter * interp0 )
{
    MemCpy ( interp, interp0, sizeof (Interpreter ) ) ;
}

Interpreter *
Interpreter_Copy ( Interpreter * interp0, uint64 type )
{
    Interpreter * interp = ( Interpreter * ) Mem_Allocate ( sizeof (Interpreter ), type ) ;
    _Interpreter_Copy ( interp, interp0 ) ;
    Interpreter_Init ( interp ) ;
    return interp ;
}

int64
Interpreter_IsDone ( Interpreter * interp, uint64 flags )
{
    //Interpreter_GetStateFromLexer ( interp ) ;
    int64 rtn = interp->Lexer0->State & flags ;
    //return GetState ( interp, flags | INTERPRETER_DONE ) ;
    //return GetState ( interp->Lexer0, END_OF_FILE ) ; //|END_OF_STRING) ;
    //if ( rtn ) Pause () ;
    if ( _LC_ ) SetState ( _LC_, LC_INTERP_DONE, rtn ) ;
    return rtn ;
}

