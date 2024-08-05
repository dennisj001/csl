
#include "../../include/csl.h"

// logic needs improving!  cf. logic.png
// 'setTttn' is a notation from the intel manuals

void
BI_SetTttN ( BlockInfo *bi, Boolean ttt, Boolean n, byte * address, int jccType )
{
    if ( bi )
    {
        bi->Ttt = ttt ;
        bi->N = n ;
        bi->TttnCode = address ? address : Here ;
        bi->JccType = jccType ;
    }
}

// should be used only if we are sure we are done with the exiting logic

void
BI_ResetLogicCode ( BlockInfo *bi ) 
{
    if ( bi )
    {
        bi->Ttt = 0, bi->N = 0, bi->JccCode = 0, bi->TttnCode = 0, bi->SetccCode = 0, bi->TestCode = 0, bi->CmpCode = 0, bi->BI_Gi = 0 ;
    }
}

BlockInfo *
Compiler_SetBiTttN ( Compiler * compiler, Boolean ttt, Boolean n, int jccType )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    BI_SetTttN ( bi, ttt, n, 0, jccType ) ;
    return bi ;
}

void
BI_CompileRecord_CmpCode_Reg ( BlockInfo *bi, int64 reg )
{
    bi->CmpCode = Here ;
    Compile_CmpCode_Reg ( reg, 0 ) ;
}

// we only have to jmp to the 'FALSE' (else/after)  else we would continue with the 'TRUE' : eg. if x else y
/*
 * == != < > <= >= && || ! & | ^ << >>  :not: + - * / ++ -- %
 * Logic rules (guidelines) for logic words in a control block :
 * distinctions:
 * combinator do block before or after control block
 * and/or, logical ops, single variables, other function words :
 * && and || or ! not | & << >> xor ~ 
 * not zero is true ; 0 or false
 *      
 * -- standard def : ttt n : common sense n : given in logic : common sense *true* meanings of logical operators 
 * - and : if false -> false :: or if true -> true
 * - and : multiple meta op
 *      control before do
 *          ttt, ! n, to false ; fall thru to true
 *      control after do
 *          ttt, ! n, to false ; final added jcc to true by Compile_BlockLogicTest
 * - or : multiple meta op
 *      control before do
 *          ttt, n, to true (do) : final added jcc to false by Compile_BlockLogicTest
 *      control after do
 *          ttt, n, to true (do) : fall thru to false
 * - other logical ops : single with 1 ( !, not, ~ ) or 2 args ( ==, !=, <=, >=) 
 *      dominated by : and, or, !
 *      cmp 
 *      control before do
 *          ttt, ! n, to false : fall thru to true
 *      control after do
 *          ttt, n, to true (do) : fall thru to false
 * - single variables :
 *      xor 0 : TTT_ZERO, NEGFLAG_NZ (zero flag not set by xor 0) => jnz to true; jz to false
 * 
 * - ops: &, |, ^, xor, <<, >>, +, -, /, *, % ) 
 * 
 * - other functions/ops : must return a value in reg or on stack : should be marked accordingly when defined
 *      only need a jccGoto at the end of a control block
 *
 * state info needed : 
 *      control before or after do : easy in C syntax ? in non-C syntax
 *      and/or domination state :
 *  
 * */
// most of this is just based on patterns yet - not a high level, top down design ??
// but now we have pretty complete info from ReadLiner_LookAroundFor_Logic better design should be possible ??!

int64
SetTttnJccGotoInfo_DoOr ( BlockInfo * bi, Rllafl * r )
{
    int64 n, jccType = 0 ;
    int64 rtrn = r->rtrn ;
    if ( ( rtrn & ( LT_AND_PREVIOUS ) ) && ( rtrn & ( LT_2_RPAREN_PREVIOUS ) ) )
        n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_FALSE ;
    else if ( ! ( rtrn & ( LT_OR_NEXT ) ) )
    {
        n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_FALSE ;
        if ( rtrn & LT_END_OF_BLOCK ) bi->State |= BI_FINISHED ;
    }
    else n = bi->Ttt ? bi->N : N_1, jccType = GI_JCC_TO_TRUE ;
    if ( jccType )
    {
        _BI_SetTttnJccGotoInfo ( bi, n, jccType ) ;
        return 1 ;
    }
    else return 0 ; // ??
}

int64
SetTttnJccGotoInfo_DoAnd ( BlockInfo * bi, Rllafl * r )
{
    int64 n, jccType = 0 ;
    int64 rtrn = r->rtrn ;

    if ( rtrn & ( LT_OR_PREVIOUS | LT_OR_NEXT ) ) SetTttnJccGotoInfo_DoOr ( bi, r ) ;
    else if ( ! ( rtrn & LT_2_LPAREN_PREVIOUS ) )
    {
        n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_FALSE ;
    }
    else n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_FALSE ;
    if ( jccType )
    {
        _BI_SetTttnJccGotoInfo ( bi, n, jccType ) ;
        return 1 ;
    }
    else return 0 ; //??
}

int64
BI_Check_SetTttnJccGotoInfo ( BlockInfo * bi, Rllafl * r, int forVar )
{
    int64 n, jccType = 0, rtrn ;
    if ( bi && r )
    {
        rtrn = r->rtrn ;
        if ( rtrn )
        {
            if ( forVar && ( rtrn & ( LT_AND_PREVIOUS | LT_AND_NEXT ) ) ) n = N_0, jccType = GI_JCC_TO_FALSE ;
            else if ( forVar && ( rtrn & ( LT_OR_PREVIOUS | LT_OR_NEXT ) ) )
            {
                if ( C_SyntaxOn && ( rtrn & LT_END_OF_BLOCK ) && ( ! ( rtrn & LT_ID ) ) ) //(rtrn & LT_END_OF_BLOCK ) 
                    n = N_0, jccType = GI_JCC_TO_FALSE, bi->State |= BI_FINISHED ;
                else n = N_1, jccType = GI_JCC_TO_TRUE ;
            }
            else if ( ( rtrn & ( LT_AND_PREVIOUS | LT_AND_NEXT ) ) ) return SetTttnJccGotoInfo_DoAnd ( bi, r ) ;
            else if ( rtrn & ( LT_OR_PREVIOUS | LT_OR_NEXT ) ) return SetTttnJccGotoInfo_DoOr ( bi, r ) ;
            else if ( C_SyntaxOn && ( rtrn & LT_END_OF_BLOCK ) && ( ! ( rtrn & LT_ID ) ) )
            {
                n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_FALSE ;
            }
        }
        if ( jccType )
        {
            _BI_SetTttnJccGotoInfo ( bi, n, jccType ) ;
            return 1 ;
        }
    }
    return 0 ;
}

void
_Compile_GetCmpLogicFromTOS ( BlockInfo *bi )
{
    //DBI_ON ;
    byte * setCc, * movedSetCc ;
    if ( bi->SetccCode )
    {
        movedSetCc = CSL_CheckSetccMovedList ( bi, bi->SetccCode ) ;
        if ( movedSetCc ) setCc = movedSetCc ;
        else setCc = bi->SetccCode ;
        SetHere ( setCc ) ;
    }
    else
    {
        // for special case optimization when control block is a register variable : setup by CSL_Interpret_C_Blocks
        // new, not thoroughly tested
        if ( bi->RegisterVariableControlWord )
        {
            if ( bi->RegisterVariableControlWord->Coding )
            {
                SetHere ( bi->RegisterVariableControlWord->Coding ) ;
                bi->bp_First = Here ;
            }
            else return ; //SetHere ( bi->RegisterVariableControlWord->StackPushRegisterCode ) ;
            Compile_CMPI ( REG, bi->RegisterVariableControlWord->RegToUse, 0, 0, 0 ) ;
            bi->RegisterVariableControlWord->Coding = 0 ;
        }
        else
        {
            Compile_Pop_To_Acc ( DSP ) ;
            Compile_CMPI ( REG, ACC, 0, 0, 0 ) ;
        }
    }
    _BI_SetTttnJccGotoInfo ( bi, bi->N ? bi->N :
        ( bi->LogicCodeWord && ( bi->LogicCodeWord->Definition == CSL_LogicalNot ) ) ? N_1 : N_0,
        GI_JCC_TO_FALSE ) ;
    //DBI_OFF ;
}

void
Compile_BlockLogicTest ( BlockInfo * bi )
{
    int64 n, jccType = 0 ;
    if ( bi )
    {
        Word * combinator = _Context_->CurrentCombinator ;
        if ( ( bi->State & ( BI_FINISHED | BI_2_LPAREN ) ) && ( ! _LC_ ) )return ;
        Word * lcw = bi->LogicCodeWord ;
        if ( ! bi->JccCode )
        {
            if ( ( ! bi->CmpCode ) ) _Compile_GetCmpLogicFromTOS ( bi ), bi->JccAddedCode = bi->JccCode, bi->JccCode = 0 ;
            else if ( lcw && ( lcw->W_MorphismAttributes & RAX_RETURN ) ) jccType = GI_JCC_TO_FALSE ;
            else if ( lcw && ( lcw->W_MorphismAttributes & CATEGORY_LOGIC ) )
            {
                if ( combinator && ( combinator->W_MorphismAttributes & DO_BEFORE_CONTROL ) ) //Definition == ( block ) CSL_DoWhileCombinator ) )
                {
                    //n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_TRUE ;
                    n = bi->Ttt ? bi->N : N_0, jccType = GI_JCC_TO_TRUE ;
                }
                else if ( ( bi->Ttt ) && ( ! ( bi->State & BI_AND ) ) )
                {
                    n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_FALSE ;
                }
            }
        }
        else if ( combinator && ( combinator->W_MorphismAttributes & ( DO_BEFORE_CONTROL ) ) ) //|PREFIX_COMBINATOR) ) )
        {
            //n = bi->Ttt ? bi->N : N_1, jccType = GI_JCC_TO_TRUE ;
            n = bi->Ttt ? ! bi->N : N_0, jccType = GI_JCC_TO_TRUE ;
            SetHere ( bi->JccCode ) ;
            GotoInfo_Remove ( ( dlnode* ) bi->BI_Gi ) ;
        }
    }
    if ( jccType ) _BI_SetTttnJccGotoInfo ( bi, n, jccType ), bi->JccAddedCode = bi->JccCode, bi->JccCode = 0 ;
}

int64
Compile_Logic_LookAround_CheckSetBI_Ttt_JccGotoInfo ( Compiler * compiler )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
    return BI_Check_SetTttnJccGotoInfo ( bi, r, 0 ) ;
}

// we only have to jmp to the 'FALSE' (else/after)  else we would continue with the 'TRUE' : eg. if x else y

void
Compiler_Set_BI_TttnAndCompileJccGotInfo ( Compiler * compiler, Boolean n )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    _BI_SetTttnJccGotoInfo ( bi, n, GI_JCC_TO_FALSE ) ;
}

// called by _Interpreter_Before_DoInfixPrefixableWord 

inline void
SetccToReg ( Boolean ttt, Boolean n, Boolean reg )
{
    _Compile_SETcc ( ttt, n, reg ) ;
    _Compile_MOVZX_BYTE_REG ( reg, reg ) ;
}

void
BI_SetccAndStackPushAcc ( BlockInfo *bi, Boolean ttt, Boolean n )
{
    Word *zero = CSL_WordList ( 0 ) ;
    byte reg = ACC ; //nb! reg should always be ACC! : immediately after the 'cmp' insn which changes the flags appropriately
    bi->SetccCode = Here ;
    //DBI_ON ;
#if OLD_Setcc    
    SetccToReg ( ttt, n, reg ) ;
    bi->BI_StackPushRegisterCode = Here ;
    _Word_CompileAndRecord_PushReg ( zero, reg, true, 0 ) ;
#else    
    Compile_MoveImm_To_TOS ( R14, 0, 8 ) ;
    _Compile_SETccRm ( ttt, n, R14 ) ;
#endif    
    //DBI_OFF ;
}

void
Finish_Compile_Cmp_Set_Tttn_Logic ( Compiler * compiler, Word * zero, Boolean ttt, Boolean n )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    if ( zero->Definition == CSL_LogicalNot )
    {
        if ( bi->LogicCodeWord && ( bi->LogicCodeWord->Definition == CSL_LogicalNot ) )
        {
            if ( bi->SetccCode ) SetHere ( bi->SetccCode ) ;
            {
                BI_SetTttN ( bi, ttt, bi->N ? ! bi->N : N_1, 0, 0 ) ; // assume we always have a negandum
                BI_SetccAndStackPushAcc ( bi, ttt, bi->N ) ;
                bi->CmpCode = 0 ; // affects Compile_BlockLogicTest
            }
        }
        else
        {
            //DBI_ON ;
            SetHere ( zero->Coding ) ;
            BI_CompileRecord_CmpCode_Reg ( bi, ACC ) ;
            BI_SetccAndStackPushAcc ( bi, ttt, n ) ;
            bi->CmpCode = 0 ; // affects Compile_BlockLogicTest
            //DBI_OFF ;
        }
        bi->LogicCodeWord = zero ;
    }
    else
    {
        BI_SetTttN ( bi, ttt, n, 0, 0 ) ;
        Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
        //if ( ( ( ! ( _AtCommandLine ( ) ) ) && ( ( r->rtrn & LT_END_OF_BLOCK ) && ( ! ( r->rtrn & LT_ID ) ) )
        if ( ( ( r->rtrn & LT_END_OF_BLOCK ) && ( ! ( r->rtrn & LT_ID ) ) )
            || ( C_SyntaxOn && ( compiler->CombinatorLevel || ( compiler->BlockLevel > 1 ) ) ) )
        {
            if ( ! _LC_ ) BI_Check_SetTttnJccGotoInfo ( bi, r, 0 ) ;
            if ( compiler->BlockLevel == 1 ) BI_SetccAndStackPushAcc ( bi, ttt, n ) ;
        }
        else BI_SetccAndStackPushAcc ( bi, ttt, n ) ;
    }
}
// SET : 0x0f 0x9setTtnn mod 000 rm/reg
// ?!? wanna use TEST insn here to eliminate need for _Compile_MOVZX_REG insn ?!? is that possible
// setTtn n : notation from intel manual 253667 ( N-N_0 ) - table B-10 : setTtn = condition codes, n is a negation bit
// setTtnn notation is used with the SET and JCC instructions

// note : intex syntax  : instruction dst, src
//        att   syntax  : instruction src, dst
// cmp : compares by subtracting src from dst, dst - src, and setting eflags like a "sub" insn 
// eflags affected : cf of sf zf af pf : Intel Instrucion Set Reference Manual for "cmp"
// ?? can this be done better with test/jcc ??
// want to use 'test eax, 0' as a 0Branch (cf. jonesforth) basis for all block conditionals like if/else, do/while, for ...

void
Compile_Cmp_Set_Tttn_Logic ( Compiler * compiler, Boolean ttt, Boolean n )
{
    //Is_DebugOn_DBI ;
    int64 optSetupFlag = CO_CheckOptimize ( compiler, 0 ) ;
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    Word *zero = CSL_WordList ( 0 ) ;
    //Stack_Push ( bi->LogicOpStack, (int64) zero ) ;
    Compiler_Word_SCHCPUSCA ( zero, 1 ) ;
    if ( optSetupFlag & CO_DONE ) return ;
    else
    {
        if ( optSetupFlag )
        {
            CompileOptimizeInfo * optInfo = compiler->OptInfo ;
            if ( bi->CmpCode ) SetHere ( bi->CmpCode ) ; // from DoVariable
            //else BI_SetTttN ( bi, ttt, n, 0, 0 ) ;
            if ( optInfo->OptimizeFlag & CO_IMM )
            {
                int64 size ;
                if ( optInfo->CO_Imm > 0xffffffff ) size = 8 ;
                else size = 0 ;
                SetHere ( zero->Coding ) ;
                if ( ( optInfo->wordArg1->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
                    && ( ! ( optInfo->wordArg1->W_ObjectAttributes & ( REGISTER_VARIABLE ) ) ) ) //&& 
                {
                    SetHere ( optInfo->wordArg1->Coding ) ;
                    bi->CmpCode = Here ;
                    Compile_CMPI ( MEM, FP, LocalOrParameterVar_Disp ( optInfo->wordArg1 ), optInfo->CO_Imm, size ) ;
                }
                else
                {
                    bi->CmpCode = Here ;
                    Compile_CMPI ( optInfo->CO_Mod, optInfo->CO_Reg, optInfo->CO_Disp, optInfo->CO_Imm, size ) ;
                }
            }
            else
            {
                if ( zero->Definition != CSL_LogicalNot )
                {
                    SetHere ( zero->Coding ) ; //WordList_SetCoding ( 0, Here ) ;
                    bi->CmpCode = Here ;
                    Compile_CMP ( optInfo->CO_Dest_RegOrMem, optInfo->CO_Mod,
                        optInfo->CO_Reg, optInfo->CO_Rm, 0, optInfo->CO_Disp, CELL_SIZE ) ;
                }
            }
        }
        else
        {
            _Compile_Move_StackN_To_Reg ( OREG, DSP, 0, 0 ) ;
            _Compile_Move_StackN_To_Reg ( ACC, DSP, - 1, 0 ) ;
            // must do the DropN before the CMP because CMP sets eflags 
#if OLD_Setcc            
            _Compile_Stack_DropN ( DSP, 2 ) ; // before cmp 
#else            
            _Compile_Stack_DropN ( DSP, 1 ) ; // before cmp 
            Compile_MoveImm_To_TOS ( R14, 0, 8 ) ;
#endif            
            WordList_SetCoding ( 0, Here ) ;
            bi->CmpCode = Here ;
            Compile_CMP ( REG, REG, ACC, OREG, 0, 0, CELL ) ;
        }
        bi->AfterCmpCode = Here ;
    }
    Finish_Compile_Cmp_Set_Tttn_Logic ( compiler, zero, ttt, n ) ;
}

int64
Compiler_Var_Compile_LogicTest ( Compiler * compiler )
{
    if ( ( ! GetState ( compiler, LC_ARG_PARSING ) ) && C_SyntaxOn
        //&& ( ( compiler->PrefixWord && ( compiler->PrefixWord->W_MorphismAttributes & LOGIC_OPT ) ) || ( ! GetState ( compiler, ( DOING_A_PREFIX_WORD ) ) ) ) )
        && ( ( ! GetState ( compiler, ( DOING_A_PREFIX_WORD ) ) ) ) )
    {
        if ( ( compiler->CombinatorLevel || ( compiler->BlockLevel > 1 ) ) )
        {
            Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
            int64 rtrn = r ? r->rtrn : 0 ;
            if ( r && ( ! ( rtrn & LT_ID ) ) && ( rtrn & ( LT_OR_PREVIOUS | LT_OR_NEXT | LT_AND_PREVIOUS | LT_AND_NEXT | LT_END_OF_BLOCK ) ) )
            {
                BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
                BI_ResetLogicCode ( bi ) ;
                BI_CompileRecord_CmpCode_Reg ( bi, ACC ) ;
                return BI_Check_SetTttnJccGotoInfo ( bi, r, 1 ) ;
            }
        }
    }
    return 0 ;
}

int64
Check_Logic ( BlockInfo *bi, Boolean ttt, Boolean negFlag )
{
    if ( _Compiler_->CombinatorLevel || ( _Compiler_->BlockLevel > 1 ) || ( ! bi->TttnCode ) )
    {
        Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
        if ( ( ! C_SyntaxOn ) && ( ( r->rtrn & LT_END_OF_BLOCK ) && ( ! ( r->rtrn & LT_ID ) ) ) )
        {
            //int64 ttt, negFlag, jccType ;
            ///ttt = TTT_ZERO, negFlag = N_0, jccType = GI_JCC_TO_FALSE ;
            //_BI_SetTttnJccGotoInfo ( bi, ttt, negFlag, jccType ) ;
            _BI_SetTttnJccGotoInfo ( bi, N_0, GI_JCC_TO_FALSE ) ;
            bi->State |= BI_FINISHED ;

            return 1 ;
        }
    }
    return 0 ;
}

// these are the jcc to true ; with OR we can use jcc to true, jcc not to false ; with AND we can use jcc not to false 

void
Compile_LogicalNot ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_ZERO, N_0 ) ; //jnz to true, jz to false
}

//  logical equals - "=="

void
Compile_Equals ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_EQUAL, N_0 ) ; // ??
}

void
Compile_DoesNotEqual ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_EQUAL, N_1 ) ; // ??
}

void
Compile_LessThan ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_LESS, N_0 ) ;
}

void
Compile_GreaterThan ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_LE, N_1 ) ;
}

void
Compile_LessThanOrEqual ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_LE, N_0 ) ;
}

void
Compile_GreaterThanOrEqual ( Compiler * compiler )
{
    Compile_Cmp_Set_Tttn_Logic ( compiler, TTT_LESS, N_1 ) ;
}

#if 0

void
Compile_LogicalAnd ( Compiler * compiler )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
    bi->IiFlags |= LOGIC_FLAG ;
    Compile_Logical_X_Group1 ( _Compiler_, AND, TTT_ZERO, N_0 ) ;
    Rllafl * r = bi->BI_Rllafl ;
    if ( r && ( r->rtrn & LT_2_RPAREN_NEXT ) && ( ! ( r->rtrn & LT_END_OF_BLOCK ) ) )
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_FALSE, Here, 1 ) ;
}
#endif
#if 1

void
Compile_LogicalAnd ( Compiler * compiler )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
    Compile_Move_Reg_To_Reg ( RAX, R14, 0 ) ;
    _Compile_Stack_Drop ( R14 ) ;
    Compile_CMPI ( 0, RAX, 0, 0, 0 ) ;
    _Compile_Jcc ( JCC8, TTT_ZERO, N_0, 16 ) ;
    Compile_CMPI ( 0, RAX, - 8, 0, 0 ) ;
    _Compile_Jcc ( JCC8, TTT_ZERO, N_0, 9 ) ;
    Compile_MoveImm_To_TOS ( R14, 1, 8 ) ;
    _Compile_JumpToDisp ( 7, 0 ) ;
    Compile_MoveImm_To_TOS ( R14, 0, 8 ) ;
    if ( r && ( ! ( r->rtrn & LT_OR_NEXT ) ) || ( ( r->rtrn & LT_END_OF_BLOCK ) ) )
        _BI_SetTttnJccGotoInfo ( bi, N_0, GI_JCC_TO_FALSE ) ;
}
#elif 0

void
Compile_LogicalAnd ( Compiler * compiler )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    if ( bi ) bi->State |= ( ( uint64 ) 1 << AND ) ;
    bi->IiFlags |= LOGIC_FLAG ;
    Compile_Move_Reg_To_Reg ( RDX, R14, 0 ) ;
    Compile_CMPI ( MEM, RDX, 0, 0, 0 ) ;
    //_Compile_Jcc ( JCC8, TTT_ZERO, N_0, 16 ) ;
    //SetccToReg ( TTT_ZERO, N_0, RAX )
    _Compile_SETccRm ( TTT_ZERO, N_0, RDX ) ;
    _Compile_Stack_Drop ( RDX ) ;
    Compile_CMPI ( MEM, RDX, 0, 0, 0 ) ;
    _Compile_SETccRm ( TTT_ZERO, N_0, RDX ) ;
    //jeq b
    //_Compile_Jcc ( JCC8, TTT_ZERO, N_0, 9 ) ;
    SetccToReg ( TTT_ZERO, N_0, RCX ) ;
    Compile_MoveImm_To_TOS ( R14, 1, 8 ) ;
    //ret //jmp c
    _Compile_JumpToDisp ( 7, 0 ) ;
    //b: mov 0 [r14]
    //Compile_MoveImm_To_Mem ( R14, 0, 1 ) ;
    Compile_MoveImm_To_TOS ( R14, 0, 8 ) ;
    //c: ret
    //_Compile_Return ( ) ;
    //DBI_OFF ;
    //if ( ! Check_Logic ( bi, TTT_ZERO, N_0 ) ) BI_SetccAndStackPushAcc ( bi, TTT_ZERO, N_1 ) ; // looks ??
    Rllafl * r = bi->BI_Rllafl ;
    if ( r && ( r->rtrn & LT_2_RPAREN_NEXT ) && ( ! ( r->rtrn & LT_END_OF_BLOCK ) ) )
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_FALSE, Here, 1 ) ;
    bi->JccCode = 0 ;
}
#elif 1

void
Compile_LogicalAnd ( Compiler * compiler )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    //if ( bi ) 
    //bi->State |= ( ( uint64 ) 1 << AND ) ;
    bi->IiFlags |= LOGIC_FLAG ;
    LogicalAnd ( ) ;
    Compile_CMPI ( MEM, R14, 0, 0, 0 ) ;
    //DBI_OFF ;
    _BI_SetTttnJccGotoInfo ( bi, N_0, GI_JCC_TO_FALSE ) ;
}
#endif

void
Compile_LogicalOr ( Compiler * compiler )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
    bi->IiFlags |= LOGIC_FLAG ;
    //DBI_ON ;
    Compile_Logical_X_Group1 ( _Compiler_, OR, TTT_ZERO, N_0 ) ;
    //DBI_OFF ;
    bi->JccCode = 0 ;
}

void
_BI_SetTttnJccGotoInfo ( BlockInfo *bi, Boolean n, int64 jccType )
{
    Boolean ttt = bi->Ttt ? bi->Ttt : TTT_ZERO ;
    BI_SetTttN ( bi, ttt, n, 0, jccType ) ;
    Compile_JccGotoInfo ( bi, jccType ) ;
}

byte *
_BI_Compile_Jcc_FlipBiN ( BlockInfo *bi, byte * jmpToAddress )
{
    byte insn = GetState ( _CSL_, JCC8_ON ) ? JCC8 : 0 ;
    int64 n = ! bi->N ;
    byte * compiledAtAddress = Compile_Jcc ( bi->Ttt, n, jmpToAddress, insn ) ; // we do need to store and get this logic set by various conditions by the compiler : _Compile_SET_Tttn_REG
    return compiledAtAddress ;

}

// bi->JccCode may be set by a previous logic op

byte *
BI_Compile_Jcc_FlipN ( BlockInfo *bi, byte * jmpToAddress, Boolean logicFlag )
{
    if ( bi->JccCode ) SetHere ( bi->JccCode ) ;
    else if ( bi->TttnCode ) SetHere ( bi->TttnCode ) ;
    bi->JccCode = Here ;
    bi->ActualCopiedToJccCode = Here ;
    byte * compiledAtAddress = _BI_Compile_Jcc_FlipBiN ( bi, jmpToAddress ) ;
    if ( logicFlag ) Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
    return compiledAtAddress ;
}

byte *
BI_Compile_JccFlipN ( BlockInfo *bi, byte * jmpToAddress ) // , int8 nz
{
    byte * compiledAtAddress ;
    if ( bi->JccCode ) SetHere ( bi->JccCode ) ;
    if ( bi->TttnCode ) compiledAtAddress = BI_Compile_Jcc_FlipN ( bi, jmpToAddress, 0 ) ;
    else
    {
        Compile_BlockLogicTest ( bi ) ;
        compiledAtAddress = bi->JccCode ;
    }
    return compiledAtAddress ;
}

// non-combinator 'if'

byte *
Compiler_Compile_JccFlipN ( Compiler * compiler, int64 bindex ) // , int8 nz
{
    BlockInfo *bi = ( BlockInfo * ) _Stack_Pick ( compiler->CombinatorBlockInfoStack, bindex ) ; // -1 : remember - stack is zero based ; stack[0] is top
    byte * compiledAtAddress = BI_Compile_JccFlipN ( bi, 0 ) ;
    return compiledAtAddress ;
}

void
CSL_If_ConditionalExpression ( )
{
    if ( CompileMode )
    {
        byte * compiledAtAddress = Compiler_Compile_JccFlipN ( _Compiler_, 0 ) ;
        // N, ZERO : use inline|optimize logic which needs to get flags immediately from a 'cmp', jmp if the zero flag is not set
        // for non-inline|optimize ( reverse polarity : cf. _Compile_Jcc comment ) : jmp if cc is not true; cc is set by setcc after 
        // the cmp, or is a value on the stack. 
        // We cmp that value with zero and jmp if this second cmp sets the flag to zero else do the immediately following block code
        // ?? an explanation of the relation of the setcc terms with the flags is not clear to me yet (20110801) from the intel literature ?? 
        // but by trial and error this works; the logic i use is given in _Compile_Jcc.
        // ?? if there are problems check this area ?? cf. http://webster.cs.ucr.edu/AoA/Windows/HTML/IntegerArithmetic.html
        Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
    }
    else
    {
        if ( String_IsPreviousCharA_ ( _Context_->ReadLiner0->InputLine, _Context_->Lexer0->TokenStart_ReadLineIndex - 1, '}' ) )
            CSL_If2Combinator ( ) ;
        else if ( String_IsPreviousCharA_ ( _Context_->ReadLiner0->InputLine, _Context_->Lexer0->TokenStart_ReadLineIndex - 1, '#' ) )
            CSL_If_ConditionalInterpret ( ) ;
        else if ( GetState ( _Context_, C_SYNTAX | PREFIX_MODE | INFIX_MODE ) &&
            ( ! ( String_Equal ( _Lexer_->OriginalToken, "?" ) ) ) ) CSL_If_PrefixCombinators ( ) ;
        else
        {
            int64 value ;
            Interpreter * interp = _Context_->Interpreter0 ;
            if ( value = DataStack_Pop ( ) )
            {
                byte * token ;
                // interpret until ":", "else" or "endif"
                token = Interpret_C_Until_NotIncluding_Token5 ( interp, ( byte* ) "else",
                    ( byte* ) "endif", ( byte* ) ":", ( byte* ) ")", ( byte* ) "#", 0, 1, 0 ) ;
                if ( ( token == 0 ) || ( String_Equal ( token, "endif" ) ) ) return ;
                Parse_SkipUntil_EitherToken_OrNewline ( ( byte* ) "endif", 0 ) ;
                //Parse_SkipUntil_EitherToken_OrNewline ( ( byte* ) "endif", "\n" ) ;
            }
            else
            {
                // skip until ":" or "else"
                Parse_SkipUntil_EitherToken_OrNewline ( ( byte* ) ":", ( byte* ) "else" ) ;
                Interpret_C_Until_NotIncluding_Token5 ( interp, ( byte* ) ";", ( byte* ) ",",
                    ( byte* ) "endif", ( byte* ) "#", ( byte* ) ")", 0, 1, 0 ) ;
            }
        }
    }
}

// same as CSL_JMP

void
CSL_Else ( )
{
    if ( CompileMode )
    {
        byte * compiledAtAddress = Compile_UninitializedJump ( ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_FALSE, Here, 1 ) ;
    }
    else
    {
        if ( String_IsPreviousCharA_ ( _Context_->ReadLiner0->InputLine, _Context_->Lexer0->TokenStart_ReadLineIndex - 1, '#' ) )
            CSL_Else_ConditionalInterpret ( ) ;
        else Interpret_Until_Token ( _Context_->Interpreter0, ( byte* ) "endif", 0 ) ;
    }
}

void
CSL_EndIf ( )
{
    if ( CompileMode )
    {
        //BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
        //if ( _Compiler_->CombinatorLevel <= 1 ) 
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        //else 
        //CSL_InstallGotoCallPoints_Keyed ( bi, GI_JCC_TO_TRUE, Here, 0 ) ;
    }
    else Compiler_Init ( _Compiler_, 0 ) ;
}

void
CSL_LessThan ( ) // <
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_LessThan ( compiler ) ;
    else
    {
        int64 top, b ;
        top = DataStack_Pop ( ) ;
        b = _DataStack_GetTop ( ) ;
        _DataStack_SetTop ( ( int64 ) ( b < top ) ) ;
    }
}

void
CSL_LessThanOrEqual ( ) // <
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_LessThanOrEqual ( compiler ) ;
    else
    {
        int64 top, b ;
        top = DataStack_Pop ( ) ;
        b = _DataStack_GetTop ( ) ;
        _DataStack_SetTop ( ( int64 ) ( b <= top ) ) ;
    }
}

// ( b top | b > top ) dpans

void
CSL_GreaterThan ( ) // >
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_GreaterThan ( compiler ) ;
    else
    {
        int64 top, b ;
        top = DataStack_Pop ( ) ;
        b = _DataStack_GetTop ( ) ;
        _DataStack_SetTop ( ( int64 ) ( b > top ) ) ;
    }
}

void
CSL_GreaterThanOrEqual ( ) // >
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_GreaterThanOrEqual ( compiler ) ;
    else
    {
        int64 top, b ;
        top = DataStack_Pop ( ) ;
        b = _DataStack_GetTop ( ) ;
        _DataStack_SetTop ( ( int64 ) ( b >= top ) ) ;
    }
}

void
CSL_Logic_Equals ( ) // == 
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_Equals ( compiler ) ;
    else
    {
        int64 top, b ;
        top = DataStack_Pop ( ) ;
        b = _DataStack_GetTop ( ) ;
        _DataStack_SetTop ( ( int64 ) ( b == top ) ) ;
    }
}

void
CSL_Logic_DoesNotEqual ( ) // !=
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_DoesNotEqual ( compiler ) ;
    else
    {
        int64 top, b ;
        top = DataStack_Pop ( ) ;
        b = _DataStack_GetTop ( ) ;
        _DataStack_SetTop ( ( int64 ) ( b != top ) ) ;
    }
}

void
CSL_LogicalNot ( ) // not
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_LogicalNot ( compiler ) ;
    else TOS = ( ! TOS ) ;
}

// with 'and' we jump to the false block 
// and fall thru to the true block

void
CSL_LogicalAnd ( ) // and
{
    if ( CompileMode ) Compile_LogicalAnd ( _Compiler_ ) ;
    else
    {
        LogicalAnd ( ) ;
    }
}

// with 'or' we jump to the the true block 
// and add an extra test at the end of a 
// control block to jump to the false block

void
CSL_LogicalOr ( ) // or
{
    if ( CompileMode ) Compile_LogicalOr ( _Compiler_ ) ;
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] || TOS ;
        DataStack_Drop ( ) ;
    }
}

void
CSL_LogicalXor ( ) // xor
{
    Compiler * compiler = _Compiler_ ;
    if ( CompileMode ) Compile_Logical_X_Group1 ( compiler, XOR, TTT_ZERO, N_0 ) ;
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] ^ TOS ;
        DataStack_Drop ( ) ;
    }
}

void
Interpreter_Logic_CheckPrefix ( Word * word )
{
    if ( C_SyntaxOn )
    {
        //Compiler * compiler = _Compiler_ ;
        if ( word->Definition == CSL_LogicalAnd )
        {
            Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
            if ( ( r->rtrn & LT_OR_PREVIOUS ) && ( ! ( r->rtrn & LT_2_LPAREN_PREVIOUS ) ) )
            {
                //Print_Defined_LogicVariable ( r ) ;
                CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_TRUE, Here, 1 ) ;
            }
        }
        else if ( word->Definition == CSL_LogicalOr )
        {
            Rllafl * r = ReadLiner_LookAroundFor_Logic ( 0 ) ;
            if ( ( r->rtrn & LT_AND_PREVIOUS ) && ( ! ( r->rtrn & LT_2_LPAREN_PREVIOUS ) ) )
            {
                CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_FALSE, Here, 1 ) ;
            }
        }
    }
}

