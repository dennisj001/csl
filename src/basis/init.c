#include "../include/csl.h"

// all except namespaces and number base
// this is called by the main interpreter _CSL_Interpret

void
CSL_RuntimeInit ( CSL * csl, int64 cntxDelFlag )
{
    //CSL_LogOff ( ) ;
    csl->SC_QuoteMode = 0 ;
    csl->SC_Word = 0 ;
    if ( cntxDelFlag )
    {
        int64 i, stackDepth = Stack_Depth ( csl->ContextStack ) ;
        for ( i = 0 ; i < stackDepth ; i ++ ) CSL_Context_PopDelete ( csl ) ;
    }
    SetState_TrueFalse ( csl, CSL_RUN, DEBUG_MODE ) ;
    SetState ( csl->Debugger0, DBG_ACTIVE, false ) ;
    DebugOff ;
    SetBuffersUnused ( 1 ) ;
    d0 ( Buffer_PrintBuffers ( ) ) ;
    DefaultColors ;
    //LC_Reset ( ) ;
    CSL_CheckInitDataStack ( ) ;
    _CSL_TypeStackReset ( ) ;
    CSL_RecycleInit_CSL_N_M_Node_WordList ( ) ;
    Context_ClearQualifyingNamespace ( ) ;
}

void
OVT_RuntimeInit ( )
{
    OVT_FreeTempMem ( ) ;
}

void
_CSL_Init_SessionCore ( CSL * csl, Boolean cntxDelFlag, Boolean promptFlag )
{
    Context * cntx = csl->Context0 ;
    if ( csl->InitSessionCoreTimes++ ) OVT_RuntimeInit ( ) ;
    _System_Init ( cntx->System0 ) ;
    ReadLine_Init ( cntx->ReadLiner0, _CSL_Key ) ;
    Lexer_Init ( cntx->Lexer0, 0, 0, CONTEXT ) ;
    Finder_Init ( cntx->Finder0 ) ;
    Compiler_Init ( cntx->Compiler0, 0 ) ;
    Interpreter_Init ( cntx->Interpreter0 ) ;
    LC_Init_Runtime ( ) ;
    CSL_RuntimeInit ( csl, cntxDelFlag ) ;
    _bigNum_Init ( 16 ) ;
}

void
CSL_SessionInit ( )
{
    _CSL_Init_SessionCore ( _CSL_, 0, 1 ) ;
}

void
CSL_ResetAll_Init ( CSL * csl )
{
    Exception *e = _O_->OVT_Exception ;
#if 0
    byte * startDirectory ;
    //if ( ! GetState ( _O_, OVT_IN_USEFUL_DIRECTORY ) ) startDirectory = ( byte* ) "/usr/local/lib/csl" ;
    if ( ! GetState ( _O_, OVT_IN_USEFUL_DIRECTORY ) ) startDirectory = ( byte* ) "/usr/local/lib/csl/namespaces" ;
    else startDirectory = ( byte* ) "namespaces" ;
    DataObject_New ( NAMESPACE_VARIABLE, 0, ( byte* ) "_startDirectory_", 0, NAMESPACE_VARIABLE, 0, 0, ( int64 ) startDirectory, 0, 0, 0, 0 ) ;
#endif    
    _CSL_Init_SessionCore ( csl, 1, 0 ) ;
    //CSL_DebugOn () ; //Pause () ;
    if ( e->RestartCondition >= RESET_ALL )
    {
        e->StartIncludeTries = 0 ;
        _CSL_Namespace_NotUsing ( ( byte* ) "BigNum" ) ;
        _CSL_Namespace_NotUsing ( ( byte* ) "Lisp" ) ;
        if ( _O_->StartupFilename )
        {
            _O_->Verbosity = 0 ;
            CSL_ContextNew_IncludeFile (( byte* ) "./namespaces/sinit.csl", 0) ;
            CSL_ContextNew_IncludeFile (_O_->StartupFilename, 0) ;
        }
        else
        {
            if ( ! e->StartIncludeTries ++ )
            {
                _CSL_ContextNew_InterpretString ( csl, _O_->InitString ) ;
                _CSL_ContextNew_InterpretString ( csl, _O_->StartupString ) ;
            }
            else if ( e->StartIncludeTries < 3 )
            {
                AlertColors ;
                CSL_ContextNew_IncludeFile (( byte* ) "./namespaces/init.csl", 0) ;
                if ( _O_->ErrorFilename )
                {
                    if ( strcmp ( ( char* ) _O_->ErrorFilename, "Debug Context" ) )
                    {
                        iPrintf ( "\nError : \"%s\" include error!\n", e->Location ? e->Location : _O_->ErrorFilename ) ;
                    }
                }
                DefaultColors ;
            }
        }
    }
    if ( Verbosity () > 3 )
    {
        iPrintf ( " \nInternal Namespaces have been initialized.  " ) ;
        OVT_ShowMemoryAllocated ( ) ;
    }
    //if ( ( _O_->InitSessionCoreTimes == 1 ) || ( ! Verbosity () ) ) Verbosity () = 1 ;
}

void
_CSL_InitialAddWordToNamespace ( Word * word, byte * containingNamespaceName, byte * superNamespaceName )
// this is only called at startup where we want to add the namespace to the RootNamespace
{
    Namespace *ns, *sns = _CSL_->Namespaces ;
    if ( superNamespaceName )
    {
        sns = Namespace_FindOrNew_SetUsing ( superNamespaceName, sns, 1 ) ;
        sns->State |= USING ;
    }
    ns = Namespace_FindOrNew_SetUsing ( containingNamespaceName, sns, 1 ) ; // find new namespace or create anew
    Namespace_DoAddWord ( ns, word ) ; // add word to new namespace
}

void
_CSL_CPrimitiveNewAdd ( const char * name, byte * pb_TypeSignature, uint64 opInsnGroup, uint64 opInsCode, block b, uint64 morphismAttributes,
    uint64 objectAttributes, uint64 lispAttributes, const char *nameSpace, const char * superNamespace )
{
    Word * word = _Word_New ( ( byte* ) name, CPRIMITIVE | morphismAttributes, objectAttributes, lispAttributes, 1, 0, DICTIONARY ) ;
    _DObject_ValueDefinition_Init ( word, ( int64 ) b, BLOCK, 0, 0 ) ;
    _CSL_InitialAddWordToNamespace ( word, ( byte* ) nameSpace, ( byte* ) superNamespace ) ;
    if ( morphismAttributes & INFIXABLE ) word->W_TypeAttributes |= WT_INFIXABLE ;
    else if ( morphismAttributes & PREFIX ) word->W_TypeAttributes |= WT_PREFIX ;
    else if ( morphismAttributes & PREFIXABLE ) word->W_TypeAttributes |= WT_PREFIXABLE ;
    else if ( morphismAttributes & C_PREFIX_RTL_ARGS ) word->W_TypeAttributes |= WT_C_PREFIX_RTL_ARGS ;
    else word->W_TypeAttributes |= WT_POSTFIX ;
    if ( lispAttributes & W_COMMENT ) word->W_TypeAttributes |= W_COMMENT ;
    if ( lispAttributes & W_PREPROCESSOR ) word->W_TypeAttributes |= W_PREPROCESSOR ;
    word->W_ObjectAttributes = objectAttributes ;
    word->W_OpInsnCode = opInsCode ;
    if ( opInsnGroup & (WT_BYTE|WT_WORD|WT_LONG) ) word->W_TypeAttributes |= opInsnGroup ;
    word->W_OpInsnGroup = opInsnGroup ;
    if ( pb_TypeSignature ) strncpy ( ( char* ) &word->W_TypeSignatureString, ( char* ) pb_TypeSignature, 8 ) ;
}

void
CSL_AddCPrimitives ( )
{
    int64 i ;
    for ( i = 0 ; CPrimitives [ i ].ccp_Name ; i ++ )
    {
        CPrimitive p = CPrimitives [ i ] ;
        _CSL_CPrimitiveNewAdd ( p.ccp_Name, ( byte* ) p.pb_TypeSignature, p.OpInsnCodeGroup, p.OpInsnCode, p.blk_Definition, p.ui64_MorphismAttributes, p.ui64_ObjectAttributes, p.ui64_LispAttributes, ( char* ) p.NameSpace, ( char* ) p.SuperNamespace ) ;
    }
    //_CSL_CPrimitiveNewAdd ( p.ccp_Name, p.blk_Definition, p.ui64_CAttribute, p.ui64_CAttribute2, p.ui64_LAttribute, ( char* ) p.NameSpace, ( char* ) p.SuperNamespace ) ;
}

