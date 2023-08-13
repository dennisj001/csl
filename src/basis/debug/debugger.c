
#include "../../include/csl.h"

#if 0

void
dbg_new ( Word * w )
{
    if ( ( uint64 ) w == 0x7ffff4928020 )
        iPrintf ( "\n got it at %s", Context_Location ( ) ) ;
}
#endif

Boolean
DBG_Interpret_Loop_Test ( Debugger * debugger )
{
    Boolean rtn = ( ( ! GetState ( debugger, DBG_INTERPRET_LOOP_DONE ) ) || GetState ( debugger, DBG_STEPPING ) ||
        ( ( GetState ( debugger, DBG_AUTO_MODE ) ) && ( ! ( GetState ( debugger, DBG_EVAL_AUTO_MODE ) ) ) ) ) ;
    return rtn ;
}

void
Debugger_InterpreterLoop ( Debugger * debugger )
{
    do
    {
        Debugger_DoState ( debugger ) ;
        if ( ! GetState ( _Debugger_, DBG_AUTO_MODE | DBG_AUTO_MODE_ONCE ) )
        {
            while ( ( debugger->Key = Key ( ) ) == - 1 ) ;
            if ( debugger->Key != 'z' ) debugger->SaveKey = debugger->Key ;
        }
        SetState ( _Debugger_, DBG_AUTO_MODE_ONCE, false ) ;
        debugger->CharacterFunctionTable [ debugger->CharacterTable [ debugger->Key ] ] ( debugger ) ;
    }
    while ( DBG_Interpret_Loop_Test ( debugger ) ) ;
    debugger->LastPreSetupWord = debugger->w_Word ;
    SetState ( debugger, ( DBG_STACK_OLD | DBG_INTERPRET_LOOP_DONE ), true ) ;
    SetState ( debugger, DBG_STEPPING, false ) ;
    if ( GetState ( debugger, ( DBG_SETUP_ADDRESS ) ) )
    {
        SetState ( debugger, ( DBG_SETUP_ADDRESS ), false ) ;
        SetState ( debugger->w_Word, W_STEPPED, true ) ;
    }
    else if ( GetState ( debugger, DBG_STEPPED ) //&& ( ! Stack_Depth ( debugger->ReturnStack ) )
        && ( ! ( GetState ( debugger, DBG_RUNTIME_BREAKPOINT ) ) ) )
    {
        SetState ( debugger, DBG_STEPPING, false ) ;
        siglongjmp ( _Context_->JmpBuf0, 1 ) ;
    }
}

void
Debugger_Setup_RecordState ( Debugger * debugger, Word * word, byte * token, byte * address )
{
    if ( word && ( word->W_AliasOf ) )
    {
        debugger->w_Alias = word ;
        debugger->w_AliasOf = Word_UnAlias ( word ) ;
    }
    debugger->PreHere = debugger->SpecialPreHere ? debugger->SpecialPreHere : Here ;
    if ( word ) debugger->RL_ReadIndex = word->W_RL_Index ;
    debugger->w_Word = word ;
    SetState ( debugger, DBG_COMPILE_MODE, CompileMode ) ;
    SetState_TrueFalse ( debugger, DBG_ACTIVE | DBG_INFO | DBG_PROMPT,
        DBG_CANT_STEP_EVAL | DBG_BRK_INIT | DBG_CONTINUE_MODE | DBG_INTERPRET_LOOP_DONE | DBG_PRE_DONE | DBG_CONTINUE | DBG_STEPPING | DBG_STEPPED ) ;
    debugger->SaveDsp = debugger->AddressModeSaveDsp = _DspReg_ ;
    if ( ! debugger->StartHere ) Debugger_Set_StartHere ( debugger ) ;
    debugger->WordDsp = _DspReg_ ;
    debugger->SaveTOS = TOS ;
    debugger->Token = ( word && word->Name ) ? word->Name : token ;
    if ( address > ( byte* ) 0x700000000000 )
    {
        SetState ( debugger, DBG_SETUP_ADDRESS, true ) ;
        SetState ( debugger, ( DBG_AUTO_MODE | DBG_CONTINUE_MODE ), false ) ;
        debugger->DebugAddress = address ;
        Debugger_SetupStepping ( debugger ) ;
    }
}

void
Debugger_Setup_ResetState ( Debugger * debugger )
{
    if ( ! GetState ( debugger, DBG_STEPPING ) ) debugger->DebugAddress = 0 ;
    debugger->w_Alias = 0 ;
    debugger->w_AliasOf = 0 ;
    debugger->w_Word = 0 ;
    debugger->Token = 0 ;
    debugger->SpecialPreHere = 0 ;
}

void
Debugger_Setup_SaveState ( Debugger * debugger, Word * word )
{
    SetState ( debugger, DBG_MENU, false ) ;
    debugger->LastPreSetupWord = word ;
}

void
Debugger_Interpret ( Debugger * debugger, Word * word, byte * token, byte * address )
{
    Debugger_Setup_RecordState ( debugger, word, token, address ) ;

    DebugColors ;
    Debugger_InterpreterLoop ( debugger ) ; // core of this function
    DefaultColors ;

    Debugger_Setup_SaveState ( debugger, word ) ;
    //if ( Is_DebugOn ) iPrintf ( "" ) ;
}

Boolean
Dbg_ShouldWeSetup ( Debugger * debugger, Word * word, byte * token, byte * address, Boolean force, int64 debugLevel )
{
    Boolean rtn = ( ( _CSL_->DebugLevel >= debugLevel ) && ( force || GetState ( _Compiler_, LC_ARG_PARSING ) || address || token
        || ( word && ( ( word != debugger->LastPreSetupWord )
        && ( word->WL_OriginalWord ? ( word->WL_OriginalWord != debugger->LastPreSetupWord ) : 1 ) || debugger->RL_ReadIndex != word->W_RL_Index ) ) ) ) ;
    return rtn ;
}

Boolean
Debugger_PreSetup ( Debugger * debugger, Word * word, byte * token, byte * address, Boolean force, int64 debugLevel )
{
    Boolean rtn = false ;
    if ( Is_DebugModeOn && Is_DebugShowOn )
    {
        if ( Dbg_ShouldWeSetup ( debugger, word, token, address, force, debugLevel ) )
        {
            if ( ( ! word ) && ( ! token ) ) word = Context_CurrentWord ( ) ;
            if ( token && ( ! word ) ) word = _Interpreter_TokenToWord ( _Interpreter_, token, - 1, - 1 ) ;
            if ( force || ( word && word->Name[0] ) || token )
            {
                Debugger_Interpret ( debugger, word, token, address ) ;
                rtn = true ;
            }
        }
    }
    return rtn ;
}

void
_Debug_Setup ( Word * word, byte * token, byte * address, Boolean force, int64 debugLevel )
{
    Debugger_PreSetup ( _Debugger_, word, token, address, force, debugLevel ) ;
}

void
_Debugger_PostShow ( Debugger * debugger, Word * word, int64 debugLevel, Boolean force )
{
    if ( word && ( word->W_MorphismAttributes & DEBUG_WORD ) ) return ;
    else _Debugger_ShowEffects ( debugger, word, GetState ( debugger, DBG_STEPPING ), debugLevel, force ) ;
}

void
DebugRuntimeBreakpoint ( )
{
    Debugger * debugger = _Debugger_ ;
    if ( ! GetState ( debugger, ( DBG_BRK_INIT | DBG_CONTINUE_MODE ) ) )
    {
        SetState ( debugger, ( DBG_UDIS | DBG_BRK_INIT | DBG_RUNTIME_BREAKPOINT ), true ) ;
        if ( ! GetState ( debugger, ( DBG_STEPPING | DBG_AUTO_MODE ) ) )
        {
            Debugger_On ( debugger ) ;
            if ( ( ! debugger->DebugAddress ) && debugger->cs_Cpu->Rsp )
            {
                // remember : _Compile_CpuState_Save ( _Debugger_->cs_Cpu ) ; is called thru _Compile_Debug : <dbg>/<dso>
                debugger->DebugAddress = Debugger_GetDbgAddressFromRsp ( debugger, debugger->cs_Cpu ) ;
                debugger->w_Word = Word_GetFromCodeAddress ( debugger->DebugAddress ) ;
            }
            iPrintf ( "\nDebugRuntimeBreakpoint : debugger->DebugAddress = %lx", debugger->DebugAddress ) ;
            Debugger_SetupStepping ( debugger ) ;
            SetState_TrueFalse ( debugger, DBG_RUNTIME | DBG_ACTIVE | DBG_RUNTIME_BREAKPOINT | DEBUG_SHTL_OFF,
                DBG_INTERPRET_LOOP_DONE | DBG_PRE_DONE | DBG_CONTINUE | DBG_NEWLINE | DBG_PROMPT | DBG_INFO | DBG_MENU ) ;
        }
    }
    else
    {
        debugger->DebugAddress += 3 ; // 3 : sizeof 'call rax' insn :: skip the call else we would recurse to here
        if ( GetState ( debugger, ( DBG_CONTINUE_MODE ) ) ) SetState ( debugger, ( DBG_BRK_INIT | DBG_RUNTIME_BREAKPOINT ), true ) ;
    }
    SetState ( _Debugger_, ( DBG_AUTO_MODE | DBG_AUTO_MODE_ONCE ), false ) ;
    Debugger_Interpret ( debugger, debugger->w_Word, 0, debugger->DebugAddress ) ;
    SetState ( debugger, DBG_BRK_INIT | DBG_RUNTIME_BREAKPOINT | DEBUG_SHTL_OFF, false ) ;
    if ( GetState ( debugger, DBG_STEPPED ) ) //&& ( ! Stack_Depth ( debugger->ReturnStack ) ) )
    {
        SetState ( debugger, DBG_STEPPING, false ) ;
        siglongjmp ( _Context_->JmpBuf0, 1 ) ;
    }
}

void
_Debugger_Init ( Debugger * debugger )
{
    if ( debugger )
    {
        Stack_Init ( debugger->ReturnStack ) ;
        if ( ! Is_DebugOn )
        {
            debugger->StartHere = 0 ;
            debugger->PreHere = 0 ;
            debugger->DebugAddress = 0 ;
            debugger->w_Word = 0 ;
            debugger->CopyRSP = 0 ;
            SetState ( debugger, ( DBG_UDIS | DBG_STACK_OLD ), true ) ;
        }
    }
}

void
Debugger_Init ( Debugger * debugger, Cpu * cpu, Word * word, byte * address )
{
    if ( ! Is_DebugOn )
    {
        _Debugger_Init ( debugger ) ;
        Debugger_UdisNew ( debugger ) ;
        debugger->SaveDsp = _DspReg_ ;
        debugger->InitDsp = _DspReg_ ;
        debugger->SaveTOS = TOS ;
        debugger->Key = 0 ;
    }

    DebugColors ;
    if ( address ) debugger->DebugAddress = address ;
    if ( ! GetState ( debugger, DBG_BRK_INIT ) ) debugger->State = DBG_MENU | DBG_INFO | DBG_PROMPT ;
    if ( debugger->DebugAddress ) debugger->w_Word = word = Word_GetFromCodeAddress ( debugger->DebugAddress ) ;
    debugger->Menu =
        "\nDebug Menu at : \n%s :\n(m)enu, so(U)rce, dum(p), (e)val, (d)is, dis(a)ccum, dis(A)ccum, (r)egisters, (l)ocals, (v)ariables, (I)nfo, (W)dis, s(h)ow, debu(g) "
        "\n(R)eturnStack, sto(P), (S)tate, (c)ontinue, (s)tep, (o)ver, (i)nto, o(u)t, t(h)ru, s(t)ack, auto(z), (V)erbosity, (q)uit, a(B)ort, (u)dis toggle"
        "\nusi(N)g, s(H)ow DebugWordList, sh(O)w CompilerWordList, Goto(L)ist_Print, T(y)peStackPrint, Debug_Show_Of(f), Debug_Show_O(n), (w)diss, e(x)it"
        "\n'\\n' - escape, , '\\\' - <esc> - escape, ' ' - <space> - continue" ;
}

void
Debugger_Off ( Debugger * debugger, int64 debugOffFlag )
{
    if ( Is_DebugOn )
    {
        _Debugger_Init ( debugger ) ;
        SetState ( debugger->cs_Cpu, CPU_SAVED, false ) ;
        SetState ( _Debugger_, DBG_BRK_INIT | DBG_ACTIVE | DBG_STEPPING | DBG_PRE_DONE | DBG_AUTO_MODE | DBG_EVAL_AUTO_MODE | DBG_SHOW, false ) ;
        if ( debugOffFlag ) DebugOff ;
    }
}

void
Debugger_On ( Debugger * debugger )
{
    if ( ( ! Is_DebugOn ) || GetState ( debugger, ( DBG_BRK_INIT | DBG_RUNTIME_BREAKPOINT ) ) )
    {
        Debugger_Init ( debugger, debugger->cs_Cpu, 0, 0 ) ;
        DebugOn ;
        DebugShow_On ;
    }
    SetState_TrueFalse ( debugger, ( DBG_MENU | DBG_INFO | DBG_SOURCE | DBG_SHOW | DBG_UDIS ),
        ( DBG_CONTINUE_MODE | DBG_CANT_STEP_EVAL | DBG_STEPPING | DBG_AUTO_MODE | DBG_PRE_DONE | DBG_INTERPRET_LOOP_DONE ) ) ;
    debugger->LastPreSetupWord = 0 ;
    debugger->LastScwi = 0 ;
    debugger->PreHere = 0 ;
    debugger->ReturnAddress = 0 ;
    Debugger_Set_StartHere ( debugger ) ;
    _Debugger_ShowOn ( debugger ) ;
}

// remember : the debugger is not calling and returning with call/ret instructions 
// no, those are emulated we are just pushing the return address on our debugger->ReturnStack 
// and executing the instructions starting at the call address 
// the ret insn just pops our debugger->ReturnStack
// so there will be no change in the rsp reg during this process

byte *
Debugger_GetDbgAddressFromRsp ( Debugger * debugger, Cpu * cpu )
{
    Word * word, *lastWord = 0, *currentlyRunning = Word_UnAlias ( _Context_->CurrentlyRunningWord ) ;
    byte * addr, *retAddr ;
    dllist * retStackList = List_New ( COMPILER_TEMP ) ;
    int64 i0, i1, i2, d ;
    if ( Is_DebugOn && ( Verbosity ( ) > 1 ) ) CSL_PrintReturnStack ( ) ;
    for ( i0 = 0 ; i0 < ( K ) ; i0 ++ ) // K : sizeof ReturnStack
    {
        addr = ( ( byte* ) cpu->Rsp[i0] ) ;
        word = Word_UnAlias ( Word_GetFromCodeAddress ( addr ) ) ;
        if ( word && ( String_Equal ( word->Name, "<dbg>" ) || String_Equal ( word->Name, "<d:dbg>" ) || String_Equal ( word->Name, "<dso>" ) ) )
        {
            debugger->DebugAddress = ( byte* ) cpu->Rsp[i0 + 1] ;
        }
        if ( word )
        {
            _List_PushNew_1Value ( retStackList, WORD_RECYCLING, 0, cpu->Rsp[i0] ) ;
            if ( String_Equal ( word->Name, currentlyRunning->Name ) )
                break ;
        }
        else break ;
    }
    d = List_Depth ( retStackList ) ;
    if ( d > 1 )
    {
        Stack_Init ( debugger->ReturnStack ) ;
        for ( i1 = 0 ; i1 < d ; i1 ++ )
        {
            retAddr = ( byte * ) List_Pop_1Value ( retStackList ) ;
            if ( retAddr )
            {
                word = Word_UnAlias ( Word_GetFromCodeAddress ( retAddr ) ) ;
                if ( word && ( word == lastWord ) ) continue ;
                lastWord = word ;
            }
            Stack_Push ( debugger->ReturnStack, ( uint64 ) retAddr ) ;
        }
        int64 rsDepthPick = 1 ;
        debugger->DebugAddress = ( byte* ) _Stack_Pick ( debugger->ReturnStack, rsDepthPick ) ;
        for ( i2 = 0 ; i2 <= rsDepthPick ; i2 ++ ) Stack_Pop ( debugger->ReturnStack ) ; // pop the intro functions
    }
    else debugger->DebugAddress = ( byte* ) cpu->Rsp[0] ;
done:
    debugger->w_Word = Word_UnAlias ( Word_GetFromCodeAddress ( debugger->DebugAddress ) ) ; // 21 : code size back to <dbg>
    debugger->ReturnAddress = debugger->DebugAddress + _Udis_GetInstructionSize ( debugger->Udis, debugger->DebugAddress ) ;
    return debugger->DebugAddress ;
}

byte *
Debugger_GetStateString ( Debugger * debugger )
{
    byte * buffer = Buffer_DataCleared ( _CSL_->DebugB ) ;
    sprintf ( ( char* ) buffer, "%s : %s : %s",
        GetState ( debugger, DBG_STEPPING ) ? "Stepping" : ( CompileMode ? ( char* ) "Compiling" : ( char* ) "Interpreting" ),
        ( GetState ( _CSL_, INLINE_ON ) ? ( char* ) "InlineOn" : ( char* ) "InlineOff" ),
        ( GetState ( _CSL_, CO_ON ) ? ( char* ) "OptimizeOn" : ( char* ) "OptimizeOff" )
        ) ;
    buffer = String_New ( ( byte* ) buffer, TEMPORARY ) ;

    return buffer ;
}

void
Debugger_Set_StartHere ( Debugger * debugger )
{
    debugger->StartHere = Here ;
}

void
Debugger_NextToken ( Debugger * debugger )
{
    if ( ReadLine_IsThereNextChar ( _Context_->ReadLiner0 ) ) debugger->Token = Lexer_ReadToken ( _Context_->Lexer0 ) ;
    else debugger->Token = 0 ;
}

void
Debugger_CurrentToken ( Debugger * debugger )
{
    debugger->Token = _Context_->Lexer0->OriginalToken ;
}

void
Debugger_Parse ( Debugger * debugger )
{
    Lexer_ParseObject ( _Context_->Lexer0, _Context_->Lexer0->OriginalToken ) ;
}

void
_Debugger_FindAny ( Debugger * debugger )
{
    if ( debugger->Token ) debugger->w_Word = CSL_FindInAnyNamespace ( debugger->Token ) ;
}

void
Debugger_FindAny ( Debugger * debugger )
{
    _Debugger_FindAny ( debugger ) ;
    if ( debugger->w_Word ) iPrintf ( ( byte* ) "\nFound Word :: %s.%s\n", debugger->w_Word->S_ContainingNamespace->Name, debugger->w_Word->Name ) ;
    else iPrintf ( ( byte* ) "\nToken not found : %s\n", debugger->Token ) ;
}

void
Debugger_GotoList_Print ( Debugger * debugger )
{
    Compiler_GotoList_Print ( ) ;
}

void
Debugger_Print_LispDefinesNamespace ( Debugger * debugger )
{
    LC_Print_LispDefinesNamespace ( ) ;
}

void
Debugger_FindUsing ( Debugger * debugger )
{
    if ( debugger->Token ) debugger->w_Word = Finder_Word_FindUsing ( _Context_->Finder0, debugger->Token, 0 ) ;
}

void
_Debugger_PrintDataStack ( int64 depth )
{
    Set_DataStackPointer_FromDspReg ( ) ;
    _Stack_Print ( _DataStack_, ( byte* ) "DataStack", depth, 0 ) ;
}

void
Debugger_Variables ( Debugger * debugger )
{
    CSL_Variables ( ) ;
}

void
Debugger_Using ( Debugger * debugger )
{
    CSL_Using ( ) ;
}

void
Debugger_Continue ( Debugger * debugger )
{
    if ( GetState ( debugger, DBG_RUNTIME_BREAKPOINT ) || ( GetState ( debugger, DBG_STEPPING ) && debugger->DebugAddress ) )
    {
        // always step thru code : only way known to maintain correct stack !! 
        SetState ( debugger, ( DBG_CONTINUE_MODE ), true ) ;
        if ( debugger->DebugAddress )
        {
            debugger->Key = debugger->SaveKey ;
            // continue stepping thru
            Debugger_StepLoop ( debugger ) ;
        }
        SetState ( debugger, ( DBG_CONTINUE_MODE ), false ) ;
        if ( ! GetState ( debugger, DBG_STEPPING ) ) DebugOff ;
        if ( ! debugger->DebugAddress ) SetState_TrueFalse ( debugger, ( DBG_INTERPRET_LOOP_DONE ), DBG_AUTO_MODE ) ;
    }
    else
    {
        if ( ! GetState ( debugger, DBG_EVAL_MODE ) ) DebugOff ;
        SetState_TrueFalse ( debugger, DBG_INTERPRET_LOOP_DONE, DBG_CONTINUE ) ;
    }
}

// by 'eval' we stop debugger->Stepping and //continue thru this word as if we hadn't stepped

void
_Debugger_Eval ( Debugger * debugger, int64 continueFlag )
{
    debugger->SaveStackDepth = DataStack_Depth ( ) ;
    debugger->WordDsp = _DspReg_ ;
    SetState ( debugger, ( DBG_EVAL_MODE ), true ) ;
    if ( continueFlag )
    {
        if ( Debugger_IsStepping ( debugger ) ) Debugger_Continue ( debugger ) ;
        SetState ( debugger, DBG_EVAL_AUTO_MODE, true ) ;
    }
    else if ( GetState ( debugger, DBG_AUTO_MODE ) ) SetState ( debugger, DBG_EVAL_AUTO_MODE, true ) ;
    SetState_TrueFalse ( debugger, DBG_INTERPRET_LOOP_DONE, DBG_CONTINUE ) ;
    //if ( ! GetState ( debugger, DBG_STEPPING|DBG_CONTINUE_MODE ) ) DebugOff ;
    if ( ! GetState ( debugger, DBG_AUTO_MODE ) ) SetState ( debugger, ( DBG_EVAL_MODE ), false ) ;
}

void
Debugger_Eval ( Debugger * debugger )
{
    _Debugger_Eval ( debugger, GetState ( debugger, DBG_RUNTIME_BREAKPOINT ) ) ;
}

void
Debugger_SetupNextToken ( Debugger * debugger )
{
    Debugger_NextToken ( debugger ) ;
    Debugger_FindUsing ( debugger ) ;
}

void
Debugger_Info ( Debugger * debugger )
{
    SetState ( debugger, DBG_INFO, true ) ;
}

#if DEBUG
void
Debugger_Dbg ( Debugger * debugger )
{
    dbg (0, debugger->w_Word, 0) ;
}
#endif

void
Debugger_UdisToggle ( Debugger * debugger )
{
    if ( GetState ( debugger, DBG_UDIS | DBG_UDIS_ONE ) ) SetState ( debugger, DBG_UDIS | DBG_UDIS_ONE, false ) ;
    else SetState ( debugger, DBG_UDIS, true ) ;
}

void
Debugger_DoMenu ( Debugger * debugger )
{
    SetState ( debugger, DBG_MENU | DBG_PROMPT | DBG_NEWLINE, true ) ;
}

void
Debugger_Stack ( Debugger * debugger )
{
    CSL_PrintDataStack ( ) ;
}

void
Debugger_ReturnStack ( Debugger * debugger )
{
    CSL_PrintReturnStack ( ) ;
}

void
Debugger_Source ( Debugger * debugger )
{
    Word * scWord = Compiling ? _Context_->CurrentWordBeingCompiled : GetState ( debugger, DBG_STEPPING ) ?
        Word_UnAlias ( Debugger_GetWordFromAddress ( debugger ) ) : _Context_->CurrentlyRunningWord ;
    _CSL_Source ( scWord, 0 ) ; //debugger->w_Word ? debugger->w_Word : CSL->DebugWordListWord, 0 ) ;
    SetState ( debugger, DBG_INFO, true ) ;
}

void
_Debugger_CpuState_Show ( )
{
    _CpuState_Show ( _Debugger_->cs_Cpu ) ;
}

void
_Debugger_CpuState_CheckSave ( Debugger * debugger )
{
    if ( ! GetState ( debugger->cs_Cpu, CPU_SAVED ) )
    {
        debugger->SaveCpuState ( ) ;
        SetState ( debugger->cs_Cpu, CPU_SAVED, true ) ;
    }
}

void
Debugger_CpuState_CheckSaveShow ( Debugger * debugger )
{
    _Debugger_CpuState_CheckSave ( debugger ) ;
    _Debugger_CpuState_Show ( ) ;
    //if ( GetState ( debugger, DBG_STEPPING ) ) Debugger_UdisOneInstruction ( debugger, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
}

void
Debugger_Registers ( Debugger * debugger )
{
    Debugger_CpuState_CheckSaveShow ( debugger ) ;
}

void
Debugger_Quit ( Debugger * debugger )
{

    iPrintf ( "\nDebugger_Quit.\n" ) ;
    Debugger_Off ( debugger, 1 ) ;
    _Throw ( QUIT ) ;
}

void
Debugger_Abort ( Debugger * debugger )
{

    iPrintf ( "\nDebugger_Abort.\n" ) ;
    Debugger_Off ( debugger, 1 ) ;
    _Throw ( ABORT ) ;
}

void
Debugger_Stop ( Debugger * debugger )
{
    iPrintf ( "\nDebugger_Stop.\n" ) ;
    Debugger_Off ( debugger, 1 ) ;
    _Throw ( STOP ) ;
}

void
Debugger_InterpretLine_WithStartString ( byte * str )
{
    _CSL_Contex_NewRun_1 ( _CSL_, ( ContextFunction_1 ) CSL_InterpretPromptedLine, str ) ; // can't clone cause we may be in a file and we want input from stdin
}

void
Debugger_InterpretLine ( )
{
    Debugger_InterpretLine_WithStartString ( ( byte* ) "" ) ;
}

void
Debugger_Escape ( Debugger * debugger )
{
    uint64 saveSystemState = _Context_->System0->State ;
    uint64 saveDebuggerState = debugger->State, svScState = GetState ( _CSL_, SOURCE_CODE_ON ) ;
    SetState ( _CSL_, SOURCE_CODE_ON, false ) ;
    SetState ( _Context_->System0, ADD_READLINE_TO_HISTORY, true ) ;
    SetState_TrueFalse ( debugger, DBG_COMMAND_LINE | DBG_ESCAPED, DBG_ACTIVE ) ;
    _Debugger_ = Debugger_Copy ( debugger, TEMPORARY ) ;
    DefaultColors ;
    DebugOff ;
    int64 svcm = Get_CompileMode ( ) ;
    Set_CompileMode ( false ) ;
    byte * lexerTokenBuffer = Buffer_DataCleared ( _CSL_->DebuggerEscape ) ;
    strcpy ( ( char* ) lexerTokenBuffer, ( char* ) _CSL_->TokenBuffer ) ;

    //Context * cntx = CSL_Context_PushNew ( _CSL_ ) ; // experimental
    Debugger_InterpretLine ( ) ;
    //CSL_Context_PopDelete ( _CSL_ ) ;

    strcpy ( ( char* ) _CSL_->TokenBuffer, ( char* ) lexerTokenBuffer ) ;
    Set_CompileMode ( svcm ) ;
    DebugOn ;
    DebugColors ;
    _Debugger_ = debugger ;
    SetState ( _Context_->System0, ADD_READLINE_TO_HISTORY, saveSystemState ) ; // reset state 
    debugger->State = saveDebuggerState ;
    _Context_->System0->State = saveSystemState ;
    SetState_TrueFalse ( debugger, DBG_ACTIVE | DBG_INFO, DBG_STEPPED | DBG_AUTO_MODE | DBG_AUTO_MODE_ONCE | DBG_INTERPRET_LOOP_DONE | DBG_COMMAND_LINE | DBG_ESCAPED ) ;
    SetState ( _CSL_, SOURCE_CODE_ON, svScState ) ;
}

void
Debugger_AutoMode ( Debugger * debugger )
{
    if ( ! GetState ( debugger, DBG_AUTO_MODE ) )
    {
        if ( ( debugger->SaveKey == 's' ) || ( debugger->SaveKey == 'o' ) || ( debugger->SaveKey == 'i' )
            || ( debugger->SaveKey == 'e' ) || ( debugger->SaveKey == 'c' ) ) // not everything makes sense here
        {
            //AlertColors ;
            DebugColors ;
            //if ( debugger->SaveKey == 'e' ) DebugOff ;
            if ( debugger->SaveKey == 'c' )
            {
                iPrintf ( "\nContinuing : automatically repeating key \'e\' ..." ) ;
                debugger->SaveKey = 'e' ;
            }
            else iPrintf ( "\nDebugger :: Starting AutoMode : automatically repeating key :: \'%c\' ...", debugger->SaveKey ) ;
            DefaultColors ;
            SetState ( debugger, DBG_AUTO_MODE, true ) ;
        }
        else iPrintf ( "\nDebugger :: AutoMode : does not support repeating key :: \'%c\' ...", debugger->SaveKey ) ;
    }
    debugger->Key = debugger->SaveKey ;

    if ( GetState ( debugger, DBG_STEPPING ) ) Debugger_Continue ( debugger ) ;
}

void
Debugger_OptimizeToggle ( Debugger * debugger )
{
    if ( GetState ( _CSL_, CO_ON ) ) SetState ( _CSL_, CO_ON, false ) ;
    else CSL_OptimizeOn ( ) ;
    _CSL_SystemState_Print ( 0 ) ;
}

void
Debugger_CodePointerUpdate ( Debugger * debugger )
{
    if ( debugger->w_Word && ( ! debugger->DebugAddress ) )
    {
        debugger->DebugAddress = ( byte* ) debugger->w_Word->Definition ;
        iPrintf ( "\ncodePointer = 0x%08x", ( int64 ) debugger->DebugAddress ) ;
    }
}

void
Debugger_Dump ( Debugger * debugger )
{
    if ( ! debugger->w_Word )
    {
        if ( debugger->DebugAddress ) Dump ( ( byte * ) debugger->DebugAddress, ( uint64 ) ( Here - ( int64 ) debugger->DebugAddress ), 8 ) ;
    }

    else Dump ( ( byte * ) debugger->w_Word->CodeStart, ( uint64 ) debugger->w_Word->S_CodeSize, 8 ) ;
    SetState ( debugger, DBG_INFO, true ) ;
}

void
Debugger_Default ( Debugger * debugger )
{
    if ( isgraph ( debugger->Key ) ) iPrintf ( "\ndbg :> %c <: is not an assigned key code", debugger->Key ) ;
    else iPrintf ( "\ndbg :> <%d> <: is not an assigned key code", debugger->Key ) ;
}

void
Debugger_Verbosity ( Debugger * debugger )
{
    iPrintf ( "\nVerbosity = %d", Verbosity ( ) ) ;
}

void
_Debugger_ShowOff ( Debugger * debugger )
{
    DebugShow_Off ;
    SetState ( debugger, DBG_SHOW, false ) ;
}

void
_Debugger_ShowOn ( Debugger * debugger )
{
    DebugShow_On ;
    SetState ( debugger, DBG_SHOW | DBG_UDIS, true ) ;
}

void
Debugger_ShowOff ( )
{
    _Debugger_ShowOff ( _Debugger_ ) ;
}

void
Debugger_ShowOn ( )
{
    _Debugger_ShowOn ( _Debugger_ ) ;
}

void
_Debugger_State ( Debugger * debugger )
{
    byte * buf = Buffer_DataCleared ( _CSL_->DebugB2 ) ;
    _CSL_GetSystemState_String0 ( buf ) ;
    iPrintf ( buf ) ;
}

void
_Debugger_Copy ( Debugger * debugger, Debugger * debugger0 )
{
    MemCpy ( debugger, debugger0, sizeof (Debugger ) ) ;
}

Debugger *
Debugger_Copy ( Debugger * debugger0, uint64 type )
{
    Debugger * debugger = ( Debugger * ) Mem_Allocate ( sizeof (Debugger ), type ) ;
    _Debugger_Copy ( debugger, debugger0 ) ;
    return debugger ;
}

void
Debugger_Delete ( Debugger * debugger )
{
    Mem_FreeItem ( _OMS_->OvtMemChunkList, ( byte* ) debugger ) ;
}

Debugger *
Debugger_New ( uint64 type )
{
    Debugger * debugger = ( Debugger * ) Mem_Allocate ( sizeof (Debugger ), type ) ;
    debugger->cs_Cpu = CpuState_New ( type ) ;
    debugger->StepInstructionBA = ByteArray_AllocateNew ( 8 * K, type ) ; //Debugger_ByteArray_AllocateNew ( 8 * K, type ) ;
    debugger->ReturnStack = Stack_New ( K, type ) ;
    debugger->BreakReturnStack = Stack_New ( 256, type ) ;

    Debugger_TableSetup ( debugger ) ;
    SetState ( debugger, DBG_INTERPRET_LOOP_DONE, false ) ;
    SetState ( debugger, DBG_STEPPING, false ) ;
    Debugger_UdisNew ( debugger ) ;
    SetState ( debugger, DBG_UDIS, true ) ;

    return debugger ;
}

// nb! : not test for a while

void
_CSL_Debug_AtAddress ( byte * address )
{
    if ( ! GetState ( _Debugger_, DBG_ACTIVE ) ) Debugger_Init ( _Debugger_, 0, 0, address ) ;
    else _CSL_DebugContinue ( 1 ) ;
}

void
_CSL_DebugContinue ( int64 autoFlagOff )
{
    if ( GetState ( _Debugger_, DBG_AUTO_MODE ) )
    {
        if ( autoFlagOff ) SetState ( _Debugger_, DBG_AUTO_MODE, false ) ;
    }
}

void
Debugger_WordList_Show_All ( Debugger * debugger )
{
    _CSL_SC_WordList_Show ( 0, 0, 0 ) ;
}

void
Debugger_WordList_Show_InUse ( Debugger * debugger )
{
    _CSL_SC_WordList_Show ( 0, 1, 0 ) ;
}

void
Debugger_Compiler_WordList_Show ( Debugger * debugger )
{
    CSL_Compiler_WordList_Show ( 0, 1, 0 ) ;
}

void
Debugger_ShowTypeWordStack ( Debugger * debugger )
{
    CSL_ShowTypeWordStack ( ) ;
}

void
Debugger_Exit ( Debugger * debugger )
{
    SetState ( debugger, DBG_INTERPRET_LOOP_DONE, true ) ;
}

void
Debugger_Wdiss ( Debugger * debugger )
{
    DataStack_Push ( ( int64 ) debugger->w_Word ) ;
    Word * word = Finder_Word_FindUsing ( _Finder_, "wdiss", 0 ) ;
    Block_Eval ( word->Definition ) ;
}

void
Debugger_TableSetup ( Debugger * debugger )
{
    int64 i ;
    for ( i = 0 ; i < 128 ; i ++ ) debugger->CharacterTable [ i ] = 0 ;
    debugger->CharacterTable [ 0 ] = 0 ;
    debugger->CharacterTable [ 'o' ] = 1 ;
    debugger->CharacterTable [ 'i' ] = 1 ;
    debugger->CharacterTable [ 's' ] = 1 ;
    debugger->CharacterTable [ 'h' ] = 1 ;
    debugger->CharacterTable [ 'e' ] = 2 ;
    debugger->CharacterTable [ 'w' ] = 3 ;
    debugger->CharacterTable [ 'd' ] = 4 ;
    debugger->CharacterTable [ 'I' ] = 5 ;
    debugger->CharacterTable [ 'm' ] = 6 ;
    //debugger->CharacterTable [ 'T' ] = 7 ;
    debugger->CharacterTable [ 't' ] = 7 ;
    debugger->CharacterTable [ 'Z' ] = 8 ;
    debugger->CharacterTable [ 'U' ] = 9 ;
    debugger->CharacterTable [ 'r' ] = 10 ;
    debugger->CharacterTable [ 'g' ] = 43 ;
    debugger->CharacterTable [ 'c' ] = 11 ;
    debugger->CharacterTable [ ' ' ] = 11 ;
    debugger->CharacterTable [ 'q' ] = 12 ;
    debugger->CharacterTable [ 'f' ] = 14 ;
    debugger->CharacterTable [ '\\'] = 15 ;
    debugger->CharacterTable [ '\n' ] = 15 ;
    debugger->CharacterTable [ 27 ] = 15 ;
    debugger->CharacterTable [ 'G' ] = 16 ;
    debugger->CharacterTable [ 'n' ] = 17 ;
    debugger->CharacterTable [ 'p' ] = 18 ;
    debugger->CharacterTable [ 'a' ] = 20 ;
    debugger->CharacterTable [ 'z' ] = 21 ;
    debugger->CharacterTable [ 'W' ] = 22 ;
    debugger->CharacterTable [ 'B' ] = 23 ;
    debugger->CharacterTable [ 'P' ] = 24 ;
    debugger->CharacterTable [ 'l' ] = 25 ;
    debugger->CharacterTable [ 'v' ] = 26 ;
    debugger->CharacterTable [ 'S' ] = 27 ;
    debugger->CharacterTable [ 'A' ] = 28 ;
    debugger->CharacterTable [ 'N' ] = 29 ;
    debugger->CharacterTable [ 'R' ] = 30 ;
    debugger->CharacterTable [ 'H' ] = 31 ;
    debugger->CharacterTable [ 'O' ] = 32 ;
    debugger->CharacterTable [ 'Q' ] = 33 ;
    debugger->CharacterTable [ 'L' ] = 34 ;
    debugger->CharacterTable [ 'X' ] = 35 ;
    debugger->CharacterTable [ 'y' ] = 36 ;
    debugger->CharacterTable [ 'w' ] = 37 ;
    debugger->CharacterTable [ 'x' ] = 38 ;
    debugger->CharacterTable [ 'V' ] = 39 ;
    debugger->CharacterTable [ 'f' ] = 40 ;

    debugger->CharacterTable [ 'u' ] = 42 ;
    debugger->CharacterTable [ 'T' ] = 43 ;

    // debugger : system related
    debugger->CharacterFunctionTable [ 0 ] = Debugger_Default ;
    debugger->CharacterFunctionTable [ 1 ] = Debugger_Step ;
    debugger->CharacterFunctionTable [ 2 ] = Debugger_Eval ;

    // debugger internal
    debugger->CharacterFunctionTable [ 4 ] = Debugger_Dis ;
    debugger->CharacterFunctionTable [ 5 ] = Debugger_Info ;
    debugger->CharacterFunctionTable [ 6 ] = Debugger_DoMenu ;
    debugger->CharacterFunctionTable [ 7 ] = Debugger_Stack ;
    //debugger->CharacterFunctionTable [ 8 ] = Debugger_DWL_ShowList ;
    debugger->CharacterFunctionTable [ 9 ] = Debugger_Source ;
    debugger->CharacterFunctionTable [ 10 ] = Debugger_Registers ;
    debugger->CharacterFunctionTable [ 11 ] = Debugger_Continue ;
    debugger->CharacterFunctionTable [ 12 ] = Debugger_Quit ;
    debugger->CharacterFunctionTable [ 13 ] = Debugger_Parse ;
    debugger->CharacterFunctionTable [ 14 ] = Debugger_FindAny ;
    debugger->CharacterFunctionTable [ 15 ] = Debugger_Escape ;
    debugger->CharacterFunctionTable [ 16 ] = Debugger_OptimizeToggle ;
    debugger->CharacterFunctionTable [ 17 ] = Debugger_CodePointerUpdate ;
    debugger->CharacterFunctionTable [ 18 ] = Debugger_Dump ;
    //debugger->CharacterFunctionTable [ 19 ] = Debugger_ConsiderAndShowWord ; // fix me
    debugger->CharacterFunctionTable [ 20 ] = Debugger_DisassembleAccumulated ;
    debugger->CharacterFunctionTable [ 21 ] = Debugger_AutoMode ;
    debugger->CharacterFunctionTable [ 22 ] = Debugger_WDis ;
    debugger->CharacterFunctionTable [ 23 ] = Debugger_Abort ;
    debugger->CharacterFunctionTable [ 24 ] = Debugger_Stop ;
    debugger->CharacterFunctionTable [ 25 ] = Debugger_Locals_Show ;
    debugger->CharacterFunctionTable [ 26 ] = Debugger_Variables ;
    debugger->CharacterFunctionTable [ 27 ] = _Debugger_State ;
    debugger->CharacterFunctionTable [ 28 ] = Debugger_DisassembleTotalAccumulated ;
    debugger->CharacterFunctionTable [ 29 ] = Debugger_Using ;
    debugger->CharacterFunctionTable [ 30 ] = Debugger_ReturnStack ;
    debugger->CharacterFunctionTable [ 31 ] = Debugger_WordList_Show_All ;
    debugger->CharacterFunctionTable [ 32 ] = Debugger_Compiler_WordList_Show ;
    //debugger->CharacterFunctionTable [ 33 ] = Debugger_CSLRegisters ;
    debugger->CharacterFunctionTable [ 34 ] = Debugger_GotoList_Print ;
    debugger->CharacterFunctionTable [ 35 ] = Debugger_Print_LispDefinesNamespace ;
    debugger->CharacterFunctionTable [ 36 ] = Debugger_ShowTypeWordStack ;
    debugger->CharacterFunctionTable [ 37 ] = Debugger_Wdiss ;
    debugger->CharacterFunctionTable [ 38 ] = Debugger_Exit ;
    debugger->CharacterFunctionTable [ 39 ] = Debugger_Verbosity ;
    debugger->CharacterFunctionTable [ 40 ] = _Debugger_ShowOff ;
    debugger->CharacterFunctionTable [ 41 ] = _Debugger_ShowOn ;
    debugger->CharacterFunctionTable [ 42 ] = Debugger_UdisToggle ;
#if DEBUG    
    debugger->CharacterFunctionTable [ 43 ] = Debugger_Dbg ;
#endif    
}

