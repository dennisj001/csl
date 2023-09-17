#include "../../include/csl.h"

//#define   LC_Debug( lc, state, flag ) 

#if LC_FUNCTION_VARS

void
CSL_LC_FindWord ( )
{
    byte * name = ( byte * ) _DspReg_ [ 0 ] ;
    _DspReg_ [ 0 ] = ( int64 ) LC_FindWord ( name ) ;
}

void
Compile_CallFunctionVariable ( ListObject * lfunction )
{
    DBI_ON ;
    _Compile_Stack_Push ( DSP, RAX, ( int64 ) lfunction->Name ) ;
    Compile_Call ( ( byte* ) CSL_LC_FindWord ) ;
    _Compile_Move_StackN_To_Reg ( RAX, DSP, 0, CELL ) ;
    Compile_Move_Rm_To_Reg ( RAX, RAX, 0x88, CELL ) ; // 0x88 : Definition offset in Word
    _Compile_CallReg ( RAX, REG ) ;
    _Compile_Stack_PushReg ( DSP, RAX, CELL ) ;
    DBI_OFF ;
}
#endif

Boolean
LO_IsQuoted ( ListObject *lo )
{
    return (( lo->State & QUOTED ) || ( ( lo->State & QUASIQUOTED ) && ( ! ( lo->State & ( UNQUOTED | UNQUOTE_SPLICE ) ) ) ) ) ;
}

// LC_Eval is bringing several functions into one : /// pre = 0.938.024 == ///

ListObject *
LC_Eval ( ListObject *l0, ListObject *locals, Boolean applyFlag )
{
    _LC_->EvalDepth ++ ;
    _LC_->ApplyFlag = applyFlag ;
    _LC_->Locals = locals ;
    return _LC_Eval ( l0 ) ;
}

ListObject *
_LC_Eval ( ListObject *l0 )
{
    _LC_->L0 = l0 ;
    if ( _LC_->L0 )
    {
        _LC_->L1 = _LC_->L0 ; // default 
        SetState ( _LC_, LC_EVAL, true ) ;

        if ( kbhit ( ) ) OpenVmTil_Pause ( ) ;
        LC_Debug ( _LC_, LC_EVAL, 1 ) ;
        ///_LC_EvalSymbol ( )
        if ( LO_IsQuoted ( _LC_->L0 ) ) return _LC_->L0 ;
        else
        {
            if ( _LC_->L0->W_LispAttributes & ( LIST | LIST_NODE ) )
                ///LC_EvalList ( )
            {
                _LC_->EvalListDepth ++ ;
evalList:
                //SetState ( _LC_, LC_EVAL_LIST, true ) ;
                LO_CheckEndBlock ( ) ;
                LO_CheckBeginBlock ( ) ;
                _LC_->ParenLevel ++ ;
                ListObject *lfirst = LO_First ( _LC_->L0 ) ;
                LC_Debug ( _LC_, LC_EVAL_LIST, 1 ) ;
                if ( lfirst )
                {
                    if ( LO_IsQuoted ( lfirst ) ) return lfirst ;
                    if ( lfirst->W_LispAttributes & ( T_LISP_SPECIAL | T_LISP_MACRO ) )
                    {
                        _LC_->Lfirst = lfirst ;
                        _LC_->L1 = LC_SpecialFunction ( ) ;
                        _LC_->ParenLevel -- ;
                    }
                    else
                    {
                        ListObject *lfunction = _LC_Eval ( lfirst ) ; //, _LC_->Locals, _LC_->ApplyFlag ) ;
                        _LC_->Largs0 = LO_Next ( lfirst ) ;
#if 0                        
                        _LC_EvalList ( ) ;
#else                        

                        //void  _LC_EvalList ( )
                        {
                            SetState ( _LC_, _LC_EVAL_LIST, true ) ;
                            LC_Debug ( _LC_, _LC_EVAL_LIST, 1 ) ;
                            ListObject *l1 = 0, *lnode, *lnext, *le, *locals = _LC_->Locals ;
                            if ( _LC_->Largs0 )
                            {
                                l1 = LO_New ( LIST, 0 ) ;
                                for ( lnode = _LC_->Largs0 ; lnode ; lnode = lnext )
                                {
                                    lnext = LO_Next ( lnode ) ;
                                    //le = LC_Eval ( lnode, locals, _LC_->ApplyFlag ) ; // _LC_->Locals could be changed by eval
                                    _LC_->Locals = locals ; // _LC_->Locals could be changed by eval
                                    le = _LC_Eval ( lnode ) ;
                                    //LO_AddToTail ( l1, LO_CopyOne ( le ) ) ;
                                    LO_AddToTail ( l1, le ) ;
                                }
                            }
                            _LC_->Largs = l1 ;
                            LC_Debug ( _LC_, _LC_EVAL_LIST, 0 ) ;
                            SetState ( _LC_, _LC_EVAL_LIST, false ) ;
                        }
#endif                        
                        ///LC_Apply ( )
                        _LC_->Lfunction = lfunction ;
                        SetState ( _LC_, LC_APPLY, true ) ;
                        LC_Debug ( _LC_, LC_APPLY, 1 ) ;
                        if ( _LC_->ApplyFlag && lfunction && ( ( lfunction->W_MorphismAttributes & ( CPRIMITIVE | CSL_WORD ) ) ||
                            ( lfunction->W_LispAttributes & ( T_LISP_COMPILED_WORD | T_LC_IMMEDIATE ) ) ) )
                        {
                            ///_LO_Apply ( )
                            if ( ( !_LC_->Largs ) && ( lfunction->W_MorphismAttributes & ( CSL_WORD ) ) || ( lfunction->W_LispAttributes & ( T_LC_IMMEDIATE ) ) // allows for lisp.csl macros !? but better logic is probably available
                                || ( lfunction->W_LispAttributes & T_LISP_CSL_COMPILED ) )
                            {
                                Interpreter_DoWord ( _Context_->Interpreter0, lfunction->Lo_CSL_Word, lfunction->W_RL_Index, lfunction->W_SC_Index ) ;
                                _LC_->L1 = nil ; //default
                            }
                            else if ( _LC_->Largs ) _LC_->L1 = _LO_Do_FunctionBlock ( lfunction, _LC_->Largs ) ;
                            else
                            {
                                _LC_->ParenLevel -- ;
                                LO_CheckEndBlock ( ) ;
                                SetState ( _LC_, LC_COMPILE_MODE, false ) ;
                                _LC_->L1 = lfunction ;
                            }
                            SetState ( _LC_, LC_APPLY, false ) ;
                        }
                        else if ( lfunction && ( lfunction->W_LispAttributes & T_LAMBDA ) && lfunction->Lo_LambdaBody )
                        {
                            // LambdaArgs, the formal args, are not changed by LO_Substitute (locals - lvals are just essentially 'renamed') and thus don't need to be copied
                            _LC_->FunctionParameters = lfunction->Lo_LambdaParameters, _LC_->FunctionArgs = _LC_->Largs ;
                            LC_Substitute ( ) ;
                            _LC_->Locals = _LC_->Largs ;
                            _LC_->L0 = lfunction->Lo_LambdaBody ;
                            goto evalList ; // save stack
                        }
                        else
#if ! LC_FUNCTION_VARS                                
                            //if ( ! LC_CompileMode )
                        {
                            //these cases seems common sense for what these situations should mean and seem to add something positive to the usual lisp/scheme semantics !?
                            if ( ! _LC_->Largs ) _LC_->L1 = lfunction ;
                            else
                            {
                                LO_AddToHead ( _LC_->Largs, lfunction ) ;
                                _LC_->L1 = _LC_->Largs ;
                            }
                            if ( ! ( lfunction->W_MorphismAttributes & COMBINATOR ) ) SetState ( _LC_, LC_COMPILE_MODE, false ) ;
                        }
                        //else
#else
                            {
                                if ( ( ! ( GetState ( _LC_, _LC_EVAL_LIST ) ) ) && ( lfunction->W_LispAttributes & T_LISP_SYMBOL ) && ( GetState ( _LC_, LC_COMPILE_MODE ) ) )
                                {
                                    if ( ! ( lfunction->W_LispAttributes & T_LC_FUNCTION_VAR_DONE ) )
                                    {
                                        lfunction->W_LispAttributes |= ( T_LC_FUNCTION_VAR ) ;
                                        //_LO_PrintWithValue ( _LC_->Lread, "LC_Apply : _LC_->Lread = ", "", 1 ) ;
                                        //_LO_PrintWithValue ( lfunction, "LC_Apply : lfunction = ", "", 1 ) ;
                                        //_LO_PrintWithValue ( _LC_->Largs, "LC_Apply : _LC_->Largs = ", "", 1 ) ;
                                        //_LC_->L1 = _LO_Do_FunctionBlock ( ( ListObject * ) lfunction->S_Value, _LC_->Largs ) ;
                                        LO_CompileOrInterpretArgs ( _LC_->Largs ) ;
                                        lfunction = Compiler_CopyDuplicatesAndPush ( lfunction, - 1, - 1 ) ;
                                        Compile_CallFunctionVariable ( lfunction ) ;
                                        lfunction->W_LispAttributes |= ( T_LC_FUNCTION_VAR_DONE ) ;
                                    }
                                }
                            }
#endif
                        SetState ( _LC_, LC_APPLY, false ) ;
                        LC_Debug ( _LC_, LC_APPLY, 0 ) ;
                    }
                }
                LC_Debug ( _LC_, LC_EVAL_LIST, 0 ) ;
                SetState ( _LC_, LC_EVAL_LIST, false ) ;
            }
            else if ( _LC_->L0->W_LispAttributes & T_LISP_SYMBOL )
            {
                if ( _LC_->L1 )
                {
                    LC_Debug ( _LC_, LC_EVAL_SYMBOL, 1 ) ;
                    _LC_->LWord = LC_FindWord ( _LC_->L1->Name ) ;
                    if ( _LC_->LWord )
                    {
                        _LC_->LWord->W_SC_Index = _LC_->L1->W_SC_Index ;
                        _LC_->LWord->W_RL_Index = _LC_->L1->W_RL_Index ;
                        if ( _LC_->LWord->W_LispAttributes & T_LAMBDA )
                        {
                            _LC_->Sc_Word = _LC_->L1 = _LC_->LWord ;
                            _LC_->LWord->State = _LC_->L1->State ;
                        }
                        else if ( _LC_->LWord->W_LispAttributes & T_LC_DEFINE ) _LC_->L1 = ( ListObject * ) _LC_->LWord->Lo_Value ;
                        else if ( ( _LC_->LWord->W_MorphismAttributes & ( CPRIMITIVE | CSL_WORD ) )
                            || ( _LC_->LWord->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE | NAMESPACE_VARIABLE ) )
                            || ( _LC_->LWord->W_LispAttributes & ( T_LISP_COMPILED_WORD ) ) )
                        {
                            _LC_->L1->Lo_Value = _LC_->LWord->W_Value ;
                            _LC_->L1->Lo_CSL_Word = _LC_->LWord ;
                            _LC_->L1->W_MorphismAttributes |= _LC_->LWord->W_MorphismAttributes ;
                            _LC_->L1->W_ObjectAttributes |= _LC_->LWord->W_ObjectAttributes ;
                            _LC_->L1->W_LispAttributes |= _LC_->LWord->W_LispAttributes ;
                        }
                        else
                        {
                            _LC_->LWord->State = _LC_->L1->State ;
                            _LC_->L1 = _LC_->LWord ;
                        }
                        if ( ( CompileMode ) && LO_CheckBeginBlock ( ) ) _LO_CompileOrInterpret_One ( _LC_->L1, 0 ) ;
                        if ( _LC_->LWord->W_MorphismAttributes & COMBINATOR ) LC_InitForCombinator ( _LC_ ) ;
                        if ( ( ! ( _LC_->LWord->W_LispAttributes & T_LAMBDA ) ) )
                        {
                            _LC_->LWord->W_MySourceCodeWord = _LC_->Sc_Word ;
                            _LC_->L1->W_MySourceCodeWord = _LC_->Sc_Word ;
                        }
                    }
                }
                LC_Debug ( _LC_, LC_EVAL_SYMBOL, 0 ) ;
            }
            else if ( GetState ( _LC_, LC_DEBUG_ON ) ) CSL_Show_SourceCode_TokenLine ( _LC_->L0, "LC_Debug : ", 0, _LC_->L0->Name, "" ) ;
        }
        SetState ( _LC_, LC_EVAL, false ) ;
        LC_Debug ( _LC_, LC_EVAL, 0 ) ;
    }
    _LC_->EvalDepth -- ;
    _LC_->L1 = LO_CopyOne ( _LC_->L1 ) ;
    return _LC_->L1 ;
}

void
_LC_EvalList ( )
{
    SetState ( _LC_, _LC_EVAL_LIST, true ) ;
    LC_Debug ( _LC_, _LC_EVAL_LIST, 1 ) ;
    ListObject *l1 = 0, *lnode, *lnext, *le, *locals = _LC_->Locals ;
    if ( _LC_->Largs0 )
    {
        l1 = LO_New ( LIST, 0 ) ;
        for ( lnode = _LC_->Largs0 ; lnode ; lnode = lnext )
        {
            lnext = LO_Next ( lnode ) ;
            //le = LC_Eval ( lnode, locals, _LC_->ApplyFlag ) ; // _LC_->Locals could be changed by eval
            _LC_->Locals = locals ; // _LC_->Locals could be changed by eval
            le = _LC_Eval ( lnode ) ;
            //LO_AddToTail ( l1, LO_CopyOne ( le ) ) ;
            LO_AddToTail ( l1, le ) ;
        }
    }
    _LC_->Largs = l1 ;
    LC_Debug ( _LC_, _LC_EVAL_LIST, 0 ) ;
    SetState ( _LC_, _LC_EVAL_LIST, false ) ;
}
