
#include "../include/csl.h"

// ?? the logic of exceptions *definitely* could be reworked ??
// beware getting tangled in exceptions could produce unwanted behavior
// this file may need to be rethought but it may be ok for now

int64
OVT_CheckThrowState ( )
{
    Exception *e = _O_->OVT_Exception ;
    OVT_SetRestartCondition ( e->RestartCondition ) ;
    if ( e->RestartCondition == COMPLETE_INITIAL_START )
    {
        iPrintf ( "%s", e->ExceptionMessage ) ;
        if ( ! ( e->Signal & ( SIGSEGV | SIGBUS ) ) ) if ( OVT_Pause ( 0 ) ) return 1 ;
        _OVT_SigLongJump ( & _O_->JmpBuf0 ) ;
    }
    if ( GetState ( _O_, OVT_THROW ) ) CSL_FullRestartComplete ( ) ;
    else if ( ( e->Signal & ( SIGSEGV | SIGBUS ) ) ) SetState ( _O_, OVT_THROW, true ) ;
    return 0 ;
}

// OVT_Throw and related functions should be rethought and/or cleaned up
// this is still somewhat of a mess : just haven't take time to fix

void
OVT_Throw ( ) //, Boolean pausedFlag )
{
    Exception *e = _O_->OVT_Exception ;
    sigjmp_buf * jb ;
    Word * eword ;
    if ( e->Signal )
    {
        if ( ( e->Signal == SIGTERM ) || ( e->Signal == SIGKILL ) || ( e->Signal == SIGQUIT ) || ( e->Signal == SIGSTOP ) || ( e->Signal == SIGHUP ) ) OVT_Exit ( ) ;
        else if ( ( e->Signal == SIGSEGV ) || ( e->Signal == SIGBUS ) )
        {
            e->SigSegvs ++ ;
            if ( e->SigSegvs < 2 ) OpenVmTil_ShowExceptionInfo ( ) ;
            jb = & _O_->JmpBuf0 ;
            OVT_SetRestartCondition ( INITIAL_START ) ;
            e->Message = "signal == SIGSEGV" ;
            _OVT_SimpleFinal_Key_Pause ( ) ;
            goto jump ;
        }
        else if ( ( e->Signal == SIGCHLD ) )
        {
            jb = & _O_->JmpBuf0 ;
            OVT_SetRestartCondition ( INITIAL_START ) ;
            e->Message = "signal == SIGCHLD" ;
            _OVT_SimpleFinal_Key_Pause ( ) ;
            goto jump ;
        }
        if ( ( e->RestartCondition > QUIT ) || ( e->Signal == SIGFPE ) )
        {
            if ( e->SigSegvs < 2 )
            {
                jb = & _CSL_->JmpBuf0 ;
                OpenVmTil_ShowExceptionInfo ( ) ;
                SetState ( _O_, OVT_PAUSE, true ) ;
                //pausedFlag ++ ;
                OVT_SetRestartCondition ( ABORT ) ;
            }
            else OVT_SetRestartCondition ( INITIAL_START ) ;
            if ( e->SigSegvs > 3 ) _OVT_SigLongJump ( & _O_->JmpBuf0 ) ; //OVT_Exit ( ) ;
            else if ( ( e->SigSegvs > 1 ) || ( e->RestartCondition == INITIAL_START ) ) jb = & _O_->JmpBuf0 ;
        }
        else jb = & _CSL_->JmpBuf0 ;
    }
    if ( OVT_CheckThrowState ( ) ) return ;
    _CSL_->IncludeFileStackNumber = 0 ; // we are now back at the command line
    if ( GetState ( _O_, OVT_FRC ) )
    {
        jb = & _O_->JmpBuf0 ;
    }
    else
    {
        if ( ! GetState ( _O_, OVT_PAUSE ) ) _OpenVmTil_ShowExceptionInfo ( ) ;
        if ( ! e->Signal )
        {
            if ( e->RestartCondition >= INITIAL_START ) jb = & _O_->JmpBuf0 ;
            else jb = & _CSL_->JmpBuf0 ;
        }
        //OVT_SetExceptionMessage ( _O_ ) ;
        byte * buffer = Buffer_DataCleared ( _O_->ExceptionBuffer ) ;
        eword = e->ExceptionWord = _Context_->CurrentTokenWord ; //_Context_->CurrentEvalWord ;
        snprintf ( buffer, BUFFER_IX_SIZE, "\n%s\n%s %s from %s : the current evalWord is %s.%s -> ...",
            e->ExceptionMessage ? ( char* ) e->ExceptionMessage : "", ( jb == & _CSL_->JmpBuf0 ) ? "reseting csl" : "restarting OpenVmTil",
            ( e->Signal == SIGSEGV ) ? ": SIGSEGV" : "", Context_Location ( ),
            ( eword ? ( eword->S_ContainingNamespace ? eword->S_ContainingNamespace->Name : ( byte* ) "" ) : ( byte* ) "" ), ( eword ? eword->Name : ( byte* ) "" ) ) ;
        iPrintf ( "%s", buffer ) ;
        if ( ! GetState ( _O_, OVT_PAUSE ) )
        {
            if ( ( e->SignalExceptionsHandled < 2 ) && ( e->SigSegvs < 2 ) )
            {
                if ( OVT_Pause ( 0 ) ) return ;
            }
            else if ( e->SigSegvs < 3 ) e->Message = "OVT_Throw", _OVT_SimpleFinal_Key_Pause ( ) ;
        }
    }
jump:
    _O_->Pbf8[0] = 0 ; // newline prompt control
    _OVT_SigLongJump ( jb ) ;
}

void
_OpenVmTil_ShowExceptionInfo ( )
{
    Exception *e = _O_->OVT_Exception ;
    Word * word = e->ExceptionWord ;
    Debugger * debugger = _Debugger_ ;
    DebugOn ;
    //if ( _Context_->CurrentlyRunningWord ) CSL_Show_SourceCode_TokenLine ( _Context_->CurrentlyRunningWord, "",
    //    e->RestartCondition, _Context_->CurrentlyRunningWord->Name, "" ) ;
    if ( ! e->ExceptionCode & ( STACK_ERROR | STACK_OVERFLOW | STACK_UNDERFLOW ) ) Debugger_Stack ( debugger ) ;
    if ( ! word )
    {
        word = Finder_Word_FindUsing ( _Finder_, e->ExceptionToken, 1 ) ;
        if ( ! word )
        {
            if ( e->SigAddress ) word = Word_GetFromCodeAddress ( ( byte* ) e->SigAddress ) ;
            if ( ( ! debugger->w_Word ) && ( ! debugger->Token ) )
            {
                debugger->w_Word = _Context_->CurrentEvalWord ? _Context_->CurrentEvalWord : _Context_->LastEvalWord ;
            }
        }
    }
    if ( word ) Word_Disassemble ( word ) ;
    AlertColors ;
    SetState ( debugger, DBG_INFO, true ) ;
    Debugger_Locals_Show ( debugger ) ;

    if ( GetState ( debugger, DBG_STEPPING ) )
    {
        Debugger_Registers ( debugger ) ;
        Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "", ( byte* ) "" ) ;
    }
    Debugger_ShowInfo ( debugger, e->ExceptionMessage, e->Signal ) ;
    if ( word != _Context_->LastEvalWord ) _CSL_Source ( word, 0 ) ;
    iPrintf ( "\nOpenVmTil_SignalAction : address = 0x%016lx : %s", e->SigAddress, e->Location ) ;
}

int64
OpenVmTil_ShowExceptionInfo ( )
{
    Exception *e = _O_->OVT_Exception ;
    int64 rtrn = 0 ;
    if ( Verbosity ( ) )
    {
        if ( e->ExceptionMessage )
        {
            iPrintf ( "\n%s : %s\n",
                //e->ExceptionMessage, e->ExceptionSpecialMessage ? e->ExceptionSpecialMessage : Context_Location ( ) ) ;
                e->ExceptionMessage, e->ExceptionSpecialMessage ? e->ExceptionSpecialMessage : e->Location ) ;
        }
        if ( ( e->SigSegvs < 2 ) && ( e->SignalExceptionsHandled ++ < 2 ) && _CSL_ )
        {
            _DisplaySignal ( e->Signal ) ;
            _OpenVmTil_ShowExceptionInfo ( ) ;
        }
    }
    else
    {
        rtrn = OVT_Pause ( 0 ) ;
        e->Signal = 0 ;
    }
    return rtrn ;
}

void
OVT_PauseInterpret ( Context * cntx, byte key )
{
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * svPrompt ;
    Boolean svDbgState = GetState ( _CSL_, DEBUG_MODE ) ; //| _DEBUG_SHOW_ ) ;
    Boolean svcm = GetState ( cntx->Compiler0, ( COMPILE_MODE ) ) ;
    Boolean svath = GetState ( rl, ADD_TO_HISTORY ) ;
    int64 lastKey = 0, loop = 0 ;
    OpenVmTil_AddStringToHistoryOn ( ) ;
    Set_CompileMode ( false ) ;
    DebugOff ;
    ReadLine_Init ( rl, _CSL_Key ) ;
    SetState ( cntx, AT_COMMAND_LINE, true ) ;
    if ( ( key <= ' ' ) || ( key == '\\' ) ) key = 0 ;
    iPrintf ( "\nPause interpreter : hit <enter> with an empy line to exit : \n" ) ;
    do
    {
        svPrompt = ReadLine_GetPrompt ( rl ) ;
        ReadLine_SetPrompt ( rl, "=> " ) ;
        _O_->Pbf8[0] = 0 ; // trigger for Prompt
        DoPrompt ( ) ;
        _ReadLine_GetLine ( rl, key ) ;
        if ( ReadLine_PeekNextChar ( rl ) < ' ' ) break ; // '\n', <esc>, etc.
        SetState ( _Lexer_, ( END_OF_LINE | END_OF_STRING ), false ) ; // controls Interpret_ToEndOfLine
        Interpret_ToEndOfLine ( cntx->Interpreter0 ) ;
        if ( key == lastKey ) loop ++ ; // stop looping here, just noticed 0.907.39x??
        else loop = 0 ;
        lastKey = key ;
    }
    while ( loop < 5 ) ;
    ReadLine_SetPrompt ( rl, svPrompt ) ;
    SetState ( cntx, AT_COMMAND_LINE, false ) ;
    ReadLine_SetRawInputFunction ( rl, ReadLine_GetNextCharFromString ) ;
    SetState ( _Context_->ReadLiner0, ADD_TO_HISTORY, svath ) ;
    SetState ( cntx->Compiler0, ( COMPILE_MODE ), svcm ) ;
    //SetState ( _CSL_, DEBUG_MODE | _DEBUG_SHOW_, svDbgState ) ;
    SetState ( _CSL_, DEBUG_MODE, svDbgState ) ;
}

int64
OVT_Pause ( byte * prompt )
{
    Exception *e = _O_->OVT_Exception ;
    int64 rtrn = 0 ;
    if ( _CSL_ && _Context_ )
    {
        SetState ( _O_, OVT_PAUSE, true ) ;
        Debugger * debugger = _Debugger_ ;
        if ( _Context_->CurrentlyRunningWord ) _Debugger_->ShowLine = ( byte* ) "" ;
        byte * buffer = Buffer_DataCleared ( _CSL_->StringInsertB4 ), *defaultPrompt =
            ( byte * ) "\n%s\n%s : at %s : %s:: <key>/(c)ontinue (d)ebugger s(t)ack '\\'/(i)interpret (q)uit e(x)it, <esc> cancel%s" ;
        snprintf ( ( char* ) buffer, BUFFER_IX_SIZE, prompt ? ( char* ) prompt : ( char* ) defaultPrompt,
            e->ExceptionMessage ? ( char* ) e->ExceptionMessage : "\r",
            c_gd ( "pause" ), e->Location,
            c_gd ( _Debugger_->ShowLine ? _Debugger_->ShowLine : String_RemoveFinalNewline ( _Context_->ReadLiner0->InputLine ) ),
            c_gd ( "\n-> " ) ) ;
        DebugColors ;
        int64 tlw = Strlen ( defaultPrompt ) ;
        if ( tlw > _CSL_->TerminalLineWidth ) _CSL_->TerminalLineWidth = tlw ;
        if ( e->SignalExceptionsHandled ) iPrintf ( "\n_OVT_Pause : Signals Handled = %d : signal = %d : restart condition = %d\n",
            e->SignalExceptionsHandled, e->Signal, e->RestartCondition ) ;
        do
        {
            if ( ( ! debugger->w_Word ) && ( ! debugger->Token ) ) debugger->w_Word = Context_CurrentWord ( ) ;
            _Debugger_ShowInfo ( _Debugger_, ( byte* ) "\r", e->Signal, 0 ) ;
            iPrintf ( "%s", buffer ) ;

            int64 key = Key ( ) ;
            Clear_Terminal_Line ( ) ;
            //iPrintf ( "\nPause : press <enter> or <space> to continue : \n" ) ;
            switch ( key )
            {
                case 'x': case 'X':
                {
                    byte * msg = ( byte * ) "Exit csl from pause?" ;
                    iPrintf ( "\n%s : 'x' to e(x)it csl : any other <key> to cancel%s", msg, c_gd ( "\n-> " ) ) ;
                    key = Key ( ) ;
                    if ( ( key == 'x' ) || ( key == 'X' ) ) OVT_Exit ( ) ;
                    break ;
                }
                case 'q':
                {
                    byte * msg = ( byte * ) "Quit to interpreter loop from pause?" ;
                    iPrintf ( "\n%s : 'q' to (q)uit : any other key to cancel%s", msg, c_gd ( "\n-> " ) ) ;
                    key = Key ( ) ;
                    if ( ( key == 'q' ) || ( key == 'Q' ) ) DefaultColors, _CSL_Quit ( 1 ) ;
                    break ;
                }
                case ESC:
                {
                    iPrintf ( "\ncancelling last command and continuing as before with DebugOff\n" ) ;
                    CSL_DebugOff ( ) ;
                    rtrn = 1 ;
                    goto done ;
                }
                case 't':
                {
                    CSL_PrintDataStack ( ) ;
                    break ;
                }
                case '\\':
                case 'd':
                {
                    _CSL_DebugOn ( ) ;
                    goto done ;
                    //fall thru to interpret
                }
                case 'i':
                {
                    if ( _O_->OVT_Exception->RestartCondition < RESET_ALL )
                    {
                        Context * cntx = CSL_Context_PushNew ( _CSL_ ) ;
                        OVT_PauseInterpret ( cntx, 0 ) ; //key ) ;
                        CSL_Context_PopDelete ( _CSL_ ) ;
                        break ;
                    }
                    goto done ;
                }
                case 'c':
                default: return 0 ; // continue ;
            }
        }
        while ( 1 ) ;
    }
done:
    DefaultColors ;
    SetState ( _O_, OVT_PAUSE, false ) ;

    return rtrn ;
}

int64
_OpenVmTil_Pause ( )
{
    Exception *e = _O_->OVT_Exception ;
    //e->Location = msg ;
    if ( e )
    {
        iPrintf ( "\n%s", e->Location ) ;
        e->RestartCondition = PAUSE ;
    }
    return OVT_Pause ( 0 ) ;
}

void
OpenVmTil_Pause ( )
{
    Exception *e = _O_->OVT_Exception ;
    DebugColors ;
    if ( e ) e->Location = Context_Location ( ) ;
    _OpenVmTil_Pause ( ) ;
}

void
OVT_ResetSignals ( int64 signals )
{
    sigset_t signal_set ;
    sigemptyset ( &signal_set ) ;
    sigaddset ( &signal_set, signals ) ;
    sigprocmask ( SIG_UNBLOCK, &signal_set, NULL ) ;
}

void
_OVT_SigLongJump ( sigjmp_buf * jb )
{
    siglongjmp ( *jb, 1 ) ;
}

void
OVT_SigLongJump ( byte * restartMessage, sigjmp_buf * jb )
{
    printf ( "\n%s\n", restartMessage ), fflush ( stdout ) ;
    _OVT_SigLongJump ( jb ) ;

}

void
OVT_SetRestartCondition ( int64 restartCondition )
{
    Exception *e = _O_->OVT_Exception ;
    e->LastRestartCondition = e->RestartCondition ;
    e->RestartCondition = restartCondition ;
}

// OVT_Throw needs to be reworked ???

void
OpenVmTil_Throw ( byte * excptMessage, byte * specialMessage, int64 restartCondition, int64 infoFlag )
{
    Exception *e = _O_->OVT_Exception ;
    e->ExceptionMessage = excptMessage ;
    _O_->Thrown = e->RestartCondition = restartCondition ;
    e->ExceptionSpecialMessage = specialMessage ;
#if 1
    //LinuxInit ( ) ; // reset termios
    if ( e->InfoFlag = infoFlag )
    {
        if ( OpenVmTil_ShowExceptionInfo ( ) ) return ;
    }
#endif     
    OVT_Throw ( ) ;
}

void
_OpenVmTil_LongJmp_WithMsg ( int64 restartCondition, byte * msg, int64 pauseFlag )
{
    OpenVmTil_Throw ( msg, 0, restartCondition, 0 ) ;
}

void
OpenVmTil_SignalAction ( int signal, siginfo_t * si, void * uc ) //nb. void ptr (uc) necessary 
{
    if ( _O_ )
    {
        Exception *e = _O_->OVT_Exception ;
        e->Signal = signal ;
        e->SigAddress = si->si_addr ; //( Is_DebugOn && _Debugger_->DebugAddress ) ? _Debugger_->DebugAddress : si->si_addr ;
        //e->Location = ( ( ! ( signal & ( SIGSEGV | SIGBUS ) ) ) && _Context_ ) ? ( byte* ) c_gd ( Context_Location ( ) ) : ( byte* ) "" ;
        e->Location = _Context_ ? ( byte* ) c_gd ( Context_Location ( ) ) : ( byte* ) "" ;
        if ( ( signal == SIGTTIN ) || ( signal == SIGCHLD ) ) return ;
        if ( ( signal != SIGWINCH ) && ( signal != SIGCHLD ) ) iPrintf ( "\nOpenVmTil_SignalAction :: signal = %d\n", signal ) ; // 28 = SIGWINCH window resizing
        if ( ( signal == SIGTERM ) || ( signal == SIGKILL ) || ( signal == SIGQUIT ) || ( signal == SIGSTOP ) ) OVT_Exit ( ) ;
        OVT_ResetSignals ( e->Signal ) ;
        if ( ( signal >= SIGCHLD ) || ( signal == SIGTRAP ) ) //||( signal == SIGBUS ))
        {
            if ( ( signal != SIGCHLD ) && ( signal != SIGWINCH ) && ( signal != SIGTRAP ) ) OpenVmTil_ShowExceptionInfo ( ) ;
            else
            {
                // ignore this category -- just return
                e->SigAddress = 0 ; //|| ( signal == SIGWINCH ) ) _O_->SigAddress = 0 ; // 17 : "CHILD TERMINATED" : ignore; its just back from a shell fork
                e->Signal = 0 ;
            }
        }
        else
        { //++ e->SigSegvs ;
            if ( e->SigSegvs >= 2 )
            {
                //if ( e->SigSegvs >= 2 ) 
                OVT_SeriousErrorPause ( "OpenVmTil_SignalAction : SigSegv" ) ;
                _OVT_SigLongJump ( & _O_->JmpBuf0 ) ;
            }
            else OVT_Throw ( ) ;
        }
    }
    else _OVT_SigLongJump ( & _OSMS_->JmpBuf0 ) ; // ?! doesn't work
}

void
CSL_Exception ( int64 exceptionCode, byte * message, int64 restartCondition )
{
    Exception *e = _O_->OVT_Exception ;
    AlertColors ;
    e->ExceptionMessage = message ;
    e->ExceptionCode = exceptionCode ;
    e->RestartCondition = restartCondition ;
    //e->Location = ( ( ! ( signal & ( SIGSEGV | SIGBUS ) ) ) && _Context_ ) ? ( byte* ) c_gd ( Context_Location ( ) ) : ( byte* ) "" ;
    e->Location = _Context_ ? ( byte* ) c_gd ( Context_Location ( ) ) : ( byte* ) "" ;
    iPrintf ( "\n\nCSL_Exception at %s : %s\n", e->Location, message ? message : ( byte* ) "" ) ;
    switch ( exceptionCode )
    {
        case CASE_NOT_LITERAL_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Syntax Error : \"case\" only takes a literal/constant as its parameter after the block", 0, restartCondition, 1 ) ;
            break ;
        }
        case DEBUG_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Debug Error : User is not in debug mode", 0, restartCondition, 1 ) ;
            break ;
        }
        case OBJECT_REFERENCE_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Object Reference Error", 0, restartCondition, 1 ) ;
            break ;
        }
        case OBJECT_SIZE_ERROR:
        {
            byte * b = Buffer_DataCleared ( _CSL_->ScratchB2 ) ;
            sprintf ( ( char* ) b, "Exception : Warning : Class object size is 0. Did you declare 'size' for %s? ",
                _Context_->CurrentlyRunningWord ? _Context_->CurrentlyRunningWord->ContainingNamespace->Name : ( byte * ) "" ) ;
            OpenVmTil_Throw ( b, 0, restartCondition, 1 ) ;
            break ;
        }
        case STACK_OVERFLOW:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Stack Overflow", 0, restartCondition, 1 ) ;
            break ;
        }
        case STACK_UNDERFLOW:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Stack Underflow", 0, restartCondition, 1 ) ;
            break ;
        }
        case STACK_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Stack Error", 0, restartCondition, 1 ) ;
            break ;
        }
        case SEALED_NAMESPACE_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : New words can not be added to sealed namespaces", 0, restartCondition, 1 ) ;
            break ;
        }
        case NAMESPACE_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Namespace (Not Found?) Error", 0, restartCondition, 1 ) ;
            break ;
        }
        case SYNTAX_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Syntax Error", message, restartCondition, 1 ) ;
            break ;
        }
        case NESTED_COMPILE_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Nested Compile Error", 0, restartCondition, 1 ) ;
            break ;
        }
        case COMPILE_TIME_ONLY:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Compile Time Use Only", 0, restartCondition, 1 ) ;
            break ;
        }
        case BUFFER_OVERFLOW:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Buffer Overflow", 0, restartCondition, 1 ) ;
            break ;
        }
        case MEMORY_ALLOCATION_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Memory Allocation Error", 0, restartCondition, 1 ) ;
            break ;
        }
        case LABEL_NOT_FOUND_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Word not found. Misssing namespace qualifier?", 0, QUIT, 1 ) ;
            break ;
        }
        case NOT_A_KNOWN_OBJECT:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Not a known object", message, QUIT, 1 ) ;
            break ;
        }
        case ARRAY_DIMENSION_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Array has no dimensions!?", 0, QUIT, 1 ) ;
            break ;
        }
        case INLINE_MULTIPLE_RETURN_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Multiple return points in a inlined function", 0, restartCondition, 1 ) ;
            break ;
        }
        case MACHINE_CODE_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : in machine coding", 0, restartCondition, 1 ) ;
            break ;
        }
        case VARIABLE_NOT_FOUND_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Variable not found error", 0, restartCondition, 1 ) ;
            break ;
        }
        case USEAGE_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Useage Error", 0, restartCondition, 1 ) ;
            break ;
        }
        case FIX_ME_ERROR:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Fix Me", 0, restartCondition, 1 ) ;
            break ;
        }
        case OUT_OF_CODE_MEMORY:
        {
            OpenVmTil_Throw ( ( byte* ) "Exception : Out of Code Memory : Increase Code Memory Size for Startup!!", 0, INITIAL_START, 1 ) ;
            break ;
        }
        default:
        {
            OpenVmTil_Throw ( message, 0, restartCondition, 1 ) ;
            break ;
        }
    }
    return ;
}

void
CSL_SystemBreak ( )
{
    _OpenVmTil_LongJmp_WithMsg ( BREAK, ( byte* ) "System.interpreterBreak : returning to main interpreter loop.", 0 ) ;
}

void
_CSL_Quit ( int64 pauseFlag )
{
    _OpenVmTil_LongJmp_WithMsg ( QUIT, ( byte* ) "Quit : Session Memory, temporaries, are reset.", pauseFlag ) ;
}

void
CSL_Quit ( )
{
    _CSL_Quit ( 0 ) ;
}

void
CSL_Abort ( )
{
    _OpenVmTil_LongJmp_WithMsg ( ABORT, ( byte* ) "Abort : Session Memory and the DataStack are reset (as in a cold restart).", 0 ) ;
}

void
CSL_DebugStop ( )
{
    _OpenVmTil_LongJmp_WithMsg ( STOP, ( byte* ) "Stop : Debug Stop. ", 0 ) ;
}

void
CSL_ResetAll ( )
{
    _OpenVmTil_LongJmp_WithMsg ( RESET_ALL, ( byte* ) "ResetAll. ", 0 ) ;
}

void
CSL_Restart ( )
{
    _OpenVmTil_LongJmp_WithMsg ( RESTART, ( byte* ) "Restart. ", 0 ) ;
}

void
CSL_WarmInit ( )
{
    _CSL_Init_SessionCore ( _CSL_, 1, 1 ) ;
}

// cold init

void
CSL_RestartInit ( )
{
    _OpenVmTil_LongJmp_WithMsg ( RESET_ALL, ( byte* ) "Restart Init... ", 0 ) ;
}

void
CSL_FullRestart ( )
{
    Exception *e = _O_->OVT_Exception ;
    e->Signal = 0 ;
    SetState ( _O_, OVT_THROW, false ) ;
    _OpenVmTil_LongJmp_WithMsg ( INITIAL_START, ( byte* ) "Full Initial Re-Start : ...", 0 ) ;
}

void
CSL_FullRestartComplete ( )
{
    Exception *e = _O_->OVT_Exception ;
    e->Signal = 0 ;
    _O_->State = OVT_FRC ;
    iPrintf ( "\nHistory will be deleted with this FullRestartComplete... \n" ) ;
    //_O_->Verbosity = 0 ;
    _OpenVmTil_LongJmp_WithMsg ( COMPLETE_INITIAL_START, ( byte* ) "Complete Initial Re-Start : ...\n", 0 ) ;
}

void
_Error ( byte * msg, uint64 state )
{
    AlertColors ;
    //if ( ( state ) >= PAUSE )
    CSL_NewLine ( ) ;
    CSL_Location ( ) ;
    iPrintf ( c_a ( msg ) ) ;
    if ( _Context_->CurrentlyRunningWord )
        CSL_Show_SourceCode_TokenLine ( _Context_->CurrentlyRunningWord, "",
        _O_->OVT_Exception->RestartCondition, _Context_->CurrentlyRunningWord ? _Context_->CurrentlyRunningWord->Name : ( byte* ) "", "" ) ;
    else ReadLine_ShowInfo ( _ReadLiner_ ) ;
    Pause ( ) ;
    if ( ( state ) >= QUIT )
    {
        Throw ( msg, "", state ) ;
    }
    else DebugColors ;
}

void
OVT_ExceptionState_Print ( )
{
    Exception *e = _O_->OVT_Exception ;
    iPrintf ( "\nSignalExceptionsHandled = %d ; SigSegvs = %d ; Restarts = %d\nStartedTimes = %d ; RestartCondition = %s ; LastRestartCondtion = %s\n",
        e->SignalExceptionsHandled, e->SigSegvs, e->Restarts, e->StartedTimes, Convert_RestartCondtion ( e->LastRestartCondition ),
        Convert_RestartCondtion ( e->RestartCondition ) ) ;
}

byte
_OVT_SimpleFinal_Key_Pause ( )
{
    Exception *e = _O_->OVT_Exception ;
    byte key = 0 ;
    if ( ! GetState ( e, EXCEPTION_SIMPLE_KEY_PAUSED ) )
    {
        SetState ( e, EXCEPTION_SIMPLE_KEY_PAUSED, true ) ;
        byte * msg = e->Message ;
        //OVT_CheckThrowState ( ) ;
        //byte * msg = Buffer_DataCleared ( ovt->PrintBuffer ) ;
        CSL_Show_ErrorCommandLine ( ) ;
        if ( ! msg ) msg = "" ;
        byte * instr = ".: (p)ause, e(x)it, <key> restart" ;
        if ( e->SigSegvs < 3 ) printf ( "\n%s\n%s : at %s : (SIGSEGVs == %ld)", msg, instr,
            //( ( e->SigSegvs < 2 ) ? Context_Location ( ) : ( byte* ) "" ), e->SigSegvs ), fflush ( stdout ) ;
            e->Location, e->SigSegvs ), fflush ( stdout ) ;
        //( ( byte* ) "" ), e->SigSegvs ), fflush ( stdout ) ;
        key = Key ( ) ;
        if ( key == 'p' )
        {
            Pause ( ) ;
            return false ;
        }
        else if ( key == 'x' ) OVT_Exit ( ) ;
        SetState ( e, EXCEPTION_SIMPLE_KEY_PAUSED, true ) ;
    }
    return key ;
}

void
OVT_SeriousErrorPause ( byte * msg )
{
    Exception *e = _O_->OVT_Exception ;
    e->Message = msg ;
    if ( e->SigSegvs < 3 ) _OVT_SimpleFinal_Key_Pause ( ) ;
    else
    {
        fflush ( stdout ) ;
        OVT_Exit ( ) ;
    }
}

Exception *
Exception_New ( )
{
    Exception * e = ( Exception * ) Mem_Allocate ( sizeof (Exception ), EXCEPTION_SPACE ) ;
    return e ;
}

void
Exception_Init ( Exception * e )
{
    Mem_Clear ( ( byte* ) e, sizeof (Exception ) ) ;
}

#if 0

void
OVT_Assert ( Boolean testBoolean, byte * msg )
{
    d1
        (
    if ( ! ( testBoolean ) )
    {
        iPrintf ( "\nAssert failed : %s : at %s", msg ? msg : ( byte* ) "", _Context_Location ( _Context_ ) ) ;
            _Throw ( QUIT ) ;
    }
    ) ;
}

void
OVT_SetExceptionMessage ( OpenVmTil * ovt )
{
    if ( ovt->OVT_Exception->RestartCondition == INITIAL_START ) ovt->ExceptionMessage = ( byte* ) "Full Initial Re-Start : ..." ;
    else if ( ovt->OVT_Exception->RestartCondition == ABORT ) ovt->ExceptionMessage = ( byte* ) "Aborting : ..." ;
}
#endif

