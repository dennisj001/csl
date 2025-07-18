
#include "../../include/csl.h"
// old docs
//LambdaCalculus * _LC_ ;
// --------------------------------------------------------
// LC  : an abstract core of a prefix language related to Lambda Calculus with list objects
//       (it seems neither lisp nor scheme as they exist meet my more abstract intent so it's "Lambda Calculus")
// LO  : ListObject
// nb. : a Word is a ListObject is a Namespace is a DObject 
//     : a ListObject is a Word is a Namespace is a DObject 
// ---------------------------------------------------------

// lisp type lists can maybe be thought of as not rpn - not reverse polish notation, 
// not postfix but prefix - cambridge polish/prefix notation blocks

// sections  
// _LO_Eval
// _LO_Apply 
// _LO_Read 
// _LO_Print
// _LO_SpecialFunctions
// LO_Repl
// LO_...misc : _L0_New _L0_Copy
// LC_x

//===================================================================================================================
//| LO_Eval
//===================================================================================================================
// #define EVAL(x)         (isNum(x)? x : isSym(x)? val(x) : evList(x)) // picolisp

// for calling 'C' functions such as printf or other system functions
// where the arguments are pushed first from the end of the list like 'C' arguments

// subst : lisp 1.5
// set the value of the lambda parameters to the function call values - a beta reduction in the lambda calculus 

//
// nb. we must not use the word as the ListObject because it is on its own namespace list
// so we create a new ListObject with a pointer to the word for its values/properties

// can we just use the 'word' instead of this

//===================================================================================================================
//| LO Misc : _LO_FindWord _LO_New _LO_Copy
//===================================================================================================================

ListObject *
_LO_New_RawStringOrLiteral ( Lexer * lexer, byte * token, int64 value, int64 qidFlag, int64 tsrli, int64 scwi )
{
    if ( GetState ( lexer, KNOWN_OBJECT ) )
    {
        uint64 objectAttributes = qidFlag ? OBJECT : LITERAL ;
        uint64 morphismAttributes = lexer->L_MorphismAttributes ;
        //_DObject_New ( byte * name, uint64 value, uint64 morphismType, uint64 objectType, uint64 lispType, uint64 functionType, byte * function, int64 arg,
        //    int64 addToInNs, Namespace * addToNs, uint64 allocType )
        int64 allocType = GetState ( _Compiler_, LC_ARG_PARSING ) ? DICTIONARY : LISP ;
        Word * word = _DObject_New ( lexer->OriginalToken, value, IMMEDIATE | morphismAttributes,
            ( lexer->L_ObjectAttributes | LITERAL ), objectAttributes, lexer->L_ObjectAttributes | objectAttributes, ( byte* ) _DataObject_Run, 0, 0, 0, allocType ) ;
        if ( qidFlag ) word->W_ObjectAttributes &= ~ T_LISP_SYMBOL ;
        else if ( lexer->L_ObjectAttributes & ( T_RAW_STRING ) )
        {
            // nb. we don't want to do this block with literals it slows down the eval and is wrong
            word->W_LispAttributes |= ( T_LISP_SYMBOL | T_RAW_STRING ) ;
            word->Lo_Value = ( int64 ) word->Lo_Name ;
        }
        word->Lo_CSL_Word = word ;
        if ( qidFlag ) word->W_MorphismAttributes &= ~ T_LISP_SYMBOL ;
        word->W_SC_Index = scwi ;
        word->W_RL_Index = tsrli ;
        return word ;
    }
    else
    {
        iPrintf ( "\n%s ?\n", ( char* ) token ) ;
        CSL_Exception ( NOT_A_KNOWN_OBJECT, 0, QUIT ) ;
        return 0 ;
    }
}

ListObject *
_LO_New ( uint64 lispAttributes, uint64 morphismAttributes, uint64 objectAttributes, byte * name, byte * value, Word * word, uint64 allocType, Namespace * addToNs, int64 tsrli, int64 scwi )
{
    //_DObject_New ( byte * name, uint64 value, uint64 morphismType, uint64 objectType, uint64 lispType, uint64 functionType, byte * function, int64 arg,
    //    int64 addToInNs, Namespace * addToNs, uint64 allocType )
    ListObject * l0 = _DObject_New ( word ? word->Name : name ? name : ( byte* ) "", ( uint64 ) value, morphismAttributes, ( objectAttributes ), lispAttributes,
        ( lispAttributes & T_LISP_SYMBOL ) ? word ? word->RunType : 0 : 0, 0, 0, 0, addToNs, LISP ) ; //addToNs, LISP ) ;
    if ( lispAttributes & LIST ) _LO_ListInit ( l0, allocType ) ;
    else if ( lispAttributes & LIST_NODE ) l0->S_SymbolList = ( dllist* ) value ;
    else if ( word )
    {
        //Word * word = Compiler_CopyDuplicatesAndPush ( word0, tsrli, scwi ) ;
        l0->Lo_CSL_Word = word ;
        word->Lo_CSL_Word = word ;
        l0->W_OriginalCodeText = word->W_OriginalCodeText ;
        word->W_SC_Index = scwi ;
        word->W_RL_Index = tsrli ;
    }
    l0->W_SC_Index = scwi ;
    l0->W_RL_Index = tsrli ;
    return l0 ;
}

ListObject *
LO_First ( ListObject * l0 )
{
    if ( l0 && ( ! ( l0->W_LispAttributes & ( T_NIL ) ) ) )
    {
        if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) return ( ListObject* ) dllist_First ( ( dllist* ) ( dllist * ) l0->Lo_List ) ;
        else return l0 ;
    }
    return 0 ;
}

ListObject *
_LO_Last ( ListObject * l0 )
{
    if ( l0 && ( ! ( l0->W_LispAttributes & ( T_NIL ) ) ) )
    {
        if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) return ( ListObject* ) dllist_Last ( ( dllist * ) l0->Lo_List ) ;
        else return l0 ;
    }
    return 0 ;
}

Word *
LC_FindWord ( byte * name )
{
    LambdaCalculus * lc = _LC_ ;
    Word * word = 0 ;
    ListObject * locals = lc->Locals ;
    if ( GetState ( _Compiler_, LC_ARG_PARSING ) ) word = Finder_Word_FindUsing ( _Context_->Finder0, name, 0 ) ;
    else
    {
        if ( locals ) word = _Finder_FindWord_InOneNamespace ( _Finder_, locals, name ) ;
        if ( ! word )
        {
            word = _Finder_FindWord_InOneNamespace ( _Finder_, lc->LispDefinesNamespace, name ) ;
            if ( ! word )
            {
#if 0        
                if ( Is_DebugModeOn )
                {
                    iPrintf ( "\n\nLC_FindWord : LispDefinesNamespace : can't find : name = %s\n", name ) ;
                    LC_Print_LispDefinesNamespace ( ) ;
                }
#endif        
                word = _Finder_FindWord_InOneNamespace ( _Finder_, lc->LispNamespace, name ) ; // prefer Lisp namespace
                if ( ! word )
                {
#if 0        
                    if ( Is_DebugModeOn )
                    {
                        iPrintf ( "\n\nLC_FindWord : LispNamespace : can't find : name = %s\n", name ) ;
                        LC_Print_LispNamespace ( ) ;
                    }
#endif        
                    word = Finder_Word_FindUsing ( _Finder_, name, 0 ) ;
                }
            }
        }
    }
    return word ;
}

Boolean
LO_strcat ( byte * buffer, byte * buffer2 )
{
    if ( Strlen ( ( char* ) buffer2 ) + Strlen ( ( char* ) buffer ) >= BUFFER_IX_SIZE )
    {
        Error ( "LambdaCalculus : LO_strcat : buffer overflow.", QUIT ) ;
    }
    else strncat ( ( char* ) buffer, ( char* ) buffer2, BUFFER_IX_SIZE ) ;
    buffer2 [0] = 0 ;
    return true ;
}

void
_LO_ListInit ( ListObject * l0, uint64 allocType )
{
    l0->Lo_List = _dllist_New ( allocType ) ; //( dllist* ) l0 ;
    l0->W_LispAttributes |= LIST ; // a LIST_NODE needs to be initialized also to be also a LIST
}

ListObject *
LO_List_New ( uint64 allocType )
{
    ListObject * l0 = LO_New ( LIST, 0 ) ;
    _LO_ListInit ( l0, allocType ) ;
    return l0 ;
}

ListObject *
_LO_CopyOne ( ListObject * l0, uint64 allocType )
{
    ListObject * l1 = 0 ;
    if ( l0 )
    {
        l1 = Word_Copy ( l0, allocType ) ;
        // nb. since we are coping the car/cdr are the same as the original so we must clear them else when try to add to the list and remove first it will try to remove from a wrong list so ...
        l1->S_Car = 0 ;
        l1->S_Cdr = 0 ;
        if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) l1->W_LispAttributes |= ( LIST | LIST_NODE ) ;
    }
     return l1 ;
}

// copy a whole list or a single node

ListObject *
_LO_Copy ( ListObject * l0, uint64 allocType )
{
    ListObject * lnew = 0, *l1, *lnext, *lcopy ;
    if ( l0 )
    {
        if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) lnew = LO_List_New ( allocType ) ;
        for ( l1 = LO_First ( l0 ) ; l1 ; l1 = lnext )
        {
            lnext = LO_Next ( l1 ) ;
            lcopy = _LO_CopyOne ( l1, allocType ) ;
            if ( lnew ) LO_AddToTail ( lnew, lcopy ) ;
            else return lcopy ;
        }
    }
    return lnew ;
}

//===================================================================================================================
//| LO_Repl
//===================================================================================================================

void
LC_EvalPrint ( ListObject * lread )
{
    _LC_->L1 = LC_Eval ( lread, 0, 1 ) ;
    //if ( Verbosity ( )  ) iPrintf ( "EvalDepth = %ld : EvalListDepth= %ld\n", _LC_->EvalDepth, _LC_->EvalListDepth ) ;
    _LC_->EvalDepth = 0 ;
    _LC_->EvalListDepth = 0 ;
    LO_Print ( _LC_->L1 ) ;
    CSL_NewLine ( ) ;
    SetState ( _LC_, LC_PRINT_ENTERED, false ) ;
    SetBuffersUnused ( 1 ) ;
    _LC_->ParenLevel = 0 ;
    _LC_->Sc_Word = 0 ;
    Compiler_Init ( _Context_->Compiler0, 0 ) ; // we could be compiling a csl word as in oldLisp.csl
}

ListObject *
_LC_Read_ListObject ( LambdaCalculus * lc, int64 parenLevel, int64 startReadIndex )
{
    lc->ParenLevel = parenLevel ;
    ListObject * l0 = LC_Read ( ) ;
    return l0 ;
}

void
_LC_ReadEvalPrint_ListObject ( int64 parenLevel, int64 continueFlag, uint64 itemQuoteState )
{
    LambdaCalculus * lc = _LC_ ;
    Lexer * lexer = _Context_->Lexer0 ;
    Compiler * compiler = _Context_->Compiler0 ;
    Compiler_Init ( compiler, 0 ) ; // we could be compiling a csl word as in oldLisp.csl
    int64 svTypeCheckState = GetState ( _CSL_, DBG_TYPECHECK_ON ) ;
    byte *svDelimiters = lexer->TokenDelimiters ;
    SetState ( _Context_, LISP_MODE, true ) ;
    compiler->InitHere = Here ;
    if ( ! parenLevel ) CSL_InitSourceCode ( _CSL_ ) ;
    else CSL_InitSourceCode_WithCurrentInputChar ( _CSL_, 1 ) ;
    SetState ( _CSL_, DBG_TYPECHECK_ON, false ) ;

    if ( lc && parenLevel ) lc->QuoteState = lc->ItemQuoteState ;
    else lc = LC_Init_Runtime ( ) ;
    SetState ( lc, LC_INTERP_DONE, false ) ;
    LC_LispNamespaceOn ( ) ;
    LC_Init_Variables ( lc ) ;
    lc->ItemQuoteState = itemQuoteState ;
    lc->ParenLevel = parenLevel ;
    int64 * svDsp = _DspReg_ ;
    SetState ( lc, LC_EVAL_PRINT, true ) ;
    lc->Lread = _LC_Read_ListObject ( lc, parenLevel, 0 ) ;
    LC_EvalPrint ( lc->Lread ) ;
    LC_Debug ( lc, LC_EVAL_PRINT, 0 ) ;
    SetState ( lc, LC_EVAL_PRINT, false ) ;
    LC_ClearTempNamespace ( ) ;
    _DspReg_ = svDsp ;

    if ( ! continueFlag ) Lexer_SetTokenDelimiters ( lexer, svDelimiters, 0 ) ;
    SetState ( _Context_, LISP_MODE, false ) ;
    SetState ( _CSL_, DBG_TYPECHECK_ON, svTypeCheckState ) ;
}

void
LC_ReadEvalPrint_ListObject ( )
{
    _LC_ReadEvalPrint_ListObject ( 0, 1, 0 ) ;
}

void
LC_ReadEvalPrint_AfterAFirstLParen ( )
{
    _LC_ReadEvalPrint_ListObject ( 1, 0, 0 ) ;
    SetState ( _CSL_, SOURCE_CODE_STARTED, false ) ;
}

void
LC_ReadEvalPrint ( )
{
    _LC_ReadEvalPrint_ListObject ( 0, 0, 0 ) ;
}

void
LC_ReadInitFile ( byte * filename )
{
    CSL_ContextNew_IncludeFile ( filename, 0 ) ;
}

void
_LO_Repl0 ( Context * cntx, byte * initFilename )
{
    SetState ( _Context_, LISP_MODE, true ) ;
    iPrintf ( "csl lisp : type '..', 'exit', 'quit', 'bye' to exit lisp interpreter" ) ;
    //if ( ! initFilename ) initFilename = ( byte* ) "namespaces/compiler/lcinit.csl" ; //
    LC_ReadInitFile ( initFilename ) ;
    SetState ( _Context_, AT_COMMAND_LINE, true ) ;
    _Repl ( cntx, ( block ) LC_ReadEvalPrint_ListObject ) ;
    SetState ( _Context_, LISP_MODE, false ) ;
    LC_LispNamespacesOff ( ) ;
    iPrintf ( "\nleaving csl lisp : returning to csl interpreter\n" ) ;
}

void
LO_Repl0 ( Context * cntx, byte * initFilename )
{
    if ( ! initFilename ) initFilename = ( byte* ) "namespaces/compiler/lcinit.csl" ; //
    _LO_Repl0 ( cntx, initFilename ) ;
}

void
LO_Repl ( )
{
    int64 * svDsp = _DspReg_ ;
    byte * initFilename = ( byte* ) DataStack_Pop ( ) ;
    _CSL_Contex_NewRun_1 ( _CSL_, (ContextFunction_1) LO_Repl0, initFilename ) ;
    _Set_DataStackPointers ( svDsp ) ;
}

//===================================================================================================================
//| LC_ : lambda calculus
//===================================================================================================================

void
LC_ReadList ( )
{
    LambdaCalculus * lc = LC_Init_Runtime ( ) ;
    ListObject * l0 = _LC_Read_ListObject ( lc, 1, 0 ) ;
    DataStack_Push ( ( int64 ) l0 ) ;
}

void
_LC_SaveDsp ( LambdaCalculus * lc )
{
    if ( lc ) lc->SaveStackPointer = _DspReg_ ;
}

void
_LC_ResetStack ( LambdaCalculus * lc )
{
    if ( lc && ( lc->SaveStackPointer ) ) _DspReg_ = lc->SaveStackPointer ;
}

void
LC_RestoreStack ( )
{
    _LC_ResetStack ( _LC_ ) ;
}

void
LC_SaveStack ( )
{
    _LC_SaveDsp ( _LC_ ) ;
}

void
LC_FinishSourceCode ( )
{
    _LC_->LC_SourceCode = _CSL_GetSourceCode ( ) ;
}

void
_LC_ClearDefinesNamespace ( LambdaCalculus * lc )
{
    if ( lc ) _Namespace_Clear ( lc->LispDefinesNamespace, 1 ) ;
}

void
LC_ClearDefinesNamespace ( )
{
    _LC_ClearDefinesNamespace ( _LC_ ) ;
}

void
LC_Print_LispDefinesNamespace ( )
{
    iPrintf ( "\n LC_Print_LispDefinesNamespace : printing ...\n" ) ;
    List_PrintNames ( _LC_->LispDefinesNamespace->W_List, - 1, 0 ) ;
    iPrintf ( "\n" ) ;
}

void
LC_Print_LispNamespace ( )
{
    iPrintf ( "\n LC_Print_LispNamespace : printing ...\n" ) ;
    List_PrintNames ( _LC_->LispNamespace->W_List, - 1, 0 ) ;
    iPrintf ( "\n" ) ;
}

void
_LC_ClearTempNamespace ( LambdaCalculus * lc )
{
    if ( lc ) _Namespace_Clear ( lc->LispTempNamespace, 1 ) ;
    OVT_MemListFree_LispTemp ( ) ;
}

void
LC_ClearTempNamespace ( )
{
    _LC_ClearTempNamespace ( _LC_ ) ;
}

void
LC_LispNamespacesOff ( )
{
    if ( _LC_ )
    {
        Namespace_SetAsNotUsing ( ( byte* ) "LispTemp" ) ;
        Namespace_SetAsNotUsing ( ( byte* ) "LispDefines" ) ;
        Namespace_SetAsNotUsing ( ( byte* ) "Lisp" ) ;
        Context_ClearQualifyingNamespace ( ) ;
        if ( _LC_->SavedTypeCheckState && GetState ( _CSL_, TYPECHECK_ON ) ) CSL_TypeCheckOn ( ) ;
        SetState ( _LC_, LC_DEBUG_ON, false ) ;
    }
}

void
LC_LispNamespaceOn ( )
{
    Namespace_ActivateAsPrimary ( ( byte* ) "Lisp" ) ;
    _LC_->SavedTypeCheckState = GetState ( _CSL_, TYPECHECK_ON ) ;
    CSL_TypeCheckOff ( ) ; // too much LC stack sometimes and not needed
}

void
LC_Init_Variables ( LambdaCalculus * lc )
{
    lc->L0 = 0 ;
    lc->L1 = 0 ;
    lc->Lfirst = 0 ;
    lc->Lfunction = 0 ;
    lc->Locals = 0 ;
    lc->Largs0 = 0 ;
    lc->Largs = 0 ;
    lc->Largs1 = 0 ;
    lc->Code = 0 ;
    lc->ApplyFlag = 0 ;
    lc->ArrayBaseObject = 0 ;
    lc->BaseObject = 0 ;
    lc->LC_Here = Here ;
}

LambdaCalculus *
_LC_Init_Runtime ( LambdaCalculus * lc )
{
    if ( lc->QuoteStateStack ) _Stack_Init ( lc->QuoteStateStack, 256 ) ;
    LC_ClearTempNamespace ( ) ;
    lc->SavedCodeSpace = 0 ;
    _LC_SaveDsp ( lc ) ;
    lc->ParenLevel = 0 ;
    lc->QuoteState = 0 ;
    lc->ItemQuoteState = 0 ;
    lc->DebuggerState = 0 ;
    lc->DebuggerSetupFlag = 0 ;
    DLList_Recycle_WordList ( lc->Lambda_SC_WordList ) ;
    LC_SaveStackPointer ( lc ) ;
    LC_Init_Variables ( lc ) ;
    Stack_Init ( lc->CombinatorInfoStack ) ;
    return lc ;
}

LambdaCalculus *
LC_Init_Runtime ( )
{
    LambdaCalculus * lc = _LC_ ;
    if ( ! lc ) lc = LC_New ( ) ;
    else _LC_Init_Runtime ( lc ) ;
    return lc ;
}

LambdaCalculus *
_LC_Init ( LambdaCalculus * lc )
{
    if ( lc )
    {
        _LC_ = lc ; // some functions use _LC_
        lc->LispNamespace = Namespace_Find ( ( byte* ) "Lisp" ) ;
        lc->LispDefinesNamespace = Namespace_FindOrNew_SetUsing ( ( byte* ) "LispDefines", 0, 1 ) ;
        lc->LispTempNamespace = Namespace_FindOrNew_SetUsing ( ( byte* ) "LispTemp", 0, 1 ) ;
        _LC_Init_Runtime ( lc ) ;
        LC_ClearDefinesNamespace ( ) ;
        lc->OurCSL = _CSL_ ;
        int64 svds = GetState ( _CSL_, _DEBUG_SHOW_ ) ;
        int64 svsco = IsSourceCodeOn ;
        DebugShow_Off ;
        //CSL_DbgSourceCodeOff ( ) ;
        lc->Nil = DataObject_New ( T_LC_DEFINE, 0, ( byte* ) "nil", 0, 0, T_NIL, 0, 0, 0, 0, 0, - 1 ) ;
        lc->True = DataObject_New ( T_LC_DEFINE, 0, ( byte* ) "true", 0, 0, 0, 0, ( uint64 ) true, 0, 0, 0, - 1 ) ;
        lc->buffer = Buffer_DataCleared ( lc->PrintBuffer ) ;
        lc->outBuffer = Buffer_DataCleared ( lc->OutBuffer ) ;
        SetState ( _CSL_, DEBUG_SOURCE_CODE_MODE, svsco ) ;
        SetState ( _CSL_, _DEBUG_SHOW_, svds ) ;
        DLList_Recycle_WordList ( lc->Lambda_SC_WordList ) ;
        lc->Sc_Word = 0 ;
        _LC_ = lc ;
    }
    return lc ;
}

void
_LC_Delete ( LambdaCalculus * lc )
{
    if ( lc )
    {
        LC_ClearTempNamespace ( ) ;
        LC_ClearDefinesNamespace ( ) ;
        OVT_MemListFree_LispTemp ( ) ;
        _OVT_MemListFree_LispSpace ( ) ;
    }
    _LC_ = 0 ;
}

void
CSL_LC_Delete ( )
{
    _LC_Delete ( _LC_ ) ;
}

LambdaCalculus *
_LC_Create ( )
{
    LambdaCalculus * lc = ( LambdaCalculus * ) Mem_Allocate ( sizeof (LambdaCalculus ), LISP ) ;
    lc->QuoteStateStack = Stack_New ( 256, LISP ) ; // LISP_TEMP : is recycled by OVT_FreeTempMem in _CSL_Init_SessionCore called by _CSL_Interpret
    lc->PrintBuffer = _CSL_->LC_PrintB ; // ( BUFFER_SIZE ) ;
    lc->OutBuffer = _CSL_->LC_OutB ; //Buffer_NewLocked ( BUFFER_SIZE ) ;
    lc->CombinatorInfoStack = _Stack_Allocate ( 64, LISP ) ;
    return lc ;
}

LambdaCalculus *
LC_New ( )
{
    LambdaCalculus * lc = _LC_ ;
    if ( lc ) _LC_Delete ( lc ) ; //LC_ClearDefinesNamespace ( ) ;
    lc = _LC_Create ( ) ;
    lc = _LC_Init ( lc ) ;
    _LC_ = lc ;
    return lc ;
}

void
LC_On ( )
{
    LC_New ( ) ;
    LC_LispNamespaceOn ( ) ;
}

LambdaCalculus *
LC_Reset ( )
{
    return LC_New ( ) ;
}

LambdaCalculus *
LC_Init ( )
{
    LambdaCalculus * lc ;
    if ( _LC_ ) lc = _LC_Init_Runtime ( _LC_ ) ;
    else lc = LC_New ( ) ;
    return lc ;
}

void
List_Eval ( )
{
    //LambdaCalculus * lc = LC_Init_Runtime ( ) ;
    //LC_LispNamespaceOn ( ) ;
    ListObject * l0 = ( ListObject * ) DataStack_Pop ( ), *l1 ;
    l1 = LC_Eval ( l0, 0, 1 ) ;
    DataStack_Push ( ( int64 ) l1 ) ;
}

#if 0

void
Lexer_DoReplMacro ( Lexer * lexer )
{
    //ReadLine_UnGetChar ( lexer->ReadLiner0 ) ; // let the repl re-get the char 
    Lexer_FinishTokenHere ( lexer ) ;
    LC_ReadEvalPrint ( ) ;
    SetState ( lexer, LEXER_RETURN_NULL_TOKEN, true ) ;
}

void
Lexer_CheckMacroRepl ( Lexer * lexer )
{
    byte nextChar = ReadLine_PeekNextNonWhitespaceChar ( lexer->ReadLiner0 ) ;
    if ( ( nextChar == '(' ) ) //|| ( nextChar == ',' ) )
    {
        Lexer_DoReplMacro ( lexer ) ;
    }
}
#endif

