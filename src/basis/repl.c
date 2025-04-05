#include "../include/csl.h"

void
_Repl ( Context * cntx, block repl )
{
    Lexer * lexer = cntx->Lexer0 ;
    ReadLiner * rl = lexer->ReadLiner0 ;
    byte * token ;

    byte * snp = rl->NormalPrompt, *sap = rl->AltPrompt ;
    SetState ( _LC_, LC_REPL, true ) ;
    if ( _LC_ )
    {
        rl->AltPrompt = ( byte* ) "lc< " ;
        rl->NormalPrompt = ( byte* ) "lc> " ;
    }
    else
    {
        rl->AltPrompt = ( byte* ) "==< " ;
        rl->NormalPrompt = ( byte* ) "==> " ;
    }
    //SetState ( _O_->psi_PrintStateInfo, PSI_NEWLINE, true ) ;
    SetState ( cntx->System0, ADD_READLINE_TO_HISTORY, true ) ;
    start:
    while ( ! setjmp ( cntx->JmpBuf0 ) )
    {
        while ( ! GetState ( lexer, END_OF_FILE|END_OF_STRING) )
        {
            uint64 * svDsp = _DspReg_ ;
            //iPrintf ( "<= " ) ;
            DoPrompt () ;
            //LC_SaveStack ( ) ; // ?!? maybe we should do this stuff differently : literals are pushed on the stack by the interpreter
            ReadLine_GetLine () ;
            //if ( strstr ( ( char* ) rl->InputLineString, ".." ) || strstr ( ( char* ) rl->InputLineString, "bye" ) || strstr ( ( char* ) rl->InputLineString, "exit" )  || strstr ( ( char* ) rl->InputLineString, "x" ) ) 
            if ( strstr ( ( char* ) rl->InputLineString, ".." ) || strstr ( ( char* ) rl->InputLineString, "exit" )  || strstr ( ( char* ) rl->InputLineString, "quit" ) || strstr ( ( char* ) rl->InputLineString, "bye" ) ) 
            {
                token = Lexer_ReadToken ( _Lexer_ ) ;
                goto done ;
            }
            repl ( ) ;
            //iPrintf ( "\n" ) ;
            _DspReg_ = svDsp ;
       }
    }
    {
        AlertColors ;
        iPrintf ( "\n_Repl Error ... continuing" ) ;
        DefaultColors ;
        goto start ;
    }
done:
    rl->NormalPrompt = snp ;
    rl->AltPrompt = sap ;
    SetState ( _LC_, LC_REPL, false ) ;
    iPrintf ( "\n_Repl Exiting ... " ) ;
    DefaultColors ;
    if (String_Equal ( token, "bye") ) OVT_Exit () ;
}

