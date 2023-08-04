#include "../include/csl.h"

void
CSL_Debug_AtAddress ( )
{
    byte * address ;
    address = ( byte* ) DataStack_Pop ( ) ;
    _CSL_Debug_AtAddress ( address ) ;
}

void
_CSL_Debugger_Locals_Show ( )
{
    Debugger_Locals_Show ( _Debugger_ ) ;
    //Pause ( ) ;
}

void
CSL_Debugger_Locals_Show ( )
{
    if ( GetState ( _Debugger_, DBG_AUTO_MODE ) ) _CSL_Debugger_Locals_Show ( ) ;
}

void
_CSL_DebugInfo ( )
{
    _Debugger_->w_Word = _Context_->CurrentlyRunningWord ;
    Debugger_ShowInfo ( _Debugger_, ( byte* ) "\ninfo", 0 ) ;
}

// put this '<dbg>' into csl code for a runtime break into the debugger

void
CSL_DebugInfo ( )
{
    if ( Verbosity () )
    {
        _CSL_DebugInfo ( ) ;
        Debugger_Source ( _Debugger_ ) ;
    }
}

void
_CSL_DebugOn ( )
{
    Debugger * debugger = _Debugger_ ;
    //if ( ! Is_DebugOn )
    {
        if ( Verbosity () > 1 ) iPrintf ( "\nCSL_DebugOn : at %s", Context_Location ( ) ) ;
        debugger->DebugRSP = 0 ;
        Debugger_On ( debugger ) ;
    }
#if 0  // no : because it makes debugger ineffective for words that aren't on the using list
    byte * nextToken = Lexer_Peek_NextToken (cntx->Lexer0, 0) ;
    debugger->EntryWord = Finder_Word_FindUsing (cntx->Interpreter0->Finder0, nextToken, 0) ;
    _Context_->SourceCodeWord = debugger->EntryWord ;
#endif     
}

void
CSL_DebugOn ( )
{
    if ( ! Is_DebugOn ) _CSL_DebugOn ( ) ;
}

void
CSL_DebugOff ( )
{
    Debugger_Off ( _Debugger_, 1 ) ;
}

void
CSL_DebugRuntimeBreakpoint ( )
{
    if ( ( ! CompileMode ) ) DebugRuntimeBreakpoint ( ) ;
}

void
CSL_DebugRuntimeBreakpoint_IsDebugShowOn ( )
{
    if ( Is_DebugShowOn ) DebugRuntimeBreakpoint ( ) ;
}

void
CSL_DebugRuntimeBreakpoint_IsDebugOn ( )
{
    if ( ( ! CompileMode ) && Is_DebugOn ) 
        DebugRuntimeBreakpoint ( ) ;
}
void
CSL_DebugRuntimeBreakpoint_IsDebugLevel1 ( )
{
    if ( ( ! CompileMode ) && (_CSL_->DebugLevel == 1) ) 
        DebugRuntimeBreakpoint ( ) ;
}
void
CSL_DebugRuntimeBreakpoint_IsDebugLevel2 ( )
{
    if ( ( ! CompileMode ) && (_CSL_->DebugLevel == 2) ) 
        DebugRuntimeBreakpoint ( ) ;
}

