
#include "../../include/csl.h"

void
LC_Debug ( LambdaCalculus * lc, int64 state, Boolean setupFlag )
{
    if ( GetState ( lc, LC_DEBUG_ON ) )
    {
        Debugger * debugger = _Debugger_ ;
        DebugColors ;
        lc->DebuggerSetupFlag = setupFlag ;
        lc->DebuggerState = state ;
        //if ( lc->ApplyFlag ) LC_Debug_Output ( lc ) ; // done by Debugger_InterpreterLoop
        SetState_TrueFalse ( debugger, ( DBG_NEWLINE | DBG_LC_DEBUG | ( ( GetState ( lc, LC_DEBUG_MENU_SHOWN ) ? 0 : DBG_MENU ) ) ), ( DBG_INFO | DBG_PROMPT ) ) ; //| DBG_MENU ) ) ;
        byte * svDbgMenu = debugger->Menu ;
        debugger->Menu = "\nDebug Menu at : \n%s :\n(m)enu, so(U)rce, (e)val, sto(P), (S)tate, (c)ontinue, s(t)ack, auto(z), (V)erbosity, (q)uit, a(B)ort, usi(N)g, e(x)it\n '\\\' - <esc> - escape, ' ' - <space> - continue" ;
        Debugger_InterpreterLoop ( debugger ) ;
        debugger->Menu = svDbgMenu ;
        lc->DebuggerState = 0 ;
        SetState ( lc, LC_DEBUG_MENU_SHOWN, true ) ;
        DefaultColors ;
    }
}

void
_LO_Debug_Output ( ListObject * l0, byte * descript )
{
    _LO_PrintWithValue ( l0, descript, "", 1 ) ;
}

void
LO_Debug_Output ( ListObject * l0, byte * descript, int64 force )
{
    if ( LC_DEFINE_DBG || force ) _LO_Debug_Output ( l0, descript ) ;
}

void
LC_Debug_Output ( LambdaCalculus * lc )
{
    int64 compiled = false ;
    if ( lc->DebuggerSetupFlag )
    {
        if ( lc->L0 ) _LO_PrintWithValue ( lc->L0, "LC_Debug_Output : lc->L0 = ", "", 1 ) ;
        switch ( lc->DebuggerState )
        {
            case LC_APPLY:
            {
                iPrintf ( "\nLC_Apply " ) ;
                _LO_PrintWithValue ( lc->Lfunction->Lo_LambdaBody, ": Lfunction = ", "", 1 ),
                    _LO_PrintWithValue ( lc->Largs, ": lc->Largs = ", "", 0 ) ;
                CSL_Show_SourceCode_TokenLine ( lc->Lfunction, "LC_Debug : ", 0, lc->Lfunction->Name, "" ) ;
                lc->LC_Here = Here ;
                break ;
            }
            case LC_EVAL:
            {
                if ( lc->L0 ) _LO_PrintWithValue ( lc->L0, "LC_Eval : l0 = ", "", 1 ) ; //, iPrintf ( "%s", lc->L0->Name ) ;
                break ;
            }
            case LC_SPECIAL_FUNCTION:
            {
                _LO_PrintWithValue ( lc->Lfirst, "LC_SpecialFuncion : lc->Lfirst = ", "", 1 ) ; //, _LO_PrintWithValue ( lc->Locals, " : locals = ", "", 0 ) ;
                break ;
            }
            case LC_EVAL_LIST:
            {
                if ( lc->L0 ) _LO_PrintWithValue ( lc->L0, "LC_EvalList : l0 = ", "", 1 ) ;
                break ;
            }
            case LC_EVAL_SYMBOL:
            {
                if ( lc->L0 ) _LO_PrintWithValue ( lc->L0, "_LC_EvalSymbol : l0 = ", "", 1 ) ;
                break ;
            }
            case LC_COND:
            {
                _LO_PrintWithValue ( lc->L0, "LO_Cond : lc->L0 = ", "", 1 ) ;
                break ;
            }
            default: break ;
        }
    }
    else
    {
        switch ( lc->DebuggerState )
        {
            case LC_EVAL_SYMBOL:
            {
                if ( lc->L1 ) _LO_PrintWithValue ( lc->L1, "_LC_EvalSymbol : l1 = ", "", 1 ) ;
                break ;
            }
            case LC_APPLY:
            {
                if ( lc->Lfunction )
                {
                    if ( Here > lc->LC_Here ) compiled = true ;
                    lc->LC_Here = Here ;
                    iPrintf ( "\nLC_Apply " ) ;
                    _LO_PrintWithValue ( lc->Lfunction->Lo_LambdaBody, ": Lfunction = ", "", 1 ),
                        _LO_PrintWithValue ( lc->Largs, ": Largs = ", "", 0 ), ( Compiling && compiled ) ? iPrintf ( ": Compiled" ) : 0 ; //_LO_PrintWithValue ( lc->L1, ": L1 = ", "", 0 ) ;
                    CSL_Show_SourceCode_TokenLine ( lc->Lfunction, "LC_Debug : ", 0, lc->Lfunction->Name, "" ) ;
                }
                break ;
            }
            case LC_EVAL_PRINT:
            {
                if ( lc->L1 )_LO_PrintWithValue ( lc->L1, "LC_EvalPrint : lc->L1 = ", ( lc->ParenLevel <= 0 ) ? "\n" : "", 0 ), ( Compiling && compiled ) ? iPrintf ( " : Compiled" ) : 0 ;
                break ;
            }
            case LC_EVAL:
            {
                if ( Here > lc->LC_Here ) compiled = true ;
                lc->LC_Here = Here ;
                if ( lc->L0 ) _LO_PrintWithValue ( lc->L0, "LC_Eval : l0 = ", "", 1 ) ;
                else if ( lc->Lread ) _LO_PrintWithValue ( lc->Lread, "LC_Eval : lread = ", "", 1 ) ;
                //if ( lc->L1 ) _LO_PrintWithValue ( lc->L1, " : l1 = ", "", 0 ), ( Compiling && compiled ) ? iPrintf ( " : Compiled" ) : 0 ;
                //_LO_PrintWithValue ( lc->L1, " : l1 = ", "", 0 ), ( Compiling && compiled ) ? iPrintf ( " : Compiled" ) : 0 ;
                if ( ( lc->L1 ) && ( ! ( GetState ( lc, LC_EVAL_PRINT ) ) ) )
                {
                    _LO_PrintWithValue ( lc->L1, "LC_Eval  : lc->L1 = ", ( lc->ParenLevel <= 1 ) ? "\n" : "", 0 ), ( Compiling && compiled ) ? iPrintf ( " : Compiled" ) : 0 ;
                }
                break ;
            }
            case LC_EVAL_LIST:
            {
                _LO_PrintWithValue ( lc->Lfunction, "LC_EvalList : lfunction = ", "", 1 ), _LO_PrintWithValue ( lc->Largs, " : largs = ", "", 0 ), _LO_PrintWithValue ( lc->Locals, " : locals = ", "", 0 ) ;
                if ( lc->Lfunction ) CSL_Show_SourceCode_TokenLine ( lc->Lfunction, "LC_Debug : ", 0, lc->Lfunction->Name, "" ) ;
                break ;
            }
            case LC_SUBSTITUTE:
            {
                _LO_PrintWithValue ( lc->FunctionParameters, "LO_Substitute : lambdaParameters = ", "", 1 ), _LO_PrintWithValue ( lc->FunctionArgs, " : funcCallValues = ", "", 0 ) ;
                break ;
            }
            case LO_DEFINE:
            case LO_DEFINEC:
            {
                _LO_PrintWithValue ( lc->Lread, "_LO_Define : lread = ", "", 1 ) ;
                _LO_PrintWithValue ( lc->L0, "_LO_Define : l0 = ", "", 1 ) ;
                //_LO_PrintWithValue ( lc->Lfunction, "_LO_Define : Function = ", "", 1 ) ;
                iPrintf ( "\n_LO_Define : function name : %s : ", lc->L1->Name ) ;
                _LO_PrintWithValue ( lc->FunctionParameters, " : LambdaParameters = ", "", 0 ) ;
                break ;
            }
            case LC_COND:
            {
                _LO_PrintWithValue ( lc->L1, "LO_Cond : lc->L1 = ", "", 1 ) ;
                //iPrintf ( "LO_Cond : " ) ;
                break ;
            }
            case LC_SPECIAL_FUNCTION:
            {
                _LO_PrintWithValue ( lc->L1, "LC_SpecialFuncion : l1 = ", "", 1 ) ; 
                break ;
            }
            default: break ;
        }
    }
}
