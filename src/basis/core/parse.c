
#include "../../include/csl.h"

void
Lexer_ParseAsAString ( Lexer * lexer )
{
    if ( lexer->OriginalToken [ 0 ] == '"' )
    {
        lexer->L_ObjectAttributes = ( T_STRING | KNOWN_OBJECT ) ;
        lexer->LiteralString = _String_UnBox ( lexer->OriginalToken ) ;
    }
    else if ( ( lexer->OriginalToken [ 0 ] == ( byte ) '\'' ) && ( strlen ( ( char* ) lexer->OriginalToken ) > 1 ) )
    {
        lexer->L_ObjectAttributes = ( T_CHAR | KNOWN_OBJECT ) ;
        lexer->Literal = ( int64 ) lexer->OriginalToken [ 1 ] ; //buffer  ;
    }
    else
    {
        lexer->L_ObjectAttributes = ( T_RAW_STRING | KNOWN_OBJECT ) ;
        lexer->LiteralString = lexer->OriginalToken ;
    }
    SetState ( lexer, KNOWN_OBJECT, true ) ;
}

void
_Lexer_ParseBinary ( Lexer * lexer, int64 offset )
{
    byte * token = & lexer->OriginalToken [offset] ;
    int64 cc = 0, i, l = Strlen ( ( char* ) token ) ; // 8 bits/byte
    byte current ;
    for ( i = 0 ; i < l ; i ++ )
    {
        current = token [ l - i - 1 ] ; // 1 : remember zero based array indexing
        if ( current == '1' )
            cc += ( 1 << i ) ;
        else if ( current == '0' )
            continue ;
        else if ( current == ' ' )
            continue ;
        else
        {
            SetState ( lexer, KNOWN_OBJECT, false ) ;
            Lexer_Exception ( token, NOT_A_KNOWN_OBJECT, ( byte * ) "\n_Lexer_ParseBinary : non binary digits with number base 2" ) ;
        }
    }
    SetState ( lexer, KNOWN_OBJECT, true ) ;
    lexer->Literal = cc ;
}

void
Lexer_ParseBinary ( Lexer * lexer, byte * token, int64 offset )
{
    _Lexer_ParseBinary ( lexer, offset ) ;
    if ( GetState ( lexer, KNOWN_OBJECT ) )
    {
        lexer->L_ObjectAttributes = ( T_INT | KNOWN_OBJECT ) ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    //else Lexer_ParseAsAString ( lexer ) ;
}

void
Lexer_ParseBigNum ( Lexer * lexer, byte * token )
{
    //Namespace * qidNs = _Context_->QidInNamespace ;
    //if ( qidNs && ( ! String_Equal ( qidNs->Name, "Int") ) && Namespace_IsUsing ( ( byte* ) "BigNum" ) )
    if ( Namespace_IsUsing ( ( byte* ) "BigNum" ) )
    {
        mpfr_t *bfr = ( mpfr_t* ) _BigNum_New ( token ) ;
        lexer->Literal = ( int64 ) bfr ;
        lexer->L_ObjectAttributes = ( T_BIG_NUM | KNOWN_OBJECT ) ;
        lexer->Token_CompiledDataFieldByteSize = 8 ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
    }
}
// return boolean 0 or 1 if lexer->Literal value is pushed

Boolean
Lexer_ScanForHexInt ( Lexer * lexer, byte * token )
{
    int64 i, sr, scrap, slt = Strlen ( token ) ;
    if ( sr = sscanf ( ( char* ) token, HEX_INT_FRMT, ( uint64* ) & lexer->Literal ) )
    {
        for ( i = 1 ; sr && i < slt ; i ++ )
        {
            sr = sscanf ( ( char* ) &token[i], HEX_INT_FRMT, ( int64* ) & scrap ) ;
        }
        if ( ! sr ) lexer->Literal = 0 ;
    }
    return sr ;
}

void
_Lexer_ParseHex ( Lexer * lexer, byte * token )
{
    // use 0d format for decimal numbers with hex NumberBase state
    if ( sscanf ( ( char* ) token, INT_FRMT_FOR_HEX, ( int64* ) & lexer->Literal ) )
    {
        lexer->L_ObjectAttributes = ( T_INT | KNOWN_OBJECT ) ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    else if ( Lexer_ScanForHexInt ( lexer, token ) ) //sscanf ( ( char* ) token, HEX_INT_FRMT, ( uint64* ) & lexer->Literal ) && sscanf ( ( char* ) &token[1], HEX_INT_FRMT, ( int64* ) & scrap ))
    {
        lexer->L_ObjectAttributes = ( T_INT | KNOWN_OBJECT ) ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    else if ( sscanf ( ( char* ) token, HEX_UINT_FRMT, ( uint64* ) & lexer->Literal ) )
    {
        lexer->L_ObjectAttributes = ( T_INT | KNOWN_OBJECT ) ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    else if ( sscanf ( ( char* ) token, LISP_HEX_FRMT, ( uint64* ) & lexer->Literal ) )
    {
        lexer->L_ObjectAttributes = ( T_INT | KNOWN_OBJECT ) ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    else Lexer_ParseAsAString ( lexer ) ;
}

void
_Lexer_ParseDecimal ( Lexer * lexer, byte * token )
{
    float f ;
    // use 0x format for hex numbers with decimal NumberBase state
    //D1 (String_Equal (_ReadLiner_->Filename, "namespaces/test/math.csl") ? CSL_CpuState_Current_Show ( ) : 0 ) ; //Debugger_CSLRegisters (_Debugger_) : 0 ) ;
    if ( sscanf ( ( char* ) token, HEX_UINT_FRMT, ( uint64* ) & lexer->Literal ) ||
        sscanf ( ( char* ) token, INT_FRMT_FOR_HEX, ( uint64* ) & lexer->Literal ) ||
        sscanf ( ( char* ) token, INT_FRMT, ( uint64* ) & lexer->Literal ) ||
        sscanf ( ( char* ) token, LISP_DECIMAL_FRMT, ( uint64* ) & lexer->Literal ) )
    {
        // ??? this need to be fixed : small integers are not necessarily to be considered as less than 8 bytes ???
        //if ( lexer->Literal ) // promote '0 to 8 bytes //  '0' ?
        {
            if ( abs ( lexer->Literal ) < 256 )
            {
                lexer->L_ObjectAttributes = ( T_BYTE | KNOWN_OBJECT ) ;
                lexer->Token_CompiledDataFieldByteSize = 1 ;
            }
            else if ( abs ( lexer->Literal ) <= 65535 )
            {
                lexer->L_MorphismAttributes = ( KNOWN_OBJECT ) ;
                lexer->L_ObjectAttributes |= ( T_INT16 ) ;
                lexer->Token_CompiledDataFieldByteSize = 2 ;
            }
            else if ( abs ( lexer->Literal ) <= 2147483647 )
            {
                lexer->L_MorphismAttributes = ( KNOWN_OBJECT ) ;
                lexer->L_ObjectAttributes = ( T_INT32 ) ;
                lexer->Token_CompiledDataFieldByteSize = 4 ;
            }
        }
#if 0        
        if ( lexer->L_ObjectAttributes )
        {
            lexer->L_ObjectAttributes = ( T_INT | KNOWN_OBJECT ) ;
            lexer->Token_CompiledDataFieldByteSize = 8 ;
        }
#endif        
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    else if ( sscanf ( ( char* ) token, "%f", &f ) )
    {
        lexer->L_ObjectAttributes = ( T_FLOAT | KNOWN_OBJECT ) ;
        SetState ( lexer, KNOWN_OBJECT, true ) ;
        Lexer_ParseBigNum ( lexer, token ) ;
    }
    else Lexer_ParseAsAString ( lexer ) ;
}

void
Lexer_ParseObject ( Lexer * lexer, byte * token )
{
    int64 offset = 0 ;
    lexer->OriginalToken = token ;
    lexer->Literal = 0 ;
    lexer->L_MorphismAttributes = 0 ;
    lexer->L_ObjectAttributes = 0 ;
    if ( token )
    {
        if ( ( token [0] == '0' ) || ( token [0] == '#' ) ) // 0x or #x following scheme notation
        {
            char c ;
            if ( ( c = tolower ( token [1] ) ) == 'x' )
            {
                token [1] = c ;
                _Lexer_ParseHex ( lexer, token ) ; // #x
                return ;
            }
            else if ( ( c = tolower ( token [1] ) ) == 'b' )
            {
                //if ( token [0] == '#' ) // following scheme notation
                {
                    offset = 2 ;
                    Lexer_ParseBinary ( lexer, token, offset ) ; // #b or 0b
                    return ;
                }
            }
            else if ( tolower ( token [1] ) == 'd' )
            {
                _Lexer_ParseDecimal ( lexer, token ) ; // 0d or #d
                return ;
            }
            //else if ( tolower ( token [1] ) == 'o' ) goto doOctal ; // #o
        }
        if ( NUMBER_BASE_GET ( ) == 10 ) _Lexer_ParseDecimal ( lexer, token ) ;
        else if ( NUMBER_BASE_GET ( ) == 2 ) Lexer_ParseBinary ( lexer, token, 0 ) ;
        else if ( NUMBER_BASE_GET ( ) == 16 ) _Lexer_ParseHex ( lexer, token ) ;
    }
}

byte *
Parse_Macro ( int64 type )
{
    byte * value ;
    Lexer * lexer = _Context_->Lexer0 ;
    if ( type == STRING_MACRO )
    {
        value = Lexer_ReadToken ( lexer ) ;
        while ( ! String_Equal ( ";", ( char* ) Lexer_ReadToken ( lexer ) ) ) ; // nb. we take only the first string all else ignored
    }
    else if ( type == TEXT_MACRO ) // until ';' or ' ' 
    {
        int64 n = 0 ;
        byte nc, *buffer = Buffer_DataCleared ( _CSL_->ScratchB1 ) ;
        buffer [0] = 0 ;
        do
        {
            nc = _ReadLine_GetNextChar ( _Context_->ReadLiner0 ) ;
            if ( ( nc == ';' ) || ( nc == ' ' ) )
            {
                buffer [ n ] = 0 ;
                break ;
            }
            buffer [ n ++ ] = nc ;
        }
        while ( nc ) ;
        value = String_New ( ( byte* ) buffer, TEMPORARY ) ;
        //Buffer_SetAsUnused ( b ) ;
    }
    return value ;
}

byte *
Lexer_ParseTerminatingMacro ( Lexer * lexer, byte termChar, Boolean stringFlag )
{
    ReadLiner * rl = _ReadLiner_ ;
    byte * token ;
    //Word * word = 0 ;
    if ( ( ! ( GetState ( _Compiler_, ( COMPILE_MODE | ASM_MODE | LC_ARG_PARSING | LC_CSL ) ) ) ) && ( ! GetState ( _CSL_, SOURCE_CODE_STARTED ) ) )
        CSL_InitSourceCode_WithCurrentInputChar ( _CSL_, 0 ) ;
    _CSL_->SC_QuoteMode = true ;
start:
    do
    {
        lexer->TokenInputByte = ReadLine_NextChar ( rl ) ;
        if ( lexer->TokenInputByte == '\\' ) _BackSlash ( lexer, 1 ) ;
        else Lexer_Append_ConvertedCharacterToTokenBuffer ( lexer ) ;
    }
    while ( lexer->TokenInputByte != termChar ) ;
    _AppendCharacterToTokenBuffer ( lexer, 0 ) ; // null terminate TokenBuffer
    SetState ( lexer, LEXER_DONE, true ) ;
    _CSL_->SC_QuoteMode = false ;
    CSL_StringMacros_Do ( lexer->TokenBuffer ) ;
    token = String_New ( lexer->TokenBuffer, STRING_MEM ) ;
    if ( Lexer_StringMacros_Do ( lexer ) ) goto start ;
    if ( termChar == '\"' )
    {
        if ( stringFlag )
        {
            token[ strlen ( token ) - 1 ] = 0 ;
            return &token [1] ;
        }
    }
    return token ;
}

int64
_CSL_ParseQid_Token ( byte * token0 )
{
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    Finder * finder = cntx->Finder0 ;
    int64 nst ;
    Word * word = 0, *ns = 0 ;
    byte * token = token0 ; // finderToken is also a flag of the caller : (finderToken > 0) ? finder : parser
    while ( 1 )
    {
        if ( ! token )
        {
            token = Lexer_ReadToken ( lexer ) ;
            if ( token && ( token [0] == '.' ) ) token = Lexer_ReadToken ( lexer ) ;
        }
        if ( token && _Lexer_IsTokenForwardDotted ( lexer, 0 ) )
        {
            if ( token0 ) cntx->BaseObject = 0 ;
            if ( ns ) word = _Finder_FindWord_InOneNamespace ( _Finder_, ns, token ) ;
            else word = Finder_Word_FindUsing ( _Finder_, token, 1 ) ; // maybe need to respect a possible qualifying namespace ??
            if ( word ) nst = word->W_ObjectAttributes ;
            if ( word && ( nst & THIS ) )
            {
                ns = word ;
                _Context_SetAsQidInNamespace ( ns ) ;
            }
                //else if ( word && ( nst = word->W_ObjectAttributes & ( token0 ? ( NAMESPACE_TYPE | THIS ) : ( C_TYPE | C_CLASS | NAMESPACE | THIS ) ) ) )
            else if ( word && ( nst & ( token0 ? ( THIS ) : ( C_TYPE | C_CLASS | STRUCT | OBJECT | THIS ) ) ) )
            {
                ns = word ;
                Finder_SetQualifyingNamespace ( finder, ns ) ;
                _Context_SetAsQidInNamespace ( ns ) ;

            }
            else if ( token0 )
            {
                //if ( ! nst ) _CSL_Do_Dot ( cntx, word ) ;
                break ;
            }
            token = 0 ;
        }
        else break ;
    }
#if 0    
    if ( ns )
    {
        //word = _Finder_FindWord_InOneNamespace ( _Finder_, ns, token ) ;
        if ( token0 ) return ( int64 ) word ;
        else return ( int64 ) token ; //( word ? word->Name : token ) ;
    }
    else
#endif        
    {
        if ( token0 ) return ( int64 ) word ;
        else return ( int64 ) token ;
    }
}

// _CSL_SingleQuote : ?? seems way to complicated and maybe should be integrated with Lexer_ParseObject

void
_CSL_SingleQuote ( )
{
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    Compiler * compiler = cntx->Compiler0 ;
    Word *word, * sqWord = CSL_WordList ( 0 ) ; //single quote word
    byte buffer [5], *buf = buffer ;
    byte c0, c1, c2 ;
    uint64 charLiteral = 0 ;

    _CSL_->SC_QuoteMode = true ;
    if ( ( ! ( GetState ( compiler, ( COMPILE_MODE | ASM_MODE | LC_ARG_PARSING | LC_CSL ) ) ) )
        && ( ! GetState ( _CSL_, SOURCE_CODE_STARTED ) ) ) CSL_InitSourceCode_WithCurrentInputChar ( _CSL_, 0 ) ;
    c0 = _ReadLine_PeekOffsetChar ( rl, 0 ) ; // parse a char type, eg. 'c' 
    c1 = _ReadLine_PeekOffsetChar ( rl, 1 ) ;
    if ( sqWord && sqWord->Name[0] == '\'' && ( c1 == '\'' ) || ( c0 == '\\' ) ) // parse a char type, eg. 'c' 
    {
        // notation :: c0 = original ' ; c1 = next char, etc.
        c0 = _ReadLine_GetNextChar ( rl ) ;
        c1 = _ReadLine_GetNextChar ( rl ) ;
        buffer[0] = '\'' ;
        buffer[1] = c0, _CSL_AppendCharToSourceCode ( _CSL_, c0 ) ;
        if ( c0 == '\\' )
        {

            c2 = _ReadLine_GetNextChar ( rl ) ; // the closing '\''
            if ( c1 == 't' ) charLiteral = 0x9 ;
            else if ( c1 == 'n' ) charLiteral = 0xa ;
            else if ( c1 == 'r' ) charLiteral = 0xd ;
            else if ( c1 == 'b' ) charLiteral = 0x8 ;
            buffer[2] = c1, _CSL_AppendCharToSourceCode ( _CSL_, c1 ) ;
            ;
            buffer[3] = '\'', _CSL_AppendCharToSourceCode ( _CSL_, '\'' ) ; // c3
            buffer[4] = 0 ;
        }
        else
        {
            charLiteral = c0 ;
            buffer[2] = '\'', _CSL_AppendCharToSourceCode ( _CSL_, '\'' ) ; // c2
            buffer[3] = 0 ;
        }
        CSL_WordLists_PopWord ( ) ; // pop the "'" token
        word = _DObject_New ( buf, charLiteral, IMMEDIATE, LITERAL | CONSTANT, 0, LITERAL, ( byte* ) _DataObject_Run, 0, 0, 0, DICTIONARY ) ;
        word->CompiledDataFieldByteSize = 1 ;
        Interpreter_DoWord ( _Interpreter_, word, - 1, - 1 ) ; //_Lexer_->TokenStart_ReadLineIndex ) ;
    }
    else
    {
        if ( ( ! Compiling ) && ( ! ( GetState ( compiler, DOING_CASE ) ) ) ) CSL_InitSourceCode_WithName ( _CSL_, lexer->OriginalToken, 0 ) ;
        byte * token = ( byte* ) _CSL_ParseQid_Token ( 0 ) ;
        DataStack_Push ( ( int64 ) token ) ;
        if ( ( ! AtCommandLine ( rl ) ) && ( ! GetState ( _CSL_, SOURCE_CODE_STARTED ) ) )
            CSL_InitSourceCode_WithName ( _CSL_, token, 0 ) ;
    }
    _CSL_->SC_QuoteMode = false ;
}

void
CSL_CheckDo_KeywordOperand ( )
{
    Compiler * compiler = _Compiler_ ;
    Word * word ;
    byte *token1, * token = Lexer_ReadToken ( _Lexer_ ) ; // remember this was a peeked word
    if ( token[0] == '\"' )
    {
        token1 = Lexer_ParseTerminatingMacro ( _Lexer_, '\"', 0 ) ;
        word = Lexer_ParseToken_ToWord ( _Lexer_, token1, - 1, - 1 ) ;
    }
    else
    {
        word = Interpreter_InterpretAToken ( _Interpreter_, token, - 1, - 1 ) ;
        if ( token[0] == '\'' ) word = _CSL_Literal ( ) ;
    }
    //if ( ( token[0] != '@' ) && ( ! compiler->ReturnLParenOperandWord ) ) compiler->ReturnLParenOperandWord = word ;
    if ( ( ! ( word->W_MorphismAttributes & DEBUG_WORD ) ) && ( token[0] != '@' ) ) compiler->ReturnLParenOperandWord = word ;
}

Word *
CSL_Parse_Interpret_KeywordOperand ( Word * word, Boolean otherwiseFlag )
{
    Compiler * compiler = _Compiler_ ;
    byte * token = 0 ;
    compiler->ReturnLParenOperandWord = 0 ;
    if ( word && word->W_MorphismAttributes & T_TOS ) return 0 ; // && ( GetState ( compiler, RETURN_TOS ) ) ) return 0 ;
    if ( word && ( word->Name [0] == '(' ) ) // remember this was a peeked word
    {
        token = Lexer_ReadToken ( _Lexer_ ) ; // remember this was a peeked word
        while ( token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ), ( token [0] != ')' ) ) CSL_CheckDo_KeywordOperand ( ) ;
        //Lexer_ReadToken ( _Lexer_ ) ; // read final ')' which was just peeked
    }
    else if ( otherwiseFlag ) //&& ( ! ( word->W_MorphismAttributes & ( T_TOS ) ) ) )
    {
        while ( token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ), ( ( token [0] != '{' ) && ( token [0] != '}' ) && ( token [0] != ';' ) ) )
            CSL_CheckDo_KeywordOperand ( ) ;
    }
    //if (token) Lexer_ReadToken ( _Lexer_ ) ; // read final ')' which was just peeked
    //if ( compiler->ReturnLParenOperandWord ) 
    return compiler->ReturnLParenOperandWord ? compiler->ReturnLParenOperandWord : word ;
    //else return word ;
}


