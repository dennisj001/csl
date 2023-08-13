
#include "../include/csl.h"

void
CSL_Run ( CSL * csl, int64 restartCondition )
{
    while ( 1 )
    {
        if ( ! sigsetjmp ( csl->JmpBuf0, 0 ) )
        {
            if ( ! csl ) csl = CSL_New ( _O_->OVT_CSL ) ;
            _CSL_ReStart ( csl, restartCondition ) ;
            Ovt_RunInit ( _O_ ) ;
            CSL_InterpreterRun ( ) ;
        }
    }
}

void
CSL_RunInit ( )
{
    if ( _O_->Signal > QUIT ) CSL_DataStack_Init ( ) ;
    else if ( DataStack_Underflow ( ) || ( DataStack_Overflow ( ) ) ) CSL_PrintDataStack ( ) ;
}

void
_CSL_ReStart ( CSL * csl, int64 restartCondition )
{
    CSL_RunInit ( ) ;
    switch ( restartCondition )
    {
        case 0:
        case COMPLETE_INITIAL_START:
        case INITIAL_START:
        case FULL_RESTART:
        case RESTART:
        case RESET_ALL: CSL_ResetAll_Init ( csl ) ;
        default:
        case ABORT: Set_DataStackPointer_FromDspReg ( ) ;
        case QUIT:
        case STOP: ;
    }
}

void
_CSL_CpuState_CheckSave ( )
{
    if ( ! GetState ( _CSL_->cs_Cpu, CPU_SAVED ) )
    {
        _CSL_->SaveCpuState ( ) ;
        SetState ( _CSL_->cs_Cpu, CPU_SAVED, true ) ;
    }
}

void
CSL_CpuState_Show ( )
{
    _CpuState_Show ( _CSL_->cs_Cpu ) ;
}

void
CSL_CpuState_Current_Show ( )
{
    _CSL_->SaveCpu2State ( ) ;
    _CpuState_Show ( _CSL_->cs_Cpu2 ) ;
    _CSL_->RestoreCpu2State ( ) ;
}

void
CSL_CpuState_CheckShow ( )
{
    _CSL_CpuState_CheckSave ( ) ;
    CSL_CpuState_Show ( ) ;
}

void
CSL_Debugger_CheckSaveCpuStateShow ( )
{
    Debugger_CpuState_CheckSaveShow ( _Debugger_ ) ;
}

void
CSL_Debugger_UdisOneInsn ( )
{
    Debugger_UdisOneInstruction ( _Debugger_, 0, _Debugger_->DebugAddress, ( byte* ) "\r\r", ( byte* ) "" ) ; // current insn
}

void
CSL_Debugger_State_CheckSaveShow ( )
{
    CSL_Debugger_CheckSaveCpuStateShow ( ) ;
    //if ( Verbosity () > 3 ) Debugger_PrintReturnStackWindow () ;
}

void
CSL_Debugger_SaveCpuState ( )
{
    _Debugger_CpuState_CheckSave ( _Debugger_ ) ;
}

void
CSL_PrintReturnStackWindow ( )
{
    _PrintNStackWindow ( ( uint64* ) _CSL_->cs_Cpu->Rsp, ( byte* ) "CSL C ReturnStack (RSP)", ( byte* ) "RSP", 4 ) ;
}

void
_CSL_NamespacesInit ( CSL * csl )
{
    Namespace * ns = DataObject_New ( NAMESPACE, 0, ( byte* ) "Namespaces", 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
    ns->State |= USING ; // nb. _Namespace_SetState ( ns, USING ) ; // !! can't be used with "Namespaces"
    csl->Namespaces = ns ;
    CSL_AddCPrimitives ( ) ;
}

void
_CSL_DataStack_Init ( CSL * csl )
{
    _Stack_Init ( _CSL_->DataStack, _O_->DataStackSize ) ;
    _DspReg_ = _CSL_->DataStack->StackPointer ;
    csl->SaveDsp = _DspReg_ ;
}

void
CSL_DataStack_Init ( )
{
    _CSL_DataStack_Init ( _CSL_ ) ;
    if ( Verbosity () > 2 ) iPrintf ( "\nData Stack reset." ) ;
}

void
CSL_Setup_For_DObject_ValueDefinition_Init ( )
{
    _CSL_MachineCodePrimitive_NewAdd ( "call_ToAddressThruSREG_TestAlignRSP", CSL_WORD | CSL_ASM_WORD, 0,
        & _CSL_->Call_ToAddressThruSREG_TestAlignRSP, ( byte* ) Compile_Call_ToAddressThruSREG_TestAlignRSP, - 1 ) ;
}

void
CSL_AfterWordReset ( )
{
    CSL_TypeStackReset ( ) ;
    SetState ( _CSL_, RT_DEBUG_ON, false ) ;
}

void
_CSL_Init ( CSL * csl, Namespace * nss )
{
    uint64 allocType = T_CSL ;
    _CSL_ = csl ;
    // TODO : organize these buffers and their use 
    csl->OriginalInputLineB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->InputLineB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->svLineB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->SourceCodeBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->LC_PrintB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->LC_OutB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->LC_DefineB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->TokenB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->ScratchB1 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->ScratchB2 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->ScratchB3 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->ScratchB4 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->ScratchB5 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->DebugB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->DebugB1 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->DebugB2 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->DebugB3 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->DebugB4 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->FormatRemoval = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB2 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB3 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB4 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB5 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB6 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringInsertB7 = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->TabCompletionBuf = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StringMacroB = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->StrCatBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->TabCompletion = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->DebuggerEscape = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->Preprocessor = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    csl->OriginalInputLine = Buffer_DataCleared ( csl->OriginalInputLineB ) ;
    csl->SC_Buffer = Buffer_DataCleared ( csl->SourceCodeBuffer ) ;
    csl->TokenBuffer = Buffer_DataCleared ( csl->TokenB ) ;
    SetState ( csl, CSL_RUN | CO_ON | INLINE_ON, true ) ;

    if ( Verbosity () > 2 ) iPrintf ( "\nSystem Memory is being reallocated.  " ) ;

    csl->ContextStack = Stack_New ( 256, allocType ) ;
    csl->TypeWordStack = Stack_New ( _O_->DataStackSize, allocType ) ;
    csl->CSL_N_M_Node_WordList = _dllist_New ( T_CSL ) ;

    _Context_ = csl->Context0 = _Context_New ( csl ) ;

    csl->Debugger0 = Debugger_New ( allocType ) ;
    csl->cs_Cpu = CpuState_New ( allocType ) ;
    csl->cs_Cpu2 = CpuState_New ( allocType ) ;
    csl->PeekPokeByteArray = ByteArray_AllocateNew ( 32, allocType ) ;
    CSL_Setup_For_DObject_ValueDefinition_Init ( ) ; // nb! before _CSL_NamespacesInit
    if ( nss ) csl->Namespaces = nss ;
    else _CSL_NamespacesInit ( csl ) ;
    // with _O_->RestartCondition = STOP from Debugger_Stop
    if ( csl->SaveDsp && csl->DataStack ) _DspReg_ = csl->SaveDsp ;
    else
    {
        csl->DataStack = Stack_New ( _O_->DataStackSize, T_CSL ) ;
        _DspReg_ = csl->DataStack->StackPointer ;
        csl->SaveDsp = _DspReg_ ;
    }
    CSL_MachineCodePrimitive_AddWords ( csl ) ; // in any case we need to reinit these for eg. debugger->SaveCpuState (), etc.
    csl->StoreWord = Finder_FindWord_AnyNamespace ( _Finder_, ( byte* ) "store" ) ;
    csl->PokeWord = Finder_FindWord_InOneNamespace ( _Finder_, ( byte* ) "Compiler", ( byte* ) "=" ) ; //Finder_FindWord_AnyNamespace ( _Finder_, ( byte* ) "=" ) ;
    csl->RightBracket = Finder_FindWord_AnyNamespace ( _Finder_, ( byte* ) "]" ) ;
    csl->InfixNamespace = Namespace_Find ( ( byte* ) "Infix" ) ;
    csl->StringNamespace = Namespace_Find ( ( byte* ) "String" ) ;
    csl->BigNumNamespace = Namespace_Find ( ( byte* ) "BigNum" ) ;
    csl->IntegerNamespace = Namespace_Find ( ( byte* ) "Integer" ) ;
    CSL_ReadTables_Setup ( csl ) ;
    CSL_LexerTables_Setup ( csl ) ;
    csl->LC = 0 ;
    csl->SC_QuoteMode = 0 ;
    csl->EndBlockWord = Finder_FindWord_InOneNamespace ( _Finder_, ( byte* ) "C_Syntax", ( byte* ) "}" ) ;
    csl->BeginBlockWord = Finder_FindWord_InOneNamespace ( _Finder_, ( byte* ) "Reserved", ( byte* ) "{" ) ;
    SetState ( csl, SOURCE_CODE_ON, true ) ;
}

void
CSL_ResetMemory ( CSL * csl )
{
    if ( csl->ContextStack )
    {
        while ( Stack_Depth ( csl->ContextStack ) )
        {
            Context * cntx = ( Context* ) _Stack_Pop ( csl->ContextStack ) ;
            NamedByteArray_Delete ( cntx->ContextNba, 0 ) ;
        }
        if ( csl->Context0 ) NamedByteArray_Delete ( csl->Context0->ContextNba, 0 ) ;
    }
    OVT_MemListFree_Session ( ) ;
    OVT_MemListFree_ContextMemory ( ) ;
    OVT_MemListFree_LispTemp ( ) ;
    OVT_MemListFree_TempObjects ( ) ;
    OVT_MemListFree_Buffers ( ) ;
    OVT_MemListFree_CompilerTempObjects ( ) ;
    _OVT_MemListFree_CSLInternal ( ) ;
    if ( Verbosity () > 1 ) OVT_ShowMemoryAllocated ( ) ;
}

CSL *
CSL_New ( CSL * csl )
{
    // nb. not all of this logic has really been needed or used or tested; it should be reworked according to need
    Namespace * nss = 0 ;
    if ( csl )
    {
        if ( _O_->RestartCondition < RESET_ALL )
        {
            nss = csl->Namespaces ; // in this case (see also below) only preserve Namespaces, all else is recycled and reinitialized
            if ( csl->LogFILE ) CSL_LogOff ( ) ;
        }
        CSL_ResetMemory ( csl ) ;
    }
    else nss = 0 ;
    _Context_ = 0 ;
    csl = ( CSL* ) Mem_Allocate ( sizeof ( CSL ), OPENVMTIL ) ;
    _CSL_Init ( csl, nss ) ;
    Linux_SetupSignals ( &csl->JmpBuf0, 1 ) ;
    return csl ;
}

void
CSL_OptimizeOn ( )
{
    SetState ( _CSL_, CO_ON, true ) ;
}

void
_CSL_OptimizeOff ( )
{
    SetState ( _CSL_, CO_ON, false ) ;
}

void
CSL_OptimizeOff ( )
{
    _CSL_OptimizeOff ( ) ;
    //_Printf ( (byte*) "\nCurrently optimize cannot be turned off else c syntax will not compile correctly in some cases, 
    // other problems may be there also." ) ;
    //Pause () ;
}

void
CSL_StringMacrosOn ( )
{
    SetState ( _CSL_, STRING_MACROS_ON, true ) ;
    _CSL_StringMacros_Init ( ) ;
}

void
CSL_StringMacrosOff ( )
{
    SetState ( _CSL_, STRING_MACROS_ON, false ) ;
    SetState ( &_CSL_->Sti, STI_INITIALIZED, false ) ;
}

void
CSL_InlineOn ( )
{
    SetState ( _CSL_, INLINE_ON, true ) ;
}

void
CSL_InlineOff ( )
{
    SetState ( _CSL_, INLINE_ON, false ) ;
}

void
CSL_DebugLevel ( )
{
    Do_C_Pointer_StackAccess ( ( byte* ) & _CSL_->DebugLevel ) ;
}

void
CSL_SaveDebugInfo ( Word * word, uint64 allocType )
{
    word = word ? word : _Context_->CurrentWordBeingCompiled ? _Context_->CurrentWordBeingCompiled : 0 ;
    if ( word )
    {
        Compiler * compiler = _Compiler_ ;
        if ( ! allocType ) allocType = COMPILER_TEMP ;
        if ( ! GetState ( word, DEBUG_INFO_SAVED ) )
        {
            if ( ! word->NamespaceStack ) // already done earlier
            {
                if ( ! word->W_SC_WordList )
                {
                    word->W_SC_WordList = _CSL_->CSL_N_M_Node_WordList ;
                    if ( _LC_ ) _LC_->Lambda_SC_WordList = word->W_SC_WordList ;
                    _CSL_->CSL_N_M_Node_WordList = _dllist_New ( WORD_RECYCLING ) ;
                }
                else List_Init ( _CSL_->CSL_N_M_Node_WordList ) ;
                if ( compiler->NumberOfVariables )
                {
                    word->W_NumberOfVariables = compiler->NumberOfVariables ;
                    word->NamespaceStack = Stack_Copy ( compiler->LocalsCompilingNamespacesStack, WORD_RECYCLING ) ;
                    //if ( Is_DebugOn ) Word_Show_NamespaceStackWords ( word ) ;
                    Namespace_RemoveAndReInitNamespacesStack_ClearFlag ( compiler->LocalsCompilingNamespacesStack, 0, 0 ) ; // don't clear ; keep words for source code debugging, etc.
                    _Namespace_RemoveFromUsingList_ClearFlag ( compiler->LocalsNamespace, 0, 0 ) ;
                }
                SetState ( word, DEBUG_INFO_SAVED, true ) ;
            }
        }
    }
    else CSL_DeleteDebugInfo ( ) ;
}

void
CSL_DeleteWordDebugInfo ( Word * word )
{
    if ( word )
    {
        if ( GetState ( word, DEBUG_INFO_SAVED ) )
        {
            if ( word->NamespaceStack ) // already done earlier
            {
                if ( ( word->W_SC_WordList ) ) //&& (word != _Debugger_->w_Word)) 
                {
                    if ( ! ( word->W_TypeAttributes & WT_INIT ) )
                    {
                        DLList_Recycle_WordList ( word->W_SC_WordList ) ; // 'init' : a temp fix??
                        word->W_SC_WordList = 0 ;
                    }
                    Namespace_RemoveAndReInitNamespacesStack_ClearFlag ( word->NamespaceStack, 1, 1 ) ; // don't clear ; keep words for source code debugging, etc.
                }
                word->NamespaceStack = 0 ; // this was allocate COMPILER_TEMP which will be deleted with 'rdi'
            }
            word->W_SC_WordList = 0 ; // this was allocate COMPILER_TEMP which will be deleted with 'rdi'
            SetState ( word, DEBUG_INFO_SAVED, false ) ;
        }
        word->W_SC_WordList = 0 ; // this was allocate COMPILER_TEMP which will be deleted with 'rdi'
    }
}

void
CSL_DeleteDebugInfo ( )
{
    Compiler_FreeLocalsNamespaces ( _Compiler_ ) ;
    if ( ! _Context_->CurrentWordBeingCompiled ) CSL_RecycleInit_CSL_N_M_Node_WordList ( ) ;
}

void
_CSL_FinishWordDebugInfo ( Word * word )
{
    if ( word && ( ! GetState ( _CSL_, ( RT_DEBUG_ON | GLOBAL_SOURCE_CODE_MODE ) ) ) ) CSL_DeleteDebugInfo ( ) ;
    else CSL_SaveDebugInfo ( word, 0 ) ;
}

void
CSL_PP_IncludeFileOnly_On ( )
{
    SetState ( _CSL_, PP_INCLUDE_FILES_ONLY, true ) ;
}

void
CSL_PP_IncludeFileOnly_Off ( )
{
    SetState ( _CSL_, PP_INCLUDE_FILES_ONLY, false ) ;
}

void
CSL_LC_DebugOn ( )
{
    LambdaCalculus * lc ;
    if ( lc = _LC_ ) SetState ( lc, LC_DEBUG_ON, true ) ;
    CSL_DebugOn ( ) ;
}

void
CSL_LC_DebugOff ( )
{
    LambdaCalculus * lc ;
    if ( lc = _LC_ ) SetState ( lc, LC_DEBUG_ON, false ) ;
    CSL_DebugOff ( ) ;
}

int64
_CSL_TerminalLineWidth ( CSL * csl )
{
    int64 tw = GetTerminalWidth ( ), i = 0 ;
    if ( tw == 0 ) tw = 140 ; 
    //while ( tw == 0 ) iPrintf ( "\rGetTerminalWidth ...\r%c[J", ESC ), tw = GetTerminalWidth ( ), i ++ ;
    if ( tw != csl->TerminalLineWidth ) csl->TerminalLineWidth = tw ;
    return csl->TerminalLineWidth ;
}

int64
TerminalLineWidth ( )
{
    _CSL_TerminalLineWidth ( _CSL_ ) ;
}

