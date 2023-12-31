#include "../../include/csl.h"

Boolean
LO_IsQuoted ( ListObject *l0 )
{
    return (( l0->State & QUOTED ) || ( ( l0->State & QUASIQUOTED ) && ( ! ( l0->State & ( UNQUOTED | UNQUOTE_SPLICE ) ) ) ) ) ;
}

ListObject *
LC_Eval ( ListObject *l0, ListObject *locals, Boolean applyFlag )
{
    //if ( _LC_->ParenLevel == 0 ) if ( ! sigsetjmp ( _LC_->LC_JmpBuf, 0 ) ) ;
    LambdaCalculus * lc = _LC_ ;
    ListObject *l1 = l0 ; // default
    lc->ApplyFlag = applyFlag ; //= ! GetState (lc, LC_DEFINE_MODE);
    lc->Locals = locals ;
    SetState ( lc, LC_EVAL, true ) ;
    lc->L0 = l0 ;
    if ( kbhit ( ) == ESC ) OpenVmTil_Pause ( ) ;
    LC_Debug ( lc, LC_EVAL, 1 ) ;
    if ( l0 && ( ! LO_IsQuoted ( l0 ) ) )
    {
        if ( l0->W_LispAttributes & T_LISP_SYMBOL ) l1 = _LC_EvalSymbol ( ) ;
        else if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) ) l1 = LC_EvalList ( ) ;
        else if ( GetState ( lc, LC_DEBUG_ON ) ) CSL_Show_SourceCode_TokenLine ( l0, "LC_Debug : ", 0, l0->Name, "" ) ;
    }
    SetState ( lc, LC_EVAL, false ) ;
    lc->L1 = l1 ;
    LC_Debug ( lc, LC_EVAL, 0 ) ;
    return l1 ;
}

ListObject *
LC_EvalList ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *lfunction, *lfirst ;
    LO_CheckEndBlock ( ) ;
    LO_CheckBeginBlock ( ) ;
    lc->ParenLevel ++ ;
    lfirst = _LO_First ( lc->L0 ) ;
    LC_Debug ( lc, LC_EVAL_LIST, 1 ) ;
    if ( lfirst )
    {
        //lc->Lfunction0 = lfirst ;
        if ( lfirst->W_LispAttributes & ( T_LISP_SPECIAL | T_LISP_MACRO ) )
        {
            if ( LO_IsQuoted ( lfirst ) ) return lfirst ;
            lc->Lfirst = lfirst ;
            lc->L1 = LC_SpecialFunction ( ) ;
            lc->ParenLevel -- ;
        }
        else
        {
            lfunction = LC_Eval ( lfirst, lc->Locals, lc->ApplyFlag ) ;
#if 0            
        if ( String_Equal ( lfunction->Name, "cc")) 
            Printf ("") ;
        if (( lfunction == lc->L0 ) && (  lfunction->W_LispAttributes & ( T_LC_DEFINE  ) ) ) 
                siglongjmp ( lc->LC_JmpBuf, 1 ) ;
        //lc->Lfunction0 = l1 ;
            //if ( ! lc->Lfunction0 ) 
            //if (( lc->ParenLevel == 1 ) && ( lc->Lfunction0 == lfunction ) ) //&& String_Equal (lc->Lfunction0->Name, lfunction->Name)) 
#endif            
            lc->Largs0 = _LO_Next ( lfirst ) ;
            lc->Largs = _LC_EvalList ( ) ;
            lc->Lfunction = lfunction ;
            lc->L1 = LC_Apply ( ) ;
        }
    }
    LC_Debug ( lc, LC_EVAL_LIST, 0 ) ;
    return lc->L1 ;
}

ListObject *
_LC_EvalSymbol ( )
{
    LambdaCalculus * lc = _LC_ ;
    Word *w ;
    ListObject *l1 = lc->L0 ; // default 
    if ( l1 )
    {
        LC_Debug ( lc, LC_EVAL_SYMBOL, 1 ) ;
        w = LC_FindWord ( l1->Name ) ;
        if ( w )
        {
            w->W_SC_Index = l1->W_SC_Index ;
            w->W_RL_Index = l1->W_RL_Index ;
            if ( w->W_LispAttributes & T_LAMBDA )
            {
                lc->Sc_Word = l1 = w ;
                w->State = l1->State ;
            }
            else if ( w->W_LispAttributes & T_LC_DEFINE ) l1 = ( ListObject * ) w->Lo_Value ;
            else if ( ( w->W_MorphismAttributes & ( CPRIMITIVE | CSL_WORD ) )
                || ( w->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE | NAMESPACE_VARIABLE ) )
                || ( w->W_LispAttributes & ( T_LISP_COMPILED_WORD ) ) )
            {
                l1->Lo_Value = w->W_Value ;
                l1->Lo_CSL_Word = w ;
                l1->W_MorphismAttributes |= w->W_MorphismAttributes ;
                l1->W_ObjectAttributes |= w->W_ObjectAttributes ;
                l1->W_LispAttributes |= w->W_LispAttributes ;
            }
            else
            {
                w->State = l1->State ;
                l1 = w ;
            }
            if ( ( CompileMode ) && LO_CheckBeginBlock ( ) ) _LO_CompileOrInterpret_One ( l1, 0 ) ;
            if ( w->W_MorphismAttributes & COMBINATOR ) LC_InitForCombinator ( lc ) ;
            if ( ( ! ( w->W_LispAttributes & T_LAMBDA ) ) )
            {
                w->W_MySourceCodeWord = lc->Sc_Word ;
                l1->W_MySourceCodeWord = lc->Sc_Word ;
            }
        }
    }
    // else the node evals to itself
    lc->L1 = LO_CopyOne ( l1 ) ;
    //if ( lc->L1->W_LispAttributes & ( T_LC_DEFINE  ) ) lc->Lfunction0 = lc->L1 ; 
    LC_Debug ( lc, LC_EVAL_SYMBOL, 0 ) ;
    return lc->L1  ;
}

ListObject *
_LC_EvalList ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *l1 = 0, *lnode, *lnext, *le, *locals = lc->Locals ;
    if ( lc->Largs0 )
    {
        l1 = LO_New ( LIST, 0 ) ;
        for ( lnode = lc->Largs0 ; lnode ; lnode = lnext )
        {
            lnext = _LO_Next ( lnode ) ;
#if 0            
            if (lnode == lc->Lfunction0 ) 
                siglongjmp ( lc->LC_JmpBuf, 1 ) ;
#endif            
            le = LC_Eval ( lnode, locals, lc->ApplyFlag ) ; // lc->Locals could be changed by eval
            LO_AddToTail ( l1, LO_CopyOne ( le ) ) ;
        }
    }
    return l1 ;
}

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
            lnext = _LO_Next ( lfirst ) ;
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
