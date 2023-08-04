
#include "../include/csl.h"

void
_CSL_Colon ( Boolean initSC )
{
    CSL_RightBracket ( ) ;
    if ( initSC ) CSL_SourceCode_Init ( ) ;
    CSL_Token ( ) ;
    CSL_Word_New ( ) ;
    CSL_BeginBlock ( ) ;

    byte c = Lexer_NextPrintChar ( _Lexer_ ) ;
    if ( ( c == '(' ) ) //&& ( ! GetState ( _Interpreter_, PREPROCESSOR_DEFINE ) ) )
    {
        Lexer_ReadToken ( _Lexer_ ) ;
        CSL_LocalsAndStackVariablesBegin ( ) ;
    }
}

void
CSL_Colon ( )
{
    _CSL_Colon ( 1 ) ;
}

Word *
CSL_WordInitFinal ( )
{
    block b = ( block ) DataStack_Pop ( ) ;
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    Word_InitFinal ( word, ( byte* ) b ) ;
    return word ;
}

void
CSL_SemiColon ( )
{
    CSL_EndBlock ( ) ;
    CSL_WordInitFinal ( ) ;
}

void
AddressToWord ( )
{
    DataStack_Push (( int64 ) Finder_FindWordFromAddress_AnyNamespace ( _Context_->Finder0, ( byte* ) DataStack_Pop ( )) ) ;
}

void
Word_Definition ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) word->Definition) ;
}
 
void
Word_Value ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) word->W_Value) ;
}

void
Word_ValueEqual ( )
{
    int64 value = DataStack_Pop ( ) ;
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    word->W_Value = value ;
}

void
Word_Xt_LValue ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) & word->Definition) ;
}

#if  0

void
Word_DefinitionStore ( )
{
    Word * word = ( Word * ) DataStack_Pop ( ) ;
    block b = ( block ) TOS ; // leave word on tos for anticipated further processing
    _Word_DefinitionStore ( word, b ) ;
}
#endif

void
Word_DefinitionEqual ( )
{
    block b = ( block ) DataStack_Pop ( ) ;
    Word * word = ( Word* ) TOS ; // leave word on tos for anticipated further processing
    _Word_DefinitionStore ( word, b ) ;
}

void
Word_CodeStart ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) word->CodeStart) ;
}

void
Word_CodeSize ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) word->S_CodeSize) ;
}

void
CSL_Word_Run ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    Word_Morphism_Run ( word ) ;
}

void
CSL_Word_Eval ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    Word_Eval ( word ) ;
}

void
Word_Finish ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    _Word_Finish ( word ) ;
}

// ?!? : nb. macros and _Word_Begin may need to be reworked

byte *
_Word_Begin ( )
{
    CSL_SourceCode_Init ( ) ;
    byte * name = Lexer_ReadToken ( _Context_->Lexer0 ) ;
    return name ;
}

void
Word_Add ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    _Word_Add ( word, 1, 0 ) ;
}

// forth : "create"

void
CSL_Word_New ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    Word * word = Word_New ( name ) ;
    CSL_WordList_Init ( word ) ; //nb! so we need to use source code debug before creating a new word
    DataStack_Push (( int64 ) word) ;
}

// ( token block -- word )
// postfix 'word' takes a token and a block

void
CSL_Word ( )
{
    block b = ( block ) DataStack_Pop ( ) ;
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    DataObject_New ( CSL_WORD, 0, name, 0, 0, 0, 0, ( int64 ) b, 0, 0, 0, - 1 ) ;
}

void
CSL_Alias ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    _CSL_Alias ( word, name, 0 ) ;
}

void
CSL_FindAlias ( )
{
    byte * wname = ( byte* ) DataStack_Pop ( ) ;
    Word * word = Finder_Word_FindUsing ( _Finder_, wname, 0 ) ;
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    _CSL_Alias ( word, name, 0 ) ;
}
#if 0

void
CSL_Eval_C_Rtl_ArgList ( ) // C : x86 : ABI = 32 : protocol : right to left arguments from a list pushed on the stack
{
    Word_CompileRun_C_ArgList ( ( Word * ) DataStack_Pop ( ) ) ;
}
#endif

//invokes ultimately _CSL_StringMacros_Do in _Lexer_ParseTerminatingMacro
// not fully implemented or utilized
void
CSL_TextMacro ( )
{
    _CSL_Macro ( TEXT_MACRO, ( byte* ) Do_TextMacro ) ;
}

//invokes ultimately _CSL_StringMacros_Do in _Lexer_ParseTerminatingMacro
// not fully implemented or utilized
void
CSL_StringMacro ( )
{
    _CSL_Macro ( STRING_MACRO, ( byte* ) Do_StringMacro ) ;
}

void
Word_Name ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) word->Name) ;
}

Location *
Location_New ( )
{
    Lexer * lexer = _Lexer_ ;
    Location * loc = ( Location * ) Mem_Allocate ( sizeof ( Location ), ( CompileMode ? OBJECT_MEM : OBJECT_MEM ) ) ;
    loc->Filename = lexer->Filename ;
    loc->LineNumber = lexer->LineNumber ;
    loc->CursorPosition = _Context_->Lexer0->CurrentReadIndex ;
    return loc ;
}

void
_Location_Printf ( Location * loc )
{
    if ( loc ) iPrintf ( "\nRun Time Location : %s %d.%d", loc->Filename, loc->LineNumber, loc->CursorPosition ) ;
}

void
CSL_Location_Printf ( )
{
    Location * loc = ( Location* ) DataStack_Pop ( ) ;
    _Location_Printf ( loc ) ;
}

void
Location_PushNew ( )
{
    Location * loc = Location_New ( ) ;
    _Compile_Stack_Push ( DSP, RAX, ( int64 ) loc ) ;
}

// related to forth does>
// do> does> <do

#if 0

void
CSL_Do ( )
{
    CSL_LeftBracket ( ) ; // interpret mode
    byte * token = Interpret_C_Until_NotIncluding_Token5 (_Interpreter_, ( byte* ) "does>", ( byte* ) "<do", ( byte* ) ";", ( byte* ) ",", 0, 0 , 0) ;
    _CSL_RightBracket ( ) ;
}

void
CSL_Does ( )
{
    Word * saveWord = _Context_->CurrentWordBeingCompiled ;
    _CSL_RightBracket ( ) ;
    Interpret_C_Until_NotIncluding_Token5 (_Interpreter_, ( byte* ) "<does", ( byte* ) "<;", ( byte* ) ";", ( byte* ) ",", 0, 0 , 0) ;
    _Context_->CurrentWordBeingCompiled = saveWord ;
}

#elif 1  

void
CSL_Do ( )
{
    CSL_LeftBracket ( ) ; // interpret mode
    byte * token = Interpret_C_Until_NotIncluding_Token5 (_Interpreter_, ( byte* ) "does>", ( byte* ) "<do", ( byte* ) ";", ( byte* ) ",", 0, 0, 0 , 0) ;
    _CSL_RightBracket ( ) ;
}

void
CSL_Does ( )
{
    Word * saveWord = _Context_->CurrentWordBeingCompiled ;
    _CSL_RightBracket ( ) ;
    CSL_BeginBlock ( ) ;
    byte * token = Interpret_C_Until_NotIncluding_Token5 (_Interpreter_, ( byte* ) "<does", ( byte* ) "<;", ( byte* ) ";", ( byte* ) ",", 0, 0, 0 , 0) ;
    CSL_EndBlock ( ) ;
    CSL_BlockRun ( ) ;
    if ( _String_EqualSingleCharString ( token, ';' ) ) { DataStack_Push ((int64)_Compiler_->Current_Word_Create) ; Word_DefinitionEqual ( ) ; } // for use with 'create - 'wordNew like ans forth ??
    _Context_->CurrentWordBeingCompiled = saveWord ;
}

#else

void
CSL_Do ( )
{
    CSL_LeftBracket ( ) ; // interpret mode
    byte * token = Interpret_C_Until_NotIncluding_Token5 (_Interpreter_, ( byte* ) "does>", ( byte* ) "<do", ( byte* ) ";", ( byte* ) ",", 0, 0 , 0) ;
    _CSL_RightBracket ( ) ;
}

void
CSL_Does ( )
{
    Word * saveWord = _Context_->CurrentWordBeingCompiled ;
    _CSL_RightBracket ( ) ;
    CSL_BeginBlock ( ) ;
    Interpret_C_Until_NotIncluding_Token5 (_Interpreter_, ( byte* ) "<does", ( byte* ) "<;", ( byte* ) ";", ( byte* ) ",", 0, 0 , 0) ;
    CSL_EndBlock ( ) ;
    CSL_BlockRun ( ) ;
    _Context_->CurrentWordBeingCompiled = saveWord ;
}

#endif

void
Word_Namespace ( )
{
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (( int64 ) _Word_Namespace ( word )) ;
}

void
CSL_Keyword ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word ) word->W_MorphismAttributes |= KEYWORD ;
}

void
CSL_Immediate ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word ) word->W_MorphismAttributes |= IMMEDIATE ;
}

void
CSL_ImmediateOff ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word ) word->W_MorphismAttributes &= ~IMMEDIATE ;
}

void
CSL_Syntactic ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word ) word->W_ObjectAttributes |= SYNTACTIC ;
}

void
CSL_IsImmediate ( void )
{
#if 0    
    Word * word = ( Word* ) TOS ;
    TOS = ( word->CAttribute & IMMEDIATE ) ;
#else
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    DataStack_Push (word->W_MorphismAttributes & IMMEDIATE) ;
#endif    
}

void
CSL_Inline ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word ) word->W_MorphismAttributes |= INLINE ;
}

void
CSL_Set_TypeSignature ( void )
{
    byte * typeSignature = ( byte* ) DataStack_Pop ( ) ;
    Word * word = _CSL_->LastFinished_Word ; // nb! must be LastFinished_Word because the signature is a literal and it is the LastFinished_DObject
    if ( word ) Strcpy ( word->W_TypeSignatureString, typeSignature ) ;
}

void
CSL_Prefix ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        word->W_MorphismAttributes |= PREFIX ;
        word->W_TypeAttributes |= (WT_PREFIX) ; //|WT_PREFIXABLE) ;
    }
}

void
CSL_Prefixable ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        //word->W_MorphismAttributes |= PREFIX ;
        word->W_TypeAttributes |= WT_PREFIXABLE ;
    }
}

void
CSL_NPrefix ( void )
{
    uint64 numberOfPrefixedArgs = DataStack_Pop ( ) ;
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        word->W_MorphismAttributes |= PREFIX ;
        word->W_TypeAttributes |= WT_PREFIX ;
        word->W_NumberOfPrefixedArgs = numberOfPrefixedArgs ;
    }
}

void
CSL_C_Prefix ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        word->W_MorphismAttributes |= C_PREFIX | C_PREFIX_RTL_ARGS ;
        word->W_TypeAttributes |= WT_C_PREFIX_RTL_ARGS ;
    }
}

#if 0

void
CSL_C_Return ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        word->W_MorphismAttributes |= C_RETURN | C_PREFIX_RTL_ARGS ;
        word->T_WordAttributes |= WT_C_PREFIX_RTL_ARGS ;
    }
}
#endif

void
CSL_Void_Return ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        //word->W_MorphismAttributes &= ~ C_RETURN ;
        word->W_MorphismAttributes |= VOID_RETURN ;
        if ( GetState ( _Context_, C_SYNTAX ) )
        {
            word->W_MorphismAttributes |= C_PREFIX_RTL_ARGS ;
            word->W_TypeAttributes |= WT_C_PREFIX_RTL_ARGS ;
        }
    }
}

void
CSL_RAX_Return ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word )
    {
        //word->W_MorphismAttributes &= ~ C_RETURN ;
        word->W_MorphismAttributes |= RAX_RETURN ;
    }
}

void
CSL_DebugWord ( void )
{
    Word * word = _CSL_->LastFinished_Word ;
    if ( word ) word->W_MorphismAttributes |= DEBUG_WORD ;
}

void
Symbol_Print ( Symbol * symbol )
{
    iPrintf ( "%s", symbol->Name ) ;
}

void
Symbol_List_Print ( dllist * list )
{
    iPrintf ( "\nSymbol List : " ) ;
    dllist_Map ( list, ( MapFunction0 ) Symbol_Print ) ;
}

void
_PrintWord ( dlnode * node, int64 * n )
{
    Word * word = ( Word * ) node ;
    _Word_Print ( word ) ;
    ( *n ) ++ ;
}

void
_Words ( Symbol * symbol, MapFunction1 mf, int64 n )
{
    Namespace * ns = ( Namespace * ) symbol ;
    iPrintf ( "\n - %s :> ", ns->Name ) ;
    dllist_Map1 ( ns->Lo_List, mf, n ) ;
}

void
_DoWords ( Symbol * symbol, int64 * n )
{
    _Words ( symbol, ( MapFunction1 ) _PrintWord, ( int64 ) n ) ;
}

int64
_CSL_PrintWords ( int64 state )
{
    int64 n = 0 ;
    _CSL_NamespacesMap ( ( MapSymbolFunction2 ) _DoWords, state, ( int64 ) & n, 0 ) ;
    if ( Verbosity () > 3 ) iPrintf ( "\nCSL : WordsAdded = %d : WordMaxCount = %d", _CSL_->WordsAdded, _CSL_->FindWordMaxCount ) ;
    return n ;
}

void
CSL_Words ( )
{
    iPrintf ( "\nWords :\n - <namespace> ':>' <word list>" ) ;
    int64 n = _CSL_PrintWords ( USING ) ;
    iPrintf ( "\n" INT_FRMT " words on the 'using' Namespaces List ::", n ) ;
    if ( Verbosity () > 3 ) iPrintf ( "\nCSL : WordsAdded = %d : WordMaxCount = %d", _CSL_->WordsAdded, _CSL_->FindWordMaxCount ) ;
}

void
_Variable_Print ( Word * word )
{
    iPrintf ( c_ud ( " %s = %x ;" ), word->Name, word->W_Value ) ;
}

void
_PrintVariable ( dlnode * node, int64 * n )
{
    Word * word = ( Word * ) node ;
    if ( word->W_ObjectAttributes & NAMESPACE_VARIABLE )
    {
        _Variable_Print ( word ) ;
        ( *n ) ++ ;
    }
}

void
_Variables ( Symbol * symbol, MapFunction1 mf, int64 n )
{
    int64 pre_n = * ( int64* ) n ;
    Namespace * ns = ( Namespace * ) symbol ;
    iPrintf ( "\n - %s :> ", ns->Name ) ;
    dllist_Map1 ( ns->Lo_List, mf, n ) ;
    if ( *( int64* ) n == pre_n ) iPrintf ( "\r" ) ;
}

void
_PrintVariables ( Symbol * symbol, int64 * n )
{
    _Variables ( symbol, ( MapFunction1 ) _PrintVariable, ( int64 ) n ) ;
}

int64
_CSL_PrintVariables ( int64 nsStatus )
{
    int64 n = 0 ;
    _CSL_NamespacesMap ( ( MapSymbolFunction2 ) _PrintVariables, nsStatus, ( int64 ) & n, 0 ) ;
    return n ;
}

void
CSL_Variables ( )
{
    iPrintf ( "\nGlobal Variables :\n - <namespace> ':>' <variable '=' value ';'>*" ) ;
    int64 n = _CSL_PrintVariables ( USING ) ;
    iPrintf ( "\n" INT_FRMT " global variables on the 'using' Namespaces List", n ) ;
}

void
_CSL_NamespaceWords ( )
{
    int64 n = 0 ;
    Namespace * ns = ( Namespace * ) DataStack_Pop ( ) ;
    if ( ns )
    {
        _DoWords ( ( Symbol * ) ns, &n ) ;
        iPrintf ( "\n" INT_FRMT " words in %s namespace", n, ns->Name ) ;
    }
    else iPrintf ( "\nError : can't find that namespace" ) ;
}

void
CSL_NamespaceWords ( )
{
    byte * name = ( byte * ) DataStack_Pop ( ) ;
    Namespace * ns = _Namespace_Find ( name, 0, 0 ) ;
    DataStack_Push (( int64 ) ns) ;
    _CSL_NamespaceWords ( ) ;
}

void
CSL_AllWords ( )
{
    iPrintf ( "\n - <namespace> ':>' <word list>" ) ;
    iPrintf ( "\n'using' Namespaces List ::" ) ;
    int64 n = _CSL_PrintWords ( USING ) ;
    iPrintf ( "\n" INT_FRMT " words on the Currently 'using' Namespaces List", n ) ;
    iPrintf ( "\n'notUsing' Namespaces List ::" ) ;
    int64 usingWords = _CSL_->FindWordCount ;
    int64 m = _CSL_PrintWords ( NOT_USING ) ;
    iPrintf ( "\n" INT_FRMT " words on the 'notUsing' List", m ) ;
    iPrintf ( "\n" INT_FRMT " total words", n + m ) ;
    int64 notUsingWords = _CSL_->FindWordCount ;
    _CSL_->FindWordCount = usingWords + notUsingWords ;
    CSL_WordAccounting ( ( byte* ) "CSL_AllWords" ) ;
}

