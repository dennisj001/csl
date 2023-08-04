#include "../../include/csl.h"
//===================================================================================================================
//| LO_SpecialFunction(s) 
//===================================================================================================================

ListObject *
_LO_Definec ( ListObject * l0 )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *value, *l1, *value1, *l2, *lnext ;
    Word * word, *idLo ;
    SetState ( _CSL_, _DEBUG_SHOW_, 1 ) ;
    if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( l0, "\n_LO_Define : l0 = ", "", 1 ) ;
    if ( ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) ) // scheme 'define : ( define ( func args) funcBody )
    {
        idLo = _LO_First ( l0 ) ;
        ListObject * value0, *lambda ;
        value = LO_List_New ( LISP ) ;
        value0 = _LO_Next ( l0 ) ;
        LO_AddToHead ( value, value0 ) ; // body
        value1 = LO_List_New ( LISP ) ;
        for ( l2 = _LO_Next ( idLo ) ; l2 ; l2 = lnext )
        {
            lnext = _LO_Next ( l2 ) ;
            LO_AddToTail ( value1, l2 ) ;
            l2->W_ObjectAttributes |= LOCAL_VARIABLE ;
        }
        value1->W_LispAttributes |= ( LIST | LIST_NODE ) ; 
        LO_AddToHead ( value, value1 ) ;
        lambda = _LO_Read_DoToken ( _LC_, "lambda", 0, - 1, - 1 ) ;
        LO_AddToHead ( value, lambda ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( idLo, "\n_LO_Define : idLo = ", "", 1 ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value0, "\n_LO_Define : value0 = ", "", 1 ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value, "\n_LO_Define : value = ", "", 1 ) ;
        //value->W_LispAttributes |= T_LAMBDA ;
        //locals1 = 0 ;
    }
    else idLo = l0, value = _LO_Next ( l0 ) ;
    idLo = idLo->Lo_CSL_Word ;
    word = DataObject_New ( T_LC_DEFINE, 0, ( byte* ) idLo->Name, 0, NAMESPACE_VARIABLE, 0, 0, 0, 0, LISP, l0->W_RL_Index, l0->W_SC_Index ) ;
    CSL_WordList_Init ( word ) ;
    word->Definition = 0 ; // reset the definition from LO_Read
    _Context_->CurrentWordBeingCompiled = word ;
    word->Lo_CSL_Word = word ;
    Namespace_DoAddWord ( lc->Locals ? lc->Locals : lc->LispDefinesNamespace, word ) ; // put it at the beginning of the list to be found first
    if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( idLo, "\n_LO_Define : idLo = ", "", 1 ) ;
    if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value, "\n_LO_Define : value = ", "", 1 ) ;
    value = LC_Eval ( value, 0, 0 ) ; // 0 : don't apply
    if ( ( value && ( value->W_LispAttributes & T_LAMBDA ) ) )
    {
        value->Lo_LambdaParameters = _LO_Copy ( value->Lo_LambdaParameters, LISP ) ;
        value->Lo_LambdaBody = _LO_Copy ( value->Lo_LambdaBody, LISP ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( idLo, "\n_LO_Define : function = ", "", 1 ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value->Lo_LambdaParameters, "\n_LO_Define : value->Lo_LambdaParameters = ", "", 1 ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value->Lo_LambdaBody, "\n_LO_Define : value->Lo_LambdaBody = ", "", 1 ) ;
    }
    else value = _LO_Copy ( value, LISP ) ; // this value object should now become part of LISP non temporary memory
    SetState ( _CSL_, _DEBUG_SHOW_, false ) ;
    word->Lo_Value = ( uint64 ) value ; // used by eval
    word->W_LispAttributes |= ( T_LC_DEFINE | T_LISP_SYMBOL ) ;
    word->State |= LC_DEFINED ;

    // the value was entered into the LISP memory, now we need a temporary carrier for LO_Print : yes, apparently, but why?
    l1 = DataObject_New ( T_LC_NEW, 0, word->Name, word->W_MorphismAttributes,
        word->W_ObjectAttributes, word->W_LispAttributes, 0, ( int64 ) value, 0, LISP, - 1, - 1 ) ; // all words are symbols
    l1->W_LispAttributes |= ( T_LC_DEFINE | T_LISP_SYMBOL ) ;
    l1->W_OriginalCodeText = word->W_OriginalCodeText = lc->LC_SourceCode ;
    if ( LC_CompileMode ) l1->W_SC_WordList = word->W_SC_WordList = _LC_->Lambda_SC_WordList ;
    SetState ( lc, ( LC_DEFINE_MODE ), false ) ;
    _CSL_FinishWordDebugInfo ( l1 ) ;
    _Word_Finish ( l1 ) ;
    //word->W_SC_WordList = _CSL_->CSL_N_M_Node_WordList ;
    return l1 ;
}

ListObject *
_LO_Define ( ListObject * l0 )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *value, *l1, *locals1 = 0, *value1, *l2, *lnext ;
    Word * word, *idLo ;
    SetState ( _CSL_, _DEBUG_SHOW_, LC_DEFINE_DBG ) ;
    if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( l0, "\n_LO_Define : l0 = ", "", 1 ) ;
    if ( ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) ) // scheme 'define : ( define ( func args) funcBody )
    {
        idLo = _LO_First ( l0 ) ;
        value1 = LO_List_New ( LISP ) ;
        for ( l2 = _LO_Next ( idLo ) ; l2 ; l2 = lnext )
        {
            lnext = _LO_Next ( l2 ) ;
            LO_AddToTail ( value1, l2 ) ;
            l2->W_ObjectAttributes |= LOCAL_VARIABLE ;
        }
        value1->W_LispAttributes |= ( LIST | LIST_NODE ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( locals1, "\n_LO_Define : locals1 = ", "", 1 ) ;
        locals1 = value1 ;
    }
    else idLo = l0 ;
    value = _LO_Next ( l0 ) ;
    idLo = idLo->Lo_CSL_Word ;
    word = DataObject_New ( T_LC_DEFINE, 0, ( byte* ) idLo->Name, 0, NAMESPACE_VARIABLE, 0, 0, 0, 0, LISP, l0->W_RL_Index, l0->W_SC_Index ) ;
    CSL_WordList_Init ( word ) ;
    word->Definition = 0 ; // reset the definition from LO_Read
    _Context_->CurrentWordBeingCompiled = word ;
    word->Lo_CSL_Word = word ;
    SetState ( lc, ( LC_DEFINE_MODE ), true ) ;
    Namespace_DoAddWord ( lc->Locals ? lc->Locals : lc->LispDefinesNamespace, word ) ; // put it at the beginning of the list to be found first
    if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( idLo, "\n_LO_Define : idLo = ", "", 1 ) ;
    if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value, "\n_LO_Define : value = ", "", 1 ) ;
    if ( locals1 )
    {
        word->Lo_LambdaParameters = _LO_Copy ( ( ListObject* ) locals1, LISP ) ;
        word->Lo_LambdaBody = _LO_Copy ( value, LISP ) ;
        word->W_LispAttributes |= T_LAMBDA ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( idLo, "\n_LO_Define : function = ", "", 1 ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value->Lo_LambdaParameters, "\n_LO_Define : value->Lo_LambdaParameters = ", "", 1 ) ;
        if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value->Lo_LambdaBody, "\n_LO_Define : value->Lo_LambdaBody = ", "", 1 ) ;
    }
    else
    {
        value = LC_Eval ( value, 0, 0 ) ; // 0 : don't apply
        if ( ( value && ( value->W_LispAttributes & T_LAMBDA ) ) )
        {
            value->Lo_LambdaParameters = _LO_Copy ( value->Lo_LambdaParameters, LISP ) ;
            value->Lo_LambdaBody = _LO_Copy ( value->Lo_LambdaBody, LISP ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( idLo, "\n_LO_Define : function = ", "", 1 ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value->Lo_LambdaParameters, "\n_LO_Define : value->Lo_LambdaParameters = ", "", 1 ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( value->Lo_LambdaBody, "\n_LO_Define : value->Lo_LambdaBody = ", "", 1 ) ;
        }
        else value = _LO_Copy ( value, LISP ) ; // this value object should now become part of LISP non temporary memory
    }
    SetState ( _CSL_, _DEBUG_SHOW_, false ) ;
    word->Lo_Value = ( uint64 ) value ; // used by eval
    word->W_LispAttributes |= ( T_LC_DEFINE | T_LISP_SYMBOL ) ;
    word->State |= LC_DEFINED ;

    // the value was entered into the LISP memory, now we need a temporary carrier for LO_Print : yes, apparently, but why?
    l1 = DataObject_New ( T_LC_NEW, 0, word->Name, word->W_MorphismAttributes,
        word->W_ObjectAttributes, word->W_LispAttributes, 0, ( int64 ) value, 0, LISP, - 1, - 1 ) ; // all words are symbols
    l1->W_LispAttributes |= ( T_LC_DEFINE | T_LISP_SYMBOL ) ;
    l1->W_OriginalCodeText = word->W_OriginalCodeText = lc->LC_SourceCode ;
    if ( LC_CompileMode ) l1->W_SC_WordList = word->W_SC_WordList = _LC_->Lambda_SC_WordList ;
    //SetState ( lc, ( LC_DEFINE_MODE ), false ) ;
    _CSL_FinishWordDebugInfo ( l1 ) ;
    _Word_Finish ( l1 ) ;
    //word->W_SC_WordList = _CSL_->CSL_N_M_Node_WordList ;
    return l1 ;
}

ListObject *
_LO_MakeLambda ( ListObject * l0 )
{
    ListObject *args, *body, *lambda, *lnew, *body0 ;
    // allow args to be optionally an actual parenthesized list or just vars after the lambda
    if ( GetState ( _LC_, LC_DEFINE_MODE ) ) lambda = _Context_->CurrentWordBeingCompiled ;
    else lambda = _Word_New ( ( byte* ) "<lambda>", WORD_CREATE, 0, 0, 0, 0, DICTIONARY ) ; // don't _Word_Add : must *not* be "lambda" else it will wrongly replace the lambda T_SPECIAL_FUNCTION word in LO_Find
    args = l0 ;
    body0 = _LO_Next ( l0 ) ;
    if ( args->W_LispAttributes & ( LIST | LIST_NODE ) ) args = _LO_Copy ( args, LISP_ALLOC ) ; // syntactically the args can be enclosed in parenthesis or not
    else
    {
        lnew = LO_New ( LIST, 0 ) ;
        do
        {
            LO_AddToTail ( lnew, _LO_CopyOne ( args, LISP_ALLOC ) ) ;
        }
        while ( ( args = _LO_Next ( args ) ) != body0 ) ;
        args = lnew ;
    }
    if ( ( body0->W_LispAttributes & ( LIST | LIST_NODE ) ) ) body = _LO_Copy ( body0, LISP_ALLOC ) ;
    else
    {
        lnew = LO_New ( LIST, 0 ) ;
        LO_AddToTail ( lnew, _LO_CopyOne ( body0, LISP_ALLOC ) ) ;
        body = lnew ;
    }
    if ( GetState ( _LC_, LC_COMPILE_MODE ) )
    {
        SetState ( _LC_, LC_LAMBDA_MODE, true ) ;
        block codeBlk = CompileLispBlock ( args, body ) ;
        SetState ( _LC_, LC_LAMBDA_MODE, false ) ;
        //*lambda->W_PtrToValue = ( uint64 ) codeBlk ;
        lambda->W_Value = ( uint64 ) codeBlk ;
    }
    if ( ! LC_CompileMode ) // nb! this needs to be 'if' not 'else' or else if' because the state is sometimes changed by CompileLispBlock, eg. for function parameters
    {
        lambda->Lo_CSL_Word = lambda ;
        lambda->Lo_LambdaParameters = args ;
        lambda->Lo_LambdaBody = body ;
        lambda->W_LispAttributes |= T_LAMBDA | T_LISP_SYMBOL ;
        //lambda->CAttribute = 0 ;
    }
    return lambda ;
}

ListObject *
LO_Lambda ( ListObject * l0 )
{
    // lambda signature is "lambda" or an alias like "/\", /.", etc.
    //ListObject *lambdaSignature = _LO_First ( l0 ) ;
    Word * lambda = _LO_MakeLambda ( _LO_Next ( l0 ) ) ;
    lambda->W_LispAttributes |= T_LAMBDA ;
    return lambda ;
}

// (define macro (lambda (id (args) (args1)) ( 'define id ( lambda (args)  (args1) ) ) ) )

ListObject *
_LO_Macro ( ListObject * l0, ListObject * locals )
{
    ListObject *idNode = _LO_Next ( l0 ) ;
    //l0 = _LO_Define ( ( byte* ) "macro", idNode, locals ) ;
    l0 = _LO_Define ( idNode ) ;
    l0->W_LispAttributes |= T_LISP_MACRO ;
    if ( l0->Lo_CSL_Word ) l0->Lo_CSL_Word->W_LispAttributes |= T_LISP_MACRO ;
    if ( GetState ( _CSL_, DEBUG_MODE ) ) LO_Print ( l0 ) ;
    return l0 ;
}

ListObject *
LO_CompileDefine ( ListObject * l0 )
{
    SetState ( _Context_->Compiler0, RETURN_TOS, true ) ;
    SetState ( _LC_, (LC_COMPILE_MODE|LC_DEFINE_MODE), true ) ;
    ListObject * idNode = _LO_Next ( l0 ) ;
    l0 = _LO_Define ( idNode ) ;
    SetState ( _LC_, (LC_COMPILE_MODE|LC_DEFINE_MODE), false ) ;
    return l0 ;
}

ListObject *
LO_CompileDefinec ( ListObject * l0 )
{
    SetState ( _Context_->Compiler0, RETURN_TOS, true ) ;
    SetState ( _LC_, (LC_COMPILE_MODE|LC_DEFINE_MODE), true ) ;
    ListObject * idNode = _LO_Next ( l0 ) ;
    l0 = _LO_Definec ( idNode ) ;
    SetState ( _LC_, (LC_COMPILE_MODE|LC_DEFINE_MODE), false ) ;
    return l0 ;
}

ListObject *
LO_Define ( ListObject * l0 )
{
    l0 = LO_CompileDefine ( l0 ) ;
    return l0 ;
}

ListObject *
LO_Definec ( ListObject * l0 )
{
    l0 = LO_CompileDefinec ( l0 ) ;
    return l0 ;
}

// setq

ListObject *
LO_Set (ListObject * lfirst)
{
    ListObject *l1, * lsymbol, *value, *lset ;
    // lfirst is the 'set' signature
    for ( l1 = lfirst ; lsymbol = _LO_Next ( l1 ) ; l1 = value )
    {
        value = _LO_Next ( lsymbol ) ;
        if ( value )
        {
            if ( _LC_->LetFlag ) lset = _Finder_FindWord_InOneNamespace ( _Finder_, _LC_->Locals, lsymbol->Name ) ;
            else if ( lset = LC_FindWord ( lsymbol->Name ) )
            {
                if ( lset->W_ObjectAttributes & NAMESPACE_VARIABLE )
                {
                    Word_Morphism_Run ( lset->Lo_CSL_Word ) ;   
                    DataStack_Push ( value->Lo_Value  ) ;
                    CSL_Poke () ;
                    //continue ;
                }
            }
            if ( ! lset ) lset = _LO_Define ( lsymbol ) ;
            lset->Lo_Value = value->Lo_Value ;
            LO_Print ( lset ) ;
        }
        else break ;
    }
    return 0 ;
}

ListObject *
LO_Let ( ListObject * lfirst, ListObject * locals )
{
    _LC_->LetFlag = 1 ;
    ListObject * l1 = LO_Set (lfirst) ;
    _LC_->LetFlag = 0 ;
    return l1 ;
}

ListObject *
_LO_Cons ( ListObject *first, ListObject * second ) //, uint64 allocType )
{
    ListObject * l0 = LO_New ( LIST, 0 ) ;
    LO_AddToTail ( l0->Lo_List, first ) ;
    LO_AddToTail ( l0->Lo_List, second ) ;

    return l0 ;
}

ListObject *
LO_If ( ListObject * lfirst, ListObject * locals ) //, int64 ifFlag, ListObject * l0 )
{
    return LO_Cond ( lfirst, locals ) ; //, ifFlag, l0 ) ;
}

/* not exactly but ...
LIST: '(' ( LIST | ATOM ) + ')'
COND : LIST | ATOM
IFTRUE : LIST | ATOM
CONDITIONAL : ( cond ( <cond clause>+ )
COND_FUNC : '(' 'cond' ( CONDITIONAL )+ ( IFTRUE | 'else' IFTRUE ) ? ')'
 * r7r2 : formal syntax
The following extensions to BNF are used to make the de-
scription more concise: <thing>* means zero or more occur-
rences of <thing>; and <thing>+ means at least one <thing>.
<expression> : <identifier>
    | <literal>
    | <procedure call>
    | <lambda expression>
    | <conditional>
    | <assignment>
    | <derived expression>
    | <macro use>
    |<macro block>
    | <includer>
<conditional> : (if <test> <consequent> <alternate>)
<test> : <expression>
<test> : <expression> 
<command> : <expression>
<sequence> : <command>* <expression> 
<cond clause> : (<test> <sequence>) | (<test>)
<derived expression> : (cond <cond clause>+ ) | (cond <cond clause>* (else <sequence>)) | ...
 */
#define IS_COND_MORPHISM_TYPE(word) ( word->W_MorphismAttributes & ( CATEGORY_OP|KEYWORD|ADDRESS_OF_OP|BLOCK|T_LAMBDA ) || ( word->W_LispAttributes & ( T_LAMBDA ) ) )

ListObject *
LO_Cond ( ListObject * lfirst, ListObject * locals )//, int64 ifFlag, ListObject * l0 )
{
    Compiler * compiler = _Context_->Compiler0 ;
    LambdaCalculus * lc = _LC_ ;
    ListObject *condClause, *nextCondClause, * test, *sequence, * resultNode = nil, * result = nil, *testResult ;
    int64 timt = 0 ; //, ccilt = 0 ; // timt : test is morphism type ; ccilt : condClause is list type
    int64 ifFlag = ( int64 ) ( lfirst->Lo_CSL_Word->W_LispAttributes & T_LISP_IF ) ;
    int64 numBlocks, d1, d0 = Stack_Depth ( compiler->CombinatorBlockInfoStack ), testValue ;
    if (GetState ( lc, LC_DEBUG_ON )  )  _LO_PrintWithValue ( lc->L0, "\nLO_Cond : lc->L0 = ", "", 1 ) , _LO_PrintWithValue ( lc->L00, "\nLO_Cond : lc->L00 = ", "", 1 ) ;
    ListObject * idLo = lfirst ;
    if ( condClause = _LO_Next ( idLo ) ) // 'cond' is id node ; skip it.
    {
        do
        {
            // first determine test and sequence
            //if ( DEFINE_DBG ) _LO_PrintWithValue ( condClause, "\nLO_Cond : condClause = ", "", 1 ) ;
            if ( ! ( sequence = _LO_Next ( condClause ) ) )
            {
                if ( condClause->W_LispAttributes & ( LIST | LIST_NODE ) )
                {
                    do
                    {
                        test = _LO_First ( condClause ) ;
                        if ( sequence = _LO_Next ( test ) ) break ;
                        else condClause = test ;
                    }
                    while ( condClause->W_LispAttributes & ( LIST | LIST_NODE ) ) ;
                }
                else
                {
                    resultNode = condClause ;
                    if ( CompileMode ) result = LC_Eval ( lc, resultNode, locals, lc->ApplyFlag ) ;
                    break ;
                }
            }
            else test = _LO_First ( condClause ) ;
            if ( timt = IS_COND_MORPHISM_TYPE ( test ) ) test = condClause ;
            if ( String_Equal ( test->Name, "else" ) )
            {
                resultNode = _LO_Next ( test ) ;
                if ( CompileMode )
                {
                    result = LC_Eval ( lc, resultNode, locals, lc->ApplyFlag ) ;
                    resultNode = 0 ;
                }
                break ;
            }
            if ( sequence && ( ! ( sequence = _LO_Next ( test ) ) ) )
            {
                resultNode = test ;
                if ( CompileMode ) result = LC_Eval ( lc, resultNode, locals, lc->ApplyFlag ) ;
                else break ;
            }

            if ( ! ( nextCondClause = _LO_Next ( sequence ) ) )
                nextCondClause = _LO_Next ( condClause ) ;
#if 0            
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( lfirst, "\nLO_Cond : l0 = ", "", 1 ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( condClause, "\nLO_Cond : condClause = ", "", 1 ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( sequence, "\nLO_Cond : sequence = ", "", 1 ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( nextCondClause, "\nLO_Cond : nextCondClause = ", "", 1 ) ;
            if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( test, "\nLO_Cond : test = ", "", 1 ) ;
#endif            
            // we have determined test and sequence
            // either return result or find next condClause
            if ( GetState ( lc, LC_DEBUG_ON ) ) CSL_Show_SourceCode_TokenLine ( test, "LC_Debug : ", 0, test->Name, "" ) ;
            testResult = LC_Eval ( lc, test, locals, 1 ) ;
            testValue = ( testResult && ( testResult->Lo_Value ) ) ;
            //if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( testResult, "\nLO_Cond : testResult = ", "", 1 ) ;
            //if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( sequence, "\nLO_Cond : eval : sequence = ", "", 1 ) ;
            if ( CompileMode ) result = LC_Eval ( lc, sequence, locals, lc->ApplyFlag ) ;
            if ( testValue )
            {
                if ( ! CompileMode )
                {
                    resultNode = sequence ;
                    break ;
                }
            }
            else
            {
                resultNode = nextCondClause ;
                if ( CompileMode )
                {
                    //if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( resultNode, "\nLO_Cond : resultNode = ", "", 1 ) ;
                    result = LC_Eval ( lc, resultNode, locals, lc->ApplyFlag ) ;
                }
                //ccilt = condClause->W_LispAttributes & ( LIST | LIST_NODE ) ;
                //if ( ( ! ( condClause->W_LispAttributes & ( LIST | LIST_NODE ) ) ) || ( ifFlag ) ) break ; //{ resultNode = test ; break ; }
                if ( ifFlag ) break ; //{ resultNode = test ; break ; }
            }
            //tilt = 0 ;
            condClause = nextCondClause ;
        }
        while ( condClause ) ;
        if ( idLo->W_MorphismAttributes & COMBINATOR )
        {
            d1 = Stack_Depth ( compiler->CombinatorBlockInfoStack ) ;
            numBlocks = d1 - d0 ;
            CSL_CondCombinator ( numBlocks ) ;
        }
        else
        {
            if ( resultNode )
            {
                //if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( resultNode, "\nLO_Cond : before eval : resultNode = ", "", 1 ) ;
                result = LC_Eval ( lc, resultNode, locals, lc->ApplyFlag ) ;
                //if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( result, "\nLO_Cond : after eval : result = ", "\n", 1 ) ;
            }
        }
    }
    //return nil ;
    return result ;
}

// lisp 'list' function
// lfirst must be the first element of the list

ListObject *
_LC_List ( LambdaCalculus * lc, ListObject * lfirst )
{
    ListObject * lnew = LO_New ( LIST, 0 ), *l1, *lnext ;
    for ( l1 = lfirst ; l1 ; l1 = lnext )
    {
        lnext = _LO_Next ( l1 ) ;
        if ( l1->W_LispAttributes & ( LIST | LIST_NODE ) ) l1 = _LC_List ( lc, _LO_First ( l1 ) ) ;
        else l1 = LC_Eval ( lc, LO_CopyOne ( l1 ), 0, 1 ) ;
        LO_AddToTail ( lnew, l1 ) ;
    }
    return lnew ;
}

ListObject *
LO_List ( ListObject * lfirst )
{
    // 'list' is first node ; skip it.
    ListObject * l1 = _LC_List ( _LC_, _LO_Next ( lfirst ) ) ;
    return l1 ;
}

ListObject *
LO_Begin ( ListObject * lfirst, ListObject * locals )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject * leval, *lnext ;
    // 'begin' is first node ; skip it.
    SetState ( lc, LC_BEGIN_MODE, true ) ;
    if ( lfirst )
    {
        for ( lfirst = _LO_Next ( lfirst ) ; lfirst ; lfirst = lnext )
        {
            lnext = _LO_Next ( lfirst ) ;
            leval = LC_Eval ( lc, lfirst, locals, 1 ) ;
        }
    }
    else leval = 0 ;
    SetState ( lc, LC_BEGIN_MODE, false ) ;
    return leval ;
}

ListObject *
LO_Car ( ListObject * lfirst )
{
    ListObject * l1 = _LO_Next ( lfirst ) ;
    if ( l1->W_LispAttributes & ( LIST_NODE | LIST ) ) return LO_CopyOne ( _LO_First ( l1 ) ) ; //( ListObject * ) l1 ;
    else return LO_CopyOne ( l1 ) ;
}

ListObject *
LO_Cdr ( ListObject * lfirst )
{
    ListObject * l1 = _LO_Next ( lfirst ) ;
    if ( l1->W_LispAttributes & ( LIST_NODE | LIST ) ) return _LO_Next ( _LO_First ( l1 ) ) ; //( ListObject * ) l1 ;
    return ( ListObject * ) _LO_Next ( l1 ) ;
}

ListObject *
LO_Eval ( ListObject * lfirst )
{
    ListObject * l1 = _LO_Next ( lfirst ) ;
    return LC_Eval ( _LC_, l1, 0, 1 ) ;
}

void
_LO_Semi ( Word * word )
{
    if ( word )
    {
        LambdaCalculus * lc = _LC_ ;
        CSL_EndBlock ( ) ;
        block blk = ( block ) DataStack_Pop ( ) ;
        Word_InitFinal ( word, ( byte* ) blk ) ;
        word->W_LispAttributes |= T_LISP_CSL_COMPILED ;
        word->W_OriginalCodeText = lc->LC_SourceCode ;
        word->W_SC_WordList = lc->Lambda_SC_WordList ;
        lc->Lambda_SC_WordList = 0 ;
    }
}

Word *
_LO_Colon ( ListObject * lfirst )
{
    Context * cntx = _Context_ ;
    ListObject *lcolon = lfirst, *lname, *ldata ;
    lname = _LO_Next ( lcolon ) ;
    ldata = _LO_Next ( lname ) ;
    _CSL_Namespace_NotUsing ( ( byte* ) "Lisp" ) ; // nb. don't use Lisp words when compiling csl
    CSL_RightBracket ( ) ;
    Word * word = Word_New ( lname->Name ) ;
    SetState ( cntx->Compiler0, COMPILE_MODE, true ) ;
    CSL_InitSourceCode_WithName ( _CSL_, lname->Name, 1 ) ;
    cntx->CurrentWordBeingCompiled = word ;
    _CSL_RecycleInit_CSL_N_M_Node_WordList ( _CSL_->CSL_N_M_Node_WordList, 1 ) ;
    CSL_WordList_PushWord ( _LO_CopyOne ( lcolon, DICTIONARY ) ) ;
    CSL_BeginBlock ( ) ;

    return word ;
}

// compile csl code in Lisp/Scheme

ListObject *
_LO_CSL ( ListObject * lfirst )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    LambdaCalculus * lc = 0 ;
    ListObject *ldata, *word = 0, *word1 ; //, *lcolon ;
    if ( lc = _LC_ )
    {
        if ( GetState ( lc, LC_READ ) )
        {
            SetState ( lc, LC_READ_MACRO_OFF, true ) ;
            return 0 ;
        }
        SetState ( lc, LC_INTERP_MODE, true ) ;
    }
    _CSL_Namespace_NotUsing ( ( byte * ) "Lisp" ) ; // nb. don't use Lisp words when compiling csl
    SetState ( cntx, LC_CSL, true ) ;
    SetState ( compiler, LISP_MODE, false ) ;
    _CSL_RecycleInit_CSL_N_M_Node_WordList ( _CSL_->CSL_N_M_Node_WordList, 1 ) ;
    CSL_WordList_PushWord ( _LO_CopyOne ( lfirst, DICTIONARY ) ) ;
    for ( ldata = _LO_Next ( lfirst ) ; ldata ; ldata = _LO_Next ( ldata ) )
    {
        if ( ldata->W_LispAttributes & ( LIST | LIST_NODE ) )
        {
            _CSL_Parse_LocalsAndStackVariables ( 1, 1, ldata, compiler->LocalsCompilingNamespacesStack, 0 ) ;
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) "tick" ) || String_Equal ( ldata->Name, ( byte * ) "'" ) )
        {
            ldata = _LO_Next ( ldata ) ;
            Lexer_ParseObject ( _Lexer_, ldata->Name ) ;
            DataStack_Push ( ( int64 ) _Lexer_->Literal ) ;
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) "s:" ) )
        {
            //lcolon = ldata ;
            CSL_DbgSourceCodeOn ( ) ;
            word = _LO_Colon ( ldata ) ;
            ldata = _LO_Next ( ldata ) ; // bump ldata to account for name - skip name
        }
        else if ( _String_EqualSingleCharString ( ldata->Name, ':' ) )
        {
            //lcolon = ldata ;
            word = _LO_Colon ( ldata ) ;
            ldata = _LO_Next ( ldata ) ; // bump ldata to account for name - skip name
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) "return" ) )
        {
            ldata = _LO_Next ( ldata ) ;
            CSL_DoReturnWord ( ldata ) ;
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) ";s" ) && ( ! GetState ( cntx, C_SYNTAX ) ) )
        {
            CSL_DbgSourceCodeOff ( ) ;
            _LO_Semi ( word ) ;
        }
        else if ( _String_EqualSingleCharString ( ldata->Name, ';' ) && ( ! GetState ( cntx, C_SYNTAX ) ) )
        {
#if 0            
            // in case we have more than one ":" on our list ...
            ListObject *ldata1 = _LO_Next ( ldata ) ; // bump ldata to account for name
            word->W_OriginalCodeText = String_New_SourceCode ( _CSL_->SC_Buffer ) ;
            if ( ldata1 && String_Equal ( ldata1->Name, ( byte * ) ":" ) )
            {
                CSL_InitSourceCode_WithName ( _CSL_, ( byte* ) "(", 1 ) ;
            }
#endif            
            _LO_Semi ( word ) ;
        }
        else //if ( ldata )
        {
            word1 = _Interpreter_TokenToWord ( cntx->Interpreter0, ldata->Name, ldata->W_RL_Index, ldata->W_SC_Index ) ;
            Interpreter_DoWord ( cntx->Interpreter0, word1, ldata->W_RL_Index, ldata->W_SC_Index ) ;
        }
    }
    SetState ( cntx, LC_CSL, false ) ;
    if ( lc )
    {
        SetState ( lc, LC_INTERP_DONE, true ) ;
        SetState ( lc, LC_READ_MACRO_OFF, false ) ;
        _LC_ = lc ;
        //LC_RestoreStack ( ) ;
    }
    Namespace_DoNamespace_Name ( ( byte * ) "Lisp" ) ;
    if ( ! CompileMode )
    {
        Compiler_Init ( compiler, 0 ) ;
        CSL_AfterWordReset ( ) ;
    }
    return nil ;
}

