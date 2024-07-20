
#include "../../include/csl.h"

BlockInfo *
BI_Block_Copy ( BlockInfo * bi, byte* dstAddress, byte * srcAddress, int64 bsize, int64 control )
{
    Compiler * compiler = _Compiler_ ;
    if ( ! bi ) bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    byte * saveHere = Here, * saveSrcAddress = srcAddress, *svHere = 0 ;
    ud_t * ud = Debugger_UdisNew ( _Debugger_ ) ;
    int64 isize, left, offset, insnSize ;
    if ( dstAddress ) SetHere ( dstAddress ) ;
    bi->CopiedToStart = Here ;
    for ( left = bsize ; ( left > 0 ) ; srcAddress += isize )
    {
        CO_PeepHole_Optimize ( ) ;
        isize = _Udis_GetInstructionSize ( ud, srcAddress ) ;
        left -= isize ;
        CSL_AdjustDbgSourceCodeAddress ( srcAddress, Here ) ;
        CSL_AdjustLabels ( srcAddress ) ;
        byte insn = ( * srcAddress ) ;
        if ( insn == _RET )
        {
            if ( ( left > 0 ) && ( ( insn + 1 ) != NOOP ) ) //  noop from our logic overwrite
            {
                // ?? unable at present to compile inline with more than one return in the block
                SetHere ( saveHere ) ;
                Compile_Call ( saveSrcAddress ) ;
            }
            else break ; // don't include RET
        }
        else if ( insn == CALL32 )
        {
            int32 offset = * ( int32* ) ( srcAddress + 1 ) ; // 1 : 1 byte CALLI32 opCode
            if ( ! offset )
            {
                dllist_Map1 ( compiler->GotoList, ( MapFunction1 ) AdjustGotoInfo, ( int64 ) ( srcAddress ) ) ;
                CSL_SetupRecursiveCall ( ) ;
                continue ;
            }
            else
            {
                byte * jcAddress = JumpCallInstructionAddress ( srcAddress ) ;
                Word * word = Word_GetFromCodeAddress ( jcAddress ) ;
                if ( word )
                {
                    _Word_Compile ( word ) ;
                    continue ;
                }
            }
        }
        else if ( IsSetCcInsn ( srcAddress ) )
        {
            //_Debugger_Disassemble ( _Debugger_, 0, srcAddress, 0, 1 ) ;
            GotoInfo_NewAddAsSetcc ( bi, srcAddress, Here ) ;
        }
        else if ( ( insn == JMP32 ) || ( insn == JMP8 ) )
        {
            insnSize = 1 ;
            int32 offset ;
            if ( insn == JMP32 ) offset = * ( int32* ) ( srcAddress + insnSize ) ; // 1 : 1 byte JMPI32 opCode - e9
            else offset = * ( int8 * ) ( srcAddress + insnSize ) ;
            if ( offset ) dllist_Map1 ( compiler->GotoList, ( MapFunction1 ) AdjustGotoInfo, ( int64 ) ( srcAddress ) ) ;
            else dllist_Map1 ( compiler->GotoList, ( MapFunction1 ) AdjustGotoInfo, ( int64 ) srcAddress ) ; //, ( int64 ) end ) ;
        }
        else if ( ( insn == JCC32 ) || IS_INSN_JCC8 ( insn ) )
        {
            dllist_Map1 ( compiler->GotoList, ( MapFunction1 ) AdjustGotoInfo, ( int64 ) srcAddress ) ; //, ( int64 ) end ) ;
            bi->JccCode = Here ;
#if 1 // this has sometimes *maybe* caused problems ??
            if ( ( insn == JCC32 ) || (insn == JCC8) )
            {
                //byte noop4 [] = { 0x66, 0x90, 0x66, 0x90 } ; 
                //byte noop4 [] = { 0x90, 0x90, 0x90, 0x90 } ; 
                //if (insn == JCC8)
                int32 offset = GetDispForCallOrJumpFromInsnAddr ( srcAddress ) ;
                if ( ( ! ( bi->IiFlags & LOGIC_FLAG ) ) && offset && ( CheckOffset ( offset, 1 ) ) )
                {
#if 0    
                    if ( Is_DebugOn ) Debugger_Disassemble ( _Debugger_, saveSrcAddress, bsize, 1 ) ;
#endif    
                    svHere = Here ;
#if 0    
                    if ( Is_DebugOn ) iPrintf ( "\nsrcAddress = %lx : Here = %lx", srcAddress, Here ) ;
#endif                    
                    byte opCode = * ( srcAddress + 1 ), tttn = opCode & 0xf ; // jcc32
                    byte newOpCode = 0x7 << 4 | tttn ;

                    _Compile_Int8 ( newOpCode ) ; // JCC8
                    _Compile_Int8 ( offset + 4 ) ; // 4 : diff in insn sizes
                    byte noop4 [] = { 0x0f, 0x1f, 0x40, 0x00 } ; //nop [rax]
                    _CompileN ( noop4, 4 ) ;
                    //DBI_OFF ;
#if 0   
                    if ( Is_DebugOn ) Debugger_Disassemble ( _Debugger_, svHere, 6, 0 ) ;
#endif    
                    continue ;
                }
            }
#endif            
        }
        if ( insn == NOOP ) continue ;
        _CompileN ( srcAddress, isize ) ;
    }
    _Block_RecordCopy ( bi ) ;
#if 1    
    //if ( Is_DebugOn || svHere ) //( control == 1 )  
    //    Debugger_Disassemble ( _Debugger_, bi->CopiedToStart, bi->CopiedSize, 1 ) ;
    if ( Is_DebugOn && svHere ) //( control == 1 )  
        Debugger_Disassemble ( _Debugger_, svHere, bi->CopiedSize, 1 ),
        Debugger_Disassemble ( _Debugger_, bi->CopiedToStart, bi->CopiedSize, 1 ) ;
    ;
#endif    
    return bi ;
}

BlockInfo *
BI_CopyCompile ( BlockInfo *bi, byte * srcAddress, Boolean cntrlBlkFlg )
{
    BI_Block_Copy ( bi, Here, srcAddress ? srcAddress : bi->bp_First, bi->bp_Last - bi->bp_First, 1 ) ;
    if ( cntrlBlkFlg ) Compile_BlockLogicTest ( bi ) ;
    return bi ;
}

#if 0

BlockInfo *
Block_CopyCompile ( byte * srcAddress, int64 bindex, Boolean cntrlBlkFlg )
{
    BlockInfo *bi = BlockInfo_GetCbsStackPick ( bindex ) ;
    BlockInfo *biNew = BlockInfo_Copy ( bi ) ;
    BI_CopyCompile ( biNew, srcAddress, cntrlBlkFlg ) ;
    _Block_RecordCopy ( biNew ) ;
    return biNew ;
}
#else

BlockInfo *
Block_CopyCompile ( byte * srcAddress, int64 bindex, Boolean cntrlBlkFlg )
{
    BlockInfo *bi = BlockInfo_GetCbsStackPick ( bindex ) ;
    //BlockInfo *biNew = BlockInfo_Copy ( bi ) ;
    BI_CopyCompile ( bi, srcAddress, cntrlBlkFlg ) ;
    _Block_RecordCopy ( bi ) ;
    return bi ;
}
#endif

inline void
_Block_RecordCopy ( BlockInfo *bi )
{
    bi->CopiedToEnd = Here ;
    bi->CopiedSize = bi->CopiedToEnd - bi->CopiedToStart ;
}

#if 0

inline void
Block_RecordCopy ( BlockInfo *bi )
{
    _Block_RecordCopy ( bi ) ;
    //bi->bp_First = bi->CopiedToStart ;
    //bi->bp_Last = bi->CopiedToEnd ;
}
#endif

BlockInfo *
BlockInfo_GetCbsStackPick ( int64 bindex )
{
    Compiler * compiler = _Context_->Compiler0 ;
    BlockInfo *bi = ( BlockInfo * ) _Stack_Pick ( compiler->CombinatorBlockInfoStack, bindex ) ;
    return bi ;
}


