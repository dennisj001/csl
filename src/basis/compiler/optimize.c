
#include "../../include/csl.h"

/* ------
 * 2 args
 * ----------
 * lit lit op 
 * lit var op
 * var lit op
 * var var op
 * -------
 * op args
 * -------
 * lit op op
 * op lit op
 * op var op
 * var op op
 * op op op
 * 
 * x xword [var|lit|op] op :: xword = nonvar, nonlit, nonop
 * [var|lit|op] [var|lit|op] op :: 9 combinations + x betweenWord [var|lit|op] op :: 3 combinations = 12 total combinations
 * ----------------------------
 * 1 arg
 * -----
 * [var|lit|op] op :: 3 combinations
 * 15 total total combinations
 * 
 */
// remember this is rpn code so arg 1, the first (linearly) arg, is lower on the stack (STACK_1) than arg2, the second arg, (STACK_0)
// eg:
//      : test arg1 arg2 op ;  
// stack conventions :
// ________
// | arg2 | TOS : OREG : RCX
// | arg1 | NOS : ACC  : RAX
// --------
// need to consider also REGISTER args/parameters !!
// but we still need to consider types (sizes) and type checking
// and ARM cpu adjustments

// get arg actual setup
// convert arg setup to standard arg1=ACC, arg2=OREG (RCX)
// setup machine insn parameters 

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 

// CO Compiler_Optimize
int64
CO_CheckOptimize ( Compiler * compiler, int64 _specialReturn )
{
    int64 specialReturn = _specialReturn ? _specialReturn : compiler->OptimizeForcedReturn ;
    if ( ( ! specialReturn ) && GetState ( _CSL_, CO_ON ) )
    {
        _specialReturn = CO ( compiler, CSL_WordList ( 0 ) ) ;
    }
    return _specialReturn ? _specialReturn : compiler->OptimizeForcedReturn ;
}

// conventions :
// the 'stack' - list starts with the top being the last word processed in an rpn ordering, next on the list is the previous to last word, ...
// nb! : remember this is a rpn optimizer ; get second arg first when going right to left : arg1 arg2 op :: so op is first then arg2 then arg1
// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 
// next on this list is a 'previous' (because again this is rpn ordering; the syntactic transformation processing needs to put in rpn ordering to get here)

CompileOptimizeInfo *
CO_GetWordStackState ( Compiler * compiler, Word * word )
{
    CompileOptimizeInfo * optInfo ;
    int64 state = compiler->OptInfo->State ;
    compiler->OptInfo = COInfo_PushNew ( compiler ) ;
    optInfo = compiler->OptInfo ;
    CompileOptimizeInfo_Init ( optInfo, state ) ; // State : not used yet ??
    optInfo->opWord = word ;
    SetState ( _CSL_, IN_OPTIMIZER, true ) ;
    for ( optInfo->node = optInfo->wordNode = dllist_First ( ( dllist* ) _CSL_->CSL_N_M_Node_WordList ), optInfo->node = dlnode_Next ( optInfo->node ) ;
        optInfo->node ; optInfo->node = optInfo->nextNode )
    {
        //optInfo->rvalue = 0 ;
        optInfo->nextNode = dlnode_Next ( optInfo->node ) ;
        if ( dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_IN_USE_FLAG ) & SCN_IN_USE_FOR_OPTIMIZATION )
        {
            optInfo->wordn = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_T_WORD ) ;
            //byte * name = optInfo->wordn->Name ; //debugging
        }
        else continue ;
        if ( ( optInfo->wordn->W_MorphismAttributes & ( NO_OP_WORD | LEFT_PAREN | M_POINTER ) ) ) continue ;
        else if ( ( optInfo->wordn->W_MorphismAttributes & ( CATEGORY_OP ) ) || ( optInfo->wordn->W_MorphismAttributes & ( RIGHT_PAREN | RIGHT_BRACKET ) ) )
        {
            if ( ( optInfo->wordn->W_MorphismAttributes & RIGHT_PAREN ) )
            {
                if ( ( compiler->CombinatorLevel || ( compiler->BlockLevel > 1 ) ) && ( optInfo->opWord->W_MorphismAttributes & LOGIC_OPT ) )
                {
                    if ( ! CO_Logic_CheckForOpBetweenParentheses ( optInfo ) )
                    {
                        compiler->OptInfo->rtrn = CO_DONE ;
                        return optInfo ;
                    }
                    else continue ;
                }
                dlnode * nextNode = optInfo->nextNode ;
                Word * wordn = CO_CheckForOpBetweenParentheses ( optInfo, optInfo->node ) ;
                if ( ! wordn )
                {
                    optInfo->nextNode = nextNode ;
                    continue ;
                }
                else
                {
                    if ( optInfo->lparen2 ) optInfo->lparen1 = wordn ;
                    else optInfo->lparen2 = wordn ;
                }
            }
            if ( optInfo->wordn->Definition == CSL_DoubleQuoteMacro ) continue ;
            else if ( optInfo->wordn->W_MorphismAttributes & ( CATEGORY_OP_LOAD ) 
                || ( ( ! ( IS_MORPHISM_TYPE ( optInfo->wordn ) ) ) && ( GetState ( _Context_, (LISP_MODE) ) ) )  ) //( GetState ( _Context_, (INFIX_MODE|LISP_MODE) ) ) ) 
            {
                optInfo->rvalue ++ ; //: for recursive peek constructions like @ @ and @ @ @ etc.
                continue ;
            }
            else
            {
                optInfo->NumberOfArgs ++ ;
                if ( optInfo->wordArg2 )
                {
                    optInfo->wordArg1 = optInfo->wordn ;
                    optInfo->wordArg1Node = optInfo->node ;
                    optInfo->wordArg1_Op = true ;
                }
                else
                {
                    optInfo->wordArg2 = optInfo->wordn ;
                    optInfo->wordArg2Node = optInfo->node ;
                    optInfo->wordArg2_Op = true ;
                }
                //if ( optInfo->wordn->CAttribute2 & LEFT_PAREN ) ; else break ;
                break ;
            }
        }
        else if ( IS_MORPHISM_TYPE ( optInfo->wordn ) ) //&& ( ! IS_NON_MORPHISM_TYPE(optInfo->wordn) ) )
        {
            if ( optInfo->wordArg2 ) optInfo->xBetweenArg1AndArg2 = optInfo->wordn ;
            else optInfo->wordArg2 = optInfo->wordn ;
            break ; // no optimization possible if ( ( ! optInfo->wordArg2 ) && ( IS_MORPHISM_TYPE ( optInfo->wordn ) ) )
        }
        else if ( optInfo->wordArg2 )
        {
            optInfo->NumberOfArgs ++ ;
            optInfo->wordArg1 = optInfo->wordn ;
            optInfo->wordArg1Node = optInfo->node ;
            optInfo->wordArg1_rvalue = optInfo->rvalue ? optInfo->rvalue :
                (( GetState ( _Context_, ( C_SYNTAX | LISP_MODE ) ) ) //( GetState ( _Context_, ( C_SYNTAX | INFIX_MODE | LISP_MODE ) ) )
                && ( ! ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_EQUAL ) ) ) ) ; // rem : rvalue can be higher than 1 (cf. above for '@ @')
            optInfo->rvalue = 0 ;
            if ( optInfo->wordArg1->W_ObjectAttributes & ( CONSTANT | LITERAL ) )
            {
                optInfo->wordArg1_literal = true ;
                optInfo->CO_Imm = optInfo->wordArg2->W_Value ;
            }
            break ;
        }
        else // nb! : remember this is a rpn optimizer ; get second arg first when going right to left : arg1 arg2 op 
        {
            optInfo->NumberOfArgs ++ ;
            optInfo->wordArg2 = optInfo->wordn ;
            optInfo->wordArg2Node = optInfo->node ;
            optInfo->wordArg2_rvalue = optInfo->rvalue ? optInfo->rvalue :
                ( ( GetState ( _Context_, ( C_SYNTAX | INFIX_MODE | LISP_MODE ) ) )
                && ( ! ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_STORE ) ) ) ) ; // rem : rvalue can be higher than 1 (cf. above for '@ @')
            optInfo->rvalue = 0 ;
            if ( IsWordAttribute ( optInfo->wordArg2, W_ObjectAttributes, ( CONSTANT | LITERAL ) ) )
            {
                optInfo->wordArg2_literal = true ;
                optInfo->CO_Imm = optInfo->wordArg2->W_Value ;
            }
            //if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_LOAD ) && ( ! ( GetState ( _Context_, INFIX_MODE ) ) ) ) optInfo->wordArg2_rvalue ++ ;
            if ( IsWordAttribute ( optInfo->opWord, W_MorphismAttributes, ( CATEGORY_OP_LOAD ) ) && ( ! ( GetState ( _Context_, INFIX_MODE ) ) ) ) 
            //if ( IsWordAttribute ( optInfo->opWord, W_MorphismAttributes, ( CATEGORY_OP_LOAD ) ) ) //|| ( GetState ( _Context_, (INFIX_MODE|LISP_MODE) ) ) ) 
                optInfo->wordArg2_rvalue ++ ;
            if ( IsWordAttribute ( optInfo->opWord, W_MorphismAttributes, ( CATEGORY_OP_1_ARG | CATEGORY_OP_STACK | CATEGORY_OP_LOAD ) ) ) break ;
        }
    }
    return optInfo ;
}

Word *
CO_CheckForOpBetweenParentheses ( CompileOptimizeInfo * optInfo, dlnode * snode )
{
    Word *wordn, * rvalue = 0 ;
    for ( optInfo->node = snode ; optInfo->node ; optInfo->node = optInfo->nextNode )
    {
        optInfo->nextNode = dlnode_Next ( optInfo->node ) ;
        if ( dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_IN_USE_FLAG ) & SCN_IN_USE_FOR_OPTIMIZATION )
            wordn = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_T_WORD ) ;
        else continue ;
        if ( wordn->W_MorphismAttributes & ( CATEGORY_OP | COMBINATOR ) ) rvalue = optInfo->wordn ;
        else if ( wordn->W_MorphismAttributes & ( LEFT_PAREN ) ) break ;
    }
    return rvalue ;
}

Word *
CO_Logic_CheckForOpBetweenParentheses ( CompileOptimizeInfo * optInfo )
{
    Word *wordn ;
    dlnode * node = optInfo->node, * nextNode ;
    for ( node = dlnode_Next ( node ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        if ( dobject_Get_M_Slot ( ( dobject* ) node, SCN_IN_USE_FLAG ) & SCN_IN_USE_FOR_OPTIMIZATION )
            wordn = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) node, SCN_T_WORD ) ;
        else continue ;
        if ( ! optInfo->rparenPrevOp )
        {

            if ( ( wordn->W_MorphismAttributes & RIGHT_PAREN ) ) return 0 ;
            if ( wordn->W_MorphismAttributes & ( CATEGORY_OP | COMBINATOR ) )
            {
                optInfo->rparenPrevOp = wordn ;
                optInfo->NumberOfArgs ++ ;
                break ;
            }
            else if ( wordn->W_ObjectAttributes & ( NAMESPACE_VARIABLE | LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
            {
                optInfo->rparenPrevOp = wordn ;
                optInfo->NumberOfArgs ++ ;
                break ;
            }
        }
    }
    if ( optInfo->rparenPrevOp )
    {
        if ( ! optInfo->wordArg2 ) optInfo->wordArg2 = optInfo->rparenPrevOp ;
        else optInfo->wordArg1 = optInfo->rparenPrevOp ;
    }
    return wordn ;
}

// CO Compiler_Optimize
int64
CO ( Compiler * compiler, Word * word )
{
    if ( word )
    {
        if ( Is_DebugOn && ( Verbosity ( ) > 1 ) ) _CSL_SC_WordList_Show ( 0, 0, 0 ) ;
        CO_GetWordStackState ( compiler, word ) ;
        if ( compiler->OptInfo->rtrn != CO_DONE )
        {
            CO_SetStandardPreHere_ForDebugDisassembly ( compiler ) ;
            CO_SetupArgsToStandardLocations ( compiler ) ;
            if ( compiler->OptInfo->rtrn != CO_DONE ) CO_Setup_MachineCodeInsnParameters ( compiler, REG, REG, ACC, OREG, 0, 0 ) ;
            SetState ( _CSL_, IN_OPTIMIZER, false ) ;
        }
        return compiler->OptInfo->rtrn ;
    }
    else return 0 ;
}

void
CO_SetStandardPreHere_ForDebugDisassembly ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->wordArg1 && ( ! optInfo->wordArg1_Op ) ) SetPreHere_ForDebug ( optInfo->wordArg1->Coding ) ;
    else if ( optInfo->wordArg2 ) SetPreHere_ForDebug ( optInfo->wordArg2->Coding ) ;
}

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 
// for 1 arg ops : STACK_0= arg1 

void
CO_SetupArgsToStandardLocations ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_DUP ) ) CO_Dup ( compiler ) ;
    else if ( optInfo->wordArg1_literal && optInfo->wordArg2_literal ) CO_OptimizeOp2Literals ( compiler ) ;
    else if ( optInfo->wordArg2_Op || optInfo->xBetweenArg1AndArg2 ) CO_WordArg2Op_Or_xBetweenArg1AndArg2 ( compiler ) ;
    else if ( optInfo->rparenPrevOp ) CO_1Arg_W_RparenPrevOp ( compiler ) ;
    else if ( ( optInfo->NumberOfArgs == 2 ) && optInfo->wordArg2_literal && optInfo->opWord->W_OpInsnCode == CMP ) CO_Cmp_2Args_With_Arg2Literal ( compiler ) ;
    else if ( ( optInfo->NumberOfArgs == 2 ) && optInfo->wordArg2_literal ) CO_2Args_With_Arg2Literal ( compiler ) ;
    else if ( ( optInfo->NumberOfArgs == 2 ) || optInfo->wordArg1_Op ) CO_2Args_Or_WordArg1_Op ( compiler ) ;
    else if ( optInfo->NumberOfArgs == 1 ) CO_1Arg ( compiler ) ;
    else CO_0Args ( compiler ) ;
}

void
CO_1Arg_W_RparenPrevOp ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    if ( ! ( bi->State & BI_2_LPAREN ) )
    {
        if ( optInfo->opWord->Definition == CSL_LogicalOr ) bi->State |= BI_OR ;
        else if ( optInfo->opWord->Definition == CSL_LogicalAnd ) bi->State |= BI_AND ;
    }
    optInfo->rtrn = CO_DONE ;
}

void
CO_Cmp_2Args_With_Arg2Literal ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE )
    {
        optInfo->CO_Reg = ( optInfo->wordArg1->RegToUse | REG_LOCK_BIT ) ;
        SetHere ( optInfo->wordArg1->Coding ? optInfo->wordArg1->Coding : optInfo->wordArg2->Coding ) ;
    }
    else if ( optInfo->wordArg1->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg1 ) ;
    compiler->OptInfo->CO_Imm = optInfo->wordArg2->S_Value ;
    optInfo->OptimizeFlag |= CO_IMM ;
}

void
CO_2Args_With_Arg2Literal ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_OPEQUAL | CATEGORY_OP_EQUAL ) ) CO_2Args_Or_WordArg1_Op ( compiler ) ;
    else
    {
        if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE )
        {
            optInfo->CO_Rm = ( optInfo->wordArg1->RegToUse | REG_LOCK_BIT ) ;
            SetHere ( optInfo->wordArg1->Coding ) ; //? optInfo->wordArg1->Coding : optInfo->wordArg2->Coding, 1 ) ;
            compiler->OptInfo->CO_Imm = optInfo->wordArg2->S_Value ;
            optInfo->OptimizeFlag |= CO_IMM ;
        }
        else CO_2Args_Or_WordArg1_Op ( compiler ) ;
    }
}

// jcc nz to true ; add jcc z false

int64
CO_2Args_CheckDo_Logic ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ( compiler->CombinatorLevel || ( compiler->BlockLevel > 1 ) ) &&
        ( optInfo->opWord->W_MorphismAttributes & LOGIC_OPT ) )
    {
            return compiler->OptInfo->rtrn = CO_DONE ;
    }
    return 0 ;
}

void
CO_2Args_Or_WordArg1_Op ( Compiler * compiler )
{
    // we know ( ! ( optInfo->wordArg2_Op || optInfo->xBetweenArg1AndArg2 ) ) because we already tested for that
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->wordArg2->W_ObjectAttributes & DOBJECT ) CO_StackArgsToStandardRegs ( compiler ) ;
    else
    {
        Boolean rm = ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ? optInfo->wordArg2->RegToUse : OREG ;
        if ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_OPEQUAL )
        {
            if ( GetState ( _Context_, ( C_SYNTAX | INFIX_MODE ) ) ) SetHere ( optInfo->wordArg2->Coding ) ;
            else SetHere ( optInfo->wordArg1->Coding ) ;
        }
        else
        {
            if ( ! CO_2Args_CheckDo_Logic ( compiler ) )
            {
                optInfo->CO_Reg = ( ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ? optInfo->wordArg1->RegToUse : ACC ) | REG_LOCK_BIT ;
                if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) SetHere ( optInfo->wordArg1->Coding ? optInfo->wordArg1->Coding : optInfo->wordArg2->Coding ) ;
                else if ( optInfo->wordArg1->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg1 ) ;
                else
                {
                    if ( GetState ( _Context_, ( C_SYNTAX | INFIX_MODE ) ) )
                    {
                        SetHere ( optInfo->wordArg2->Coding ) ;
                        Compiler_Word_SCHCPUSCA ( optInfo->wordArg2, 0 ) ;
                        _Compile_Stack_PopToReg ( DSP, ACC ) ;
                    }
                    else
                    {
                        SetHere ( optInfo->wordArg1->Coding ) ;
                        CO_StandardArg ( optInfo->wordArg1, ACC, 0, optInfo->wordArg1_rvalue, 0, true ) ;
                    }
                }
                CO_StandardArg ( optInfo->wordArg2, rm, 0, optInfo->wordArg2_rvalue, 0, true ) ;
                optInfo->CO_Rm = rm | REG_LOCK_BIT ;
            }
        }
    }
}

// nb! : assumes that reg 0

Word *
CO_EqualCheck ( Compiler * compiler )
{
    int64 depth = List_Depth ( compiler->OptimizeInfoList ) ;
    if ( depth )
    {
        CompileOptimizeInfo * coi = ( COI * ) List_Pick ( compiler->OptimizeInfoList, 1 ) ;
        Word * word ;
        dlnode * node, *nextNode ;
        if ( coi && ( coi->NumberOfArgs == 2 ) )
        {
            for ( node = coi->wordArg1Node ; node && ( nextNode = dlnode_Next ( node ) ) ; node = nextNode )
            {
                word = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) nextNode, SCN_T_WORD ) ;
                if ( word->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
                {
                    if ( word->W_ObjectAttributes & REGISTER_VARIABLE ) return compiler->OptInfo->wordArg0_ForOpEqual = word ;
                    break ;
                }
            }
        }
    }
    return 0 ;
}

void
CO_WordArg2Op_Or_xBetweenArg1AndArg2 ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ( optInfo->wordArg2 && optInfo->wordArg2->StackPushRegisterCode ) ||
        ( optInfo->xBetweenArg1AndArg2 && optInfo->xBetweenArg1AndArg2->StackPushRegisterCode ) )
    {
        Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->xBetweenArg1AndArg2 ) ;
        Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg2 ) ;
        if ( ! ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_1_ARG | CATEGORY_OP_OPEQUAL ) ) )
        {
            if ( ( optInfo->wordArg2_literal ) && ( optInfo->opWord->W_OpInsnCode != CMP ) )
            {
                SetHere ( optInfo->wordArg2->Coding ) ;
                optInfo->OptimizeFlag |= CO_IMM ;
                optInfo->CO_Rm = DSP ;
                //optInfo->CO_Reg = DSP ; // nb : cmp  reg must be RAX
                optInfo->CO_Mod = MEM ;
                optInfo->rtrn = 1 ;
            }
            else if ( ! ( ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_EQUAL ) ) && CO_EqualCheck ( compiler ) ) )
            {
                Compiler_Word_SCHCPUSCA ( optInfo->wordArg2, 1 ) ;
                //if ( ! optInfo->xBetweenArg1AndArg2 )  
                //    ( optInfo->xBetweenArg1AndArg2 && ( optInfo->xBetweenArg1AndArg2->StackPushRegisterCode) ) ) 
                // assume arg is on the stack - dsp[0]
                {
                    //assume arg1 is ACC
                    Compile_Move_Reg_To_Reg ( OREG, ACC, 0 ) ;
                    _Compile_Stack_PopToReg ( DSP, ACC ) ;
                }
                optInfo->CO_Reg = ACC | REG_LOCK_BIT ; // REG_LOCK_BIT : let Setup_MachineCodeInsnParameters know we have a parameter for it in case of ACC == 0
                optInfo->CO_Rm = OREG ;
            }
            else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg2 ) ; // the rest of the code will be handled in CO_Equal
        }
    }
    else if ( optInfo->NumberOfArgs )
    {
        if ( ( optInfo->wordArg2->W_MorphismAttributes & ( RIGHT_BRACKET ) ) && ( ( GetState ( _Context_, C_SYNTAX | INFIX_MODE ) || GetState ( compiler, LC_ARG_PARSING ) ) ) )
        {
            Compile_Move_Rm_To_Reg ( OREG, DSP, 0, 0 ) ;
            Compile_Move_Rm_To_Reg ( OREG, OREG, 0, 0 ) ;
            _Compile_Move_StackN_To_Reg ( ACC, DSP, - 1, 0 ), optInfo->CO_Reg = ACC | REG_LOCK_BIT ;
            Compile_SUBI ( REG, DSP, 0, 2 * CELL, 0 ) ;
        }
        else CO_StackArgsToStandardRegs ( compiler ) ;
    }
}

void
CO_0Args ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_LOAD )
    {
        CO_Load_TOS ( ) ;
        optInfo->rtrn = CO_DONE ;
    }
    else if ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_1_ARG )
    {
        if ( optInfo->wordArg2 )
        {
            if ( optInfo->wordArg2->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2 ) ;
            else
            {
                _Compile_Move_StackN_To_Reg ( ACC, DSP, 0, 0 ), optInfo->CO_Reg = ACC | REG_LOCK_BIT ;
                Compile_SUBI ( REG, DSP, 0, CELL, 0 ) ;
            }
        }
        else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->opWord ) ;
    }

    else CO_StackArgsToStandardRegs ( compiler ) ;
}

void
CO_1Arg ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    // remember : CATEGORY_DUP && CATEGORY_OP_LOAD were handled already in Compiler_SetupArgsToStandardLocations.CO_0Args
    if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_LOAD ) ) CO_Load ( compiler ) ;
    else if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_PLUS_PLUS_MINUS_MINUS ) ) CO_IncDec ( compiler ) ;
    else if ( optInfo->wordArg2->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2 ), optInfo->CO_Reg = ACC | REG_LOCK_BIT ;

    else _Compile_Move_StackN_To_Reg ( ACC, DSP, 0, 0 ), optInfo->CO_Reg = ACC | REG_LOCK_BIT ;
}

// if we have a parameter already set in optInfo honor it else use the default 
// adjust if needed

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 
// for 1 arg ops : STACK_0= arg1 
// if ( optInfo->wordArg1 ) optInfo->wordArg1->RegToUse = optInfo->CO_Reg ;
// if ( optInfo->wordArg2 ) optInfo->wordArg2->RegToUse = optInfo->CO_Rm ;

void
CO_Setup_MachineCodeInsnParameters ( Compiler * compiler, Boolean direction, Boolean mod, Boolean reg, Boolean rm, int64 disp, Boolean forceSet )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ! optInfo->rtrn ) //!= CO_DONE )
    {
        if ( ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_1_ARG ) && ( ! ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) rm = ACC ;
        if ( ( optInfo->CO_Rm & REG_LOCK_BIT ) && ( ! forceSet ) ) optInfo->CO_Rm = ( optInfo->CO_Rm & 0xf ) ;
        else optInfo->CO_Rm = CO_CheckForRegisterVariable ( compiler, rm ) ;
        if ( optInfo->opWord->W_MorphismAttributes & BIT_SHIFT ) optInfo->CO_Rm = RAX ;

        if ( ( optInfo->CO_Reg & REG_LOCK_BIT ) && ( ! forceSet ) ) optInfo->CO_Reg = ( optInfo->CO_Reg & 0xf ) ;
        else optInfo->CO_Reg = CO_CheckForRegisterVariable ( compiler, reg ) ; // register variables override REG_ON_BIT

        optInfo->CO_Mod = mod ;
        optInfo->CO_Dest_RegOrMem = direction ;
        optInfo->CO_Disp = disp ;

        if ( ( optInfo->wordArg2 && optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE )
            || ( optInfo->wordArg1 && optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) optInfo->OptimizeFlag |= CO_REGISTER ;

        // standard arg arrangement
        if ( optInfo->wordArg1 ) optInfo->wordArg1->RegToUse = optInfo->CO_Reg ;
        if ( optInfo->wordArg2 ) optInfo->wordArg2->RegToUse = optInfo->CO_Rm ;

        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;

        optInfo->rtrn = 1 ;
    }
}

void
CO_OptimizeOp2Literals ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;

    if ( optInfo->wordArg1->Coding ) SetHere ( optInfo->wordArg1->Coding ) ;
    int64 value ;
    // a little tricky here ...
    // maybe we should setup a special compiler stack and use it here ? ... but no we are setting up for Block_Eval !!
    DataStack_Push ( ( int64 ) * optInfo_0_two->W_PtrToValue ) ;
    DataStack_Push ( ( int64 ) * optInfo_0_one->W_PtrToValue ) ;
    SetState ( compiler, COMPILE_MODE, false ) ;
    SetState ( _CSL_, CO_ON, false ) ; //prevent recursion here
    //Word_Run ( optInfo_0_zero ) ;
    Block_Eval ( optInfo_0_zero->Definition ) ; // no type checking
    SetState ( _CSL_, CO_ON, true ) ; // restore state ; CO_ON had to be true/on else we wouldn't have entered _Compiler_CheckOptimize
    SetState ( compiler, COMPILE_MODE, true ) ;
    value = DataStack_Pop ( ) ;
    SetHere ( optInfo_0_two->Coding ) ;
    Compiler_Word_SCHCPUSCA ( optInfo_0_two, 1 ) ;
    Compile_MoveImm_To_Reg ( ACC, value, CELL ) ;
    _Word_CompileAndRecord_PushReg ( optInfo->opWord, ACC, true, 0 ) ; // this is helpful in future optimizations looking for StackPushRegisterCode
    optInfo->rtrn = CO_DONE ;
}

void
CO_IncDec ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Boolean reg ;
    if ( GetState ( _Context_, C_SYNTAX ) || ( ! optInfo->wordArg2_rvalue ) )
    {
        CO_GetRmDispImm ( optInfo, optInfo->wordArg2, - 1 ) ;
        if ( ! ( optInfo->OptimizeFlag & CO_IMM ) )
        {
            if ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) reg = optInfo->wordArg2->RegToUse ;
            else reg = optInfo->CO_Rm ;
            SetHere ( optInfo->wordArg2->Coding ) ;
            if ( optInfo->wordArg2->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) ) _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) optInfo->wordArg2->W_PtrToValue, 0 ) ;
            Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
            _Compile_Group5 ( optInfo->opWord->W_OpInsnCode, ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ? REG : MEM, reg, 0, optInfo->CO_Disp, 0 ) ;
            optInfo->rtrn = CO_DONE ;
        }
    }
    else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg2 ) ; //if ( optInfo->wordArg2->StackPushRegisterCode )
}

// '@' : fetch or load

void
CO_Load ( Compiler * compiler )
{
    Context * cntx = _Context_ ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Word * wordArg2 ;
    int64 size = 0 ;
    if ( optInfo->opWord->W_TypeAttributes & WT_BYTE ) size = 1 ;
    else if ( optInfo->opWord->W_TypeAttributes & WT_WORD ) size = 2 ;
    //else if ( optInfo->opWord->W_TypeAttributes & WT_LONG ) size = 4 ;
    //Compiler_Word_SCHCPUSCA ( optInfo->opWord, 0 ) ;
    if ( optInfo->wordArg2 )
    {
        BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
        if ( optInfo->wordArg2->StackPushRegisterCode )
        {
            if ( optInfo->wordArg2->W_ObjectAttributes & DOBJECT )
            {
                _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2 ) ;
                Compile_Move_Rm_To_Reg ( ACC, ACC, 0, size ) ;
            }
            else
            {
                if ( ( optInfo->wordArg2->W_ObjectAttributes & OBJECT_FIELD ) && cntx->BaseObject )
                {
                    wordArg2 = cntx->BaseObject ;
                    CO_StandardArg ( wordArg2, ACC, size, optInfo->wordArg2_rvalue, wordArg2->Coding, true ) ;
                }
                else
                {
                    SetHere ( optInfo->wordArg2->Coding ) ;
                    CO_StandardArg ( optInfo->wordArg2, ACC, size, optInfo->wordArg2_rvalue, optInfo->wordArg2->Coding, true ) ;
                }
            }
            _Word_CompileAndRecord_PushReg ( optInfo->wordArg2, ACC, true, size ) ;
            optInfo->opWord->StackPushRegisterCode = optInfo->wordArg2->StackPushRegisterCode ; // for Compiler_RemoveLocalFrame ??
        }
        else if ( bi->CmpCode )
        {
            SetHere ( optInfo->wordArg2->Coding ) ;
            CO_StandardArg ( optInfo->wordArg2, ACC, size, optInfo->wordArg2_rvalue, optInfo->wordArg2->Coding, true ) ;
        }
        else CO_Load_TOS ( ) ;
    }
    else CO_Load_TOS ( ) ;
    Word_SetSourceCoding ( optInfo->opWord, 0 ) ; // we don't need to see its source code
    optInfo->rtrn = CO_DONE ;
}

void
CO_Dup ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 0 ) ;
    if ( optInfo->wordArg2 && optInfo->wordArg2->StackPushRegisterCode )
    {
        _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2 ) ;
        Compile_ADDI ( REG, DSP, 0, 2 * CELL, 0 ) ;
        _Compile_Move_Reg_To_StackN ( DSP, 0, ACC, 0 ) ;
        _Compile_Move_Reg_To_StackN ( DSP, - 1, ACC, 0 ) ;
    }
    else
    {
        Compile_Move_Rm_To_Reg ( ACC, DSP, 0, 0 ) ;
        Compile_ADDI ( REG, DSP, 0, sizeof (int64 ), 0 ) ;
        Compile_Move_Reg_To_Rm ( DSP, ACC, 0, 0 ) ;
    }
    optInfo->rtrn = CO_DONE ;
}

// +=, -=, *=, /=, etc.

Boolean
CO_X_OpEqual_Literal ( Word * srcWord, block op, int8 mod, int8 rm, int64 disp )
{
    if ( op == CSL_BitWise_OR )
    {
        _Compile_X_Group1_Immediate ( OR, mod, rm, disp, srcWord->W_Value, srcWord->CompiledDataFieldByteSize ) ;
        goto done ;
    }
    else if ( op == CSL_BitWise_AND )
    {
        _Compile_X_Group1_Immediate ( AND, mod, rm, disp, srcWord->W_Value, srcWord->CompiledDataFieldByteSize ) ;
        goto done ;
    }
    else if ( op == CSL_Plus )
    {
        _Compile_X_Group1_Immediate ( ADD, mod, rm, disp, srcWord->W_Value, srcWord->CompiledDataFieldByteSize ) ;
        goto done ;
    }
    else if ( op == CSL_Minus )
    {
        _Compile_X_Group1_Immediate ( SUB, mod, rm, disp, srcWord->W_Value, srcWord->CompiledDataFieldByteSize ) ;
        goto done ;
    }
    return false ;
done:
    return true ;
}

void
CO_X_OpEqual ( Compiler * compiler, block op )
{
    Word * zero = CSL_WordList ( 0 ) ;
    uint8 valueReg = ACC, mdFlag = 0, shFlag = 0, mod, rm ;
    int64 disp ;
    if ( Is_DebugOn ) _CSL_SC_WordList_Show ( 0, 0, 0 ) ;

    CO_GetWordStackState ( compiler, zero ) ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ; // nb. after _Compiler_GetOptimizeState
    Word * dstWord = optInfo->wordArg1, * srcWord = optInfo->wordArg2 ;
    Boolean dstReg = dstWord ? ( dstWord->RegToUse ? dstWord->RegToUse : ACC ) : 0 ; ///*arg1*/, srcReg = OREG ; // arg2
    Boolean srcReg = srcWord ? ( srcWord->RegToUse ? srcWord->RegToUse : OREG ) : 0 ; ///*arg1*/, srcReg = OREG ; // arg2
    CO_SetStandardPreHere_ForDebugDisassembly ( compiler ) ;
    if ( ( ! optInfo->NumberOfArgs ) || optInfo->xBetweenArg1AndArg2 ) SyntaxError ( 1 ) ; // this case may be handled better than with SyntaxError
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
    if ( optInfo->NumberOfArgs == 2 )
    {
        if ( ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ) mod = REG, rm = dstReg, disp = 0 ;
        else mod = MEM, rm = FP, disp = LocalOrParameterVar_Disp ( dstWord ) ; //dstWord /// what about if ( srcWord->CAttribute & REGISTER_VARIABLE ) ???
        SetHere ( dstWord->Coding ) ;
        //DBI_ON ;
        if ( ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ) //dstWord /// what about if ( srcWord->CAttribute & REGISTER_VARIABLE ) ???
        {
            if ( optInfo->wordArg2_literal )
            {
                if ( CO_X_OpEqual_Literal ( srcWord, op, mod, rm, disp ) ) goto done ;
            }
        }
        else if ( ( op == CSL_Multiply ) || ( op == CSL_Divide ) )
        {
            SetHere ( dstWord->Coding ) ;
            _Compile_GetVarLitObj_RValue_To_Reg ( srcWord, OREG, srcWord->CompiledDataFieldByteSize ) ;
            _Compile_GetVarLitObj_RValue_To_Reg ( dstWord, ACC, dstWord->CompiledDataFieldByteSize ) ;
            mdFlag = 1 ;
        }
        else if ( ( op == CSL_ShiftLeft ) || ( op == CSL_ShiftRight ) )
        {
            SetHere ( dstWord->Coding ) ;
            _Compile_GetVarLitObj_RValue_To_Reg ( srcWord, OREG, srcWord->CompiledDataFieldByteSize ) ;
            shFlag = 1 ;
        }
        else
        {
            if ( optInfo->wordArg2_literal )
            {
                if ( CO_X_OpEqual_Literal ( srcWord, op, mod, rm, disp ) ) goto done ;
            }
            _Compile_GetVarLitObj_RValue_To_Reg ( srcWord, ACC, srcWord->CompiledDataFieldByteSize ) ;
        }
    }
    else if ( compiler->LHS_Word )
    {
        if ( optInfo->lparen2 )
        {
            if ( optInfo->lparen2->StackPushRegisterCode )
            {
                SetHere ( optInfo->lparen2->StackPushRegisterCode ) ;
                Compile_Move_Reg_To_Reg ( OREG, ACC, 0 ) ;
            }
        }
        else CO_StandardArg ( srcWord, OREG, 0, 1, 0, true ) ; //nb! rvalue
        CO_StandardArg ( compiler->LHS_Word, OREG2, 0, 0, 0, true ) ; //nb! lvalue
        CO_StandardArg ( compiler->LHS_Word, ACC, 0, 1, 0, false ) ; //nb! rvalue
        if ( ! optInfo->lparen2 ) CO_StandardArg ( srcWord, OREG, 0, 1, 0, true ) ; //nb! rvalue
        valueReg = ACC ;
    }
    else SyntaxError ( 1 ) ;
    //else args should be set up to standard locations by Compiler_SetupArgsToStandardLocations and Setup_MachineCodeInsnParameters
    if ( mdFlag == 1 ) // mult div
    {
        optInfo->CO_Dest_RegOrMem = REG ;
        optInfo->CO_Mod = REG ;
        optInfo->CO_Reg = ACC ; //( srcWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? srcReg : ACC ;
        optInfo->CO_Rm = OREG ; //( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? dstReg : FP ;
        optInfo->CO_Disp = 0 ; //( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? 0 : LocalOrParameterVar_Disp ( dstWord ) ;
    }
    else if ( shFlag == 1 ) // shl shr
    {
        optInfo->CO_Dest_RegOrMem = MEM ;
        optInfo->CO_Mod = MEM ;
        optInfo->CO_Reg = OREG ; //( srcWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? srcReg : ACC ;
        optInfo->CO_Rm = ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? dstReg : FP ;
        optInfo->CO_Disp = ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? 0 : LocalOrParameterVar_Disp ( dstWord ) ;
    }
    else
    {
        // with direct to mem rm = reg and reg = rm
        optInfo->CO_Dest_RegOrMem = ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? REG : MEM ;
        optInfo->CO_Mod = ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? REG : MEM ;
        optInfo->CO_Rm = ( srcWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? srcReg : FP ;
        optInfo->CO_Reg = ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? dstReg : ACC ;
        optInfo->CO_Disp = ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) ? 0 : LocalOrParameterVar_Disp ( dstWord ) ;
    }
doOp:
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
    compiler->OptimizeForcedReturn = 1 ;
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
    Block_Eval ( op ) ;
    compiler->OptimizeForcedReturn = 0 ;
    SetHere ( CSL_WordList ( 0 )->StackPushRegisterCode ) ;
    if ( mdFlag == 1 )
    {
        Compile_Move_Reg_To_Rm ( FP, ACC, LocalOrParameterVar_Disp ( dstWord ), CELL ) ;
    }

    SetState ( _CSL_, IN_OPTIMIZER, false ) ;
done:
    //DBI_OFF ;
    return ;
}
// Compile_X_Literal <= Compile_Equal_Literal, Compile_Store_Literal

int64
CO_X_Literal ( Word * dstWord, Word * literalWord, int lvalueSize, Boolean dstReg, Boolean srcReg, Word * codingWord )
{
    int64 rtrn ;
    //DBI_ON ;
    CompileOptimizeInfo * optInfo = _Compiler_->OptInfo ; // nb. after _Compiler_GetOptimizeState
    if ( dstWord && ( dstWord->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
        && ( ! ( dstWord->W_ObjectAttributes & ( THIS | STRUCT | C_TYPE | C_CLASS | OBJECT ) ) ) )
    {
        if ( dstWord && ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) )
        {
            //DBI_ON ;
            SetHere ( codingWord->Coding ) ; // old 
            Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
            Compile_MoveImm_To_Reg ( dstWord->RegToUse, literalWord->W_Value, lvalueSize ) ;
            SetState ( _CSL_, IN_OPTIMIZER, false ) ;
            rtrn = DONE ;
            //DBI_OFF ;
        }
        else
        {
            SetHere ( codingWord->Coding ) ; // old 
            Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
            //Compile_MoveImm ( MEM, FP, LocalOrParameterVar_Disp ( dstWord ), literalWord->W_Value, lvalueSize ) ;
            Compile_MoveImm ( MEM, FP, LocalOrParameterVar_Disp ( dstWord ), literalWord->W_Value, lvalueSize ) ;
            SetState ( _CSL_, IN_OPTIMIZER, false ) ;
            rtrn = DONE ;
        }
    }
    else
    {
        SetHere ( literalWord->Coding ) ; // old 
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
        if ( dstWord && ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) )
        {
            Compile_MoveImm_To_Reg ( dstReg, literalWord->W_Value, lvalueSize ) ;
            rtrn = DONE ;
        }
        else
        {
            Compile_MoveImm_To_Reg ( srcReg, literalWord->W_Value, lvalueSize ) ;
            rtrn = NOT_DONE ;
        }
    }
    //DBI_OFF ;
    return rtrn ;
}

// remember this is rpn code so arg 1, the first (linearly) arg, is lower on the stack (STACK_1) than arg2, the second arg, (STACK_0)
// eg:
//      : test arg1 arg2 op ;  
// stack conventions :
// ________
// | arg2 | TOS : OREG : RCX
// | arg1 | NOS : ACC  : RAX
// --------
// need to consider also REGISTER args/parameters !!
// but we still need to consider types (sizes) and type checking
// and ARM cpu adjustments

// get arg actual setup
// convert arg setup to standard arg1=ACC, arg2=OREG (RCX)
// setup machine insn parameters 

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 

// this function probably could be improved/optimized 

void
CO_X_Equal ( Compiler * compiler, int64 op, int lvalueSize )
{
    Word * zero = CSL_WordList ( 0 ) ;
    if ( Is_DebugOn ) _CSL_SC_WordList_Show ( 0, 0, 1 ) ;
    CompileOptimizeInfo * optInfo = CO_GetWordStackState ( compiler, zero ) ; //compiler->OptInfo ; // nb. after _Compiler_GetOptimizeState
    CO_SetStandardPreHere_ForDebugDisassembly ( compiler ) ;
    CO_SetupArgsToStandardLocations ( compiler ) ;
    //if ( Is_DebugOn ) Compiler_ShowOptimizeArgs ( compiler ) ;
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
    if ( op == STORE )
    {
        Word * dstWord = optInfo->wordArg2, * srcWord = optInfo->wordArg1 ;
        Boolean dstReg = OREG /*arg2*/, srcReg = ACC ; // arg1

        if ( optInfo->wordArg1_literal ) // srcWord
        {
            if ( CO_X_Literal ( dstWord, optInfo->wordArg1, lvalueSize, srcReg, dstReg, srcWord ) == DONE ) goto done ; //return ;
        }
        else if ( optInfo->wordArg2 && ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) )
        {
            dstReg = dstWord->RegToUse ;
            srcReg = srcWord->RegToUse ;
            if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) SetHere ( optInfo->wordArg1->Coding ) ;
            else SetHere ( optInfo->wordArg2->Coding ) ;
            if ( dstReg != srcReg ) Compile_Move_Reg_To_Reg ( dstReg, srcReg, 0 ) ;
            goto done ; //return ;
        }
        if ( dstWord && srcWord && ( ( dstWord->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
            || ( ( dstWord->W_ObjectAttributes & ( THIS | STRUCT | C_TYPE | C_CLASS | OBJECT ) ) ) ) )
        {
            if ( srcWord->W_BaseObject )
                SetHere ( srcWord->W_BaseObject->StackPushRegisterCode ) ;
            else SetHere ( srcWord->StackPushRegisterCode ) ;
            if ( srcWord && ( srcWord->W_ObjectAttributes & OBJECT ) && ( GetState ( _Context_, IS_FORWARD_DOTTED ) ) ) //GetState ( _Context_, (C_SYNTAX | INFIX_MODE) ) ) //if ( GetState ( srcWord, IS_RVALUE ) || GetState ( _Interpreter_->LastWord, IS_RVALUE ) ) 
                Compile_Move_Rm_To_Reg ( srcReg, srcReg, 0, lvalueSize ) ;
            else SetHere ( dstWord->Coding ) ;
            Compile_Move_Reg_To_Rm ( FP, srcReg, LocalOrParameterVar_Disp ( dstWord ), CELL ) ;
            goto done ; //return ;
        }
        //DBI_ON ;
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
        if ( srcWord && ( srcWord->W_ObjectAttributes & O_POINTER ) ) //GetState ( _Context_, (C_SYNTAX | INFIX_MODE) ) ) //if ( GetState ( srcWord, IS_RVALUE ) || GetState ( _Interpreter_->LastWord, IS_RVALUE ) ) 
            Compile_Move_Rm_To_Reg ( srcReg, srcReg, 0, lvalueSize ) ;
        //Compile_Move_Reg_To_Rm ( dstReg, srcReg, 0, lvalueSize ) ; //optInfo->wordArg1->ObjectSize ) ; // dst = reg ; src = rm
        Compile_Move_Reg_To_Rm ( dstReg, srcReg, 0, dstWord ? dstWord->CompiledDataFieldByteSize : lvalueSize ) ;
        //DBI_OFF ;
        goto done ;
    }
    else if ( op == EQUAL )
    {
        Word * dstWord = optInfo->wordArg1, * srcWord = optInfo->wordArg2 ;
        Boolean dstReg = ACC /*arg1*/, srcReg = OREG ; // arg2
        if ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) dstReg = optInfo->wordArg1->RegToUse ;
        else dstReg = ACC ;
        if ( ( srcWord && ( srcWord->W_ObjectAttributes & REGISTER_VARIABLE ) ) || optInfo->wordArg2_Op )
        {
            srcReg = ( srcWord->RegToUse != ACC ) ? srcWord->RegToUse : OREG ;
        }
        else srcReg = OREG ;
        if ( optInfo->wordArg2_literal ) //srcWord
        {
            if ( CO_X_Literal ( optInfo->wordArg1, srcWord, lvalueSize, dstReg, srcReg, optInfo->wordArg1 ) == DONE ) goto done ; //return ;
        }
        if ( dstWord && srcWord && ( srcWord->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
            && ( ! ( srcWord->W_ObjectAttributes & ( THIS | STRUCT | C_TYPE | C_CLASS | OBJECT ) ) ) )
        {
            //DBI_ON ;
            if ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) SetHere ( dstWord->Coding ) ;
            else SetHere ( dstWord->StackPushRegisterCode ) ;
            if ( ! ( srcWord->W_ObjectAttributes & REGISTER_VARIABLE ) )
            {
                Compile_Move_Rm_To_Reg ( srcReg, FP, LocalOrParameterVar_Disp ( srcWord ), CELL ) ;
                if ( srcWord && ( srcWord->W_ObjectAttributes & O_POINTER ) ) 
                    Compile_Move_Rm_To_Reg ( srcReg, srcReg, 0, lvalueSize ) ;
            }
            else srcReg = srcWord->RegToUse ;
            if ( dstWord->W_ObjectAttributes & REGISTER_VARIABLE ) Compile_Move_Reg_To_Reg ( dstReg, srcReg, lvalueSize ) ;
            else Compile_Move_Reg_To_Rm ( dstReg, srcReg, 0, lvalueSize ) ;
            //DBI_OFF ;
            //if ( Is_DebugOn ) iPrintf ( "\n%s", Context_Location ( ) ), Pause ( ) ;
            goto done ; //return ;
        }
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
        if ( ( optInfo->NumberOfArgs == 1 ) && optInfo->wordArg2_Op )
        {
            Word * word = compiler->OptInfo->wordArg0_ForOpEqual ;
            if ( word )
            {
                Compile_Move_Reg_To_Reg ( word->RegToUse, srcWord->RegToUse, 0 ) ; // & 0xf turn off REG_ON_BIT
                if ( word->StackPushRegisterCode )
                {
                    byte * src = word->StackPushRegisterCode + STACK_PUSH_REGISTER_CODE_SIZE ;
                    BI_Block_Copy ( 0, word->StackPushRegisterCode, src, Here - src, 0 ) ;
                }
                compiler->OptInfo->wordArg0_ForOpEqual = 0 ;
                goto done ; //return ;
            }
        }
        //SetHere ( optInfo->wordArg1->Coding, 0 ) ;
        if ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) Compile_Move_Reg_To_Reg ( dstReg, srcReg, 0 ) ;
        else Compile_Move_Reg_To_Rm ( dstReg, srcReg, 0, dstWord ? dstWord->CompiledDataFieldByteSize : lvalueSize ) ;
    }
    else if ( ! optInfo->rtrn ) CO_Setup_MachineCodeInsnParameters ( compiler, REG, REG, ACC, OREG, 0, 0 ) ;
done:
    SetState ( _CSL_, IN_OPTIMIZER, false ) ;
}
// skip back WordStack words for the args of an op parameter in GetOptimizeState

void
CO_StandardArg ( Word * word, Boolean reg, Boolean size, Boolean rvalueFlag, byte * setHere, Boolean setScaFlag )
{
    if ( setHere ) SetHere ( setHere ) ;
    if ( setScaFlag ) Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    if ( ! size ) size = 8 ; //word->CompiledDataFieldByteSize ;
    if ( rvalueFlag -- )
    {
        if ( word->W_ObjectAttributes & REGISTER_VARIABLE ) reg = word->RegToUse ;
        else Compile_GetVarLitObj_RValue_To_Reg ( word, reg, 0 ) ; // nb. 0 -> size = 8 here!
        while ( rvalueFlag -- ) Compile_Move_Rm_To_Reg ( reg, reg, 0, size ) ;
    }
    else _Compile_GetVarLitObj_LValue_To_Reg ( word, reg, size ) ;
    if ( ! (_LC_ && GetState ( _LC_, LC_APPLY ) ) ) 
        word->StackPushRegisterCode = Here ; // we are not pushing this but in case we are just rewriting the code in the next arg ?
}

void
CO_StackArgsToStandardRegs ( Compiler * compiler )
{

    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    _Compile_Move_StackN_To_Reg ( OREG, DSP, 0, 0 ), optInfo->CO_Rm = OREG ;
    _Compile_Move_StackN_To_Reg ( ACC, DSP, - 1, 0 ), optInfo->CO_Reg = ACC | REG_LOCK_BIT ;
    Compile_SUBI ( REG, DSP, 0, 2 * CELL, 0 ) ;
    SetState ( optInfo, STACK_ARGS_TO_STANDARD_REGS, true ) ;
}

Boolean
CO_CheckForRegisterVariable ( Compiler * compiler, Boolean reg )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ( reg == OREG ) && ( optInfo->wordArg2 && ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) reg = optInfo->wordArg2->RegToUse ;
    else if ( ( reg == ACC ) && ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) reg = optInfo->wordArg1->RegToUse ;

    return reg ;
}

void
CO_PeepHole_CO_ForStackPopToReg ( )
{
    if ( GetState ( _CSL_, CO_ON ) )
    {
        byte * here = _O_CodeByteArray->EndIndex ;
        byte add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8 [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x06, 0x49, 0x8b, 0x06, 0x49, 0x83, 0xee, 0x08 } ;
        if ( ! memcmp ( add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8, here - 14, 14 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 14 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 14 ) ;
        }
    }
}

void
CO_PeepHole_Optimize ( )
{
    if ( GetState ( _CSL_, CO_ON ) )
    {
        byte * here = _O_CodeByteArray->EndIndex ;
        byte sub_r14_0x8__add_r14_0x8 [ ] = { 0x49, 0x83, 0xee, 0x08, 0x49, 0x83, 0xc6, 0x08 } ;
        byte add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8 [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x06, 0x49, 0x8b, 0x06, 0x49, 0x83, 0xee, 0x08 } ;
        //byte add_r14_0x8__mov_r14_rdi__mov_rax_r14__sub_r14_0x8 [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x3e, 0x49, 0x8b, 0x06, 0x49, 0x83, 0xee, 0x08 } ;
        //byte add_esi_04__mov_tos_eax_sub_esi_04 [ ] = { 0x83, 0xc6, 0x04, 0x89, 0x06, 0x83, 0xee, 0x04 } ;
        //byte mov_eax_tos_sub_esi_04_test_eax_eax [ ] = { 0x89, 0x06, 0x83, 0xee, 0x04, 0x85, 0xc0 } ;
        byte mov_r14_rax__mov_rax_r14 [] = { 0x49, 0x89, 0x06, 0x49, 0x8b, 0x06 } ;
        byte add_rax_0x0 [] = { 0x48, 0x81, 0xc0, 0x00, 0x00, 0x00, 0x00 } ;
        byte test_rax_rax_test_rax_rax [] = { 0x48, 0x85, 0xc0, 0x48, 0x85, 0xc0 } ;
        if ( ! memcmp ( sub_r14_0x8__add_r14_0x8, here - 8, 8 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 8 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 8 ) ;
        }
        else if ( ! memcmp ( add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8, here - 14, 14 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 14 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 14 ) ;
        }
        else if ( ! memcmp ( mov_r14_rax__mov_rax_r14, here - 6, 6 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 6 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 6 ) ;
        }
        else if ( ! memcmp ( add_rax_0x0, here - 7, 7 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 7 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 7 ) ;
        }
        else if ( ! memcmp ( test_rax_rax_test_rax_rax, here - 6, 6 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 6 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 6 ) ;
        }
#if 0        
        else if ( ! memcmp ( add_r14_0x8__mov_r14_rdi__mov_rax_r14__sub_r14_0x8, here - 14, 14 ) )
        {
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 14 ) ;
        }
            // this occurs one time at startup in _assertStkChk : change it where it is caused and eliminate testing every instruction !! 
        else if ( ! memcmp ( mov_eax_tos_sub_esi_04_test_eax_eax, here - 7, 7 ) )
        {

            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 7 ) ;
            Compile_TEST_Reg_To_Reg ( ACC, ACC ) ;
        }
#endif        
    }
}

void
CO_GetRmDispImm ( CompileOptimizeInfo * optInfo, Word * word, int64 suggestedReg )
{
    if ( suggestedReg == - 1 ) suggestedReg = ACC ;
    optInfo->CO_Reg = suggestedReg ;
    if ( word->W_ObjectAttributes & REGISTER_VARIABLE )
    {
        optInfo->OptimizeFlag |= CO_REGISTER ;
        optInfo->CO_Dest_RegOrMem = REG ;
        optInfo->CO_Mod = REG ;
        //optInfo->CO_Rm = optInfo->CO_Reg = ( suggestedReg != - 1 ) ? suggestedReg : word->RegToUse ;
        optInfo->CO_Rm = optInfo->CO_Reg = word->RegToUse ;
    }
    else if ( word->W_ObjectAttributes & LOCAL_VARIABLE )
    {
        optInfo->CO_Rm = FP ;
        optInfo->CO_Disp = LocalVar_Disp ( word ) ;
        optInfo->OptimizeFlag |= CO_RM ;
    }
    else if ( word->W_ObjectAttributes & PARAMETER_VARIABLE )
    {
        optInfo->CO_Rm = FP ;
        optInfo->CO_Disp = ParameterVar_Disp ( word ) ;
        optInfo->OptimizeFlag |= CO_RM ;
    }
    else if ( word->W_ObjectAttributes & ( LITERAL | CONSTANT ) )
    {
        optInfo->CO_Imm = ( int64 ) word->W_Value ;
        optInfo->OptimizeFlag |= CO_IMM ;
    }
    else if ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) )
    {
        if ( suggestedReg == - 1 ) suggestedReg = ACC ;
        //_Compile_Move_Literal_Immediate_To_Reg ( suggestedReg, ( int64 ) word->W_PtrToValue ) ; // ?? should this be here ??
        optInfo->CO_Rm = suggestedReg ;
        optInfo->OptimizeFlag |= CO_RM ;
    }
    else //if ( word->CAttribute & CATEGORY_OP_1_ARG )
    {
        optInfo->CO_Rm = DSP ;
        optInfo->OptimizeFlag |= CO_RM ;
    }
}

void
CO_Load_TOS ( )
{
    Compile_Move_Rm_To_Reg ( ACC, DSP, 0, 0 ) ;
    Compile_Move_Rm_To_Reg ( ACC, ACC, 0, 0 ) ;
    Compile_Move_Reg_To_Rm ( DSP, ACC, 0, 0 ) ;
}

void
Word_Set_StackPushRegisterCode_To_Here ( Word * word )
{
    word->StackPushRegisterCode = Here ;
}

void
_Set_To_Here_Word_StackPushRegisterCode ( Word * word )
{
    SetHere ( word->StackPushRegisterCode ) ;
}

Boolean
Word_Check_ReSet_To_Here_StackPushRegisterCode ( Word * word )
{
    if ( word && word->StackPushRegisterCode )
    {
        _Set_To_Here_Word_StackPushRegisterCode ( word ) ;
        return true ;
    }
    else return false ;
}

