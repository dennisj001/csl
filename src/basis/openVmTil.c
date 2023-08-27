
#include "../include/csl.h"
#define VERSION ((byte*) "0.938.152" ) 

// inspired by :: Foundations of Mathematical Logic [Foml] by Haskell Curry,
// CT/Oop (Category Theory, Object Oriented Programming, Type Theory), 
// Formal Language Theory (Panini, Jiva Goswami, Chomsky) 
// C/C++/C#, Lisp, RPN/Lag - Reverse Polish Notation - (Left Associative Grammar)
// (especially Java, C#, retroforth, factor), 
// Automata Theory : State Machines, Push Down Automata (PDA), Turing Machines :: 
// Also inspired by : Laws of Form, by G.S. Brown, Kurt Godel, etc.
// csl : context sensitive language; C shell (extension) language, cx
// cfr : C, Forth, reason or compiler foundations research (and development)
// cflr : C, Forth, Lisp, reason
// til : a toolkit for implementing languages (maybe even a compiler compiler) based on these ideas,
// cfrtil : an old name for csl
// !! this is only a prototype : it is still rough in spots; it needs to be extended, improved, and rewritten (at least in some places) !!

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
_OpenVmTil_Init ( OpenVmTil * ovt )
{
    ovt->MemorySpace0 = MemorySpace_New ( ovt, "DefaultMemorySpace" ) ;
    ovt->CSL_InternalSpace = NBA_OvtNew ( ( byte* ) "CSLInternalSpace", ovt->CSLSize, T_CSL ) ;
    ovt->InternalObjectSpace = NBA_OvtNew ( ( byte* ) "InternalObjectSpace", ovt->InternalObjectsSize, INTERNAL_OBJECT_MEM ) ; // used in _DObject_ValueDefinition_Init
    ovt->BufferList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->RecycledWordList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->RecycledOptInfoList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->VersionString = VERSION ;
    // ? where do we want the init file ?
    if ( _File_Exists ( ( byte* ) "./init.csl" ) )
    {
        ovt->InitString = ( byte* ) "\"./init.csl\" _include" ; // could allow override with a startup parameter
        SetState ( ovt, OVT_IN_USEFUL_DIRECTORY, true ) ;
    }
    else
    {
        ovt->InitString = ( byte* ) "\"/usr/local/lib/csl/init.csl\" _include" ; // could allow override with a startup parameter
        SetState ( ovt, OVT_IN_USEFUL_DIRECTORY, false ) ;
    }
    if ( ovt->Verbosity > 1 )
    {
        iPrintf ( "\nRestart : All memory freed, allocated and initialized as at startup. "
            "termios, verbosity and memory category allocation sizes preserved. verbosity = %d.", ovt->Verbosity ) ;
        OpenVmTil_Print_DataSizeofInfo ( 0 ) ;
    }
    ovt->PrintBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    ovt->PrintBufferCopy = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    ovt->PrintBufferConcatCopy = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    _OpenVmTil_ColorsInit ( ovt ) ;
}

OpenVmTil *
OpenVmTil_New ( OpenVmTil * ovt, int64 argc, char * argv [ ] )
{
    int64 restartCondition, startedTimes = 0, allocSize ;
    if ( ! ovt ) restartCondition = INITIAL_START ;
    else
    {
        restartCondition = FULL_RESTART ;
        startedTimes = ovt->StartedTimes ;
    }

    ovt = _OpenVmTil_Allocate ( ovt ) ;
    TimerInit ( &ovt->Timer ) ;

    OVT_SetRestartCondition ( ovt, restartCondition ) ;
    ovt->Argc = argc ;
    ovt->Argv = argv ;
    ovt->StartedTimes = startedTimes ;
    OVT_GetStartupOptions ( ovt ) ;

    allocSize = 430 * K ;
    ovt->InternalObjectsSize = 1 * M ;
    ovt->ObjectSpaceSize = 1 * M ;
    ovt->LispSpaceSize = 1 * M ;
    ovt->LispTempSize = 1 * M ;
    ovt->LispCopySize = 1 * M ;
    ovt->CompilerTempObjectsSize = 2 * allocSize ;
    ovt->BufferSpaceSize = allocSize ; //35 * ( sizeof ( Buffer ) + BUFFER_SIZE ) ;
    ovt->MachineCodeSize = allocSize ;
    ovt->StringSpaceSize = allocSize ;
    ovt->DictionarySize = 1 * M ; //100 * K ;
    ovt->CSLSize = ( 190 * K ) ;
    ovt->OpenVmTilSize = ( 6 * K ) ;
    ovt->DataStackSize = 8 * K ; //1 * MB ; //8 * MB ;
    ovt->TempObjectsSize = 200 * K ; //COMPILER_TEMP_OBJECTS_SIZE ;
    ovt->WordRecylingSize = 1 * K * ( sizeof (Word ) + sizeof (WordData ) ) ; //50 * K ; //COMPILER_TEMP_OBJECTS_SIZE ;
    ovt->SessionObjectsSize = 50 * K ;

    _OpenVmTil_Init ( ovt ) ;
    Linux_SetupSignals ( &ovt->JmpBuf0, 1 ) ;
    return ovt ;
}

void
OpenVmTil_Run ( int64 argc, char * argv [ ] )
{
    OpenVmTil * ovt ;
    int64 restartCondition = INITIAL_START, restarts = 0, sigSegvs = 0 ;
    while ( 1 )
    {
        if ( _O_ )
        {
            sigSegvs = _O_->SigSegvs ;
            restarts = ++ _O_->Restarts ;
            if ( _O_->Restarts > 20 ) OVT_Exit ( ) ;
            if (( _O_->RestartCondition == COMPLETE_INITIAL_START ) || (_O_->SigSegvs > 1 ) ) 
            {
                _OVT_SimpleFinal_Key_Pause ( _O_, "SigSegv : COMPLETE_INITIAL_START" ) ;
                OVT_FullRestartCompleteDelete ( ) ;
            }
        }
        ovt = _O_ = OpenVmTil_New ( _O_, argc, argv ) ;
        OVT_SetRestartCondition ( ovt, restartCondition ) ;
        ovt->SigSegvs = sigSegvs ;
        ovt->Verbosity = 1 ;
        ovt->Restarts = restarts ;
        if ( ovt->Restarts ) OVT_ExceptionState_Print ( ) ;
        //CSL_NewLine ( ) ;
        if ( ! sigsetjmp ( ovt->JmpBuf0, 0 ) ) // nb. siglongjmp always comes to beginning of the block 
        {
            ovt->OVT_CSL = CSL_New ( ovt->OVT_CSL ) ;
            CSL_Run ( ovt->OVT_CSL, ovt->RestartCondition ) ;
        }
        restartCondition = ovt->RestartCondition ;
        OVT_SetRestartCondition ( ovt, restartCondition ) ;
    }
}

void
Ovt_RunInit ( OpenVmTil * ovt )
{
    ovt->StartedTimes ++ ;
    OVT_SetRestartCondition ( ovt, CSL_RUN_INIT ) ;
}

void
OVT_PrintStartupOptions ( OpenVmTil * ovt )
{
    int i ;
    for ( i = ovt->Argc ; i ; i -- ) iPrintf ( "\n\nOVT_GetStartupOptions :: ovt->Argv [%d] = %s\n\n", i, ovt->Argv [i] ) ;
    iPrintf ( "\n\nOVT_GetStartupOptions :: ovt->StartupFilename = %s\n\n", ovt->StartupFilename ) ;
}

void
OVT_GetStartupOptions ( OpenVmTil * ovt )
{
    int64 i ;
    byte * arg ;
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

// only partially working ??

void
OVT_RecycleAllWordsDebugInfo ( )
{
    SetState ( _CSL_, ( RT_DEBUG_ON | GLOBAL_SOURCE_CODE_MODE ), false ) ;
    Tree_Map_Namespaces ( _CSL_->Namespaces->W_List, ( MapSymbolFunction ) CSL_DeleteWordDebugInfo ) ;
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
Verbosity () 
{
    return (_O_->Verbosity & 0x7) ; // higher bits for specific functions
}

void 
OVT_Set_UnknowStringIsErrorFlag () 
{
    SetState (_O_, OVT_UNKNOWN_STRING_IS_ERROR, true ) ; 
}

void 
OVT_UnSet_UnknowStringIsErrorFlag () 
{
    SetState (_O_, OVT_UNKNOWN_STRING_IS_ERROR, false ) ; 
}
    
