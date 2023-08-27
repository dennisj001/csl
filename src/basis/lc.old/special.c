#include "../../include/csl.h"

ListObject *
LC_SpecialFunction ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject * lfirst = lc->Lfirst, *macro, *lnext, *l1 = lc->L0 ;
    LC_Debug ( lc, LC_SPECIAL_FUNCTION, 1 ) ;
    if ( lfirst )
    {
        while ( lfirst && ( lfirst->W_LispAttributes & T_LISP_MACRO ) )
        {
            lnext = LO_Next ( lfirst ) ;
            macro = lfirst ;
            macro->W_LispAttributes &= ~ T_LISP_MACRO ; // prevent short recursive loop calling of this function thru LO_Eval below
            l1 = LC_Eval ( macro, lc->Locals, 1 ) ;
            macro->W_LispAttributes |= T_LISP_MACRO ; // restore to its true type
            lfirst = lnext ;
        }
        if ( lfirst && lfirst->Lo_CSL_Word && IS_MORPHISM_TYPE ( lfirst->Lo_CSL_Word ) )
        {
            if ( lfirst->W_MorphismAttributes & COMBINATOR ) LC_InitForCombinator ( lc ) ;
            lc->Lfirst = lfirst ;
            l1 = ( ( ListFunction0 ) ( lfirst->Lo_CSL_Word->Definition ) ) ( ) ; // ??? : does adding extra parameters to functions not defined with them mess up the the c runtime return stack
        }
        else l1 = LC_Eval ( lc->L0, lc->Locals, 1 ) ;
    }
    LC_Debug ( lc, LC_SPECIAL_FUNCTION, 0 ) ;
    return l1 ;
}
//===================================================================================================================
//| LO_SpecialFunction(s) 
//===================================================================================================================

// scheme 'define : ( define ( func args) funcBody )

ListObject *
_LO_Define_Scheme ( ListObject * idNode )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *value, *l1, *value1, *l2, *lnext ;
    Word * word, *idLo ;
    SetState ( _CSL_, _DEBUG_SHOW_, 1 ) ;
    if ( ( idNode->W_LispAttributes & ( LIST | LIST_NODE ) ) ) // scheme 'define : ( define ( func args) funcBody )
    {
        idLo = LO_First ( idNode ) ;
        ListObject * value0, *lambda ;
        value = LO_List_New ( LISP ) ;
        value0 = LO_Next ( idNode ) ;
        LO_AddToHead ( value, value0 ) ; // body
        value1 = LO_List_New ( LISP ) ;
        for ( l2 = LO_Next ( idLo ) ; l2 ; l2 = lnext )
        {
            lnext = LO_Next ( l2 ) ;
            LO_AddToTail ( value1, l2 ) ;
            l2->W_ObjectAttributes |= LOCAL_VARIABLE ;
        }
        value1->W_LispAttributes |= ( LIST | LIST_NODE ) ;
        LO_AddToHead ( value, value1 ) ;
        lambda = _LO_Read_DoToken ( "lambda", 0, - 1, - 1 ) ;
        LO_AddToHead ( value, lambda ) ;
    }
    else idLo = idNode, value = LO_Next ( idNode ) ;
    idLo = idLo->Lo_CSL_Word ;
    word = DataObject_New ( T_LC_DEFINE, 0, ( byte* ) idLo->Name, 0, NAMESPACE_VARIABLE, 0, 0, 0, 0, LISP, idNode->W_RL_Index, idNode->W_SC_Index ) ;
    CSL_WordList_Init ( word ) ;
    word->Definition = 0 ; // reset the definition from LO_Read
    _Context_->CurrentWordBeingCompiled = word ;
    word->Lo_CSL_Word = word ;
    Namespace_DoAddWord ( lc->Locals ? lc->Locals : lc->LispDefinesNamespace, word ) ; // put it at the beginning of the list to be found first
    value = LC_Eval ( value, 0, 0 ) ; // 0 : don't apply
    if ( ( value && ( value->W_LispAttributes & T_LAMBDA ) ) )
    {
        lc->FunctionParameters = value->Lo_LambdaParameters = _LO_Copy ( value->Lo_LambdaParameters, LISP ) ;
        lc->Lfunction = value->Lo_LambdaBody = _LO_Copy ( value->Lo_LambdaBody, LISP ) ;
    }
    else value = _LO_Copy ( value, LISP ) ; // this value object should now become part of LISP non temporary memory
    lc->Lvalue = value ;
    SetState ( _CSL_, _DEBUG_SHOW_, false ) ;
    word->Lo_Value = ( uint64 ) value ; // used by eval
    word->W_LispAttributes |= ( T_LC_DEFINE | T_LISP_SYMBOL ) ;
    word->State |= LC_DEFINED ;

    // the value was entered into the LISP memory, now we need a temporary carrier for LO_Print : yes, apparently, but why?
    l1 = DataObject_New ( T_LC_NEW, 0, word->Name, word->W_MorphismAttributes,
        word->W_ObjectAttributes, word->W_LispAttributes, 0, ( int64 ) value, 0, LISP, - 1, - 1 ) ; // all words are symbols
    l1->W_LispAttributes |= ( T_LC_DEFINE | T_LISP_SYMBOL ) ;
    l1->W_OriginalCodeText = word->W_OriginalCodeText = lc->LC_SourceCode ;
    if ( LC_CompileMode ) l1->W_SC_WordList = word->W_SC_WordList = lc->Lambda_SC_WordList ;
    SetState ( lc, ( LC_DEFINE_MODE ), false ) ;
    _CSL_FinishWordDebugInfo ( l1 ) ;
    _Word_Finish ( l1 ) ;
    LC_Debug ( lc, LO_DEFINEC, 0 ) ;
    return l1 ;
}

// (define fn ( lambda ( args ) ( fnbody) ) ) 
// eg. : (define yfac (lambda (yy n) (if (< n 2) (n) (* n (yy yy (- n 1))))))
// eg. : (define fibc (lambda ( n ) (ifElse (< n 2) n (+ (fibc (- n 1)) (fibc (- n 2))))))

ListObject *
_LO_Define_Lisp ( ListObject * idNode )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *value, *l1, *locals1 = 0, *value1, *l2, *lnext ;
    Word * word, *idLo ;
    SetState ( _CSL_, _DEBUG_SHOW_, LC_DEFINE_DBG ) ;
    if ( ( idNode->W_LispAttributes & ( LIST | LIST_NODE ) ) ) // scheme 'define : ( define ( func args) funcBody )
    {
        idLo = LO_First ( idNode ) ;
        value1 = LO_List_New ( LISP ) ;
        for ( l2 = LO_Next ( idLo ) ; l2 ; l2 = lnext )
        {
            lnext = LO_Next ( l2 ) ;
            LO_AddToTail ( value1, l2 ) ;
            l2->W_ObjectAttributes |= LOCAL_VARIABLE ;
        }
        value1->W_LispAttributes |= ( LIST | LIST_NODE ) ;
        locals1 = value1 ;
    }
    else idLo = idNode ;
    value = LO_Next ( idNode ) ;
    idLo = idLo->Lo_CSL_Word ;
    word = DataObject_New ( T_LC_DEFINE, 0, ( byte* ) idLo->Name, 0, NAMESPACE_VARIABLE, 0, 0, 0, 0, LISP, idNode->W_RL_Index, idNode->W_SC_Index ) ;
    CSL_WordList_Init ( word ) ;
    word->Definition = 0 ; // reset the definition from LO_Read
    _Context_->CurrentWordBeingCompiled = word ;
    word->Lo_CSL_Word = word ;
    SetState ( lc, ( LC_DEFINE_MODE ), true ) ;
    Namespace_DoAddWord ( lc->Locals ? lc->Locals : lc->LispDefinesNamespace, word ) ; // put it at the beginning of the list to be found first
    if ( locals1 )
    {
        word->Lo_LambdaParameters = _LO_Copy ( ( ListObject* ) locals1, LISP ) ;
        word->Lo_LambdaBody = _LO_Copy ( value, LISP ) ;
        word->W_LispAttributes |= T_LAMBDA ;
    }
    else
    {
        value = LC_Eval ( value, 0, 0 ) ; // 0 : don't apply
        if ( ( value && ( value->W_LispAttributes & T_LAMBDA ) ) )
        {
            value->Lo_LambdaParameters = _LO_Copy ( value->Lo_LambdaParameters, LISP ) ;
            value->Lo_LambdaBody = _LO_Copy ( value->Lo_LambdaBody, LISP ) ;
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
    if ( LC_CompileMode ) l1->W_SC_WordList = word->W_SC_WordList = lc->Lambda_SC_WordList ;
    _CSL_FinishWordDebugInfo ( l1 ) ;
    _Word_Finish ( l1 ) ;
    lc->L1 = l1 ;
    LC_Debug ( lc, LO_DEFINE, 0 ) ;
    return l1 ;
}

ListObject *
_LO_MakeLambda ( ListObject * lfirst )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *args, *body, *lambda, *lnew, *body0 ;
    // allow args to be optionally an actual parenthesized list or just vars after the lambda
    if ( GetState ( lc, LC_DEFINE_MODE ) ) lambda = _Context_->CurrentWordBeingCompiled ;
    else lambda = _Word_New ( ( byte* ) "<lambda>", WORD_CREATE, 0, 0, 0, 0, DICTIONARY ) ; // don't _Word_Add : must *not* be "lambda" else it will wrongly replace the lambda T_SPECIAL_FUNCTION word in LO_Find
    args = lfirst ;
    body0 = LO_Next ( lfirst ) ;
    if ( args->W_LispAttributes & ( LIST | LIST_NODE ) ) args = _LO_Copy ( args, LISP_ALLOC ) ; // syntactically the args can be enclosed in parenthesis or not
    else
    {
        lnew = LO_New ( LIST, 0 ) ;
        do
        {
            LO_AddToTail ( lnew, _LO_CopyOne ( args, LISP_ALLOC ) ) ;
        }
        while ( ( args = LO_Next ( args ) ) != body0 ) ;
        args = lnew ;
    }
    if ( ( body0->W_LispAttributes & ( LIST | LIST_NODE ) ) ) body = _LO_Copy ( body0, LISP_ALLOC ) ;
    else
    {
        lnew = LO_New ( LIST, 0 ) ;
        LO_AddToTail ( lnew, _LO_CopyOne ( body0, LISP_ALLOC ) ) ;
        body = lnew ;
    }
    if ( GetState ( lc, LC_COMPILE_MODE ) )
    {
        SetState ( lc, LC_LAMBDA_MODE, true ) ;
        block codeBlk = CompileLispBlock ( args, body ) ;
        SetState ( lc, LC_LAMBDA_MODE, false ) ;
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
LC_Lambda ( )
{
    LambdaCalculus * lc = _LC_ ;
    // lambda signature is "lambda" or an alias like "/\", /.", etc.
    //ListObject *lambdaSignature = LO_First ( lfirst ) ;
    ListObject *l1, *idNode = LO_Next ( lc->Lfirst ) ;
    l1 = _LO_MakeLambda ( idNode ) ;
    l1->W_LispAttributes |= T_LAMBDA ;
    return lc->L1 = l1 ;
}

// (define macro (lambda (id (args) (args1)) ( 'define id ( lambda (args)  (args1) ) ) ) )

ListObject *
LC_Macro ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *l1, *idNode = LO_Next ( lc->Lfirst ) ;
    l1 = _LO_Define_Lisp ( idNode ) ;
    l1->W_LispAttributes |= T_LISP_MACRO ;
    if ( l1->Lo_CSL_Word ) l1->Lo_CSL_Word->W_LispAttributes |= T_LISP_MACRO ;
    return lc->L1 = l1 ;
}

ListObject *
LC_Define ( )
{
    LambdaCalculus * lc = _LC_ ;
    SetState ( _Context_->Compiler0, RETURN_TOS, true ) ;
    SetState ( lc, ( LC_COMPILE_MODE | LC_DEFINE_MODE ), true ) ;
    ListObject * idNode = LO_Next ( lc->Lfirst ) ;
    lc->L1 = _LO_Define_Lisp ( idNode ) ;
    SetState ( lc, ( LC_COMPILE_MODE | LC_DEFINE_MODE ), false ) ;
    return lc->L1 ;
}

// definec

ListObject *
LC_Define_Scheme ( )
{
    LambdaCalculus * lc = _LC_ ;
    SetState ( _Context_->Compiler0, RETURN_TOS, true ) ;
    SetState ( lc, ( LC_COMPILE_MODE | LC_DEFINE_MODE ), true ) ;
    ListObject * idNode = LO_Next ( lc->Lfirst ) ;
    lc->L1 = _LO_Define_Scheme ( idNode ) ;
    SetState ( lc, ( LC_COMPILE_MODE | LC_DEFINE_MODE ), false ) ;
    return lc->L1 ;
}

// setq

ListObject *
LO_Set ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *l1, * lsymbol, *value, *lset ;
    // lfirst is the 'set' signature
    for ( l1 = lc->Lfirst ; lsymbol = LO_Next ( l1 ) ; l1 = value )
    {
        value = LO_Next ( lsymbol ) ;
        if ( value )
        {
            if ( lc->LetFlag ) lset = _Finder_FindWord_InOneNamespace ( _Finder_, lc->Locals, lsymbol->Name ) ;
            else if ( lset = LC_FindWord ( lsymbol->Name ) )
            {
                if ( lset->W_ObjectAttributes & NAMESPACE_VARIABLE )
                {
                    Word_Morphism_Run ( lset->Lo_CSL_Word ) ;
                    DataStack_Push (value->Lo_Value) ;
                    CSL_Poke ( ) ;
                    //continue ;
                }
            }
            if ( ! lset ) lset = _LO_Define_Lisp ( lsymbol ) ;
            lset->Lo_Value = value->Lo_Value ;
            LO_Print ( lset ) ;
        }
        else break ;
    }
    return 0 ;
}

ListObject *
LO_Let ( )
{
    LambdaCalculus * lc = _LC_ ;
    lc->LetFlag = 1 ;
    ListObject * l1 = LO_Set ( ) ;
    lc->LetFlag = 0 ;
    return l1 ;
}

ListObject *
LO_If ( ) //, int64 ifFlag, ListObject * lfirst )
{
    return LO_Cond ( ) ;
}

/* not exactly but ...
LIST: '(' ( LIST | ATOM ) + ')'
COND : LIST | ATOM
IFTRUE : LIST | ATOM
CONDITIONAL : ( cond ( <cond clause>+ )
COND_FUNC : '(' 'cond' ( CONDITIONAL )+ ( IFTRUE | 'else' IFTRUE ) ? ')'
 * r7r2 : formal syntax
The following extensions to BNF are used to make the description more concise: 
 *<thing>* means zero or more occurrences of <thing>; and <thing>+ means at least one <thing>.
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
<conditional> : (if <test> <sequence> <alternate>)
<test> : <expression> 
<command> : <expression>
<sequence> : <command>* <expression> 
<cond clause> : (<test> <sequence>) | (<test>)
<derived expression> : (cond <cond clause>+ ) | (cond <cond clause>* (else <sequence>)) | ...
 */
#define IS_COND_MORPHISM_TYPE(word) ( word->W_MorphismAttributes & ( CATEGORY_OP|KEYWORD|ADDRESS_OF_OP|BLOCK|T_LAMBDA ) || ( word->W_LispAttributes & ( T_LAMBDA ) ) )
// LO_Cond is not very understandable??

ListObject *
LO_Cond ( )
{
    LambdaCalculus * lc = _LC_ ;
    Compiler * compiler = _Context_->Compiler0 ;
    ListObject *condClause, *nextCondClause, * test, *sequence, * resultNode = nil, * result = nil, *testResult ;
    ListObject * idLo = lc->Lfirst, *locals = lc->Locals ;
    if ( idLo )
    {
        int64 timt = 0 ; // timt : test is morphism type 
        int64 ifFlag = ( int64 ) ( idLo->Lo_CSL_Word->W_LispAttributes & T_LISP_IF ) ;
        int64 numBlocks, d1, d0 = Stack_Depth ( compiler->CombinatorBlockInfoStack ), testValue ;
        //if ( GetState ( lc, LC_DEBUG_ON ) ) _LO_PrintWithValue ( lc->L0, "LO_Cond : lc->L0 = ", "", 1 ); //, _LO_PrintWithValue ( lc->Lread, "LO_Cond : lc->Lread = ", "", 1 ) ;
        //LC_Debug ( lc, LC_COND, 1 ) ;
        if ( condClause = LO_Next ( idLo ) ) // 'cond' is id node ; skip it.
        {
            do
            {
                // first determine test and sequence
                //LO_Debug_Output ( condClause, "LO_Cond : condClause = " ) ;
                if ( ! ( sequence = LO_Next ( condClause ) ) )
                {
                    if ( condClause->W_LispAttributes & ( LIST | LIST_NODE ) )
                    {
                        do
                        {
                            test = LO_First ( condClause ) ;
                            if ( sequence = LO_Next ( test ) ) break ;
                            else condClause = test ;
                        }
                        while ( condClause->W_LispAttributes & ( LIST | LIST_NODE ) ) ;
                    }
                    else
                    {
                        resultNode = condClause ;
                        if ( CompileMode ) result = LC_Eval ( resultNode, locals, lc->ApplyFlag ) ;
                        break ;
                    }
                }
                else test = LO_First ( condClause ) ;
                if ( timt = IS_COND_MORPHISM_TYPE ( test ) ) test = condClause ;
                if ( String_Equal ( test->Name, "else" ) )
                {
                    resultNode = LO_Next ( test ) ;
                    if ( CompileMode )
                    {
                        result = LC_Eval ( resultNode, locals, lc->ApplyFlag ) ;
                        resultNode = 0 ;
                    }
                    break ;
                }
                if ( sequence && ( ! ( sequence = LO_Next ( test ) ) ) )
                {
                    resultNode = test ;
                    if ( CompileMode ) result = LC_Eval ( resultNode, locals, lc->ApplyFlag ) ;
                    else break ;
                }

                if ( ! ( nextCondClause = LO_Next ( sequence ) ) )
                    nextCondClause = LO_Next ( condClause ) ;
                // we have determined test and sequence
                // either return result or find next condClause
                //if ( LC_DEFINE_DBG ) CSL_Show_SourceCode_TokenLine ( test, "LC_Debug : ", 0, test->Name, "" ) ;
                testResult = LC_Eval ( test, locals, 1 ) ;
                testValue = ( testResult && ( testResult->Lo_Value ) ) ;
                //LO_Debug_Output ( test, "LO_Cond : test = " ) ;
                //LO_Debug_Output ( sequence, "LO_Cond : sequence = " ) ;
                //LO_Debug_Output ( nextCondClause, "LO_Cond : nextCondClause = " ) ;
                if ( CompileMode ) result = LC_Eval ( sequence, locals, lc->ApplyFlag ) ;
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
                        result = LC_Eval ( resultNode, locals, lc->ApplyFlag ) ;
                    }
                    if ( ifFlag ) break ;
                }
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
                    result = LC_Eval ( resultNode, locals, lc->ApplyFlag ) ;
                    //if ( LC_DEFINE_DBG ) _LO_PrintWithValue ( result, "\nLO_Cond : after eval : result = ", "\n", 1 ) ;
                }
            }
        }
        lc->L1 = result ;
        //LC_Debug ( lc, LC_COND, 0 ) ;
        return result ;
    }
}

// lisp 'list' function
// l0 is the first element of the list after 'list'

ListObject *
_LC_List ( ListObject * l0 )
{
    LambdaCalculus * lc = _LC_ ;
    //l0 = LO_Next ( lc->Lfirst ) ;
    ListObject * lnew = LO_New ( LIST, 0 ), *l1, *lnext ;
    for ( l1 = l0 ; l1 ; l1 = lnext )
    {
        lnext = LO_Next ( l1 ) ;
        if ( l1->W_LispAttributes & ( LIST | LIST_NODE ) ) l1 = _LC_List ( LO_First ( l1 ) ) ;
        else l1 = LC_Eval ( LO_CopyOne ( l1 ), 0, 1 ) ;
        LO_AddToTail ( lnew, l1 ) ;
    }
    return lnew ;
}

ListObject *
LC_List ( )
{
    //LambdaCalculus * lc = _LC_ ;
    // 'list' is first node ; skip it.
    ListObject * l1 = _LC_List ( LO_Next ( _LC_->Lfirst ) ) ;
    return l1 ;
}

ListObject *
LO_Begin ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *lfirst = lc->Lfirst ;
    ListObject * leval, *lnext ;
    // 'begin' is first node ; skip it.
    SetState ( lc, LC_BEGIN_MODE, true ) ;
    if ( lfirst )
    {
        for ( lfirst = LO_Next ( lfirst ) ; lfirst ; lfirst = lnext )
        {
            lnext = LO_Next ( lfirst ) ;
            leval = LC_Eval ( lfirst, lc->Locals, 1 ) ;
        }
    }
    else leval = 0 ;
    SetState ( lc, LC_BEGIN_MODE, false ) ;
    return leval ;
}

ListObject *
LO_Car ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject * l1 = lc_eval ( LO_Next ( lc->Lfirst ) ) ; // lc->Lfirst : should be 'car'
    if ( l1 && ( l1->W_LispAttributes & ( LIST_NODE | LIST ) ) ) return  lc_eval ( LO_First ( l1 ) ) ; //( ListObject * ) l1 ;
    else return l1 ;
}

ListObject *
LO_Cdr ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject * l1 = lc_eval ( LO_Next ( lc->Lfirst ) ) ; // lc->Lfirst : should be 'cdr'
    if ( l1 && (l1->W_LispAttributes & ( LIST_NODE | LIST ) ) ) return lc_eval ( LO_Next ( LO_First ( l1 ) ) ) ; //( ListObject * ) l1 ;
    else return l1 ;
 }

ListObject *
LO_Eval ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject * l1 = LO_Next ( lc->Lfirst ) ;
    return LC_Eval ( l1, 0, 1 ) ;
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
    lname = LO_Next ( lcolon ) ;
    ldata = LO_Next ( lname ) ;
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
_LO_CSL ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    LambdaCalculus * lc = 0 ;
    ListObject *lfirst = _LC_->Lfirst ;
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
    for ( ldata = LO_Next ( lfirst ) ; ldata ; ldata = LO_Next ( ldata ) )
    {
        if ( ldata->W_LispAttributes & ( LIST | LIST_NODE ) )
        {
            _CSL_Parse_LocalsAndStackVariables ( 1, 1, ldata, compiler->LocalsCompilingNamespacesStack, 0 ) ;
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) "tick" ) || String_Equal ( ldata->Name, ( byte * ) "'" ) )
        {
            ldata = LO_Next ( ldata ) ;
            Lexer_ParseObject ( _Lexer_, ldata->Name ) ;
            DataStack_Push (( int64 ) _Lexer_->Literal) ;
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) "s:" ) )
        {
            //lcolon = ldata ;
            CSL_DbgSourceCodeOn ( ) ;
            word = _LO_Colon ( ldata ) ;
            ldata = LO_Next ( ldata ) ; // bump ldata to account for name - skip name
        }
        else if ( _String_EqualSingleCharString ( ldata->Name, ':' ) )
        {
            //lcolon = ldata ;
            word = _LO_Colon ( ldata ) ;
            ldata = LO_Next ( ldata ) ; // bump ldata to account for name - skip name
        }
        else if ( String_Equal ( ldata->Name, ( byte * ) "return" ) )
        {
            ldata = LO_Next ( ldata ) ;
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
            ListObject *ldata1 = LO_Next ( ldata ) ; // bump ldata to account for name
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
        lc = _LC_ ;
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

#if 0
// this won't work as written but here's a marker

ListObject *
_LO_Cons ( ListObject * second )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject * lcons = LO_New ( LIST, 0 ) ;
    LO_AddToTail ( lcons, lc->Lfirst ) ;
    LO_AddToTail ( lcons, second ) ;
    return lcons ;
}
#endif