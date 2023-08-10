
#include "../../include/csl.h"
// words need to be copied because a word may be used more than once in compiling a new word, 
// each needs to have their own coding, wordIndex, etc.
// this information is used by the compiler, optimizer and the debugger

Word *
_CopyDuplicateWord ( Word * word0, Boolean complete )
{
    Word * wordc ;
    wordc = Word_Copy ( word0, DICTIONARY ) ; // use DICTIONARY since we are recycling these anyway
    _dlnode_Init ( ( dlnode * ) wordc ) ; // necessary!
    wordc->W_ObjectAttributes |= ( uint64 ) RECYCLABLE_COPY ;
    wordc->StackPushRegisterCode = 0 ; // nb. used! by the rewriting optInfo
    if ( complete )
    {
        wordc->WL_OriginalWord = Word_GetOriginalWord ( word0 ) ;
        Word_SetLocation ( wordc ) ;
    }
    return wordc ;
}

Word *
CopyDuplicateWord ( dlnode * anode, Word * word0 )
{
    Word *cpdword = 0, * wordn = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) anode, SCN_T_WORD ) ;
    int64 iuoFlag = dobject_Get_M_Slot ( ( dobject* ) anode, SCN_IN_USE_FLAG ) ;
    if ( iuoFlag && ( word0 == wordn ) ) cpdword = _CopyDuplicateWord ( word0, 1 ) ;
    //else 
    //if ( word0 == wordn ) cpdword = _CopyDuplicateWord ( word0, 1 ) ;
    return cpdword ;
}

Word *
_CSL_CopyDuplicates ( Word * word0 )
{
    Word * word1, *wordToBePushed ;
    //if (( word0->W_MorphismAttributes & ( KEYWORD | CSL_WORD | T_LISP_SYMBOL ) ) ||  ( word0->W_ObjectAttributes & ( LITERAL ) ) ) word1 = _CopyDuplicateWord ( word0, 1 ) ;
#if 1
    if ( ( word0->W_MorphismAttributes & ( KEYWORD | CSL_WORD | T_LISP_SYMBOL ) ) ||
        ( word0->W_AllocType & ( OBJECT_MEM | INTERNAL_OBJECT_MEM | LISP_TEMP | TEMPORARY | COMPILER_TEMP ) ) ) word1 = _CopyDuplicateWord ( word0, 1 ) ;
    else word1 = ( Word * ) dllist_Map1_WReturn ( _CSL_->CSL_N_M_Node_WordList, ( MapFunction1 ) CopyDuplicateWord, ( int64 ) word0 ) ;
#else    
    word1 = _CopyDuplicateWord ( word0, 1 ) ;
#endif    
    if ( word1 ) wordToBePushed = word1 ;
    else wordToBePushed = word0 ;
    return wordToBePushed ;
}

Word *
Compiler_CopyDuplicatesAndPush ( Word * word0, int64 tsrli, int64 scwi )
{
    Word *wordp = 0 ;
    if ( word0 )
    {
        if ( GetState ( _Compiler_, ( COMPILE_MODE | ASM_MODE | ARRAY_MODE ) ) ) // don't we want to copy in non-compile mode too ??
        {
            wordp = _CSL_CopyDuplicates ( word0 ) ;
        }
        else wordp = word0 ;
        if ( ( word0->W_MorphismAttributes & ( DEBUG_WORD | INTERPRET_DBG ) ) || (
            word0->W_TypeAttributes & ( W_COMMENT | W_PREPROCESSOR ) ) || GetState ( _Context_, CONTEXT_PREPROCESSOR_MODE ) ) return word0 ;
        Lexer_Set_ScIndex_RlIndex ( _Lexer_, wordp, tsrli, scwi ) ;
        CSL_WordList_PushWord ( wordp ) ;
    }
    return wordp ;
}

byte *
Compiler_IncrementCurrentAccumulatedOffset ( Compiler * compiler, int64 increment )
{
    if ( compiler->AccumulatedOffsetPointer ) ( *( int64* ) ( compiler->AccumulatedOffsetPointer ) ) += ( increment ) ;
    if ( compiler->AccumulatedOptimizeOffsetPointer ) ( *( int64* ) ( compiler->AccumulatedOptimizeOffsetPointer ) ) += ( increment ) ;
    //_Debugger_->PreHere = ( ( byte* ) compiler->AccumulatedOffsetPointer ) - 3 ; // 3 : sizeof add immediate insn with rex
    return ( byte* ) compiler->AccumulatedOffsetPointer ; // 3 : sizeof add immediate insn with rex
}

void
Compiler_SetCurrentAccumulatedOffsetValue ( Compiler * compiler, int64 value )
{
    if ( compiler->AccumulatedOffsetPointer ) ( *( int64* ) ( compiler->AccumulatedOffsetPointer ) ) = ( value ) ;
    if ( compiler->AccumulatedOptimizeOffsetPointer ) ( *( int64* ) ( compiler->AccumulatedOptimizeOffsetPointer ) ) = ( value ) ;
}

void
Compiler_Init_AccumulatedOffsetPointers ( Compiler * compiler, Word * word )
{
    word->AccumulatedOffset = 0 ;
    compiler->AccumulatedOffsetPointer = 0 ;
    if ( word ) compiler->AccumulatedOptimizeOffsetPointer = & word->AccumulatedOffset ;
    else compiler->AccumulatedOptimizeOffsetPointer = 0 ;
}

void
Compiler_OptimizeRegisters ( void )
{
    //Context_ClearQidInNamespace ( ) ;
    //_Compiler_->LHS_Word = 0 ; // nb. : after NEW_INTERPRET
    _Compiler_->Lrpo = LocalsRegParameterOrder_Optimized ;
}

void
Compiler_InitRegisters ( void )
{
    //Context_ClearQidInNamespace ( ) ;
    //_Compiler_->LHS_Word = 0 ; // nb. : after NEW_INTERPRET
    _Compiler_->Lrpo = LocalsRegParameterOrder_Init ;
}

NamedByteArray *
_Compiler_SetCompilingSpace ( byte * name )
{
    NamedByteArray *nba = _OVT_Find_NBA ( name ) ;
    if ( nba ) Set_CompilerSpace ( nba->ba_CurrentByteArray ) ;
    return nba ;
}

byte *
_Compiler_GetCodeSpaceHere ( )
{
    NamedByteArray *nba = _OVT_Find_NBA ( ( byte* ) "CodeSpace" ) ;
    byte * here = _ByteArray_Here ( nba->ba_CurrentByteArray ) ;
    return here ;
}

void
Compiler_Set_LHS ( Word * word )
{
    _Compiler_->LHS_Word = word ;
    CSL_TypeStackReset ( ) ;
}

void
Compiler_SetCompilingSpace ( byte * name )
{
    _Compiler_SetCompilingSpace ( name ) ;
}

void
_Compiler_SetCompilingSpace_MakeSureOfRoom ( byte * name, int64 room )
{
    NamedByteArray * nba = _Compiler_SetCompilingSpace ( name ) ; // same problem as namespace ; this can be called in the middle of compiling another word 
    ByteArray * ba = _ByteArray_AppendSpace_MakeSure ( nba->ba_CurrentByteArray, room ) ;
    if ( ! ba ) Error_Abort ( "\n_Compiler_SetCompilingSpace_MakeSureOfRoom", "no ba?!\n" ) ;
}

void
Compiler_SetCompilingSpace_MakeSureOfRoom ( byte * name )
{
    _Compiler_SetCompilingSpace_MakeSureOfRoom ( name, 4 * K ) ;
}
#if 1
Word *
Compiler_PreviousNonDebugWord ( int64 startIndex )
{
    Word * word ;
    int64 i = startIndex ;
    if ( Verbosity () > 2 ) CSL_SC_WordList_Show ( ) ;
    for ( ; ( word = ( Word* ) CSL_WordList ( i ) ) ; i ++ )
    {
        if ( ( Symbol* ) word && ( ! ( word->W_MorphismAttributes & DEBUG_WORD ) ) ) break ;
    }
    return word ;
}
#else
Word *
Compiler_PreviousNonDebugWord ( int64 startIndex )
{
    Word * word ;
    int64 i = startIndex ;
    //if ( Verbosity () > 2 ) 
    if ( Is_DebugOn ) CSL_SC_WordList_Show ( ) ;
    for ( ; ( word = ( Word* ) CSL_WordList ( i ) ) ; i ++ )
    {
        if ( ( Symbol* ) word )
            if ( ( ! ( word->W_MorphismAttributes & DEBUG_WORD ) ) &&  ( word->W_ObjectAttributes & CLASS_TYPE ) ) break ;
    }
    return word ;
}
#endif

void
GotoInfo_Print ( dlnode * node )
{
    GotoInfo * gi = ( GotoInfo * ) node ;
    iPrintf ( "\nLabelName = %s : Type = %3d : CompileAtAddress = 0x%016lx : LabeledAddress = 0x%016lx : JmpOffsetPointer = : 0x%016lx",
        gi->pb_LabelName, gi->GI_CAttribute, gi->CompiledAtAddress, gi->LabeledAddress, gi->pb_JmpOffsetPointer ) ;
}

void
Compiler_GotoList_Print ( )
{
    Compiler * compiler = _Context_->Compiler0 ;
    iPrintf ( "\nTypes : GI_BREAK = 1 : GI_RETURN = 2 : GI_CONTINUE = 4 : GI_GOTO = 8 : GI_RECURSE = 16 : GI_LABEL = 64 : GI_GOTO_LABEL = 128" ) ;
    dllist_Map ( compiler->GotoList, ( MapFunction0 ) GotoInfo_Print ) ;
}

void
_CompileOptimizeInfo_Init ( CompileOptimizeInfo * optInfo )
{
    memset ( ( byte* ) & optInfo->State, 0, sizeof ( CompileOptimizeInfo ) - sizeof ( DLNode ) ) ;
}

void
CompileOptimizeInfo_Init ( CompileOptimizeInfo * optInfo, uint64 state )
{
    _CompileOptimizeInfo_Init ( optInfo ) ;
    dlnode * node ;
    int64 i ;
    // we don't really use optInfo->COIW much 
    for ( i = 0, node = dllist_First ( ( dllist* ) _CSL_->CSL_N_M_Node_WordList ) ; node ; node = dlnode_Next ( node ) ) // nb. this is a little subtle
    {
        if ( dobject_Get_M_Slot ( ( dobject* ) node, SCN_IN_USE_FLAG ) )
        {
            if ( i < 8 ) optInfo->COIW [ i ++ ] = ( Word * ) dobject_Get_M_Slot ( ( dobject* ) node, SCN_T_WORD ) ;
            else break ;
        }
    }
    optInfo->State = state ;
}

CompileOptimizeInfo *
_CompileOptimizeInfo_New ( uint64 type )
{
    CompileOptimizeInfo * optInfo = ( CompileOptimizeInfo * ) Mem_Allocate ( sizeof (CompileOptimizeInfo ), type ) ;
    optInfo->InUseFlag = N_LOCKED ;
    return optInfo ;
}

CompileOptimizeInfo *
CompileOptimizeInfo_New ( uint64 type )
{
    CompileOptimizeInfo * optInfo = optInfo = _CompileOptimizeInfo_New ( type ) ;
    return optInfo ;
}

CompileOptimizeInfo *
COInfo_PushNew ( Compiler * compiler )
{
    CompileOptimizeInfo * coi = CompileOptimizeInfo_New ( COMPILER_TEMP ) ;
    if ( coi )
    {
        List_Push ( compiler->OptimizeInfoList, ( dlnode* ) coi ) ;
        compiler->OptInfo = coi ; // we are using the top of the stack/list
    }
    return coi ;
}

CompileOptimizeInfo *
COInfo_New ( Compiler * compiler, uint64 type )
{
    compiler->OptInfo = _CompileOptimizeInfo_New ( type ) ;
    return compiler->OptInfo ;
}

CompileOptimizeInfo *
CompileOptInfo_NewCopy ( CompileOptimizeInfo * optInfo, uint64 type )
{
    CompileOptimizeInfo * copyOptInfo = CompileOptimizeInfo_New ( type ) ;
    MemCpy ( copyOptInfo, optInfo, sizeof (CompileOptimizeInfo ) ) ;
    return copyOptInfo ;
}

int64
Compiler_BlockLevel ( Compiler * compiler )
{
    int64 depth = _Stack_Depth ( compiler->BlockStack ) ;
    return depth ;
}

void
Compiler_FreeLocalsNamespaces ( Compiler * compiler )
{
    //we may want to see the local variables with a debugger stepping thru
    int64 clearFlag = 0 ; //! _AtCommandLine ( )  ;
    if ( compiler->NumberOfVariables || compiler->NumberOfNonRegisterLocals ) Namespace_RemoveAndReInitNamespacesStack_ClearFlag ( compiler->LocalsCompilingNamespacesStack, clearFlag, 0 ) ;
    if ( compiler->LocalsNamespace ) _Namespace_RemoveFromUsingList_ClearFlag ( compiler->LocalsNamespace, clearFlag, 0 ) ;
    compiler->LocalsNamespace = 0 ;
}

void
Compiler_Init ( Compiler * compiler, uint64 state )
{
    //int64 svTdsciState = GetState ( compiler, TDI_PARSING ) ;
    Compiler_FreeLocalsNamespaces ( compiler ) ;
    compiler->State = ( state &= ( ~ ARRAY_MODE ) ) ;
    //if ( svTdsciState ) SetState ( compiler, TDI_PARSING, true ) ;
    compiler->ContinuePoint = 0 ;
    compiler->BreakPoint = 0 ;
    compiler->InitHere = Here ;
    compiler->ParenLevel = 0 ;
    compiler->ArrayEnds = 0 ;
    compiler->NumberOfNonRegisterLocals = 0 ;
    compiler->NumberOfLocals = 0 ;
    compiler->NumberOfRegisterLocals = 0 ;
    compiler->NumberOfArgs = 0 ;
    compiler->NumberOfNonRegisterArgs = 0 ;
    compiler->NumberOfRegisterArgs = 0 ;
    compiler->NumberOfVariables = 0 ;
    compiler->NumberOfRegisterVariables = 0 ;
    compiler->NumberOfNonRegisterVariables = 0 ;
    compiler->LocalsFrameSize = 0 ;
    compiler->AccumulatedOffsetPointer = 0 ;
    compiler->ReturnVariableWord = 0 ;
    compiler->ReturnLParenOperandWord = 0 ;
    compiler->Current_Word_New = 0 ;
    compiler->BlockLevel = 0 ;
    CSL_NonCompilingNs_Clear ( compiler ) ; // for special syntax : we have a namespace but not while compiling
    compiler->LocalsNamespace = 0 ;
    compiler->Current_Word_Create = 0 ;
    Stack_Init ( compiler->PointerToJmpInsnStack ) ;
    Stack_Init ( compiler->InfixOperatorStack ) ;
    //if ( compiler->LocalsCompilingNamespacesStack ) 
    Stack_Init ( compiler->LocalsCompilingNamespacesStack ) ;
    //compiler->LocalsCompilingNamespacesStack = Stack_New ( 64, WORD_RECYCLING ) ; // allow recycling across contexts
    
    Stack_Init ( compiler->BlockStack ) ;
    Stack_Init ( compiler->CombinatorStack ) ;
    Stack_Init ( compiler->CombinatorBlockInfoStack ) ;
    _dllist_Init ( compiler->GotoList ) ;
    
    _dllist_Init ( compiler->CurrentMatchList ) ;
    _dllist_Init ( compiler->RegisterParameterList ) ;
    _dllist_Init ( compiler->OptimizeInfoList ) ;
    COInfo_PushNew ( compiler ) ;
    //SetBuffersUnused ( 0 ) ;
    SetState ( compiler, VARIABLE_FRAME, false ) ;
    //OVT_MemListFree_CompilerTempObjects ( ) ;
    //CSL_AfterWordReset ( ) ;
}

Compiler *
Compiler_New ( uint64 allocType )
{
    Compiler * compiler = ( Compiler * ) Mem_Allocate ( sizeof (Compiler ), allocType ) ;
    compiler->Lrpo = LocalsRegParameterOrder_Init ;
    compiler->BlockStack = _Stack_Allocate ( 64, allocType ) ;
    compiler->CombinatorStack = _Stack_Allocate ( 64, allocType ) ;
    compiler->InfixOperatorStack = _Stack_Allocate ( 64, allocType ) ;
    compiler->PointerToJmpInsnStack = _Stack_Allocate ( 64, allocType ) ;
    compiler->CombinatorBlockInfoStack = _Stack_Allocate ( 64, allocType ) ;
    compiler->LocalsCompilingNamespacesStack = _Stack_Allocate ( 64, allocType ) ; // allow recycling across contexts
    compiler->InternalNamespacesStack = Stack_New ( 64, allocType ) ; //initialized when using
    compiler->PostfixLists = _dllist_New ( allocType ) ;
    compiler->GotoList = _dllist_New ( allocType ) ;
    compiler->SetccMovedList = _dllist_New ( allocType ) ;
    compiler->RegisterParameterList = _dllist_New ( allocType ) ;
    compiler->OptimizeInfoList = _dllist_New ( allocType ) ;
    Compiler_Init ( compiler, 0 ) ;

    return compiler ;
}

Compiler *
Compiler_Copy ( Compiler * compiler, uint64 allocType )
{
    Compiler * compilerCopy = ( Compiler * ) Mem_Allocate ( sizeof (Compiler ), allocType ) ;
    memcpy ( compilerCopy, compiler, sizeof (Compiler ) ) ;
    return compilerCopy ;
}

void
CSL_CompileAndRecord_Word0_PushReg ( Boolean reg, Boolean recordFlag )
{

    Word * word = CSL_WordList ( 0 );
    _Word_CompileAndRecord_PushReg (word, reg, recordFlag , 0) ;
}

void
CSL_CompileAndRecord_Word0_PushRegToUse ( )
{

    Word * word = CSL_WordList ( 0 );
    _Word_CompileAndRecord_PushReg (word, word->RegToUse, true , 0) ;
}

void
CSL_CompileAndRecord_PushAccum ( )
{
    Word * word = CSL_WordList ( 0 );
    _Word_CompileAndRecord_PushReg (word, ACC, true , 0) ;
}


