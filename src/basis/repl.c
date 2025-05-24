#include "../include/csl.h"

void
_Repl ( Context * cntx, block repl )
{
    Lexer * lexer = cntx->Lexer0 ;
    ReadLiner * rl = lexer->ReadLiner0 ;
    byte * token ;

    byte * snp = rl->NormalPrompt, *sap = rl->AltPrompt ;
    if ( GetState ( cntx, JFORTH_MODE ) )
    {
        rl->AltPrompt = ( byte* ) "jf< " ;
        rl->NormalPrompt = ( byte* ) "jf> " ;
    }
    else if ( GetState ( cntx, RETRO_MODE ) )
    {
        rl->AltPrompt = ( byte* ) "rf< " ;
        rl->NormalPrompt = ( byte* ) "rf> " ;
    }
    else if ( _LC_ )
    {
        SetState ( _LC_, LC_REPL, true ) ;
        rl->AltPrompt = ( byte* ) "lc< " ;
        rl->NormalPrompt = ( byte* ) "lc> " ;
    }
    else
    {
        rl->AltPrompt = ( byte* ) "==< " ;
        rl->NormalPrompt = ( byte* ) "==> " ;
    }
    SetState ( cntx->System0, ADD_READLINE_TO_HISTORY, true ) ;
start:
    while ( ! setjmp ( cntx->JmpBuf0 ) )
    {
        while ( ! GetState ( lexer, END_OF_FILE | END_OF_STRING ) )
        {
            uint64 * svDsp = _DspReg_ ;
            if ( ! GetState ( cntx, RETRO_MODE ) )
            {
                DoPrompt ( ) ;
                ReadLine_GetLine ( ) ;
                if ( strstr ( ( char* ) rl->InputLineString, "..\n" ) || strstr ( ( char* ) rl->InputLineString, "exit" ) || strstr ( ( char* ) rl->InputLineString, "quit" ) || strstr ( ( char* ) rl->InputLineString, "bye" ) )
                {
                    token = Lexer_ReadToken ( _Lexer_ ) ;
                    goto done ;
                }
            }
            repl ( ) ;
            _DspReg_ = svDsp ;
            if ( GetState ( cntx, RETRO_MODE ) ) goto done ;
        }
    }
    AlertColors ;
    iPrintf ( "\n_Repl Error ... continuing" ) ;
    DefaultColors ;
    goto start ;
done:
    rl->NormalPrompt = snp ;
    rl->AltPrompt = sap ;
    SetState ( _LC_, LC_REPL, false ) ;
    iPrintf ( "\n_Repl Exiting ... " ) ;
    DefaultColors ;
    //if ( String_Equal ( token, "bye" ) ) OVT_Exit ( ) ;
}

