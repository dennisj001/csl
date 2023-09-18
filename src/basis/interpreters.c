
#include "../include/csl.h"

// some of these could probably be combined
byte *
Interpret_C_Until_NotIncluding_Token5 ( Interpreter * interp, byte * end1, byte * end2, byte* end3, byte* end4, byte* end5, byte * delimiters, Boolean newlineBreakFlag, Boolean charsFlag )
{
    byte * token ;
    int64 inChar ;
    Lexer * lexer = _Lexer_ ;
    do
    {
        if ( newlineBreakFlag && ReadLine_AreWeAtNewlineAfterSpaces ( _ReadLiner_ ) )
        {
            token = 0 ;
            break ;
        }
        token = _Lexer_ReadToken ( lexer, delimiters ) ;
        List_CheckInterpretLists_OnVariable ( _Compiler_->PostfixLists, token ) ;
        if ( charsFlag )
        {
            if ( ( token[0] == end1[0] ) || ( token[0] == end2[0] ) || ( token[0] == end3[0] ) || ( token[0] == end4[0] ) || ( token[0] == end5[0] ) ) break ;
        }
        else if ( String_Equal ( token, end1 ) || String_Equal ( token, end2 ) || String_Equal ( token, end3 ) 
            || String_Equal ( token, end4 ) || String_Equal ( token, end5 ) ) break ;
        if ( GetState ( _Compiler_, DOING_A_PREFIX_WORD ) && ( token[0] == ')' ) )
        {
            Interpreter_InterpretAToken ( interp, token, lexer->TokenStart_ReadLineIndex, lexer->SC_Index ) ;
            token = 0 ;
            break ;
        }
        else if ( GetState ( _Context_, C_SYNTAX ) && ( ( token[0] == ',' ) || ( token[0] == ';' ) ) )
        {
            CSL_ArrayModeOff_OptimizeOn ( ) ;
            break ;
        }
        else Interpreter_InterpretAToken ( interp, token, lexer->TokenStart_ReadLineIndex, lexer->SC_Index ) ;
        inChar = ReadLine_PeekNextChar ( _Context_->ReadLiner0 ) ;
        if ( ( inChar == 0 ) || ( inChar == - 1 ) || ( inChar == eof ) ) token = 0 ;
    }
    while ( token ) ;
    if ( token ) CSL_PushToken_OnTokenList ( token ) ;
    return token ;
}

byte *
Interpret_Until_Token ( Interpreter * interp, byte * end, byte * delimiters )
{
    byte * token ;
    int64 inChar ;
    Lexer * lexer = _Lexer_ ;
    do
    {
        token = _Lexer_ReadToken ( lexer, delimiters ) ;
        if ( String_Equal ( token, end ) )
        {
            if ( GetState ( _Compiler_, C_COMBINATOR_LPAREN ) && ( _String_EqualSingleCharString ( token, ';' ) ) ) CSL_PushToken_OnTokenList ( token ) ;
            break ;
        }
            //else if ( _String_EqualSingleCharString ( token, ',' ) ) continue ;
        else if ( _String_EqualSingleCharString ( token, ';' ) && GetState ( _Context_, C_SYNTAX ) && GetState ( _Compiler_, C_COMBINATOR_PARSING ) )
        {
            CSL_PushToken_OnTokenList ( token ) ;
            CSL_ArrayModeOff_OptimizeOn ( ) ;
            break ;
        }
        else Interpreter_InterpretAToken ( interp, token, lexer->TokenStart_ReadLineIndex, lexer->SC_Index ) ;
        inChar = ReadLine_PeekNextChar ( _Context_->ReadLiner0 ) ;
        if ( ( inChar == 0 ) || ( inChar == - 1 ) || ( inChar == eof ) ) token = 0 ;
    }
    while ( token ) ;
    return token ;
}

Word *
Interpret_DoPrefixFunction_OrUntil_RParen ( Interpreter * interp, Word * prefixFunction )
{
    Word * word = 0 ;
    if ( prefixFunction )
    {
        byte * token ;
        int64 i, flag = 0 ;
        Compiler * compiler = interp->Compiler0 ;
        while ( 1 )
        {
            token = Lexer_ReadToken ( interp->Lexer0 ) ; // skip the opening left paren
            if ( token && ( ! _String_EqualSingleCharString ( token, '(' ) ) )
            {
                if ( word = Finder_Word_FindUsing ( interp->Finder0, token, 1 ) )
                {
                    if ( word->W_MorphismAttributes & DEBUG_WORD ) continue ;
                    else flag = 1 ;
                    break ;
                }
            }
            else break ;
        }
        d0 ( if ( Is_DebugModeOn ) _CSL_SC_WordList_Show ( "\n_Interpret_PrefixFunction_Until_RParen", 0, 0 ) ) ;
        if ( prefixFunction->W_NumberOfPrefixedArgs )
        {
            SetState ( compiler, PREFIX_ARG_PARSING, true ) ;
            Interpreter_InterpretAToken ( interp, token, - 1, - 1 ) ;
            for ( i = 0 ; i < ( prefixFunction->W_NumberOfPrefixedArgs - 1 ) ; i ++ ) // -1 : we already did one above
            {
                byte * token = Lexer_ReadToken ( interp->Lexer0 ) ;
                if ( _String_EqualSingleCharString ( token, ',' ) && GetState ( _Context_, ASM_SYNTAX ) ) i -- ; // don't count it 
                Interpreter_InterpretAToken ( interp, token, - 1, - 1 ) ;
            }
            SetState ( compiler, PREFIX_ARG_PARSING, false ) ;
        }
        else
        {
            if ( flag ) Interpreter_InterpretAToken ( interp, token, - 1, - 1 ) ;
            else Interpret_Until_Token ( interp, ( byte* ) ")", ( byte* ) " ,\n\r\t" ) ;
        }
        SetState ( compiler, ( DOING_BEFORE_A_PREFIX_WORD ), false ) ;
        if ( ! GetState ( _Debugger_, DBG_INFIX_PREFIX ) ) word = Interpreter_DoWord_Default ( interp, prefixFunction, prefixFunction->W_RL_Index, prefixFunction->W_SC_Index ) ;
        SetState ( compiler, ( PREFIX_ARG_PARSING | DOING_A_PREFIX_WORD ), false ) ;
        SetState ( _Debugger_, DBG_INFIX_PREFIX, false ) ;
    }
    return word ;
}

void
_Interpret_Until_Including_Token ( Interpreter * interp, byte * end, byte * delimiters )
{
    byte * token ;
    while ( token = _Lexer_ReadToken ( interp->Lexer0, delimiters ) )
    {
        Interpreter_InterpretAToken ( interp, token, - 1, - 1 ) ;
        if ( String_Equal ( ( char* ) token, end ) ) break ;
    }
}

void
Interpret_UntilFlagged ( Interpreter * interp, int64 doneFlags )
{
    do Interpreter_InterpretNextToken ( interp ) ;
    while ( ( ! Interpreter_IsDone ( interp, doneFlags ) ) ) ;
}

void
Interpret_UntilFlagged2 ( Interpreter * interp, int64 doneFlags )
{
    do Interpreter_InterpretSelectedTokens ( interp, "#" ) ;
    while ( ( ! Interpreter_IsDone ( interp, doneFlags ) ) ) ;
}

void
Interpret_ToEndOfLine ( Interpreter * interp )
{
    ReadLiner * rl = interp->ReadLiner0 ;
    do
    {
        if ( GetState ( interp->Lexer0, ( END_OF_LINE | END_OF_STRING ) ) ) break ; // either the lexer will get a newline or the readLiner
        if ( ReadLine_AreWeAtNewlineAfterSpaces ( rl ) ) break ;
        Interpreter_InterpretNextToken ( interp ) ;
    }
    while ( ( ! Interpreter_IsDone ( interp, END_OF_FILE | END_OF_LINE | END_OF_STRING ) ) ) ;
}

void
Interpret_UntilFlaggedWithInit ( Interpreter * interp, int64 doneFlags )
{
    _Interpreter_Init ( interp ) ;
    Interpret_UntilFlagged ( interp, doneFlags ) ;
}

void
Interpret_UntilFlagged2WithInit ( Interpreter * interp, int64 doneFlags )
{
    _Interpreter_Init ( interp ) ;
    Interpret_UntilFlagged2 ( interp, doneFlags ) ;
}

void
Interpret_String ( byte *str )
{
    _CSL_ContextNew_InterpretString ( _CSL_, str ) ;
}

void
_CSL_Interpret ( CSL * csl )
{
    int loopTimes = 0 ;
    do
    {
        if ( ! _AtCommandLine ( ) ) _CSL_Init_SessionCore ( csl, 1, 1 ) ;
        else CSL_Prompt (csl, loopTimes++ < 2, 1 ) ;
        Context_Interpret ( csl->Context0 ) ;
    }
    while ( GetState ( csl, CSL_RUN ) ) ;
}

void
CSL_InterpreterRun ( )
{
    _CSL_Interpret ( _CSL_ ) ;
}

