#include "../../include/csl.h"
#define TEST_0 0

byte
OffsetSize ( byte insn )
{
    byte size = ( ( insn == JCC32 ) || ( insn == JMP32 ) || ( insn == CALL32 ) ) ? 4 : 1 ;
    return size ;
}

// Jcc/Jmp insn

byte
InsnSize ( byte insn )
{
    byte size = ( insn == JCC32 ) ? 2 : 1 ;
    return size ;
}

Boolean
CheckOffset ( int64 offset, byte offsetSize ) // offsetSize in bytes
{
    //Boolean r = ( (offset >= (2^bits -1)) && (offset <= (2^bits)) ) ;
    Boolean r = 0 ;
    switch ( offsetSize )
    {
        case 1:
        {
            r = ( ( offset >= - 127 ) && ( offset <= 128 ) ) ;
            break ;
        }
        case 2:
        {
            r = ( ( offset >= - 65535 ) && ( offset <= 65536 ) ) ;
            break ;
        }
        case 4:
        {
            r = ( ( offset >= - 4294967295 ) && ( offset <= 4294967296 ) ) ;
            break ;
        }
    }
    return r ;
}
// compileAtAddress is the address of the disp to be compiled
// for compileAtAddress of the disp : where the space has *already* been allocated
// call 32BitOffset ; <- intel call instruction format
// endOfCallingInstructionAddress + disp = jmpToAddr
// endOfCallingInstructionAddress = compileAtAddress + 4 ; for ! 32 bit disp only !
// 		-> disp = jmpToAddr - compileAtAddress - 4

/* from Intel Instruction set reference : JMP insn
A relative disp (rel8, rel16, or rel32) is generally specified as a label in assembly code, but at the machine code
level, it is encoded as a signed 8-, 16-, or 32-bit immediate value. This value is added to the value in the EIP
register. (Here, the EIP register contains the address of the instruction following the JMP instruction). When using
relative offsets, the opcode (for short vs. near jumps) and the operand-size attribute (for near relative jumps)
determines the size of the target operand (8, 16, or 32 bits).
 */
/* from Intel Instruction set reference : JMP insn
A relative offset (rel8, rel16, or rel32) is generally specified as a label in assembly code, but at the machine code
level, it is encoded as a signed 8-, 16-, or 32-bit immediate value. This value is added to the value in the EIP
register. (Here, the EIP register contains the address of the instruction following the JMP instruction). When using
relative offsets, the opcode (for short vs. near jumps) and the operand-size attribute (for near relative jumps)
determines the size of the target operand (8, 16, or 32 bits).
 */
byte *
Calculate_Address_FromOffset_ForCallOrJump ( byte * address )
{
    byte * iaddress = 0 ;
    int64 offset ;
    if ( ( * address == JMP8 ) || IS_INSN_JCC8 ( * address ) )
    {
        offset = * ( ( int8 * ) ( address + 1 ) ) ;
        iaddress = address + offset + 1 + BYTE_SIZE ;
    }
    else if ( ( * address == JMP32 ) || ( * address == CALL32 ) )
    {
        offset = * ( ( int32 * ) ( address + 1 ) ) ;
        iaddress = address + offset + 1 + INT32_SIZE ;
    }
    else if ( ( ( * address == 0x0f ) && ( ( * ( address + 1 ) >> 4 ) == 0x8 ) ) )
    {
        offset = * ( ( int32 * ) ( address + 2 ) ) ;
        iaddress = address + offset + 2 + INT32_SIZE ;
    }
    else if ( ( ( * ( uint16* ) address ) == 0xff49 ) && ( ( *( address + 2 ) == 0xd2 ) || ( *( address + 2 ) == 0xd3 ) ) ) // call r10/r11
    {
        if ( ( ( * ( uint16* ) ( address - 20 ) ) == 0xbb49 ) ) iaddress = ( byte* ) ( * ( ( uint64* ) ( address - 18 ) ) ) ; //mov r11, 0xxx in Compile_Call_TestRSP  
        else iaddress = ( byte* ) ( * ( ( uint64* ) ( address - CELL ) ) ) ;
    }
    else if ( ( ( * ( uint16* ) address ) == 0xff48 ) && ( *( address + 2 ) == 0xd3 ) ) // call rax
    {
        iaddress = ( byte* ) ( * ( ( uint64* ) ( address - CELL ) ) ) ;
    }
    return iaddress ;
}

int32
BI_CalculateOffsetForCallOrJumpOrJcc ( BlockInfo * bi ) //( byte * insnAddress, byte * jmpToAddr, byte insn, byte insnType )
{
    if ( ! bi->Insn )
    {
        if ( bi->InsnAddress )
        {
            bi->Insn = ( * bi->InsnAddress ) ;
        }
        else Error ( "\nCalculateDispForCallOrJumpOrJcc : No insn or InsnAddress", QUIT ) ;
    }
    if ( ! bi->InsnType )
    {
        if ( ( bi->Insn == JCC32 ) || ( bi->Insn == JCC8 ) ) bi->InsnType = T_JCC ;
        else bi->InsnType = ( T_JMP | T_CALL ) ;
    }
    bi->InsnSize = ( bi->Insn == JCC32 ) ? 2 : 1 ;
    if ( ( bi->InsnType & ( T_JMP | T_CALL ) ) ) //&& ( insnSize == 1 ) ) //offsetSize = 1, insnSize = 1
    {
        if ( bi->Insn )
        {
            if ( ( bi->Insn == JMP32 ) || ( bi->Insn == CALL32 ) ) bi->OffsetSize = 4 ;
            else bi->OffsetSize = 1 ;
        }
        else // deduce insn
        {
            if ( bi->InsnType == T_CALL ) bi->Insn = CALL32 ;
            else
            {
                bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + 1 ) ;
                bi->OffsetSize = 1 ;
                if ( ( bi->Disp >= - 127 ) && ( bi->Disp <= 128 ) ) bi->OffsetSize = 1, bi->Insn = JMP8 ;
                else 
                {
                    bi->OffsetSize = 4, bi->Insn = JMP32 ;
                    bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + bi->OffsetSize ) ;
                }
            }
            goto done ;
        }
        bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + bi->OffsetSize ) ;
#if 1        
        if ( ( bi->Disp >= - 127 ) && ( bi->Disp <= 128 ) && ( bi->OffsetSize == 1 ) ) bi->IiFlags |= COULD_BE_8 ;
        //else if ( ( bi->Disp >= ( - ( 65535 ) ) ) && ( bi->Disp <= ( 65536 ) ) ) bi->IiFlags |= COULD_BE_16 ;
#else        
        if ( ( bi->OffsetSize == 1 ) && CheckOffset ( bi->Disp, 1 ) ) bi->IiFlags |= COULD_BE_8 ;
        else if ( CheckOffset ( bi->Disp, 2 ) ) bi->IiFlags |= COULD_BE_16 ;
#endif        
        else bi->IiFlags |= SHOULD_BE_32 ;
    }
    else //if ( bi->InsnType & (T_JCC) ) 
    {
        if ( bi->Insn == JCC32 ) bi->OffsetSize = 4 ;
        else bi->OffsetSize = 1 ;
        bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + bi->OffsetSize ) ;
#if 1        
        if ( ( bi->Disp >= - 127 ) && ( bi->Disp <= 128 ) && ( bi->OffsetSize == 1 ) ) bi->IiFlags |= COULD_BE_8 ;
        //else if ( ( bi->Disp >= ( - ( 65535 ) ) ) && ( bi->Disp <= ( 65536 ) ) ) bi->IiFlags |= COULD_BE_16 ;
#else        
        if ( ( bi->OffsetSize == 1 ) && CheckOffset ( bi->Disp, 1 ) ) bi->IiFlags |= COULD_BE_8 ;
        else if ( CheckOffset ( bi->Disp, 2 ) ) bi->IiFlags |= COULD_BE_16 ;
#endif        
        else bi->IiFlags |= SHOULD_BE_32 ;
    }
    done :
    return bi->Disp ;
}

int32
SetOffsetForCallOrJump ( BlockInfo * bi ) //byte * insnAddr, byte * jmpToAddr, byte insnType, byte insn )
{
    int32 disp = bi->Disp = BI_CalculateOffsetForCallOrJumpOrJcc ( bi ) ; //( insnAddr, jmpToAddr, insn, insnType ) ;
    byte * offsetAddress = bi->InsnAddress + bi->InsnSize ; //insnAddr + insnSize ;
    if ( IS_INSN_JCC8 ( bi->Insn ) || ( bi->Insn == JMP8 ) ) //|| ( insn == JCC8 ) )
    {
        if ( ( disp > - 127 ) && ( disp < 128 ) ) * ( ( int8* ) offsetAddress ) = ( byte ) disp ;
#if 0 // this error hasn't caused any problems ??!        
        else 
        {
            iPrintf ( "\nPossible error for 8 bit jmp/jcc : _SetOffsetForCallOrJump : insnAddress = 0x%lx : jmpToAddress = 0x%lx : disp = 0x%lx : %ld", 
                bi->InsnAddress, bi->JmpToAddress, disp, disp ) ;
            Udis_Disassemble ( bi->InsnAddress, disp + 32 , 1 ) ;
            Error ( "\nError : _CalculateDispForCallOrJump : disp to large for instruction!", CONTINUE ) ; //QUIT ) ;
            //return -1 ;
        }
#endif         
    }
    else * ( ( int32* ) ( offsetAddress ) ) = disp ;
    return disp ;
}

int32
CalculateOffsetForCallOrJump (byte * insnAddr, byte * jmpToAddr, byte insnType )
{
    BlockInfo *bi = BlockInfo_Allocate ( ) ; // just COMPILER_TEMP
    bi->InsnAddress = insnAddr ;
    bi->JmpToAddress = jmpToAddr ;
    bi->Insn = 0 ; //insn ;// correct previous useages
    bi->InsnType = insnType ;
    return SetOffsetForCallOrJump ( bi ) ; //nb bi->Disp is set 
}

int32
GetDispForCallOrJumpFromInsnAddr ( byte * insnAddr )
{
    int32 disp ;
    if ( insnAddr )
    {
        byte insn = * insnAddr ;
        int64 insnSize = ( insn == JCC32 ) ? 2 : 1 ; //( (insn == JMPI32) || (insn == JCC8) ) ? 1 : 2 ;
        byte * offsetAddress = insnAddr + insnSize ;
        if ( insn == JCC32 )
            disp = * ( int32* ) offsetAddress ;
        else // insn == JCC8
            disp = ( int8 ) * ( byte* ) offsetAddress ;
        return disp ;
    }
    return 0 ;
}

// JE, JNE, JZ, JNZ, ... see machineCode.h
// optimize JCC32 for JCC8 

byte *
_Compile_Jcc ( byte insn, int64 ttt, int64 n, int32 disp )
{
    switch ( insn )
    {
        case ( JCC8 ):
        {
            Compile_CalculateWrite_Instruction_X64 ( 0, ( 0x7 << 4 | ttt << 1 | n ),
                0, 0, 0, DISP_B, 0, disp, BYTE, 0, 0 ) ;
            break ;
        }
        case ( JCC32 ):
        {
            Compile_CalculateWrite_Instruction_X64 ( 0x0f, ( 0x8 << 4 | ttt << 1 | n ),
                0, 0, 0, DISP_B, 0, disp, INT32_SIZE, 0, 0 ) ;
            break ;
        }
        default: break ;
    }
}

byte *
Compile_Jcc ( int64 ttt, int64 n, byte * jmpToAddr, byte insn )
{
    byte * compiledAtAddress = Here ;
    int32 disp ;
    if ( jmpToAddr )
    {
        disp = CalculateOffsetForCallOrJump (Here, jmpToAddr, T_JCC ) ;
        if ( ( disp >= - 127 ) && ( disp <= 128 ) )
        {
            _Compile_Jcc ( JCC8, ttt, n, disp ) ;
            return compiledAtAddress ;
        }
        else disp = CalculateOffsetForCallOrJump (Here, jmpToAddr, T_JCC ) ;
    }
    else disp = 0 ; // allow this function to be used to have a delayed compile of the actual address
    SetCompilerField ( CurrentTopBlockInfo, JccCode, Here ) ;
    if ( GetState ( _CSL_, JCC8_ON ) )
    {
        _Compile_Jcc ( JCC8, ttt, n, disp ) ;
        return compiledAtAddress ;
    }
    else _Compile_Jcc ( JCC32, ttt, n, disp ) ;
    return compiledAtAddress ;
}

byte *
Compile_JccGotoInfo ( BlockInfo *bi, int64 type )
{
    //Compiler * compiler = _Compiler_ ;
    byte * compiledAtAddress ;
    Word * word = CSL_WordList ( 0 ) ;
    //if ( ! bi ) bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;

    if ( type )
    {
        bi->BI_Gi = GotoInfo_NewAdd ( bi, word, &( _CSL_->SC_Buffer [ _CSL_->SC_Index - 1 ] ), type, Here ) ;
    }
    //if ( bi->JccCode ) SetHere ( bi->JccCode ) ;
    //else 
    bi->JccCode = Here ;
    compiledAtAddress = Compile_Jcc ( bi->Ttt, bi->N, 0, (GetState ( _CSL_, JCC8_ON ) ? JCC8  : JCC32 ) ) ;
    return compiledAtAddress ;
}

byte *
Compile_UninitializedJccEqualZero ( )
{
    byte * compiledAtAddress = Here ;
    //Compile_Jcc ( TTT_ZERO, N_0, 0, 0 ) ;
    _Compile_Jcc ( JCC32, TTT_ZERO, N_0, 0 ) ;
    return compiledAtAddress ;
}

byte *
Compile_UninitializedJccNotEqualZero ( )
{
    byte * compiledAtAddress = Here ;
    //Compile_Jcc ( TTT_ZERO, N_0, 0, 0 ) ;
    _Compile_Jcc ( JCC32, TTT_ZERO, N_1, 0 ) ;
    return compiledAtAddress ;
}

void
_Compile_JumpToDisp ( int32 disp, byte insn )
{
    if ( ( insn != JMP32 ) && ( ( disp > - 127 ) && ( disp <= 128 ) ) )
        //if ( ( disp > - 127 ) && ( disp <= 128 ) )
        Compile_CalculateWrite_Instruction_X64 ( 0, JMP8, 0, 0, 0, DISP_B, 0, disp, BYTE, 0, 0 ) ;
    else Compile_CalculateWrite_Instruction_X64 ( 0, JMP32, 0, 0, 0, DISP_B, 0, disp, INT32_SIZE, 0, 0 ) ;
}

void
Compile_JumpToAddress ( byte * jmpToAddr, byte insn ) // runtime
{
    int32 disp = CalculateOffsetForCallOrJump (Here, jmpToAddr, T_JMP ) ;
    _Compile_JumpToDisp ( disp, insn ) ;
}

void
Compile_JumpToRegAddress ( Boolean reg )
{

    _Compile_Group5 ( JMP, 0, reg, 0, 0, 0 ) ;
}

void
_Compile_UninitializedJmpOrCall ( Boolean jmpOrCall ) // jmpOrCall : CALL32/JMP32
{
    Compile_CalculateWrite_Instruction_X64 ( 0, jmpOrCall, 0, 0, 0, DISP_B, 0, 0, INT32_SIZE, 0, 0 ) ;
}

void
_Compile_JumpWithOffset ( int64 disp )
{
    _Compile_JumpToDisp ( disp, 0 ) ;
}

byte *
_Compile_UninitializedCall ( )
{
    byte * compiledAtAddress = Here ;
    _Compile_UninitializedJmpOrCall ( CALL32 ) ;
    return compiledAtAddress ;
}

byte *
Compile_UninitializedJump ( )
{
    byte * compiledAtAddress = Here ;
    _Compile_UninitializedJmpOrCall ( JMP32 ) ;
    return compiledAtAddress ;
}

void
_Compile_CallReg ( Boolean reg, Boolean regOrMem )
{
    _Compile_Group5 ( CALL, regOrMem, reg, 0, 0, 0 ) ;
}

GotoInfo *
_GotoInfo_Allocate ( )
{
    GotoInfo * gi = ( GotoInfo * ) Mem_Allocate ( sizeof ( GotoInfo ), TEMPORARY ) ;
    return gi ;
}

void
GotoInfo_Remove ( dlnode * node )
{
    GotoInfo * gi = ( GotoInfo* ) node ;
    dlnode_Remove ( ( dlnode * ) gi ) ;
}

GotoInfo *
GotoInfo_Init ( GotoInfo * gi, BlockInfo* bi, Word * word, byte * name, uint64 type, byte * address )
{
    int64 insnSize ; //, offsetSize ;
    if ( address )
    {
        if ( * address == JCC32 ) insnSize = 2 ;
        else insnSize = 1 ; //JCC8
    }
    gi->CompiledAtAddress = address ;
    gi->OriginalAddress = address ; //? address : Here - 5 ; // 6 : size of jmp/call insn
    gi->PtrToJmpInsn = address ;
    gi->pb_JmpOffsetPointer = address + insnSize ; //- offsetSize ; //Here - INT32_SIZE ;
    gi->pb_LabelName = name ;
    gi->GI_CAttribute = type ;
    gi->GI_BlockInfo = ( byte * ) bi ;
    gi->GI_Word = word ;
    gi->Combinator = _Context_->CurrentCombinator ; //_Compiler_->Combinator ;

    return gi ;
}

void
GotoInfo_NewAddAsSetcc ( BlockInfo* bi, byte * originalAddress, byte * movedToAddress )
{

    Compiler * compiler = _Compiler_ ;
    GotoInfo * gi = ( GotoInfo * ) _GotoInfo_Allocate ( ) ;
    gi->OriginalAddress = originalAddress ;
    gi->CompiledAtAddress = movedToAddress ;
    dllist_AddNodeToHead ( compiler->SetccMovedList, ( dlnode* ) gi ) ;
}

GotoInfo *
GotoInfo_NewAdd ( BlockInfo* bi, Word* word, byte * name, uint64 type, byte * address )
{
    Compiler * compiler = _Compiler_ ;
    GotoInfo * gi = ( GotoInfo * ) _GotoInfo_Allocate ( ) ;
    GotoInfo_Init ( gi, bi, word, name, type, address ) ;
    dllist_AddNodeToHead ( compiler->GotoList, ( dlnode* ) gi ) ;
    gi->BlockLevel = compiler->BlockLevel ;
    gi->CombinatorLevel = compiler->CombinatorLevel ;

    return gi ;
}

void
_CSL_CompileCallJmpGoto ( byte * name, uint64 type )
{
    GotoInfo_NewAdd ( 0, 0, name, type, Here ) ;
    if ( ( type == GI_RECURSE ) ) // && (! GetState ( _Context_, C_SYNTAX | PREFIX_MODE | INFIX_MODE ) ) ) // tail call recursion??
    {
        _Compile_UninitializedCall ( ) ;
    }
    else Compile_UninitializedJump ( ) ;
}

void
_CSL_Goto ( byte * name )
{
    _CSL_CompileCallJmpGoto ( name, GI_GOTO ) ;
}

void
_CSL_GotoLabel ( byte * name )
{
    _CSL_CompileCallJmpGoto ( name, GI_GOTO_LABEL ) ;
}

void
CSL_Goto ( )
{
    _CSL_Goto ( ( byte * ) DataStack_Pop ( ) ) ;
}

void
CSL_Goto_Prefix ( )
{
    byte * gotoToken = Lexer_ReadToken ( _Context_->Lexer0 ) ;
    _CSL_Goto ( gotoToken ) ;
}

void
CSL_Label ( )
{
    _CSL_Label ( ( byte* ) DataStack_Pop ( ) ) ;
}

void
CSL_Label_Prefix ( )
{
    byte * labelToken = Lexer_ReadToken ( _Context_->Lexer0 ) ;
    _CSL_Label ( labelToken ) ;
}

// 'return' is a prefix word now C_SYNTAX or not
// not satisfied yet with how 'return' works with blocks and locals ???

void
CSL_Return ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Word* wordr = CSL_WordList ( 0 ) ; // 'return
    compiler->ReturnWord = wordr ;
    byte * token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ) ;
    Word * word = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
    SetState ( _Compiler_, DOING_RETURN, true ) ;
    CSL_DoReturnWord ( word ) ;
    SetState ( _Compiler_, DOING_RETURN, false ) ;
}

void
CSL_Continue ( )
{
    _CSL_CompileCallJmpGoto ( 0, GI_CONTINUE ) ;
}

void
CSL_Break ( )
{
    _CSL_CompileCallJmpGoto ( 0, GI_BREAK ) ;
}

void
CSL_SetupRecursiveCall ( )
{
    _CSL_CompileCallJmpGoto ( 0, GI_RECURSE ) ;
}

void
AdjustGotoInfo ( dlnode * node, int64 srcAddress )
{
    GotoInfo * gi = ( GotoInfo* ) node ;
    if ( gi->CompiledAtAddress == ( byte* ) srcAddress )
    {
        gi->CompiledAtAddress = Here ;
        if ( gi->GI_BlockInfo && ( ( BlockInfo * ) gi->GI_BlockInfo )->JccCode == ( byte* ) srcAddress ) 
            ( ( BlockInfo * ) gi->GI_BlockInfo )->JccCode = Here ;
#if 1            
        if ( Is_DebugOn )
        {
            iPrintf ( "\nAdjustGotoInfo : %.24s : srcAddress = %lx : gi->CompiledAtAddress = %lx",
                gi->pb_LabelName, srcAddress, gi->CompiledAtAddress ) ;
            _Debugger_Disassemble ( _Debugger_, 0, ( byte* ) srcAddress, 6, 1 ) ;
            iPrintf ( "\nAdjustGotoInfo : %s", Context_Location ( ) ) ;
        }
#endif            
    }
}

void
AdjustLabel ( dlnode * node, int64 address )
{
    GotoInfo * gi = ( GotoInfo * ) node ;

    if ( gi->LabeledAddress == ( byte* ) address )
        gi->LabeledAddress = Here ;
}

int64
GotoInfo_ResetOffsetAndRemove ( GotoInfo * gi, byte * address, Boolean removeFlag )
{
    int64 rtrn = 0 ;
    if ( ( gi->GI_CAttribute & ( GI_JCC_TO_FALSE | GI_JCC_TO_TRUE ) ) )
    {
        if ( ! address ) address = Here ;
        byte * compiledAtAddress = gi->CompiledAtAddress ;
        BlockInfo * bi = ( ( BlockInfo * ) gi->GI_BlockInfo ) ;
        byte * jmpToAddress = bi->JmpToAddress ;
        //Compiler_Word_SCHCPUSCA ( gi->GI_Word, 1 ) ;
        //byte insn = 
        bi->Insn = * compiledAtAddress ;
        if ( ( bi->Insn == JCC32 ) || IS_INSN_JCC8 ( bi->Insn ) ) //|| ( insn == NOOP ) )
            rtrn = CalculateOffsetForCallOrJump (compiledAtAddress, jmpToAddress ? jmpToAddress : Here, T_JCC) ; // ?? could recompile the insn to JCC8 if jump < 127 ??
        else return rtrn ; // insn move for some reason
#if 1            
        if ( ( rtrn == -1 ) || Is_DebugOn )
        {
            char * format0 = ( gi->GI_CAttribute & GI_JCC_TO_TRUE ) ? "\nGotoInfo_ResetOffsetAndRemove : GI_JCC_TO_TRUE : %.24s : OriginalAddress = %lx : CompiledAtAddress = %lx" :
                "\nGotoInfo_ResetOffsetAndRemove : GI_JCC_TO_FALSE : %.24s : OriginalAddress = %lx : CompiledAtAddress = %lx" ;
            iPrintf ( format0, gi->pb_LabelName, gi->OriginalAddress, gi->CompiledAtAddress ) ;
            _Debugger_Disassemble ( _Debugger_, 0, compiledAtAddress, 6, 1 ) ;
            iPrintf ( "\nGotoInfo_ResetOffsetAndRemove : %s", Context_Location ( ) ) ;
        }
#endif            
        gi->AddressSet = true ;
    }
    else rtrn = CalculateOffsetForCallOrJump (gi->CompiledAtAddress, address, 0) ;

    if ( removeFlag ) GotoInfo_Remove ( ( dlnode* ) gi ) ;
    return rtrn ;
}

int64
_InstallGotoPoint_Key ( dlnode * node, int64 blockInfo, int64 key, byte * address, Boolean removeFlag )
{
    Compiler * compiler = _Compiler_ ;
    Word * word ;
    GotoInfo * gi = ( GotoInfo* ) node ;
    BlockInfo * bi = ( BlockInfo * ) blockInfo ;
    Word * combinator = _Context_->CurrentCombinator, *scCombinator = _Context_->SC_CurrentCombinator ;
    if ( ( key & ( GI_BREAK | GI_RETURN | GI_GOTO | GI_LABEL | GI_JCC_TO_FALSE | GI_JCC_TO_TRUE ) ) ) // if we move a block its recurse disp remains, check if this looks like at real disp pointer
    {
        if ( ( ( gi->GI_CAttribute & ( GI_GOTO ) ) && ( key & ( GI_GOTO ) ) ) )
        {
            Namespace * ns = _Namespace_Find ( ( byte* ) "__labels__", _CSL_->Namespaces, 0 ) ;
            if ( ns && ( word = _Finder_FindWord_InOneNamespace ( _Finder_, ns, gi->pb_LabelName ) ) )
            {
                GotoInfo * giw = ( GotoInfo * ) word->W_Value ;
                GotoInfo_ResetOffsetAndRemove ( gi, ( byte* ) giw->LabeledAddress, removeFlag ) ;
            }
        }
        else if ( ( gi->GI_CAttribute & GI_RETURN ) && ( key & GI_RETURN ) )
        {
            GotoInfo_ResetOffsetAndRemove ( gi, Here, removeFlag ) ;
        }
        else if ( ( gi->GI_CAttribute & GI_JCC_TO_TRUE ) && ( key & GI_JCC_TO_TRUE ) )
        {
            if ( ( ! gi->Combinator ) || ( combinator == gi->Combinator ) || ( scCombinator == gi->Combinator ) ) //|| ( bi && ( combinator == bi->OurCombinator ) )) //_Compiler_->Combinator ) ) // don't get to complicated in postfix combinators yet
                GotoInfo_ResetOffsetAndRemove ( gi, address, removeFlag ) ; // 1 : remove flag 
        }
        else if ( ( gi->GI_CAttribute & GI_JCC_TO_FALSE ) && ( key & GI_JCC_TO_FALSE ) )
        {
            if ( ( ! gi->Combinator ) || ( combinator == gi->Combinator ) || ( scCombinator == gi->Combinator ) ) // || ( bi && ( combinator == bi->OurCombinator ) ) )
                GotoInfo_ResetOffsetAndRemove ( gi, address, removeFlag ) ; // 1 : remove flag 
        }
        else if ( ( gi->GI_CAttribute & GI_BREAK ) && ( key & GI_BREAK ) )
        {
            if ( _Context_->Compiler0->BreakPoint )
            {
                GotoInfo_ResetOffsetAndRemove ( gi, _Compiler_->BreakPoint, removeFlag ) ;
            }
        }
        else if ( ( gi->GI_CAttribute & GI_CONTINUE ) && ( key & GI_CONTINUE ) )
        {
            if ( _Context_->Compiler0->ContinuePoint )
            {
                GotoInfo_ResetOffsetAndRemove ( gi, _Compiler_->ContinuePoint, removeFlag ) ;
            }
        }
        else if ( ( gi->GI_CAttribute & GI_RECURSE ) && ( key & GI_RECURSE ) )
        {
            GotoInfo_ResetOffsetAndRemove ( gi, bi->bp_First, removeFlag ) ;
        }
        else if ( ( gi->GI_CAttribute & GI_TAIL_CALL ) && ( key & GI_TAIL_CALL ) )
        {
            GotoInfo_ResetOffsetAndRemove ( gi, bi->AfterLocalFrame, removeFlag ) ;
        }
    }
    return 0 ;
}

void
_CheckForGotoPoint ( dlnode * node, int64 key, int64 * status )
{
    GotoInfo * gi = ( GotoInfo* ) node ;
    if ( gi->GI_CAttribute & key ) *status = DONE ;
}

void
_RemoveGotoPoint ( dlnode * node, int64 key, int64 * status )
{
    GotoInfo * gi = ( GotoInfo* ) node ;
    if ( gi->GI_CAttribute & key )
    {
        dlnode_Remove ( ( dlnode* ) gi ) ;
        *status = DONE ;
    }
}

void
CSL_InstallGotoCallPoints_Keyed ( BlockInfo * bi, int64 key, byte * address, Boolean removeFlag )
{

    if ( key ) dllist_Map4 ( _Compiler_->GotoList,
        ( MapFunction4wReturn ) _InstallGotoPoint_Key, ( int64 ) bi, key, ( int64 ) address, ( int64 ) removeFlag ) ;
}

byte *
CheckSetccMoved ( dlnode * node, byte * address )
{
    GotoInfo * gi = ( GotoInfo* ) node ;
    if ( gi->OriginalAddress == address )
    {

        return gi->CompiledAtAddress ;
    }
    return 0 ;
}

byte *
CSL_CheckSetccMovedList ( BlockInfo * bi, byte * address )
{
    byte * newAddress = ( byte* ) dllist_Map1_WReturn ( _Compiler_->SetccMovedList, ( MapFunction1 ) CheckSetccMoved, ( int64 ) address ) ;

    return newAddress ;
}

void
CSL_JMP ( )
{
    if ( CompileMode )
    {
        byte * compiledAtAddress = Compile_UninitializedJump ( ) ; // at the end of the 'if block' we need to jmp over the 'else block'
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
    }
    else
    {
        Error_Abort ( "", "\njmp : can only be used in compile mode." ) ;
    }
}

void
CSL_Compile_Jcc ( )
{

    int64 ttt = DataStack_Pop ( ) ;
    int64 n = DataStack_Pop ( ) ;
    byte * compiledAtAddress = Compile_Jcc ( ttt, n, 0, 0 ) ; // we do need to store and get this logic set by various conditions by the compiler : _Compile_SET_tttn_REG
    Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
}

void
CSL_Jcc_Label ( )
{
    int64 ttt = DataStack_Pop ( ) ;
    int64 n = DataStack_Pop ( ) ;
    GotoInfo_NewAdd ( 0, 0, ( byte* ) DataStack_Pop ( ), GI_GOTO, Here ) ;
    Compile_Jcc ( ttt, n, 0, 0 ) ;
}

void
CSL_JmpToHere ( )
{
    CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
}

void
Compile_Jmp_GotoInfo ( byte * name, int64 type )
{

    Compiler * compiler = _Compiler_ ;
    //DBI_ON ;
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    iPrintf ( "\nCompile_Jmp_GotoInfo : %s", Context_Location ( ) ) ;
    GotoInfo_NewAdd ( bi, CSL_WordList ( 0 ), name, type, Here ) ;
    Compile_UninitializedJump ( ) ;
    //DBI_OFF ;
}

void
Compiler_CalculateAndSetPreviousJmpOffset ( byte * jmpToAddress )
{
    // we now can not compile blocks (cf. _Compile_Block_WithLogicFlag ) if their logic is not called so depth check is necessary
    if ( _Stack_Depth ( _Compiler_->PointerToJmpInsnStack ) )
    {
        byte * insnAddr = ( byte* ) Stack_Pop ( _Compiler_->PointerToJmpInsnStack ) ;
        //SetOffsetForCallOrJump ( insnAddr ? insnAddr : Here, jmpToAddress, T_JMP, 0 ) ;
        CalculateOffsetForCallOrJump (insnAddr ? insnAddr : Here, jmpToAddress, 0) ;
    }
}

void
CSL_CalculateAndSetPreviousJmpOffset_ToHere ( )
{
    Compiler_CalculateAndSetPreviousJmpOffset ( Here ) ;
}

void
Stack_Push_PointerToJmpOffset ( byte * compiledAtAddress )
{
    Stack_Push ( _Compiler_->PointerToJmpInsnStack, ( int64 ) compiledAtAddress ) ;
}

void
CSL_AdjustLabels ( byte * srcAddress )
{
    dllist_Map1 ( _Context_->Compiler0->GotoList, ( MapFunction1 ) AdjustLabel, ( int64 ) ( srcAddress ) ) ;
}

int64
CSL_CheckForGotoPoints ( int64 key )
{
    int64 status = 0 ;
    dllist_Map_OnePlusStatus ( _Context_->Compiler0->GotoList, ( MapFunction2 ) _CheckForGotoPoint, key, &status ) ;
    return status ;
}

int64
CSL_RemoveGotoPoints ( int64 key )
{
    int64 status = 0 ;
    dllist_Map_OnePlusStatus ( _Context_->Compiler0->GotoList, ( MapFunction2 ) _RemoveGotoPoint, key, &status ) ;
    return status ;
}
