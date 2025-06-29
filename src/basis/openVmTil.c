
#include "../include/csl.h"
#define VERSION ((byte*) "0.942.120" )

// inspired by :: Foundations of Mathematical Logic [Foml] by Haskell Curry,
// Category Theory, Object Oriented Programming, Type Theory 
// Formal Language Theory (Panini, Jiva Goswami, Chomsky) 
// C/C++/C#, Lisp, RPN/Lag - Reverse Polish Notation - (Left Associative Grammar)
// (especially Java, C#, retroforth, factor), 
// Automata Theory : State Machines, Push Down Automata (PDA), Turing Machines :: 
// Also inspired by : Laws of Form, by G.S. Brown, Kurt Godel, etc.
// csl : context sensitive language; C shell (extension) language, cx
// cfr : C, Forth, reason or compiler foundations research (and development)
// cflr : C, Forth, Lisp, reason
// til : a toolkit for implementing languages (maybe even a compiler compiler) based on these ideas,
// (cf. the book Threaded Interpretive Languages, by R.G. Loeliger)
// cfrtil : an old name for csl
// !! this is a prototype : it is still rough in spots; it needs to be extended, improved, and rewritten (at least in some places) !!

OpenVmTil * _O_ ;

int
main ( int argc, char * argv [ ] )
{
    LinuxInit ( ) ;
    openvmtil ( argc, argv ) ;
}

void
openvmtil ( int64 argc, char * argv [ ] )
{
    OpenVmTil_Run ( argc, argv ) ;
}

void
OpenVmTil_Init ( OpenVmTil * ovt )
{
    ovt->MemorySpace0 = MemorySpace_New ( ovt, "DefaultMemorySpace" ) ;
    ovt->CSL_InternalSpace = NBA_OvtNew ( ( byte* ) "CSLInternalSpace", ovt->CSLSize, T_CSL ) ;
    ovt->InternalObjectSpace = NBA_OvtNew ( ( byte* ) "InternalObjectSpace", ovt->InternalObjectsSize, INTERNAL_OBJECT_MEM ) ; // used in _DObject_ValueDefinition_Init
    ovt->BufferList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->RecycledWordList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->RecycledOptInfoList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->OVT_Exception = _Exception_New ( ) ;
    ovt->VersionString = VERSION ;

    // ? where do we want the init file ?
    if ( _File_Exists ( ( byte* ) "./init.csl" ) ) ovt->InitString = ( byte* ) "\"./init.csl\" _include" ;
    else ovt->InitString = ( byte* ) "\"/usr/local/lib/csl/init.csl\" _include" ;

    if ( ovt->Verbosity > 1 )
    {
        iPrintf ( "\nRestart : All memory freed, allocated and initialized as at startup. "
            "termios, verbosity and memory category allocation sizes preserved. verbosity = %d.", ovt->Verbosity ) ;
        OpenVmTil_Print_DataSizeofInfo ( 0 ) ;
    }
    ovt->ExceptionBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    ovt->PrintBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    ovt->PrintBufferCopy = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    ovt->PrintBufferConcatCopy = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    OpenVmTil_ColorsInit ( ovt ) ;
}

OpenVmTil *
OpenVmTil_New ( OpenVmTil * ovt, int64 argc, char * argv [ ] )
{
    int64 restartCondition, startedTimes = 0, allocSize ;
    if ( ! ovt ) restartCondition = INITIAL_START ;
    else
    {
        restartCondition = FULL_RESTART ;
        startedTimes = ovt->OVT_Exception->StartedTimes ;
    }

    ovt = OpenVmTil_Allocate ( ovt ) ;
    TimerInit ( &ovt->Timer ) ;

    ovt->Argc = argc ;
    ovt->Argv = argv ;
    if ( ovt->Argc < 4 ) OVT_GetStartupOptions ( ovt ) ;

    allocSize = 1 * M ; //430 * K ;
    ovt->InternalObjectsSize = 1 * M ;
    ovt->ObjectSpaceSize = 4 * M ;
    ovt->LispSpaceSize = 2 * M ;
    ovt->LispTempSize = 1 * M ;
    ovt->LispCopySize = 1 * M ;
    ovt->CompilerTempObjectsSize = 8 * M ; //2 * allocSize ;
    ovt->BufferSpaceSize = allocSize ; //35 * ( sizeof ( Buffer ) + BUFFER_SIZE ) ;
    ovt->MachineCodeSize = allocSize ;
    ovt->StringSpaceSize = allocSize ;
    ovt->DictionarySize = 4 * M ; //100 * K ;
    ovt->CSLSize = 1 * M ; //( 190 * K ) ;
    ovt->OpenVmTilSize = ( 6 * K ) ;
    ovt->DataStackSize = 8 * K ; //1 * MB ; //8 * MB ;
    ovt->TempObjectsSize = 4 * M ; //200 * K ; //COMPILER_TEMP_OBJECTS_SIZE ;
    ovt->WordRecylingSize = 1 * M ; //2 * K * ( sizeof (Word ) + sizeof (WordData ) ) ; //50 * K ; //COMPILER_TEMP_OBJECTS_SIZE ;
    ovt->SessionObjectsSize = 2 * M ; //50 * K ;
    ovt->ForthSize = 5 * M ; //1100 * K ; //50 * K ;
    ovt->ContextSize = 2 * M ;
    ovt->ExceptionSpaceSize = 3 * K ;

    OpenVmTil_Init ( ovt ) ;
    Linux_SetupSignals ( ) ;
    OVT_SetRestartCondition ( restartCondition ) ;
    ovt->OVT_Exception->StartedTimes = startedTimes ;
    return ovt ;
}

void
OpenVmTil_Run ( int64 argc, char * argv [ ] )
{
    OpenVmTil * ovt ;
    Exception *e ; //int64 restartCondition = INITIAL_START, restarts = 0, sigSegvs = 0 ;
    int64 restarts = 0, sigSegvs = 0 ;
    while ( 1 )
    {
        if ( _O_ )
        {
            e = _O_->OVT_Exception ;
            sigSegvs = e->SigSegvs ;
            restarts = ++ e->Restarts ;
            if ( e->Restarts > 20 ) OVT_Exit ( ) ;
            if ( ( e->RestartCondition == COMPLETE_INITIAL_START ) || ( e->SigSegvs > 1 ) )
            {
                e->Message = "SigSegv : COMPLETE_INITIAL_START" ;
                _OVT_SimpleFinal_Key_Pause ( ) ;
                OVT_FullRestartCompleteDelete ( ) ;
            }
        }
        ovt = _O_ = OpenVmTil_New ( _O_, argc, argv ) ;
        e = _O_->OVT_Exception ;
        e->SigSegvs = sigSegvs ;
        ovt->Verbosity = 1 ;
        e->Restarts = restarts ;
        if ( e->Restarts ) OVT_ExceptionState_Print ( ) ;
        //SetState ( ovt, OVT_PROMPT_DONE, false ) ;
        if ( ! sigsetjmp ( ovt->JmpBuf0, 0 ) ) // nb. siglongjmp always comes to beginning of the block 
        {
            ovt->OVT_CSL = CSL_New ( ovt->OVT_CSL ) ;
            CSL_Run ( ovt->OVT_CSL, e->RestartCondition ) ;
        }

        //restartCondition = ovt->OVT_Exception->RestartCondition ;
        //OVT_SetRestartCondition ( ovt, restartCondition ) ;
    }
}

void
Ovt_RunInit ( OpenVmTil * ovt )
{
    Exception *e = _O_->OVT_Exception ;
    //static int loopTimes ;
    e->StartedTimes ++ ;
    OVT_SetRestartCondition ( CSL_RUN_INIT ) ;
    //OVT_StartupMessage ( startupMessageFlag && ( ++csl->InitSessionCoreTimes <= 2 ) ) ;
    OVT_StartupMessage ( ( ++ _CSL_->InitSessionCoreTimes <= 2 ) ) ;
    //CSL_Prompt (ovt->OVT_CSL, 1, 1 , 0) ; //++loopTimes < 2, 1 ) ;
    //SetState ( _O_, OVT_PROMPT_DONE, false ) ;
}

void
OVT_PrintStartupOptions ( OpenVmTil * ovt )
{
    int i ;
    //if ( ovt->Argc < 4 )
    {
        for ( i = ovt->Argc ; i ; i -- ) iPrintf ( "\n\nOVT_GetStartupOptions :: ovt->Argv [%d] = %s\n\n", i, ovt->Argv [i] ) ;
        iPrintf ( "\n\nOVT_GetStartupOptions :: ovt->StartupFilename = %s\n\n", ovt->StartupFilename ) ;
    }
}

void
OVT_GetStartupOptions ( OpenVmTil * ovt )
{
    int64 i ;
    byte * arg ;
    //if ( ovt->Argc < 4 )
    {
        for ( i = 0 ; i < ovt->Argc ; i ++ )
        {
            arg = ovt->Argv [ i ] ;
            if ( String_Equal ( "-m", arg ) ) ovt->TotalMemSizeTarget = ( atoi ( ovt->Argv [ ++ i ] ) * MB ) ;
                // -s : a script file with "#! csl -s" -- as first line includes the script file, the #! whole line is treated as a comment
            else if ( String_Equal ( "-f", arg ) || ( String_Equal ( "-s", arg ) ) ) ovt->StartupFilename = ( byte* ) ovt->Argv [ ++ i ] ;
            else if ( String_Equal ( "-e", arg ) ) ovt->StartupString = ( byte* ) ovt->Argv [ ++ i ] ;
        }
        OVT_PrintStartupOptions ( ovt ) ;
    }
}

// only partially working ??

void
OVT_RecycleAllWordsDebugInfo ( )
{
    SetState ( _CSL_, ( RT_DEBUG_ON | GLOBAL_SOURCE_CODE_MODE ), false ) ;
    Tree_Map_NamespacesTree ( _CSL_->Namespaces->W_List, ( MapSymbolFunction ) CSL_DeleteWordDebugInfo ) ;
    _OVT_MemListFree_WordRecyclingSpace ( ) ;
    OVT_FreeTempMem ( ) ;
    _CSL_->CSL_N_M_Node_WordList = _dllist_New ( T_CSL ) ;
}

void
OVT_ResetOutputPrintBuffer ( )
{
    //strncpy ( Buffer_Data ( _O_->PrintBufferCopy ), Buffer_Data ( _O_->PrintBuffer ), BUFFER_SIZE ) ;
    Buffer_DataCleared ( _O_->PrintBuffer ) ; //[0] = 0 ;
    _O_->PrintBuffer->B_Data[0] = 0 ;
}

int64
Verbosity ( )
{
    return (_O_ && ( _O_->Verbosity & 0x7 ) ) ; // higher bits for specific functions
}

void
OVT_Set_UnknowStringIsErrorFlag ( )
{
    SetState ( _O_, OVT_UNKNOWN_STRING_IS_ERROR, true ) ;
}

void
OVT_Set_UnknowStringPushedFlag ( )
{
    SetState ( _O_, OVT_UNKNOWN_STRING_PUSHED, true ) ;
}

void
OVT_UnSet_UnknowStringFlag ( )
{
    SetState ( _O_, ( OVT_UNKNOWN_STRING_IS_ERROR | OVT_UNKNOWN_STRING_PUSHED ), false ) ;
}

void
OVT_Exception_Init ( )
{
    Exception_Init ( _O_->OVT_Exception ) ;
}

void
_OVT_openvmtil ( )
{
    OpenVmTil_Run ( 0, 0 ) ;
}

void
OVT_openvmtil ( )
{
    //_O_->OVT_Exception->RestartCondition = COMPLETE_INITIAL_START ;
    _OVT_openvmtil ( ) ;
}

