#include "../../include/csl.h"
// a combinator can be thought of as a finite state machine that
// operates on a stack or more theoretically as a finite state control
// for a pda/turing machine but more simply as a function on a stack to
// a stack like a forth word but the items on the stack can be taken,
// depending on the combinator, as subroutine calls. The idea comes from, for me, 
// Foundations of Mathematical Logic by Haskell Curry and the joy
// and factor programming languages. It works out
// to be an intuitive idea ; you may not need to understand it, but you can
// see how it works. It simplifies syntax beyond Forth because
// it reduces the number of necessary prefix operators to one - tick ( ' = "'") = quote.

// nb : can't fully optimize if there is code between blocks
// check if there is any code between blocks if so we can't optimize fully

void
CSL_EndCombinator ( int64 quotesUsed, int64 moveFlag )
{
    Compiler * compiler = _Context_->Compiler0 ;
    BlockInfo *bi = BlockInfo_GetCbisStackPick ( quotesUsed - 1 ) ;
    Combinator * combinator ;
    if ( C_SyntaxOn ) combinator = _Context_->SC_CurrentCombinator ;
    else combinator = _Context_->CurrentCombinator ;
    byte * here = Here ;
    compiler->BreakPoint = here ;
    if ( combinator->W_MorphismAttributes & BREAK_COMBINATOR )
        CSL_InstallGotoCallPoints_Keyed ( ( BlockInfo* ) bi, GI_CONTINUE | GI_BREAK, here, 0 ) ;
    CSL_InstallGotoCallPoints_Keyed ( bi, GI_JCC_TO_FALSE, here, 1 ) ;
    compiler->CombinatorLevel -- ;
    compiler->CombinatorEndsAt = here ;
    // copy back into OriginalActualCodeStart, original code space, from where we started, CombinatorStartsAt, in CSL_BeginCombinator
    if ( moveFlag && Compiling ) BI_Block_Copy (bi, bi->OriginalActualCodeStart,
        compiler->CombinatorStartsAt, compiler->CombinatorEndsAt - compiler->CombinatorStartsAt) ; // 0 : can't generally peephole optimize (arrayTest.csl problems) ??
    _Stack_DropN ( compiler->CombinatorBlockInfoStack, quotesUsed ) ;
    if ( GetState ( compiler, LC_COMBINATOR_MODE ) ) _Stack_Pop ( _LC_->CombinatorInfoStack ) ; //??
}

// Combinators are first created after the original code using 'Here' in CSL_BeginCombinator 
// then copied into the original code locations at EndCombinator

BlockInfo *
CSL_BeginCombinator ( int64 quotesUsed )
{
    Compiler * compiler = _Context_->Compiler0 ;
    //BlockInfo *bi = ( BlockInfo * ) _Stack_Pick ( compiler->CombinatorBlockInfoStack, quotesUsed - 1 ) ; // -1 : remember - stack is zero based ; stack[0] is top
    BlockInfo *bi = BlockInfo_GetCbisStackPick ( quotesUsed - 1 ) ;
    compiler->CombinatorStartsAt = Here ;
    CalculateOffsetForCallOrJump (bi->PtrToJmpInsn, compiler->CombinatorStartsAt, T_JMP) ;
    compiler->CombinatorLevel ++ ;
    return bi ;
}
// ( q -- )

void
CSL_DropBlock ( )
{
    //block dropBlock = ( block ) TOS ;
    DataStack_DropN ( 1 ) ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 1 ) ;
        //Block_CopyCompile ( ( byte* ) dropBlock, 0, 0 ) ; // in case there are control labels
        CSL_EndCombinator ( 1, 0 ) ;
    }
}

void
CSL_BlockRun ( )
{
    block doBlock = ( block ) TOS ;
    DataStack_DropN ( 1 ) ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 1 ) ;
        Block_CopyCompile ( ( byte* ) doBlock, 0, 0 ) ;
        CSL_EndCombinator ( 1, 1 ) ; // 0 : don't copy
    }
    else
    {
        Word_DbgBlock_Eval ( 0, doBlock ) ;
        //Set_DataStackPointer_FromDspReg ( ) ;
    }
}

// ( q -- )

void
CSL_LoopCombinator ( )
{
    Compiler * compiler = _Context_->Compiler0 ;
    block loopBlock = ( block ) TOS ;
    DataStack_DropN ( 1 ) ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 1 ) ;
        byte * start = Here ;
        compiler->ContinuePoint = start ;
        Block_CopyCompile ( ( byte* ) loopBlock, 0, 0 ) ;
        Compile_JumpToAddress ( start, 0 ) ; //JMPI32 ) ;
        CSL_EndCombinator ( 1, 1 ) ;
    }
    else
    {
        while ( 1 ) Word_DbgBlock_Eval ( 0, loopBlock ) ;
    }
}

int64
CSL_WhileCombinator ( )
{
    Compiler * compiler = _Context_->Compiler0 ;
    block controlBlock = ( block ) _DspReg_ [ - 1 ], doBlock = ( block ) TOS ;
    //Boolean rcmpl = 0 ; //, svJccState = GetState ( _CSL_, JCC8_ON ) ;
    DataStack_DropN ( 2 ) ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 2 ) ;
        byte * start = Here ;//, *svHere = 0 ;
        compiler->ContinuePoint = Here ;
        d0 ( if ( Is_DebugModeOn ) _CSL_SC_WordList_Show ( ( byte* ) "\nCheckOptimize : after optimize :", 0, 0 ) ) ;
        //Word * combinator = _Context_->CurrentCombinator ;
        //BlockInfo *bico = BlockInfo_GetCbisStackPick ( 1 ) ;
        //svHere = Here ;
        //BlockInfo *bico2 = BlockInfo_Copy ( bico ) ; 
        //BlockInfo *bic = BI_CopyCompile ( bico2, ( byte* ) controlBlock, 1 ) ;
        BlockInfo *bic = Block_CopyCompile ( ( byte* ) controlBlock, 1, 1 ) ;
        compiler->CombinatorStartsAt = bic->CopiedToStart ;
        CSL_InstallGotoCallPoints_Keyed ( bic, GI_JCC_TO_TRUE, Here, 1 ) ;
        BlockInfo *bid = Block_CopyCompile ( ( byte* ) doBlock, 0, 0 ) ;
        Compile_JumpToAddress ( start, 0 ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ; // for controlBlock
        CSL_EndCombinator ( 2, 1 ) ;
    }
    else
    {
        while ( 1 )
        {
            Word_DbgBlock_Eval ( 0, controlBlock ) ;
            if ( ! DataStack_Pop ( ) ) break ;
            Word_DbgBlock_Eval ( 0, doBlock ) ;
        }
    }
    return 1 ;
}
// ( q q q q -- )

void
CSL_ForCombinator ( )
{
    Compiler * compiler = _Context_->Compiler0 ;
    block doBlock = ( block ) TOS, doPostBlock = ( block ) _DspReg_ [ - 1 ],
        controlBlock = ( block ) _DspReg_ [ - 2 ], doPreBlock = ( block ) _DspReg_ [ - 3 ] ;
    DataStack_DropN ( 4 ) ;
    if ( CompileMode )
    {
        byte * svHere = 0 ;
        CSL_BeginCombinator ( 4 ) ;
        Block_CopyCompile ( ( byte* ) doPreBlock, 3, 0 ) ;

        byte * start = Here ;

        //BlockInfo *bico = BlockInfo_GetCbsStackPick ( 1 ) ;
#if 0    
        if ( Is_DebugOn ) Debugger_Disassemble ( _Debugger_, start, Here - start, 0 ) ;
#endif    
        BlockInfo * bic = Block_CopyCompile ( ( byte* ) controlBlock, 2, 1 ) ; // 1 : jccFlag for this block
#if 0    
        if ( Is_DebugOn ) Debugger_Disassemble ( _Debugger_, start, Here - start, 0 ) ;
#endif    

        compiler->ContinuePoint = Here ;

        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_TRUE, Here, 1 ) ;
        BlockInfo * bid = Block_CopyCompile ( ( byte* ) doBlock, 0, 0 ) ;

        d0 ( _CSL_SC_WordList_Show ( ( byte* ) "for combinator : before doPostBlock", 0, 0 ) ) ;
        BlockInfo * bidpb = Block_CopyCompile ( ( byte* ) doPostBlock, 1, 0 ) ;
        Compile_JumpToAddress ( start, 0 ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        CSL_EndCombinator ( 4, 1 ) ;
    }
    else
    {
        Word_DbgBlock_Eval ( 0, doPreBlock ) ;
        do
        {
            Word_DbgBlock_Eval ( 0, controlBlock ) ;
            if ( ! DataStack_Pop ( ) ) break ;
            Word_DbgBlock_Eval ( 0, doBlock ) ;
            Word_DbgBlock_Eval ( 0, doPostBlock ) ;
        }
        while ( 1 ) ;
    }
}

void
CSL_DoWhileDoCombinator ( )
{
    Compiler * compiler = _Context_->Compiler0 ;
    block controlBlock = ( block ) _DspReg_ [ - 1 ], doBlock2 = ( block ) TOS, doBlock1 = ( block ) _DspReg_ [ - 2 ] ;
    byte * start, *svHere = 0 ;
    DataStack_DropN ( 3 ) ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 3 ) ;
        compiler->ContinuePoint = Here ;
        start = Here ;
        Block_CopyCompile ( ( byte* ) doBlock1, 2, 0 ) ;

        BlockInfo * bic = Block_CopyCompile ( ( byte* ) controlBlock, 1, 1 ) ; // 1 : jccFlag for this block

        BlockInfo * bid = Block_CopyCompile ( ( byte* ) doBlock2, 0, 0 ) ;

        Compile_JumpToAddress ( start, 0 ) ; // runtime
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        CSL_EndCombinator ( 3, 1 ) ;
    }
    else
    {
        do
        {
            Word_DbgBlock_Eval ( 0, doBlock1 ) ;
            Word_DbgBlock_Eval ( 0, controlBlock ) ;
            if ( ! DataStack_Pop ( ) ) break ;
            Word_DbgBlock_Eval ( 0, doBlock2 ) ;
        }
        while ( 1 ) ;
    }
}

// do block is before the control block => special handling : 3

int64
_CSL_DoWhileCombinator ( block controlBlock, block doBlock )
{
    Compiler * compiler = _Context_->Compiler0 ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 2 ) ;
        byte * start = Here ;
        compiler->ContinuePoint = Here ;
        BlockInfo * bid = Block_CopyCompile ( ( byte* ) doBlock, 1, 0 ) ;
        BlockInfo * bic = Block_CopyCompile ( ( byte* ) controlBlock, 0, 1 ) ; // 3 : use old version for jmp to back ref ??
        CalculateOffsetForCallOrJump (bic->JccCode ? bic->JccCode : bic->JccAddedCode, bid->CopiedToStart, T_JCC) ;
        GotoInfo_Remove ( ( dlnode* ) bic->BI_Gi ), bic->BI_Gi = 0 ;
        CSL_EndCombinator ( 2, 1 ) ;
    }
    else
    {
        do
        {
            Word_DbgBlock_Eval ( 0, doBlock ) ;
            Word_DbgBlock_Eval ( 0, controlBlock ) ;
            if ( ! DataStack_Pop ( ) ) break ;
        }
        while ( 1 ) ;
    }
    return 1 ;
}

int64
CSL_DoWhileCombinator ( )
{
    block controlBlock = ( block ) TOS, doBlock = ( block ) _DspReg_ [ - 1 ] ;
    DataStack_DropN ( 2 ) ;
    return _CSL_DoWhileCombinator ( controlBlock, doBlock ) ;
}
// ( b q -- ) 

void
CSL_If1Combinator ( )
{
    block doBlock = ( block ) DataStack_Pop ( ) ;
    if ( CompileMode )
    {
        //DBI_ON ;
        BlockInfo * bi = CSL_BeginCombinator ( 1 ) ;
        Compile_BlockLogicTest ( bi ) ;
        byte * compiledAtAddress = Compile_UninitializedJccEqualZero ( ) ;
        Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
        Block_CopyCompile ( ( byte* ) doBlock, 0, 0 ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        CSL_EndCombinator ( 1, 1 ) ;
        //DBI_OFF ;
    }
    else
    {
        if ( DataStack_Pop ( ) ) Word_DbgBlock_Eval ( 0, doBlock ) ;
    }
}

// ( q q -- )

void
CSL_If2Combinator ( )
{
    block controlBlock = ( block ) _DspReg_ [ - 1 ], doBlock = ( block ) TOS ;
    DataStack_DropN ( 2 ) ;
    if ( CompileMode )
    {
        CSL_BeginCombinator ( 2 ) ;
        Block_CopyCompile ( ( byte* ) controlBlock, 1, 1 ) ;
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_TRUE, Here, 1 ) ;
        Block_CopyCompile ( ( byte* ) doBlock, 0, 0 ) ;
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_FALSE, Here, 1 ) ;
        CSL_EndCombinator ( 2, 1 ) ;
    }
    else
    {
        int64 tvalue ;
        Word_DbgBlock_Eval ( 0, controlBlock ) ;
        if ( tvalue = DataStack_Pop ( ) ) Word_DbgBlock_Eval ( 0, doBlock ) ;
    }
}

// ( b q q -- )
// takes 2 blocks
// nb. does not drop the boolean so it can be used in a block which takes a boolean - an on the fly combinator

void
CSL_TrueFalseCombinator2 ( )
{
    int64 testCondition = _DspReg_ [ - 2 ] ;
    block trueBlock = ( block ) _DspReg_ [ - 1 ], elseBlock = ( block ) TOS ;
    DataStack_DropN ( 2 ) ;
    if ( CompileMode )
    {
        BlockInfo * bi = CSL_BeginCombinator ( 2 ) ;
        Compile_BlockLogicTest ( bi ) ;
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_TRUE, Here, 1 ) ;
        Block_CopyCompile ( ( byte* ) trueBlock, 1, 0 ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        Block_CopyCompile ( ( byte* ) elseBlock, 0, 0 ) ;
        CSL_EndIf ( ) ;
        CSL_EndCombinator ( 2, 1 ) ;
    }
    else
    {
        if ( testCondition ) Word_DbgBlock_Eval ( 0, trueBlock ) ;
        else Word_DbgBlock_Eval ( 0, elseBlock ) ;
    }
}

// ( q q q -- )
// takes 3 blocks

void
CSL_TrueFalseCombinator3 ( )
{
    block controlBlock = ( block ) Dsp ( 2 ), trueBlock = ( block ) Dsp ( 1 ), elseBlock = ( block ) Dsp ( 0 ) ;
    DataStack_DropN ( 3 ) ;
    if ( CompileMode )
    {
        //DBI_ON ;
        CSL_BeginCombinator ( 3 ) ;
        Block_CopyCompile ( ( byte* ) controlBlock, 2, 1 ) ;
        CSL_InstallGotoCallPoints_Keyed ( 0, GI_JCC_TO_TRUE, Here, 1 ) ;
        Block_CopyCompile ( ( byte* ) trueBlock, 1, 0 ) ;
        CSL_Else ( ) ;
        Block_CopyCompile ( ( byte* ) elseBlock, 0, 0 ) ;
        CSL_EndIf ( ) ;
        CSL_EndCombinator ( 3, 1 ) ;
        //DBI_OFF ;
    }
    else
    {
        Word_DbgBlock_Eval ( 0, controlBlock ) ;
        if ( DataStack_Pop ( ) ) Word_DbgBlock_Eval ( 0, trueBlock ) ;
        else Word_DbgBlock_Eval ( 0, elseBlock ) ;
    }
}

//  ( q q q -- )

inline void
CSL_IfElseCombinator ( )
{
    CSL_TrueFalseCombinator3 ( ) ;
}

inline void
CSL_If3Combinator ( )
{
    CSL_TrueFalseCombinator3 ( ) ;
}

// used by lisp cond 

void
CSL_CondCombinator ( int64 numBlocks )
{
    Compiler * compiler = _Context_->Compiler0 ;
    int64 blockIndex ;
    BlockInfo *bi ;
    byte * compiledAtAddress ;
    if ( numBlocks )
    {
        //DBI_ON ;
        CSL_BeginCombinator ( numBlocks ) ;
        for ( blockIndex = numBlocks - 1 ; blockIndex > 0 ; )
        {
            Block_CopyCompile ( ( byte* ) _DspReg_ [ - blockIndex ], blockIndex, 0 ) ;
            compiledAtAddress = Compile_UninitializedJccEqualZero ( ) ;
            Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
            bi = BlockInfo_GetCbisStackPick ( blockIndex ) ;
            Compile_BlockLogicTest ( bi ) ;
            blockIndex -- ;
            Block_CopyCompile ( ( byte* ) _DspReg_ [ - blockIndex ], blockIndex, 0 ) ;
            byte * compiledAtAddress = Compile_UninitializedJump ( ) ; // at the end of the 'if block' we need to jmp over the 'else block'
            CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
            Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
            blockIndex -- ;
        }
        if ( numBlocks % 2 )
        {
            Block_CopyCompile ( ( byte* ) _DspReg_ [ - blockIndex ], blockIndex, 0 ) ;
        }
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        CSL_EndCombinator ( numBlocks, 1 ) ;
        //DBI_OFF ;
    }
    DataStack_DropN ( numBlocks ) ;
    //CSL_PrintDataStack ( ) ;
}
