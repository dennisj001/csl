#include "../../include/csl.h"

//===================================================================================================================
//| _LO_Read 
//===================================================================================================================

ListObject *
LC_Read ()
{
    LambdaCalculus * lc = _LC_ ;
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    ListObject *l0, *lnew ;
    byte * token ;
    int64 qidFlag ;
    SetState ( lc, LC_READ, true ) ;
    if ( lc->ParenLevel ) // if ParenLevel == 0 we let LParen set up the list
    {
        lnew = LO_New ( LIST, 0 ) ;
        lnew->State = lc->QuoteState ;
    }
    else
    {
        lnew = 0 ;
    }
    //if ( lc->L0 ) rl->ReadIndex = lc->L0->W_RL_Index ; // if we had a tab completion within the parenthesis ReadIndex is affected
    //if (startReadIndex != -1)  rl->ReadIndex = startReadIndex ;
    //    rl->ReadIndex = (startReadIndex == -1) ? rl->ReadIndex : startReadIndex ;
    d0 ( if ( Is_DebugModeOn ) LO_Debug_ExtraShow ( 0, 2, 0, ( byte* ) "\nEntering _LO_Read..." ) ) ;
    do
    {
        //if ( Is_DebugModeOn ) CSL_PrintDataStack ( ) ;
        token = _Lexer_ReadToken ( lexer, ( byte* ) " ,\n\r\t" ) ;
        if ( Lexer_IsTokenQualifiedID ( lexer ) ) SetState ( cntx, CONTEXT_PARSING_QID, true ) ;
        else SetState ( cntx, CONTEXT_PARSING_QID, false ) ;
        qidFlag = GetState ( cntx, CONTEXT_PARSING_QID ) ;
        if ( token )
        {
            if ( _String_EqualSingleCharString ( token, ')' ) )
            {
                lc->ParenLevel -- ;
                break ;
            }
            else if ( l0 = LO_Read_DoToken (token, qidFlag, lexer->TokenStart_ReadLineIndex, lexer->SC_Index ) )
            {
                l0->State |= ( lc->ItemQuoteState | lc->QuoteState ) ;
                lc->ItemQuoteState = 0 ;
                if ( lnew )
                {
                    if ( ( l0->State & SPLICE ) || ( ( l0->State & UNQUOTE_SPLICE ) &&
                        ( ! ( l0->State & QUOTED ) ) ) ) LO_SpliceAtTail ( lnew, LC_Eval ( l0, 0, 1 ) ) ;
                    else LO_AddToTail ( lnew, l0 ) ;
                }
                else lnew = l0 ;
            }
        }
        else if ( GetState ( lexer, END_OF_FILE | END_OF_STRING ) ) break ;
        //else _SyntaxError ( ( byte* ) "\n_LO_Read : Syntax error : no token?\n", QUIT ) ;
        //if ( Is_DebugModeOn ) CSL_PrintDataStack ( ) ;
    }
    while ( lc->ParenLevel ) ;
    SetState ( lc, LC_READ, false ) ;
    SetState ( cntx->Finder0, QID, false ) ;
    if ( ( ! lc->ParenLevel ) && ( ! GetState ( _Compiler_, LC_ARG_PARSING ) ) ) LC_FinishSourceCode ( ) ;
    return lnew ;
}

ListObject *
_LO_Read_Do_LParen ()
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *l0 ;
    lc->QuoteState |= lc->ItemQuoteState ;
    Stack_Push ( lc->QuoteStateStack, lc->QuoteState ) ;
    //lc->QuoteState = 0 ;
    lc->ParenLevel ++ ;
    l0 = LC_Read () ;
    lc->QuoteState = Stack_Pop ( lc->QuoteStateStack ) ;
    return l0 ;
}

ListObject *
_LO_Read_DoWord (Word * word, int64 qidFlag, int64 tsrli, int64 scwi )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *l0 = 0 ;
    byte *token1 ;
    SetState ( word, QID, qidFlag ) ;
    int64 allocType = GetState ( _Compiler_, LC_ARG_PARSING ) ? DICTIONARY : LISP ;
    if ( ( word->W_LispAttributes & ( T_LISP_READ_MACRO | T_LISP_IMMEDIATE ) ) && ( ! GetState ( _LC_, LC_READ_MACRO_OFF ) ) )
    {
        Word_Eval ( word ) ;
        if ( word->W_LispAttributes & T_LISP_SPECIAL )
        {
            l0 = DataObject_New ( T_LC_NEW, word, 0, word->W_MorphismAttributes, word->W_ObjectAttributes,
                T_LISP_SYMBOL | word->W_LispAttributes, 0, word->Lo_Value, 0, allocType, tsrli, scwi ) ;
        }
    }
    else if ( word->W_LispAttributes & T_LISP_TERMINATING_MACRO )
    {
        SetState ( lc, ( LC_READ ), false ) ; // let the value be pushed in this case because we need to pop it below
        Word_Eval ( word ) ;
        token1 = ( byte * ) DataStack_Pop ( ) ;
        SetState ( lc, ( LC_READ ), true ) ;
        l0 = DataObject_New ( T_LC_LITERAL, 0, token1, 0, LITERAL | word->W_ObjectAttributes, word->W_LispAttributes, 0, _Lexer_->Literal, 0, allocType, tsrli, scwi ) ;
    }
    else
    {
        if ( ( ! GetState ( _Compiler_, LC_ARG_PARSING ) ) && ( ( word->W_ObjectAttributes & STRUCT ) || Lexer_IsTokenForwardDotted ( _Context_->Lexer0 ) ) )
        {
            Set_CompileMode ( true ) ;
            Word_ObjectRun ( word ) ;
            Set_CompileMode ( false ) ;
        }
        l0 = DataObject_New ( T_LC_NEW, word, word->Name, word->W_MorphismAttributes, word->W_ObjectAttributes,
            ( T_LISP_SYMBOL | word->W_LispAttributes ), 0, word->Lo_Value, 0, allocType, tsrli, scwi ) ;
        if ( word->W_ObjectAttributes & NAMESPACE_TYPE ) Namespace_Do_Namespace ( word ) ;
    }
    return l0 ;
}

ListObject *
_LO_Read_DoToken (byte * token, int64 qidFlag, int64 tsrli, int64 scwi )
{
    LambdaCalculus * lc = _LC_ ;
    Context *cntx = _Context_ ;
    Lexer *lexer = cntx->Lexer0 ;
    ListObject *l0 = 0 ;
    Word *word ;
    //if ( Is_DebugModeOn ) CSL_PrintDataStack ( ) ;
    if ( qidFlag ) SetState ( cntx->Finder0, QID, true ) ;
    word = LC_FindWord ( token ) ;
    if ( qidFlag ) SetState ( cntx->Finder0, QID, false ) ;
    if ( word ) l0 = _LO_Read_DoWord (word, qidFlag, tsrli, scwi ) ;
    else
    {
        int64 allocType = GetState ( _Compiler_, LC_ARG_PARSING ) ? DICTIONARY : LISP ;
        Lexer_ParseObject ( lexer, token ) ;
        l0 = DataObject_New ( T_LC_LITERAL, 0, token, 0, 0, 0, qidFlag, lexer->Literal, 0, allocType, tsrli, scwi ) ;
    }
    if ( l0 )
    {
        l0->State |= ( lc->ItemQuoteState | lc->QuoteState ) ;
        lc->ItemQuoteState = 0 ;
    }
    //if ( Is_DebugModeOn ) CSL_PrintDataStack ( ) ;
    return l0 ;
}

ListObject *
LO_Read_DoToken (byte * token, int64 qidFlag, int64 tsrli, int64 scwi )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *l0 = 0 ;
    //if ( Is_DebugOn ) _Printf ( ( byte * ) "\n_LO_Read : \'%s\' scwi = %d", token, scwi ) ;
    if ( String_Equal ( ( char * ) token, ( byte * ) "/*" ) ) CSL_ParenthesisComment ( ) ;
    else if ( String_Equal ( ( char * ) token, ( byte * ) "//" ) ) CSL_CommentToEndOfLine ( ) ;
    else if ( _String_EqualSingleCharString ( token, '(' ) ) l0 = _LO_Read_Do_LParen () ;
    else l0 = _LO_Read_DoToken (token, qidFlag, tsrli, scwi ) ;
    return l0 ;
}

void
LC_QuoteQuasiQuoteRepl ( uint64 itemQuoteState, Boolean doReplFlag )
{
    LambdaCalculus * lc = _LC_ ;
    Boolean replFlag = false ;
    if ( ! lc )
    {
        replFlag = true ;
        lc = LC_New ( ) ;
    }
    if ( ! GetState ( _Compiler_, LISP_MODE ) ) replFlag = true ;
    lc->ItemQuoteState |= itemQuoteState ;
    if ( replFlag && doReplFlag )
    {
        byte nextChar = ReadLine_PeekNextNonWhitespaceChar ( _ReadLiner_ ) ;
        if ( ( nextChar == '(' ) ) _LC_ReadEvalPrint_ListObject ( 0, 0, itemQuoteState ) ;
    }
}

void
LO_Quote ( )
{
    LC_QuoteQuasiQuoteRepl ( QUOTED, 1 ) ;
}

void
LO_QuasiQuote ( )
{
    LC_QuoteQuasiQuoteRepl ( QUASIQUOTED, 1 ) ;
}

void
LO_UnQuoteSplicing ( )
{
    LC_QuoteQuasiQuoteRepl ( UNQUOTE_SPLICE, 1 ) ;
}

void
LO_Splice ( )
{
    LC_QuoteQuasiQuoteRepl ( SPLICE, 1 ) ;
}

void
LO_UnQuote ( )
{
    LC_QuoteQuasiQuoteRepl ( UNQUOTED, 1 ) ;
}

void
LO_SpliceAtTail ( ListObject * lnew, ListObject * l0 )
{
    ListObject * l1, *lnext ;
    if ( lnew )
    {
        for ( l1 = LO_First ( l0 ) ; l1 ; l1 = lnext )
        {
            lnext = LO_Next ( l1 ) ;
            LO_AddToTail ( lnew, LO_CopyOne ( l1 ) ) ; // nb. LO_CopyOne is necessary here
        }
    }
}



