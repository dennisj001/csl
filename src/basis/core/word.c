#include "../../include/csl.h"

inline void
Context_PreWordRun_Init ( Context * cntx, Word * word )
{
    if ( word )
    {
        word->StackPushRegisterCode = 0 ; // nb. used! by the rewriting optInfo
        // keep track in the word itself where the machine code is to go, if this word is compiled or causes compiling code - used for optimization
        if ( ( GetState ( cntx->Compiler0, ( COMPILE_MODE | ASM_MODE ) ) ) ) Word_SetCodingAndSourceCoding ( word, Here ) ; // if we change it later (eg. in lambda calculus) we must change it there because the rest of the compiler depends on this
        cntx->CurrentlyRunningWord = word ;
        if ( word->W_MorphismAttributes & COMBINATOR ) _Context_->CurrentCombinator = word ; //_Compiler_->Combinator = word ;
        OVT_ResetOutputPrintBuffer ( ) ;
    }
}

inline void
Context_PostWordRun_Init ( Context * cntx, Word * word )
{
    cntx->LastRanWord = word ;
    cntx->CurrentlyRunningWord = 0 ;
}

void
Word_Morphism_Run ( Word * word )
{
    if ( word )
    {
        Context_PreWordRun_Init ( _Context_, word ) ;
        _O_->DebugOutputFlag &= ~ 1 ;
        if ( GetState ( _LC_, LC_DEBUG_ON ) || Is_DebugModeOn ) DEBUG_SETUP ( word, 0 ) ;
        Block_Eval ( word->Definition ) ;
        Context_PostWordRun_Init ( _Context_, word ) ;
    }
}

void
_Word_Eval ( Word * word )
{
    if ( word )
    {
        _Context_->CurrentEvalWord = word ;
        if ( IS_MORPHISM_TYPE ( word ) ) CSL_Typecheck ( word ) ;
        //if ( ! _CSL_->DebugLevel ) 
        DEBUG_SETUP ( word, 0 ) ;
        if ( ( word->W_MorphismAttributes & IMMEDIATE ) || ( ! CompileMode ) )
        {
            SetState ( _Context_, RUN_MODE, true ) ;
            Word_Morphism_Run ( word ) ;
            SetState ( _Context_, RUN_MODE, false ) ;
        }
        else _Word_Compile ( word ) ;
        DEBUG_SHOW ( word, 0, 0 ) ;
        _Context_->CurrentEvalWord = 0 ;
        _Context_->LastEvalWord = word ;
    }
}

void
Word_Eval ( Word * word )
{
    if ( word )
    {
        if ( ! sigsetjmp ( _Context_->JmpBuf0, 0 ) ) // siglongjmp from _Debugger_InterpreterLoop
        {
            if ( ! ( GetState ( word, W_STEPPED ) ) ) _Word_Eval ( word ) ;
        }
        else AdjustR14WithDsp ( ) ;
        SetState ( word, W_STEPPED, false ) ;
    }
}

void
Word_DbgBlock_Eval ( Word * word, block blck )
{
    if ( blck )
    {
        Context_PreWordRun_Init ( _Context_, word ) ;
        _Debug_Setup ( word, 0, ( byte* ) blck, 0, 1 ) ;
        if ( ! GetState ( _Debugger_->w_Word, W_STEPPED ) )
        {
            _Block_Eval ( blck ) ;
            DEBUG_SHOW ( word, 0, 1 ) ;
        }
        SetState ( _Debugger_->w_Word, W_STEPPED, false ) ;
        Context_PostWordRun_Init ( _Context_, word ) ;
    }
}

void
_Word_Interpret ( Word * word )
{
    Interpreter_DoWord ( _Interpreter_, word, word->W_RL_Index, word->W_SC_Index ) ;
}

void
_Word_Compile ( Word * word )
{
    Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    if ( ! word->Definition ) CSL_SetupRecursiveCall ( ) ;
    else if ( ( GetState ( _CSL_, INLINE_ON ) ) && ( word->W_MorphismAttributes & INLINE ) && ( word->S_CodeSize ) ) _Compile_WordInline ( word ) ;
    else Compile_CallWord_Check_X84_ABI_RSP_ADJUST ( word ) ;
    word->W_MySourceCodeWord = _Context_->CurrentWordBeingCompiled ;
}

Namespace *
_Word_Namespace ( Word * word )
{
    if ( word->W_ObjectAttributes & NAMESPACE ) return ( Namespace * ) word ;
    else return word->ContainingNamespace ;
}

// deep copy from word0 to word

void
_Word_Copy ( Word * word, Word * word0 )
{
    //WordData * swdata = word->W_WordData ;
    MemCpy ( word, word0, sizeof ( Word ) + sizeof ( WordData ) ) ;
    //word->W_WordData = swdata ; // restore the WordData pointer we overwrote by the above MemCpy
    word->W_WordData = ( WordData* ) ( word + 1 ) ; //swdata ; // restore the WordData pointer we overwrote by the above MemCpy
}

Word *
Word_Copy ( Word * word0, uint64 allocType )
{
    Word * word = _Word_Allocate ( allocType ) ;
    _Word_Copy ( word, word0 ) ;
    word->W_AllocType = allocType ;
    return word ;
}

void
_Word_Finish ( Word * word )
{
    DObject_Finish ( word ) ;
    //_CSL_FinishWordDebugInfo ( word ) ;
    CSL_Finish_WordSourceCode ( _CSL_, word, 0 ) ;
    CSL_TypeStackReset ( ) ;
    _CSL_->LastFinished_Word = word ;
    CSL_AfterWordReset ( ) ;
    //if ( Is_DebugOn ) Word_Show_NamespaceStackWords ( word ) ;
    //Compiler_Init ( _Compiler_, 0 ) ;
    //Compiler_FreeLocalsNamespaces ( _Compiler_ ) ;
}

void
_Word_DefinitionStore ( Word * word, block code )
{
    _DObject_ValueDefinition_Init ( word, ( int64 ) code, BLOCK, 0, 0 ) ;
}

void
Word_InitFinal ( Word * word, byte * code )
{
    _Word_DefinitionStore ( word, ( block ) code ) ;
    if ( ! word->S_ContainingNamespace ) _Word_Add ( word, 1, 0 ) ; // don't re-add if it is a recursive word cf. CSL_BeginRecursiveWord
    _Word_Finish ( word ) ;
}

void
_Word_Add ( Word * word, int64 addToInNs, Namespace * addToNs )
{
    Namespace * ins = 0, *ns ;
    if ( word->S_ContainingNamespace )
    {
        if ( Verbosity ( ) > 1 ) iPrintf ( "\n_Word_Add at %s : Can't add existing word = %s from namespace %s to a new namespace??\n so we are making a copy",
            Context_Location ( ), word->Name, word->S_ContainingNamespace->Name ) ;
        dlnode_Remove ( ( dlnode * ) word ) ;
    }
    if ( addToNs ) Namespace_DoAddWord ( addToNs, word ) ;
    else if ( addToInNs )
    {
        if ( ! ( word->W_ObjectAttributes & ( LITERAL ) ) )
        {
            Namespace * ins = CSL_In_Namespace ( ) ; //_CSL_Namespace_InNamespaceGet ( ) ;
            Namespace_DoAddWord ( ins, word ) ;
        }
    }
    if ( Verbosity ( ) > 3 )
    {
        ns = addToNs ? addToNs : ins ;
        if ( ns )
        {
            if ( word->W_MorphismAttributes & BLOCK ) iPrintf ( "\nnew Word :: %s.%s", ns->Name, word->Name ) ;
            else iPrintf ( "\nnew DObject :: %s.%s", ns->Name, word->Name ) ;
        }
    }

    if ( ( Is_DbiOn || Is_DebugOn ) && ( ( addToInNs || addToNs ) ) ) //&& word->S_ContainingNamespace ) )//&& String_Equal ("bt", word->Name) )
    {
        //_Printf ( c_ad ( "\n_Word_Add : %s.%s = %lx : at %s\n" ),
        _Printf ( "\n_Word_Add : %s.%s = %lx : at %s",
            ( word->S_ContainingNamespace ? word->S_ContainingNamespace->Name : ( byte* ) "" ), word->Name, word, Context_Location ( ) ) ;
    }
}

Word *
_Word_Allocate ( uint64 allocType )
{
    Word * word = 0 ;
    int64 size = ( sizeof ( Word ) + sizeof ( WordData ) ) ;
    word = ( Word* ) DLList_CheckRecycledForAllocation ( _O_->RecycledWordList, size ) ;
    if ( word ) OVT_RecyclingAccounting ( OVT_RA_RECYCLED ) ; //{ _O_->MemorySpace0->RecycledWordCount ++ ; _O_->MemorySpace0->WordsInRecycling -- ; }
    else
    {
        word = ( Word* ) Mem_Allocate ( size, allocType ) ;
        word->S_WAllocType = allocType ;
        _O_->MemorySpace0->WordsAllocated ++ ;
    }
    ( ( DLNode* ) word )->n_DLNode.n_Size = size ;
    word->W_WordData = ( WordData * ) ( word + 1 ) ; // nb. "pointer arithmetic"
    return word ;
}

Word *
_Word_Create ( byte * name, uint64 morphismType, uint64 objectType, uint64 lispType, uint64 allocType )
{
    Word * word = _Word_Allocate ( allocType ? allocType : DICTIONARY ) ;
    if ( allocType & ( EXISTING ) ) _Symbol_NameInit ( ( Symbol * ) word, name ) ;
        //if ( objectType & ( LITERAL | CONSTANT ) ) _Symbol_NameInit ( ( Symbol * ) word, name ) ;
    else _Symbol_Init_AllocName ( ( Symbol* ) word, name, STRING_MEM ) ;
    word->W_AllocType = allocType ;
    word->W_MorphismAttributes = morphismType ;
    word->W_ObjectAttributes = objectType ;
    word->W_LispAttributes = lispType ;
    if ( Is_NamespaceType ( word ) ) word->Lo_List = _dllist_New ( allocType ) ;
    _Compiler_->Current_Word_Create = word ;
    _CSL_->WordCreateCount ++ ;
    Lexer_Set_ScIndex_RlIndex ( _Lexer_, word, - 1, - 1 ) ; // default values
    Word_SetCodingAndSourceCoding ( word, Here ) ;
    return word ;
}

void
Word_SetLocation ( Word * word )
{
    ReadLiner * rl = _Context_->ReadLiner0 ;
    if ( rl->InputStringOriginal )
    {
        word->W_WordData->Filename = rl->Filename ;
        word->W_WordData->LineNumber = rl->LineNumber ;
        word->W_TokenStart_LineIndex = _Lexer_->TokenStart_ReadLineIndex ;
    }
}

Word *
_Word_New ( byte * name, uint64 morphismType, uint64 objectType, uint64 lispType, Boolean addToInNs, Namespace * addToNs, uint64 allocType )
{
#if 0 // debug    
    if ( String_Equal ( name, "Identifier" ) )
        iPrintf ( "" ) ;
#endif    
    Word * word = _Word_Create ( name, morphismType, objectType, lispType, allocType ) ; // csl_WORD : csl compiled words as opposed to C compiled words
    _Compiler_->Current_Word_New = word ;
    Word_SetLocation ( word ) ;
    _Word_Add ( word, addToInNs, addToNs ) ; // add to the head of the list
    //if ( Is_DebugOn ) iPrintf ( "\n_Word_New : %s added to %s at %s", word->Name, 
    //    word->S_ContainingNamespace ? word->S_ContainingNamespace->Name : (byte*) "", Context_Location ()) ;
    word->TypeNamespaceName = "Word" ;
    return word ;
}

Word *
Word_New ( byte * name )
{
    Word * word = _Word_New ( name, CSL_WORD | WORD_CREATE, 0, 0, 1, 0, DICTIONARY ) ;
    return word ;
}

void
Word_PrintOffset ( Word * word, int64 offset, int64 totalOffset )
{
    if ( word )
    {
        Context * cntx = _Context_ ;
        if ( Is_DebugModeOn ) NoticeColors ;
        byte * name = String_ConvertToBackSlash ( word->Name, 0 ) ;
        Word * baseObject = cntx->BaseObject ? cntx->BaseObject : ( word->W_BaseObject ? word->W_BaseObject : 0 ) ;
        if ( _String_EqualSingleCharString ( name, ']' ) && cntx->BaseObject )
        {
            iPrintf ( "\n\'%s\' = array end :: base object \'%s\' = 0x%lx : offset = 0x%lx : total offset = 0x%lx => address = 0x%lx",
                name, baseObject->Name, baseObject->W_Value, offset, totalOffset, baseObject->W_Value + totalOffset ) ;
        }
        else
        {
            byte * typeName ;
            if ( word->W_ObjectAttributes & O_POINTER )
            {
                byte * buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ;
                snprintf ( buffer, BUFFER_SIZE, "* %s", word->TypeNamespace ? word->TypeNamespace->Name : ( byte* ) "" ) ;
                typeName = buffer ;
            }
            else typeName = word->TypeNamespace ? word->TypeNamespace->Name : ( byte* ) "" ;
            totalOffset = cntx->Compiler0->AccumulatedOptimizeOffsetPointer ? *cntx->Compiler0->AccumulatedOptimizeOffsetPointer : - 1 ;
            iPrintf (
                "\n\'%s\' = object field :: type = %s : size = %ld : base object = \'%s\' : address = 0x%lx : field offset = 0x%lx : total offset  = 0x%lx : :-> address = 0x%lx",
                name, typeName,
                word->CompiledDataFieldByteSize,
                baseObject ? String_ConvertToBackSlash ( baseObject->Name, 0 ) : ( byte* ) "",
                baseObject ? baseObject->W_Value : 0, //(baseObject && ( ! C_SyntaxOn ) ) ? baseObject->W_Value : baseObject,
                word->WD_Offset, totalOffset, baseObject ? ( baseObject->W_Value + totalOffset ) : ( offset + totalOffset ) ) ; //( byte* ) - 1 ) ;
        }
        if ( Is_DebugModeOn ) DefaultColors ;
    }
}

byte *
_Word_SourceCodeLocation_pbyte ( Word * word )
{
    byte * b = Buffer_DataCleared ( _CSL_->ScratchB2 ) ;
    if ( word ) sprintf ( ( char* ) b, "%s.%s : %s %ld.%ld", word->ContainingNamespace->Name, word->Name, word->W_WordData->Filename,
        word->W_WordData->LineNumber, word->W_TokenStart_LineIndex ) ;
    return String_New ( b, TEMPORARY ) ;
}

void
Word_PrintName ( Word * word )
{
    if ( word ) iPrintf ( "%s ", word->Name ) ;
}

byte*
Word_Info ( Word * word )
{
    byte * buffer = 0 ;
    if ( word ) //&& word->ContainingNamespace )
    {
        buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ;
        sprintf ( ( char * ) buffer, "%s.%s", ( char* ) word->ContainingNamespace ? ( char* ) word->ContainingNamespace->Name : "<literal>", ( char * ) word->Name ) ;
    }
    return buffer ;
}

void
Word_Print ( Word * word, Word * scWord, Boolean flag )
{
    if ( word )
    {
        if ( flag ) _Debugger_Locals_ShowALocal ( _Debugger_->cs_Cpu, word, scWord ) ;
        else iPrintf ( "\n%s", Word_Info ( word ) ) ;
    }
}

void
_Word_Print ( Word * word )
{
    _Context_->WordCount ++ ;
    if ( _O_->Dbi )
    {
        iPrintf ( c_ud ( " %s" ), ( word->Name && word->Name[0] ) ? c_ud ( word->Name ) : ( byte* ) "<unnamed>" ) ;
        iPrintf ( c_gd ( " = %lx," ), word ) ;
    }
    else iPrintf ( c_ud ( " %s" ), word->Name ) ;
}

void
Word_N_M_Node_Print ( dlnode * node )
{
    Word * word = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) node, SCN_T_WORD ) ;
    _Word_Print ( word ) ;
}

void
_Word_ShowSourceCode ( Word * word0 )
{
    Word * word = Word_UnAlias ( word0 ) ;
    if ( word && word->W_WordData ) //&& word->W_OriginalCodeText ) //word->CAttribute & ( CPRIMITIVE | BLOCK ) )
    {
        byte * name, *scd ;
        if ( ( ! ( word->W_MorphismAttributes & CPRIMITIVE ) ) && word->W_OriginalCodeText )
        {
            byte * sc = Buffer_DataCleared ( _CSL_->ScratchB1 ) ;
            sc = _String_ConvertStringToBackSlash ( sc, word->W_OriginalCodeText, BUFFER_IX_SIZE, 0 ) ;
            scd = c_gd ( String_FilterMultipleSpaces ( sc, TEMPORARY ) ) ;
        }
        else scd = ( byte* ) "C Primitive" ;
        name = c_gd ( word->Name ) ;
        if ( Is_DebugOn && GetState ( _Debugger_, DBG_STEPPING ) && ( ! GetState ( _Debugger_, DBG_UDIS ) ) ) return ;
        iPrintf ( "\nSourceCode for %s.%s :> \n%s",
            word->S_ContainingNamespace ? word->S_ContainingNamespace->Name : ( byte* ) "", name, scd ) ;
    }
}

byte *
Word_GetLocalsSourceCodeString ( Word * word, byte * buffer )
{
    byte * start, * sc = word->W_OriginalCodeText ;
    int64 s, e ;
    // find and reconstruct locals source code in a buffer and parse it with the regular locals parse code
    for ( s = 0 ; sc [ s ] && sc [ s ] != '(' ; s ++ ) ;
    if ( sc [ s ] )
    {
        start = & sc [ s + 1 ] ; // right after '(' is how _CSL_Parse_LocalsAndStackVariables is set up
        for ( e = s ; sc [ e ] && sc [ e ] != ')' ; e ++ ) ; // end = & sc [ e ] ;
        if ( sc [ e ] )
        {
            Strncpy ( buffer, start, e - s + 1 ) ;
            buffer [ e - s + 1 ] = 0 ;
        }
    }
    return buffer ;
}

void
Word_ShowSourceCode ( Word * word )
{
    _CSL_Source ( word, 0 ) ;
}

Word *
Word_GetFromCodeAddress ( byte * address )
{
    Word * word = 0 ;
    if ( _Context_->Finder0 ) word = Finder_FindWordFromAddress_AnyNamespace ( _Context_->Finder0, address ) ;
    return word ;
}

Word *
Word_GetFromCodeAddress_NoAlias ( byte * address )
{
    return Finder_FindWordFromAddress_AnyNamespace_NoAlias ( _Context_->Finder0, address ) ;
}

void
_CSL_WordName_Run ( byte * name )
{
    Block_Eval ( Finder_Word_FindUsing ( _Context_->Finder0, name, 0 )->Definition ) ;
}

// alias : postfix

Word *
_CSL_Alias ( Word * word, byte * name, Namespace * addToNs )
{
    Word * alias = 0 ;
    if ( word && word->Definition )
    {
        alias = _Word_New ( name, word->W_MorphismAttributes | ALIAS, word->W_ObjectAttributes, word->W_LispAttributes, 1, addToNs, DICTIONARY ) ; // inherit type from original word
        word = Word_UnAlias ( word ) ;
        Word_InitFinal ( alias, ( byte* ) word->Definition ) ;
        alias->S_CodeSize = word->S_CodeSize ;
        alias->W_AliasOf = word ;
        alias->CompiledDataFieldByteSize = word->CompiledDataFieldByteSize ;
        alias->NamespaceStack = word->NamespaceStack ;
        Strncpy ( alias->W_TypeSignatureString, word->W_TypeSignatureString, 8 ) ;
        alias->W_SC_WordList = word->W_SC_WordList ;
    }
    else Exception ( USEAGE_ERROR, ABORT ) ;
    return alias ;
}

void
Do_TextMacro ( )
{
    Interpreter * interp = _Context_->Interpreter0 ;
    ReadLiner * rl = _Context_->ReadLiner0 ;
    ReadLiner_InsertTextMacro ( rl, interp->w_Word ) ;
    SetState ( interp, END_OF_LINE | END_OF_FILE | END_OF_STRING | DONE, false ) ; // reset a possible read newline
}

void
Do_StringMacro ( )
{
    Interpreter * interp = _Context_->Interpreter0 ;
    ReadLiner * rl = _Context_->ReadLiner0 ;
    String_InsertStringIntoStringSlot ( rl->InputLine, rl->ReadIndex, rl->ReadIndex, _String_UnBox ( ( byte* ) interp->w_Word->W_Value ), 0 ) ; // size in bytes
    SetState ( interp, END_OF_LINE | END_OF_FILE | END_OF_STRING | DONE, false ) ; // reset a possible read newline
}

void
_CSL_Macro ( int64 mtype, byte * function )
{
    byte * name = _Word_Begin ( ), *macroString ;
    macroString = Parse_Macro ( mtype ) ;
    byte * mstr = String_New ( macroString, STRING_MEM ) ;
    //_DObject_New ( byte * name, uint64 value, uint64 ctype, uint64 ltype, uint64 ftype, byte * function, int64 arg, int64 addToInNs, Namespace * addToNs, uint64 allocType )
    _DObject_New ( name, ( uint64 ) mstr, IMMEDIATE, mtype, 0, 0, function, 0, 1, 0, DICTIONARY ) ;
}

Word *
Word_GetOriginalWord ( Word * word )
{
    Word * ow1, *ow0 ;
    for ( ow0 = word, ow1 = ow0->WL_OriginalWord ; ow1 && ( ow1 != ow1->WL_OriginalWord ) ; ow0 = ow1, ow1 = ow0->WL_OriginalWord ) ;
    if ( ! ow0 ) ow0 = word ;
    return ow0 ;
}

Word *
Word_UnAlias ( Word * word )
{
    if ( word ) while ( word->W_AliasOf && ( word != word->W_AliasOf ) ) word = word->W_AliasOf ;
    return word ;
}

void
Lexer_Set_ScIndex_RlIndex ( Lexer * lexer, Word * word, int64 tsrli, int64 scwi )
{
    if ( word )
    {
        word->W_RL_Index = ( ( tsrli == - 1 ) ? lexer->TokenStart_ReadLineIndex : tsrli ) ;
        word->W_SC_Index = ( ( scwi == - 1 ) ? lexer->SC_Index : scwi ) ;
    }
}

void
Word_SetTsrliScwi ( Word * word, int64 tsrli, int64 scwi )
{
    Lexer_Set_ScIndex_RlIndex ( _Lexer_, word, tsrli, scwi ) ;
}

void
_Word_Show_NamespaceStackWords ( Word * scWord, int64 flag )
{
    int64 n, i ;
    Stack * stack = scWord->NamespaceStack ? scWord->NamespaceStack : _Compiler_->LocalsCompilingNamespacesStack ;
    for ( i = 0, n = Stack_Depth ( stack ) ; i < n ; i ++ )
    {
        Namespace * ns = ( Namespace* ) _Stack_N ( stack, i ) ;
        _Namespace_PrintWords ( ns, scWord, flag ) ;
    }
}

void
Word_Show_NamespaceStackWords ( Word * word )
{
    _Word_Show_NamespaceStackWords ( word, 0 ) ;
}

Boolean
Word_IsSyntactic ( Word * word )
{
#if 0 // huh?    
    if ( ( ! GetState ( _Debugger_, DBG_INFIX_PREFIX ) ) //&& ( ! ( word->W_ObjectAttributes & DEFINES_MACRO ) )
        && ( ( word->W_TypeAttributes & ( WT_PREFIX | WT_C_PREFIX_RTL_ARGS ) ) || ( Lexer_IsWordPrefixing ( _Lexer_, word )
        || ( ( word->W_TypeAttributes == WT_INFIXABLE ) && ( GetState ( _Context_, INFIX_MODE ) ) ) ) ) )
    {
        return true ;
    }

    else
#endif        
        return false ;
}

