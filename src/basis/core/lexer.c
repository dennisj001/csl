#include "../../include/csl.h"
// lexer.c has been strongly influenced by the ideas in the lisp reader algorithm 
// "http://www.ai.mit.edu/projects/iiip/doc/CommonLISP/HyperSpec/Body/sec_2-2.html"
// although it doesn't fully conform to them yet the intention is to be eventually somewhat of a superset of them

/*
//    !, ", #, $, %, &, ', (, ), *, +, ,, -, ., /, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, :, ;, <, =, >, ?,
// @, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, N_0, [, \, ], ^, _,
// `, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, {, |, }, ~,
 */


void
Lexer_Exception ( byte * token, uint64 exceptionNumber, byte * message )
{
    Exception *e = _O_->OVT_Exception ;
    e->ExceptionToken = token ;
    byte *buffer = Buffer_DataCleared ( _CSL_->ScratchB1 ) ;
    sprintf ( ( char* ) buffer, "%s :: %s ?\n", ( char* ) message, ( char* ) token ) ;
    CSL_Exception ( exceptionNumber, buffer, QUIT ) ;
}

Word *
_Lexer_ParseToken_ToWord ( Lexer * lexer, byte * token, int64 tsrli, int64 scwi )
{
    Context * cntx = _Context_ ;
    Word * word = 0 ;
    Lexer_ParseObject ( lexer, token ) ;
#if 1 // this is still problematic??
    //if ( ( _O_->Verbosity > 1 )&&( lexer->L_ObjectAttributes & ( T_RAW_STRING ) ) )
    if ( ( lexer->L_ObjectAttributes & ( T_RAW_STRING ) ) && ( ! C_SyntaxOn ) && (! GetState ( _Interpreter_, PREPROCESSOR_MODE ) ) )
    {
        if ( GetState ( _O_, OVT_UNKNOWN_STRING_IS_ERROR ) )
        {
            Error ( "\nUnknown String Error", QUIT ) ;
        }
        else if ( ( _O_->Verbosity > 1 ) && GetState ( _O_, OVT_UNKNOWN_STRING_PUSHED ) ) // default - not needing to be set
        {
            iPrintf ( "\nNotice : \'%s\' at %s not a known word. Pushing it as string <literal> onto the stack. [ukspOn]", token, Context_Location ( ) ) ;
        }
    }
#endif    
    lexer->ParsedToken = token ;
    word = DataObject_New ( LITERAL, 0, token, lexer->L_MorphismAttributes, lexer->L_ObjectAttributes, 0, 0, lexer->Literal, 0, 0, tsrli, scwi ) ;
    if ( word )
    {
        Word_SetTypeNamespace ( word, lexer->L_ObjectAttributes ) ;
        word->CompiledDataFieldByteSize = lexer->Token_CompiledDataFieldByteSize ;
    }
    return word ;
}

Word *
Lexer_ParseToken_ToWord ( Lexer * lexer, byte * token, int64 tsrli, int64 scwi )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Word * word = 0 ;
    Namespace * locals = compiler->LocalsNamespace ;
    if ( token )
    {
        if ( locals ) word = _Finder_FindWord_InOneNamespace ( _Finder_, locals, token ) ;
        if ( ! word ) word = Finder_Word_FindUsing ( cntx->Finder0, token, 0 ) ; // maybe need to respect a possible qualifying namespace ??
        if ( word ) _Lexer_->L_ObjectAttributes = ( KNOWN_OBJECT | CSL_WORD ) ; //SetState ( lexer, KNOWN_OBJECT, true ) ;
        else
        {
            word = _Lexer_ParseToken_ToWord ( lexer, token, tsrli, scwi ) ; //- 1, - 1 ) ;
        }
        lexer->TokenWord = word ;
    }
    return word ;
}

// csl preprocessor - string (macros) defines only

int64
Lexer_StringMacros_Do ( Lexer * lexer )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    if ( GetState ( _CSL_, DEFINES_MACROS_ON ) && ( ! GetState ( _Interpreter_, PREPROCESSOR_DEFINE ) ) )
    {
        if ( StringMacros_Do ( rl->InputLine, "Defines", lexer->TokenBuffer, lexer->TokenStart_ReadLineIndex, rl->ReadIndex ) )
        {
            rl->ReadIndex = lexer->TokenStart_ReadLineIndex ;
            return true ;
        }
    }
    return false ;
}

byte *
_Lexer_LexNextToken_WithDelimiters ( Lexer * lexer, byte * delimiters, Boolean checkListFlag, Boolean peekFlag, int reAddPeeked, uint64 state )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    byte inChar ;
    if ( ( ! checkListFlag ) || ( ! ( lexer->OriginalToken = Lexer_GetTokenNameFromTokenList ( lexer, peekFlag ) ) ) ) // ( ! checkListFlag ) : allows us to peek multiple tokens ahead if we     {
    {
start:
        Lexer_Init ( lexer, delimiters, lexer->State, CONTEXT ) ; // preserve state across init ??
        lexer->State |= state ;
        lexer->SC_Index = _CSL_->SC_Index ;
        while ( ( ! Lexer_CheckIfDone ( lexer, LEXER_DONE ) ) )
        {
            inChar = lexer->NextChar ( lexer ) ;
            Lexer_DoChar ( lexer, inChar ) ;
            lexer->Token_Length ++ ;
        }
        lexer->CurrentTokenDelimiter = lexer->TokenInputByte ;
        if ( lexer->TokenWriteIndex && ( ! GetState ( lexer, LEXER_RETURN_NULL_TOKEN ) ) )
        {
            _AppendCharacterToTokenBuffer ( lexer, 0 ) ; // null terminate TokenBuffer
            if ( Lexer_StringMacros_Do ( lexer ) ) goto start ;
            lexer->OriginalToken = String_New ( lexer->TokenBuffer, SESSION ) ; // not TEMPORARY or strings on the stack are deleted at each newline after startup
        }
        else lexer->OriginalToken = ( byte * ) 0 ; // why not goto restartToken ? -- to allow user to hit newline and get response
        lexer->Token_Length = Strlen ( ( char* ) lexer->OriginalToken ) ;
        lexer->TokenEnd_ReadLineIndex = lexer->TokenStart_ReadLineIndex + lexer->Token_Length ;
        lexer->TokenStart_FileIndex = rl->LineStartFileIndex + lexer->TokenStart_ReadLineIndex ; //- lexer->Token_Length ;
        if ( peekFlag && reAddPeeked ) CSL_PushToken_OnTokenList ( lexer->OriginalToken ) ;
        lexer->LastLexedChar = inChar ;
    }
    lexer->LastToken = lexer->OriginalToken ;

    DEBUG_SETUP_TOKEN ( lexer->OriginalToken, 1 ) ;
    if ( lexer->Token_Length == 1 )
    {
        byte lot0 = lexer->OriginalToken [0] ;
        if ( ( lot0 == '(' ) || ( ( lot0 == '{' ) && ( ! C_SyntaxOn ) ) )_Compiler_->ParenLevel ++ ;
        else if ( ( lot0 == ')' ) || ( ( lot0 == '}' ) && ( ! C_SyntaxOn ) ) )_Compiler_->ParenLevel -- ;
    }
    return lexer->OriginalToken ;
}

void
Lexer_Init ( Lexer * lexer, byte * delimiters, uint64 state, uint64 allocType )
{
    Context * cntx = _Context_ ;
    lexer->TokenBuffer = _CSL_->TokenBuffer ;
    Mem_Clear ( lexer->TokenBuffer, BUFFER_IX_SIZE ) ;
    lexer->OriginalToken = 0 ;
    lexer->Literal = 0 ;
    lexer->SC_Index = _CSL_->SC_Index ;
    if ( cntx->SpecialDelimiterCharSet )
    {
        lexer->DelimiterCharSet = cntx->SpecialDelimiterCharSet ;
        lexer->TokenDelimiters = cntx->SpecialTokenDelimiters ;
        lexer->DelimiterOrDotCharSet = cntx->SpecialDelimiterOrDotCharSet ;
    }
    else if ( delimiters ) Lexer_SetTokenDelimiters ( lexer, delimiters, allocType ) ;
    else
    {
        lexer->DelimiterCharSet = cntx->DefaultDelimiterCharSet ;
        lexer->TokenDelimiters = cntx->DefaultTokenDelimiters ;
        lexer->DelimiterOrDotCharSet = cntx->DefaultDelimiterOrDotCharSet ;
    }
    lexer->State = state & ( ~ LEXER_RETURN_NULL_TOKEN ) ;
    SetState ( lexer, KNOWN_OBJECT | LEXER_DONE | END_OF_FILE | END_OF_STRING | END_OF_LINE | LEXER_ALLOW_DOT, false ) ;
    lexer->TokenStart_ReadLineIndex = - 1 ;
    lexer->Filename = lexer->ReadLiner0->Filename ;
    lexer->LineNumber = lexer->ReadLiner0->LineNumber ;
    lexer->SC_Index = 0 ;
    lexer->Token_CompiledDataFieldByteSize = 0 ;
    Lexer_RestartToken ( lexer ) ;
}

byte
Lexer_NextPrintChar ( Lexer * lexer )
{
    //return _String_NextNonDelimiterChar ( _ReadLine_pb_NextChar ( lexer->ReadLiner0 ) - 1, lexer->DelimiterCharSet ) ;
    return _String_NextPrintChar ( _ReadLine_pb_NextChar ( lexer->ReadLiner0 ), lexer->DelimiterCharSet ) ;
}
//----------------------------------------------------------------------------------------|
//              get from/ add to head  |              | get from head      add to tail    |      
// TokenList Tail <--> TokenList Head  |<interpreter> | PeekList Head <--> PeekList Tail  |
// token token token token token token | currentToken | token token token token token ... |
//----------------------------------------------------------------------------------------|

void
_CSL_AddTokenToTailOfTokenList ( byte * token )
{
    if ( token ) dllist_AddNodeToTail ( _Lexer_->TokenList, ( dlnode* ) Lexer_Token_New ( token ) ) ;
    //if ( Is_DebugOn ) Symbol_List_Print ( CSL->TokenList ) ;
}

void
CSL_PushToken_OnTokenList ( byte * token )
{
    if ( token ) dllist_AddNodeToHead ( _Lexer_->TokenList, ( dlnode* ) Lexer_Token_New ( token ) ) ;
    //if ( Is_DebugOn ) Symbol_List_Print ( CSL->TokenList ) ;
}

Symbol *
Lexer_GetTokenFromTokenList ( Lexer * lexer, Boolean peekFlag )
{
    Symbol * tknSym = lexer->NextPeekListItem ;
    if ( ! ( peekFlag && tknSym ) ) tknSym = ( Symbol* ) dllist_First ( ( dllist* ) lexer->TokenList ) ;
    if ( tknSym )
    {
        if ( peekFlag ) lexer->NextPeekListItem = ( Symbol* ) dlnode_Next ( ( dlnode * ) tknSym ) ;
        else
        {
            lexer->NextPeekListItem = 0 ;
            dlnode_Remove ( ( dlnode* ) tknSym ) ;
        }
        lexer->TokenStart_ReadLineIndex = tknSym->S_Value ;
        lexer->TokenEnd_ReadLineIndex = tknSym->S_Value2 ;
        lexer->LineNumber = tknSym->S_Value3 ;
        lexer->OriginalToken = tknSym->S_Name ;

        return tknSym ;
    }
    return 0 ;
}

byte *
Lexer_GetTokenNameFromTokenList ( Lexer * lexer, Boolean peekFlag )
{
    Symbol * tknSym = Lexer_GetTokenFromTokenList ( lexer, peekFlag ) ;
    if ( tknSym ) return tknSym->S_Name ;
    return 0 ;
}

Symbol *
Lexer_Token_New ( byte * token )
{
    Symbol * tknSym = _Symbol_New ( token, SESSION ) ;
    tknSym->S_Value = _Context_->Lexer0->TokenStart_ReadLineIndex ;
    tknSym->S_Value2 = _Context_->Lexer0->TokenEnd_ReadLineIndex ;
    tknSym->S_Value3 = _Context_->Lexer0->LineNumber ;
    return tknSym ;
}

// ??? this word and it's callees needs to be improved ??? : not as functional as it could be

int64
_Lexer_ConsiderDebugAndCommentTokens ( byte * token, int64 evalFlag )
{
    Word * word = Finder_Word_FindUsing ( _Finder_, token, 1 ) ;
    Word_SetTsrliScwi ( word, - 1, - 1 ) ;
    //if ( word && ( word->W_MorphismAttributes & LEFT_PAREN ) && ignoreLParen ) return true ;
    //else 
    if ( word && ( word->W_TypeAttributes & W_COMMENT ) )
    {
        Word_Eval ( word ) ;
        return true ;
    }
#if 0  // this needs to be re-considered
    else if ( word && ( word->W_MorphismAttributes & DEBUG_WORD ) )
    {
        if ( evalFlag ) Word_Eval ( word ) ;
        return true ;
    }
#endif    
    return false ;
}

byte *
_Lexer_Next_Token ( Lexer * lexer, byte * delimiters, Boolean evalFlag, Boolean peekFlag, Boolean skipDbgCommentFlag )
{
    byte * token ;
    Boolean debugOrComment ;
    do
    {
        token = _Lexer_LexNextToken_WithDelimiters ( lexer, delimiters, 1, peekFlag, 0, 0 ) ; // ?? the logic here ??
        debugOrComment = _Lexer_ConsiderDebugAndCommentTokens ( token, evalFlag ) ;
    }
    while ( skipDbgCommentFlag && debugOrComment ) ;
    return token ;
}

Boolean
Lexer_IsNextWordLeftParen ( Lexer * lexer )
{
    // with this any postfix word that is not a keyword or a c rtl arg word can now be used prefix with parentheses (in PREFIX_MODE)
    byte c = Lexer_NextPrintChar ( lexer ) ;
    if ( ( c == '(' ) ) return true ;
    else return false ;
}

Boolean
Lexer_IsWordPrefixing ( Lexer * lexer, Word * word )
{
    Boolean nwlp ;
    if ( word->W_MorphismAttributes & COMBINATOR ) return false ;
    if ( GetState ( _Context_, LC_INTERPRET ) ) return true ;
    else if ( ( nwlp = Lexer_IsNextWordLeftParen ( lexer ) ) && ( word->W_TypeAttributes & ( WT_PREFIX | WT_PREFIXABLE ) ) ) return true ;
        //else if ( Namespace_IsUsing ( ( byte* ) "BigNum" ) && ( word->W_TypeAttributes & ( WT_PREFIX|WT_PREFIXABLE ) ) ) return true ;
    else if ( ( ( ! word->Name || ( word->Name[0] == '(' ) ) ) || ( ( GetState ( _Interpreter_, PREPROCESSOR_MODE ) ) && ( ( word->W_TypeAttributes & W_PREPROCESSOR ) ||
        ( ! ( word->W_MorphismAttributes & PREFIX ) ) && ( ! ( word->W_MorphismAttributes & CSL_WORD ) ) ) ) ) return false ;
    else if ( ( ( GetState ( _Context_, PREFIX_MODE ) ) && ( ! ( word->W_MorphismAttributes & ( CATEGORY_OP_OPEQUAL | CATEGORY_OP_EQUAL | KEYWORD ) ) )
        && ( ! ( GetState ( _Compiler_, DOING_RETURN ) ) ) ) )
    {
        return nwlp ; // ??
    }
    else return false ;
}

byte *
Lexer_Peek_NextToken ( Lexer * lexer, Boolean evalFlag, Boolean skipDbgCommentsFlag )
{
    byte * token = _Lexer_Next_Token ( lexer, 0, evalFlag, 0, skipDbgCommentsFlag ) ; // 0 : peekFlag off because we are reAdding it below
    CSL_PushToken_OnTokenList ( token ) ;
    return token ;
}

void
_Lexer_DoChar ( Lexer * lexer, byte c )
{
    _CSL_->LexerCharacterFunctionTable [ _CSL_->LexerCharacterTypeTable [ c ].CharInfo ] ( lexer ) ;
}

void
Lexer_DoChar ( Lexer * lexer, byte c )
{
    lexer->TokenInputByte = c ;
    _Lexer_DoChar ( lexer, c ) ;
    lexer->CurrentReadIndex = lexer->ReadLiner0->ReadIndex ;
}

void
Lexer_DoNextChar ( Lexer * lexer )
{
    Lexer_DoChar ( lexer, lexer->NextChar ( lexer ) ) ;
}

void
Lexer_LexNextToken_WithDelimiters ( Lexer * lexer, byte * delimiters )
{
    _Lexer_LexNextToken_WithDelimiters ( lexer, delimiters, 1, 0, 1, 0 ) ;
}

byte *
_Lexer_ReadToken ( Lexer * lexer, byte * delimiters )
{
    Lexer_LexNextToken_WithDelimiters ( lexer, delimiters ) ;
    return lexer->OriginalToken ;
}

byte *
Lexer_ReadToken ( Lexer * lexer )
{
    byte * token = _Lexer_ReadToken ( lexer, 0 ) ;
    return token ;
}

void
Lexer_AppendToTokenBuffer ( Lexer * lexer, byte c )
{
    lexer->TokenBuffer [ lexer->TokenWriteIndex ++ ] = c ;
    lexer->TokenBuffer [ lexer->TokenWriteIndex ] = 0 ;
}

void
_Lexer_AppendByteToTokenBuffer ( Lexer * lexer, byte c )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    if ( lexer->TokenStart_ReadLineIndex == - 1 ) // -1 : Lexer_Init marker
    {
        lexer->TokenStart_ReadLineIndex = rl->ReadIndex - 1 ; // -1 : ReadIndex has already been incremented for the next char so this char is -1
    }
    Lexer_AppendToTokenBuffer ( lexer, c ) ;
}

void
Lexer_AppendCharToTokenBuffer ( Lexer * lexer, byte chr )
{
    if ( ! GetState ( lexer, LEXER_ESCAPE_SEQUENCE ) )
    {
        _Lexer_AppendByteToTokenBuffer ( lexer, chr ) ;
        _Lexer_AppendByteToSourceCode ( lexer, chr, 0 ) ;
    }
}

void
Lexer_AppendInputedByteToTokenBuffer ( Lexer * lexer )
{
    _Lexer_AppendByteToTokenBuffer ( lexer, lexer->TokenInputByte ) ;
}

void
Lexer_AppendInputedToTokenBuffer ( Lexer * lexer )
{
    if ( ! GetState ( lexer, LEXER_ESCAPE_SEQUENCE ) )
    {
        _Lexer_AppendByteToTokenBuffer ( lexer, lexer->TokenInputByte ) ;
        _Lexer_AppendByteToSourceCode ( lexer, lexer->TokenInputByte, 0 ) ;
    }
}

void
_Lexer_Append_ConvertedCharacterToTokenBuffer ( Lexer * lexer, byte abyte )
{
    _String_AppendConvertCharToBackSlash ( TokenBuffer_AppendPoint ( lexer ), abyte, 0 ) ;
    _Lexer_AppendByteToSourceCode ( lexer, abyte, 0 ) ;
    lexer->TokenWriteIndex ++ ;
}

void
Lexer_Append_ConvertedCharacterToTokenBuffer ( Lexer * lexer )
{
    _Lexer_Append_ConvertedCharacterToTokenBuffer ( lexer, lexer->TokenInputByte ) ;
}

void
Lexer_UnAppendCharacter ( Lexer * lexer )
{
    //return lexer->TokenBuffer [ -- lexer->TokenWriteIndex ] ;
    -- lexer->TokenWriteIndex ;
}

byte
Lexer_LastChar ( Lexer * lexer )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    //return lexer->TokenBuffer [ lexer->TokenWriteIndex - 1 ] ;
    byte lastChar = rl->InputLine [ rl->ReadIndex - 2 ] ;
    return lastChar ;
}

void
Lexer_SetTokenDelimiters ( Lexer * lexer, byte * delimiters, uint64 allocType )
{
    lexer->TokenDelimiters = delimiters ;
    //if ( lexer->DelimiterCharSet ) CharSet_Init ( lexer->DelimiterCharSet, 128, delimiters ) ;
    //else 
    lexer->DelimiterCharSet = CharSet_New ( delimiters, allocType ) ;
    lexer->DelimiterOrDotCharSet = CharSet_NewDelimitersWithDot ( delimiters, allocType ) ;
}

Lexer *
Lexer_New ( uint64 allocType )
{
    Lexer * lexer = ( Lexer * ) Mem_Allocate ( sizeof (Lexer ), allocType ) ;
    lexer->ReadLiner0 = ReadLine_New ( allocType ) ;
    lexer->TokenList = _dllist_New ( allocType ) ;
    Lexer_Init ( lexer, 0, 0, allocType ) ;
    lexer->NextChar = _Lexer_NextChar ;
    return lexer ;
}

void
_Lexer_Copy ( Lexer * lexer, Lexer * lexer0, uint64 allocType )
{
    MemCpy ( lexer, lexer0, sizeof (Lexer ) ) ;
    Lexer_Init ( lexer, 0, 0, allocType ) ;
    ReadLiner * rl = ReadLine_Copy ( lexer0->ReadLiner0, allocType ) ;
    lexer->ReadLiner0 = rl ;
    lexer->NextChar = _Lexer_NextChar ;
}

Lexer *
Lexer_Copy ( Lexer * lexer0, uint64 allocType )
{
    Lexer * lexer = ( Lexer * ) Mem_Allocate ( sizeof (Lexer ), allocType ) ;
    _Lexer_Copy ( lexer, lexer0, allocType ) ;
    return lexer ;
}

void
Lexer_RestartToken ( Lexer * lexer )
{
    lexer->TokenWriteIndex = 0 ;
    lexer->Token_Length = 0 ;
}

// special case here is quoted Strings - "String literals"
// use lexer->ReadLinePosition = 0 to cause a new Token read
// or lexer->Token_ReadLineIndex = BUFFER_SIZE

void
Lexer_SourceCodeOn ( Lexer * lexer )
{
    SetState ( lexer, ( ADD_TOKEN_TO_SOURCE | ADD_CHAR_TO_SOURCE ), true ) ;
}

void
Lexer_SourceCodeOff ( Lexer * lexer )
{
    SetState ( lexer, ( ADD_TOKEN_TO_SOURCE | ADD_CHAR_TO_SOURCE ), false ) ;
}

void
_Lexer_AppendByteToSourceCode ( Lexer * lexer, byte c, int64 convert )
{
    if ( GetState ( _CSL_, SOURCE_CODE_ON ) && GetState ( lexer, ADD_CHAR_TO_SOURCE ) )
    {
        CSL_AppendCharToSourceCode ( _CSL_, c ) ;
    }
}

void
Lexer_DoDelimiter ( Lexer * lexer )
{
    _Lexer_AppendByteToSourceCode ( lexer, lexer->TokenInputByte, 0 ) ;
    // must have at least one non-delimiter to make a token
    // else keep going we just have multiple delimiters ( maybe just spaces ) in a row
    if ( lexer->TokenWriteIndex )
    {
        SetState ( lexer, LEXER_DONE, true ) ;
        return ;
    }
    else
    {
        Lexer_RestartToken ( lexer ) ; //prevent null token which complicates lexers
        return ;
    }
}

Boolean
Lexer_IsCurrentInputCharADelimiter ( Lexer * lexer )
{
    return ( Boolean ) _Lexer_IsCharDelimiter ( lexer, lexer->TokenInputByte ) ;
}

void
Lexer_Default ( Lexer * lexer )
{
    if ( Lexer_IsCurrentInputCharADelimiter ( lexer ) ) //_IsChar_Delimiter ( lexer->TokenDelimiters, lexer->TokenInputCharacter ) )
    {
        _Lexer_AppendByteToSourceCode ( lexer, lexer->TokenInputByte, 0 ) ;
        // must have at least one non-delimiter to make a token
        // else keep going we just have multiple delimiters ( maybe just spaces ) in a row
        if ( lexer->TokenWriteIndex )
        {
            SetState ( lexer, LEXER_DONE, true ) ;
            return ;
        }
        else
        {
            lexer->CurrentReadIndex ++ ;
            Lexer_RestartToken ( lexer ) ; //prevent null token which complicates lexers
            return ;
        }
    }
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
Lexer_MakeItTheNextToken ( Lexer * lexer )
{
    ReadLine_UnGetChar ( lexer->ReadLiner0 ) ; // allow to read '.' as next token
    //_CSL_UnAppendFromSourceCode ( 1 ) ;
    SetState ( lexer, LEXER_DONE, true ) ;
}

void
TerminatingMacro ( Lexer * lexer )
{
    if ( ( ! lexer->TokenWriteIndex ) || ( Lexer_LastChar ( lexer ) == '_' ) ) Lexer_Default ( lexer ) ; // allow for "_(" token 
    else ReadLine_UnGetChar ( lexer->ReadLiner0 ) ; // so NextChar will have this TokenInputCharacter for the next token
    Lexer_FinishTokenHere ( lexer ) ; // after appending the terminating macro char
    return ;
}

void
NonTerminatingMacro ( Lexer * lexer )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    Lexer_Default ( lexer ) ;
    if ( lexer->TokenWriteIndex == 1 )
    {
        byte chr = ReadLine_PeekNextChar ( rl ) ;
        if ( ( chr == 't' ) || ( chr == 'f' ) )
            return ;
        if ( ( chr == 'd' ) && ( _ReadLine_PeekOffsetChar ( rl, 1 ) == 'e' ) ) Lexer_FinishTokenHere ( lexer ) ; // #define
        else if ( chr == '$' )
        {
            do
            {
                _ReadLine_GetNextChar ( rl ) ;
                Lexer_Default ( lexer ) ;
                chr = ReadLine_PeekNextChar ( rl ) ; //_ReadLine_PeekIndexedChar ( rl, 1 ) ;
            }
            while ( chr == '$' ) ;
            Lexer_FinishTokenHere ( lexer ) ;
        }
        else if ( ( chr != 'x' ) && ( chr != 'X' ) && ( chr != 'b' ) && ( chr != 'o' ) && ( chr != 'd' ) ) Lexer_FinishTokenHere ( lexer ) ; // x/X : check for hexidecimal marker
        return ;
    }
}

int64
_Lexer_MacroChar_NamespaceCheck ( Lexer * lexer, byte * nameSpace )
{
    byte buffer [2] ;
    buffer [0] = lexer->TokenInputByte ;
    buffer [1] = 0 ;
    return _CSL_IsContainingNamespace ( buffer, nameSpace ) ;
}

void
Lexer_FinishTokenHere ( Lexer * lexer )
{
    _AppendCharacterToTokenBuffer ( lexer, 0 ) ;
    SetState ( lexer, LEXER_DONE, true ) ;
    return ;
}

void
SingleEscape ( Lexer * lexer )
{
    lexer->TokenInputByte = ReadLine_NextChar ( lexer->ReadLiner0 ) ;
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
_BackSlash ( Lexer * lexer, int64 flag )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    byte nextChar = ReadLine_PeekNextChar ( rl ), lastChar = rl->InputLine [ rl->ReadIndex - 2 ] ;
    int64 i ;
    if ( nextChar == 'n' )
    {
        _ReadLine_GetNextChar ( lexer->ReadLiner0 ) ;
        lexer->TokenInputByte = '\n' ;
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    }
    else if ( nextChar == 'r' )
    {
        _ReadLine_GetNextChar ( lexer->ReadLiner0 ) ;
        lexer->TokenInputByte = '\r' ;
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    }
    else if ( nextChar == 't' )
    {
        _ReadLine_GetNextChar ( lexer->ReadLiner0 ) ;
        lexer->TokenInputByte = '\t' ;
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    }
    else if ( lastChar == '/' ) // current lisp lambda abbreviation "/\"
    {
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    }
    else if ( nextChar == ' ' && GetState ( _Context_->Interpreter0, PREPROCESSOR_DEFINE ) )
    {
        i = ReadLiner_PeekSkipSpaces ( rl ) ;
        if ( _ReadLine_PeekOffsetChar ( rl, i ) == '\n' ) ; // do nothing - don't append the newline 
    }
    else if ( nextChar == '\n' && GetState ( _Interpreter_, PREPROCESSOR_MODE ) ) _ReadLine_GetNextChar ( lexer->ReadLiner0 ) ; // ignore the newline
    else if ( flag )
    {
        //Lexer_AppendCharacterToTokenBuffer ( lexer ) ; // the backslash
        _Lexer_AppendByteToSourceCode ( lexer, lexer->TokenInputByte, 0 ) ; // the backslash 
        _Lexer_AppendByteToTokenBuffer ( lexer, nextChar ) ; // the escaped char eg. '"'
        _Lexer_AppendByteToSourceCode ( lexer, nextChar, 0 ) ;
        _ReadLine_GetNextChar ( lexer->ReadLiner0 ) ;
    }
    else if ( ! flag ) SingleEscape ( lexer ) ; //Lexer_AppendCharacterToTokenBuffer ( lexer ) ;
}

void
BackSlash ( Lexer * lexer )
{
    _BackSlash ( lexer, 1 ) ;
}

void
_MultipleEscape ( Lexer * lexer )
{
    byte multipleEscapeChar = lexer->TokenInputByte ;
    while ( 1 )
    {
        lexer->TokenInputByte = ReadLine_NextChar ( lexer->ReadLiner0 ) ;
        if ( lexer->TokenInputByte == multipleEscapeChar ) break ;
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    }
    SetState ( lexer, LEXER_DONE, true ) ;
}

// '"'

void
DoubleQuote ( Lexer * lexer )
{
    if ( Lexer_IsCurrentInputCharADelimiter ( lexer ) ) //_IsChar_Delimiter ( lexer->TokenDelimiters, lexer->TokenInputCharacter ) )
    {
        do Lexer_AppendCharToTokenBuffer ( lexer, lexer->TokenInputByte ) ;
        while ( ( lexer->TokenInputByte = lexer->NextChar ( lexer ) ) != '"' ) ;
        Lexer_AppendCharToTokenBuffer ( lexer, lexer->TokenInputByte ) ;
        SetState ( lexer, LEXER_DONE, true ) ;
    }
    else TerminatingMacro ( lexer ) ;
}


// '->' for pointers within a string and without surrounding spaces 

void
Minus ( Lexer * lexer ) // '-':
{
    byte nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ;
    if ( ( nextChar == '-' ) || ( nextChar == '>' ) || ( nextChar == '=' ) )
    {
        if ( lexer->TokenWriteIndex ) ReadLine_UnGetChar ( lexer->ReadLiner0 ) ; // allow to read '--' as next token
        else
        {
            Lexer_AppendInputedToTokenBuffer ( lexer ) ;
            nextChar = ReadLine_NextChar ( lexer->ReadLiner0 ) ;
            Lexer_AppendCharToTokenBuffer ( lexer, nextChar ) ;
        }
        SetState ( lexer, LEXER_DONE, true ) ;
        return ;
    }
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
Plus ( Lexer * lexer ) // '+':
{
    byte nextChar ;
    if ( lexer->TokenWriteIndex )
    {
        nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ;
        if ( nextChar == '+' )
        {
            ReadLine_UnGetChar ( lexer->ReadLiner0 ) ; // allow to read '++' as next token
            SetState ( lexer, LEXER_DONE, true ) ;
            return ;
        }
    }
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
EndEscapeSequence ( Lexer * lexer )
{
    if ( GetState ( lexer, LEXER_ESCAPE_SEQUENCE ) )
    {
        SetState ( lexer, LEXER_ESCAPE_SEQUENCE, false ) ;
    }
    else Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
Escape ( Lexer * lexer )
{
    SetState ( lexer, LEXER_ESCAPE_SEQUENCE, true ) ;
}

void
ForwardSlash ( Lexer * lexer ) // '/':
{
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    byte nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ;
    if ( ( nextChar == '/' ) || ( nextChar == '*' ) )
    {
        lexer->TokenInputByte = ReadLine_NextChar ( lexer->ReadLiner0 ) ;
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
        SetState ( lexer, LEXER_DONE, true ) ;
    }
}

void
Star ( Lexer * lexer ) // '*':
{
    byte nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ), lastChar = Lexer_LastChar ( lexer ) ;
    if ( ( nextChar == '/' ) && ( ! lexer->TokenWriteIndex ) ) // '*/' comment
    {
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
        lexer->TokenInputByte = ReadLine_NextChar ( lexer->ReadLiner0 ) ;
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
    }
    else if ( GetState ( _Context_, TDI_PARSING ) && ( ( isalpha ( lastChar ) ) ) ) // || ( lastChar == '*' ) ) ) //( lastChar != ' ' ) && ( lastChar != '%' ) && ( lastChar != '.' ) && ( lastChar != '*' ) )
    {
        if ( lexer->Token_Length <= 1 ) goto done ;
        Lexer_MakeItTheNextToken ( lexer ) ;
        SetState ( lexer, LEXER_DONE, true ) ;
        //return ;
    }
    else if ( ( GetState ( _Context_, C_SYNTAX ) || GetState ( _Context_, TDI_PARSING ) ) && ( nextChar != '=' ) )
    {
done:
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
        SetState ( lexer, LEXER_DONE, true ) ;
    }
    else Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
AddressOf ( Lexer * lexer ) // ';':
{
    if ( CharTable_IsCharType ( ReadLine_PeekNextChar ( lexer->ReadLiner0 ), CHAR_ALPHA ) ) TerminatingMacro ( lexer ) ;
    else Lexer_Default ( lexer ) ;
}

void
AtFetch ( Lexer * lexer ) // ';':
{
    Lexer_Default ( lexer ) ;
    if ( _LC_ && GetState ( _LC_, LC_READ ) ) Lexer_FinishTokenHere ( lexer ) ;
}

#if 0

void
Semi ( Lexer * lexer ) // ';':
{
    if ( GetState ( _Context_, C_SYNTAX ) && ( lexer->TokenWriteIndex ) )
    {
        byte nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ;
        if ( isspace ( nextChar ) )
        {
            Lexer_MakeItTheNextToken ( lexer ) ;
            return ;
        }
    }
    Lexer_Default ( lexer ) ;
}
#else

void
Semi ( Lexer * lexer ) // ';':
{
    byte nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ;
    if ( GetState ( _Context_, C_SYNTAX ) )
    {
        if ( lexer->TokenWriteIndex )
        {
            if ( isspace ( nextChar ) )
            {
                Lexer_MakeItTheNextToken ( lexer ) ;
                return ;
            }
        }
        else
        {
            if ( nextChar == '/' ) // beginning of '//' comment : assumed?
            {
                Lexer_AppendInputedToTokenBuffer ( lexer ) ;
                SetState ( lexer, LEXER_DONE, true ) ;
                return ;
            }
            else if ( nextChar == ';' ) // beginning of ';;'
            {
                if ( GetState ( _Compiler_, LC_ARG_PARSING ) )
                {
                    ReadLine_UnGetChar ( lexer->ReadLiner0 ) ;
                    SetState ( lexer, LEXER_DONE, true ) ;
                    return ;
                }
            }
        }
    }
    Lexer_Default ( lexer ) ;
}
#endif

void
Exclam ( Lexer * lexer ) // ';':
{
    if ( GetState ( _Context_, ( C_SYNTAX | CONTEXT_PREPROCESSOR_MODE ) ) ) //&& lexer->TokenWriteIndex )
    {
        if ( lexer->TokenWriteIndex ) Lexer_MakeItTheNextToken ( lexer ) ;
        else
        {
            byte nextChar = ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ;
            if ( nextChar != '=' )
            {
                Lexer_Default ( lexer ) ;
                Lexer_FinishTokenHere ( lexer ) ;
            }
            else Lexer_Default ( lexer ) ;
        }
        return ;
    }
    else Lexer_Default ( lexer ) ;
}

void
GreaterThan ( Lexer * lexer ) // '>':
{
    if ( lexer->TokenWriteIndex )
    {
        if ( Lexer_LastChar ( lexer ) == '-' )
        {
            Lexer_AppendInputedToTokenBuffer ( lexer ) ;
            SetState ( lexer, LEXER_DONE, true ) ;
            return ;
        }
    }
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

// package the dot to be lexed as a token

void
Dot ( Lexer * lexer ) //  '.':
{
    if ( ( Lexer_LastChar ( lexer ) != '/' ) && ( ! GetState ( lexer, LEXER_ALLOW_DOT ) ) ) //allow for lisp special char sequence "/." as a substitution for lambda
    {
        int64 i ;
        if ( ( ! GetState ( lexer, PARSING_STRING ) ) ) //&& ( ! GetState ( _Context_, CONTEXT_PARSING_QUALIFIED_ID ) ) ) // if we are not parsing a String ?
        {
            if ( lexer->TokenWriteIndex )
            {
                for ( i = lexer->TokenWriteIndex - 1 ; i >= 0 ; i -- ) // go back into previous chars read, check if it is a number
                {
                    if ( ! isdigit ( lexer->TokenBuffer [ i ] ) )
                    {
                        ReadLine_UnGetChar ( lexer->ReadLiner0 ) ; // allow to read '.' as next token
                        SetState ( lexer, LEXER_DONE, true ) ;
                        return ;
                    }
                }
            }
            if ( ! isdigit ( ReadLine_PeekNextChar ( lexer->ReadLiner0 ) ) ) SetState ( lexer, LEXER_DONE, true ) ;
        }
    }
    Lexer_AppendInputedToTokenBuffer ( lexer ) ;
}

void
RightCurlyBracket ( Lexer * lexer )
{
    if ( GetState ( _Context_, TDI_PARSING ) )
    {
        Lexer_AppendInputedToTokenBuffer ( lexer ) ;
        Lexer_FinishTokenHere ( lexer ) ;
    }
    else Lexer_Default ( lexer ) ;
}

void
LeftCurlyBracket ( Lexer * lexer )
{
    if ( GetState ( _Context_, TDI_PARSING ) && ( lexer->TokenWriteIndex == 0 ) )
    {
        _Lexer_AppendByteToSourceCode ( lexer, ' ', 0 ) ;
        //Lexer_AppendInputedToTokenBuffer ( lexer ) ;
        //Lexer_FinishTokenHere ( lexer ) ;
    }
    //else 
    Lexer_Default ( lexer ) ;
}

void
Comma ( Lexer * lexer )
{
    if ( GetState ( _Context_, ASM_SYNTAX | C_SYNTAX | INFIX_MODE ) || GetState ( _Context_, TDI_PARSING ) ) //&& lexer->TokenWriteIndex )
    {
        if ( lexer->TokenWriteIndex )
        {
            Lexer_MakeItTheNextToken ( lexer ) ;
            return ;
        }
#if 0        
        else
        {
            CSL_C_Comma ( ) ;
        }
#endif        
    }
    else if ( ! GetState ( _Context_->Compiler0, LC_ARG_PARSING ) )
    {
        if ( _Lexer_MacroChar_NamespaceCheck ( lexer, ( byte* ) "Lisp" ) )
        {
            if ( _LC_ )
            {
                byte nextChar = ReadLine_PeekNextNonWhitespaceChar ( lexer->ReadLiner0 ) ;
                if ( nextChar == '@' )
                {
                    Lexer_AppendInputedToTokenBuffer ( lexer ) ; // the comma
                    byte chr = _ReadLine_GetNextChar ( lexer->ReadLiner0 ) ; // the '@'
                    lexer->TokenInputByte = chr ;
                }
                Lexer_AppendInputedToTokenBuffer ( lexer ) ;
                Lexer_FinishTokenHere ( lexer ) ;
                return ;
            }
        }
    }
    Lexer_Default ( lexer ) ;
}

void
CarriageReturn ( Lexer * lexer )
{
    NewLine ( lexer ) ;
}

void
NewLine ( Lexer * lexer )
{
    if ( AtCommandLine ( lexer->ReadLiner0 ) ) //( ! IS_INCLUDING_FILES ) || GetState ( _Debugger_, DBG_COMMAND_LINE ) )
    {
        SetState ( lexer, LEXER_DONE | END_OF_LINE, true ) ;
    }
    else
    {
        SetState ( lexer, END_OF_LINE, true ) ;
        Lexer_Default ( lexer ) ;
    }
}

void
_EOF ( Lexer * lexer ) // case eof:
{
    SetState ( lexer, LEXER_DONE | END_OF_FILE, true ) ;
}

void
_Zero ( Lexer * lexer ) // case 0
{
    //SetState ( lexer, LEXER_DONE | END_OF_STRING | LEXER_END_OF_LINE | END_OF_FILE, true ) ;
    //SetState ( lexer, LEXER_DONE | END_OF_STRING | END_OF_FILE, true ) ;
    SetState ( lexer, LEXER_DONE | END_OF_STRING, true ) ;
}

int64
Lexer_CheckIfDone ( Lexer * lexer, int64 flags )
{
    return lexer->State & flags ;
}

// the non-string lexer char input function

byte
_Lexer_NextChar ( Lexer * lexer )
{
    ReadLiner * rl = lexer->ReadLiner0 ;
    return lexer->CurrentChar = ReadLine_NextChar ( rl ) ;
}

void
Lexer_SetInputFunction ( Lexer * lexer, byte ( *lipf ) ( struct _Lexer * ) )
{
    lexer->NextChar = lipf ;
}

Boolean
_Lexer_IsTokenForwardDotted ( Lexer * lexer, int64 end )
{
    return ReadLiner_IsTokenForwardDotted ( lexer->ReadLiner0, end ? end - 1 : lexer->TokenStart_ReadLineIndex ) ;
}

Boolean
Lexer_IsTokenForwardDotted ( Lexer * lexer )
{
    return _Lexer_IsTokenForwardDotted ( lexer, 0 ) ;
}

Boolean
Lexer_IsTokenQualifiedID ( Lexer * lexer )
{
    if ( Lexer_IsTokenReverseDotted ( lexer ) ) return true ;
    else return Lexer_IsTokenForwardDotted ( lexer ) ;
}

void
CSL_LexerTables_Setup ( CSL * csl )
{
    int64 i ;
    for ( i = 0 ; i < 256 ; i ++ ) csl->LexerCharacterTypeTable [ i ].CharInfo = 0 ;
    csl->LexerCharacterTypeTable [ 0 ].CharFunctionTableIndex = 1 ;
    csl->LexerCharacterTypeTable [ '-' ].CharFunctionTableIndex = 2 ;
    csl->LexerCharacterTypeTable [ '>' ].CharFunctionTableIndex = 3 ;
    csl->LexerCharacterTypeTable [ '.' ].CharFunctionTableIndex = 4 ;
    csl->LexerCharacterTypeTable [ '"' ].CharFunctionTableIndex = 5 ;
    csl->LexerCharacterTypeTable [ '\n' ].CharFunctionTableIndex = 6 ;
    csl->LexerCharacterTypeTable [ '\\' ].CharFunctionTableIndex = 7 ;
    csl->LexerCharacterTypeTable [ eof ].CharFunctionTableIndex = 8 ;
    csl->LexerCharacterTypeTable [ '\r' ].CharFunctionTableIndex = 9 ;
    csl->LexerCharacterTypeTable [ ',' ].CharFunctionTableIndex = 10 ;

    //csl->LexerCharacterTypeTable [ '{' ].CharFunctionTableIndex = 11 ; 
    //csl->LexerCharacterTypeTable [ '}' ].CharFunctionTableIndex = 11 ;
    csl->LexerCharacterTypeTable [ '[' ].CharFunctionTableIndex = 11 ;
    csl->LexerCharacterTypeTable [ ']' ].CharFunctionTableIndex = 11 ;
    csl->LexerCharacterTypeTable [ '`' ].CharFunctionTableIndex = 11 ;
    csl->LexerCharacterTypeTable [ '(' ].CharFunctionTableIndex = 11 ;
    csl->LexerCharacterTypeTable [ ')' ].CharFunctionTableIndex = 11 ;
    csl->LexerCharacterTypeTable [ '\'' ].CharFunctionTableIndex = 11 ;
    //csl->LexerCharacterTypeTable [ '%' ].CharFunctionTableIndex = 11 ;
    //csl->LexerCharacterTypeTable [ '&' ].CharFunctionTableIndex = 11 ;
    //csl->LexerCharacterTypeTable [ ',' ].CharFunctionTableIndex = 11 ;

    csl->LexerCharacterTypeTable [ '$' ].CharFunctionTableIndex = 12 ;
    csl->LexerCharacterTypeTable [ '#' ].CharFunctionTableIndex = 12 ;

    csl->LexerCharacterTypeTable [ '/' ].CharFunctionTableIndex = 14 ;
    csl->LexerCharacterTypeTable [ ';' ].CharFunctionTableIndex = 15 ;
    csl->LexerCharacterTypeTable [ '&' ].CharFunctionTableIndex = 16 ;
    csl->LexerCharacterTypeTable [ '@' ].CharFunctionTableIndex = 17 ;
    csl->LexerCharacterTypeTable [ '*' ].CharFunctionTableIndex = 18 ;
    csl->LexerCharacterTypeTable [ '+' ].CharFunctionTableIndex = 19 ;
    csl->LexerCharacterTypeTable [ ESC ].CharFunctionTableIndex = 20 ;
    csl->LexerCharacterTypeTable [ 'm' ].CharFunctionTableIndex = 21 ;
    csl->LexerCharacterTypeTable [ '!' ].CharFunctionTableIndex = 22 ;
    csl->LexerCharacterTypeTable [ '}' ].CharFunctionTableIndex = 23 ;
    csl->LexerCharacterTypeTable [ '{' ].CharFunctionTableIndex = 24 ;

    csl->LexerCharacterFunctionTable [ 0 ] = Lexer_Default ;
    csl->LexerCharacterFunctionTable [ 1 ] = _Zero ;
    csl->LexerCharacterFunctionTable [ 2 ] = Minus ;
    csl->LexerCharacterFunctionTable [ 3 ] = GreaterThan ;
    csl->LexerCharacterFunctionTable [ 4 ] = Dot ;
    csl->LexerCharacterFunctionTable [ 5 ] = DoubleQuote ;
    csl->LexerCharacterFunctionTable [ 6 ] = NewLine ;
    csl->LexerCharacterFunctionTable [ 7 ] = BackSlash ;
    csl->LexerCharacterFunctionTable [ 8 ] = _EOF ;
    csl->LexerCharacterFunctionTable [ 9 ] = CarriageReturn ;
    csl->LexerCharacterFunctionTable [ 10 ] = Comma ;

    csl->LexerCharacterFunctionTable [ 11 ] = TerminatingMacro ;
    csl->LexerCharacterFunctionTable [ 12 ] = NonTerminatingMacro ;
    //csl->LexerCharacterFunctionTable [ 13 ] = SingleEscape ;
    csl->LexerCharacterFunctionTable [ 14 ] = ForwardSlash ;
    csl->LexerCharacterFunctionTable [ 15 ] = Semi ;
    csl->LexerCharacterFunctionTable [ 16 ] = AddressOf ;
    csl->LexerCharacterFunctionTable [ 17 ] = AtFetch ;
    csl->LexerCharacterFunctionTable [ 18 ] = Star ;
    csl->LexerCharacterFunctionTable [ 19 ] = Plus ;
    csl->LexerCharacterFunctionTable [ 20 ] = Escape ;
    csl->LexerCharacterFunctionTable [ 21 ] = EndEscapeSequence ;
    csl->LexerCharacterFunctionTable [ 22 ] = Exclam ;
    csl->LexerCharacterFunctionTable [ 23 ] = RightCurlyBracket ;
    csl->LexerCharacterFunctionTable [ 24 ] = LeftCurlyBracket ;
}

int64
Lexer_ConvertLineIndexToFileIndex ( Lexer * lexer, int64 index )
{
    return lexer->TokenStart_FileIndex = lexer->ReadLiner0->LineStartFileIndex + index ; //- lexer->Token_Length ;
}

#if 0

Word *
Lexer_Do_MakeItAutoVar ( Lexer * lexer, byte * token, int64 tsrli, int64 scwi )
// ... ??? ... make it a 'variable' 
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Word * word = 0 ;
    if ( Compiling && GetState ( cntx, C_SYNTAX ) )
    {
        if ( ! compiler->AutoVarTypeNamespace ) _Namespace_ActivateAsPrimary ( compiler->LocalsNamespace ) ;
        word = DataObject_New ( LOCAL_VARIABLE, 0, token, 0, LOCAL_VARIABLE, 0, 0, 0, 0, DICTIONARY, tsrli, scwi ) ;
    }
    else
    {
        word = DataObject_New ( NAMESPACE_VARIABLE, 0, token, 0, NAMESPACE_VARIABLE, 0, 0, 0, 0, 0, tsrli, scwi ) ;
        word->W_ObjectAttributes |= ( T_RAW_STRING ) ;
    }
    return word ;
}

Boolean
Lexer_IsWordPrefixing ( Lexer * lexer, Word * word )
{
    if ( ( ! word->Name || ( word->Name[0] == '(' ) ) ) return false ;
    if ( GetState ( _Context_, LC_INTERPRET ) ) return true ;
    else if ( ( GetState ( _Interpreter_, PREPROCESSOR_MODE ) ) && ( ( word->W_TypeAttributes & W_PREPROCESSOR ) ||
        ( ! ( word->W_MorphismAttributes & PREFIX ) ) && ( ! ( word->W_MorphismAttributes & CSL_WORD ) ) ) ) return false ;
    else if ( ( ( GetState ( _Context_, PREFIX_MODE ) ) && ( ! ( word->W_MorphismAttributes & ( CATEGORY_OP_OPEQUAL | CATEGORY_OP_EQUAL | KEYWORD ) ) )
        && ( ! ( GetState ( _Compiler_, DOING_RETURN ) ) ) ) )
    {
        return Lexer_IsNextWordLeftParen ( lexer ) ;
    }

    else return false ;
}

#endif

