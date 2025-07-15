#include "../../include/csl.h"
#define TEST_0 0

byte *
_BI_Compile_Jcc_AdjustBiN ( BlockInfo *bi, byte * jmpToAddress, int64 flipN )
{
    byte insn = 0 ; //GetState ( _CSL_, JCC8_ON ) ? JCC8 : 0 ;
    int64 n = flipN ? ( ! bi->N ) : ( bi->N ) ;
    byte * compiledAtAddress = Compile_Jcc ( bi->Ttt, n, jmpToAddress, insn ) ; // we do need to store and get this logic set by various conditions by the compiler : _Compile_SET_Tttn_REG
    return compiledAtAddress ; // Here 

}

// bi->JccCode may be set by a previous logic op

byte *
BI_Compile_Jcc_AdjustN ( BlockInfo *bi, byte * jmpToAddress, int64 flipN )
{
    bi->JccCode = Here ;
    bi->ActualCopiedToJccCode = Here ;
    byte * compiledAtAddress = _BI_Compile_Jcc_AdjustBiN ( bi, jmpToAddress, flipN ) ;
    return compiledAtAddress ;
}

byte *
BI_Compile_JccAdjustN ( BlockInfo *bi, byte * jmpToAddress, int64 flipN ) // , int8 nz
{
    byte * compiledAtAddress ;
    if ( bi->JccCode ) SetHere ( bi->JccCode ) ;
    if ( bi->TttnCode )
    {
        SetHere ( bi->TttnCode ) ;
        compiledAtAddress = BI_Compile_Jcc_AdjustN ( bi, jmpToAddress, flipN ) ;
    }
    else
    {
        Compile_BlockLogicTest ( bi ) ;
        compiledAtAddress = bi->JccCode ;
    }
    return compiledAtAddress ;
}
// non-combinator 'if'

byte *
Compiler_TestJccAdjustN ( Compiler * compiler, byte * jmpToAddress, int64 flipN ) // , int8 nz
{
    byte * compiledAtAddress ;
    BlockInfo *bi = ( BlockInfo * ) _Stack_Pick ( compiler->CombinatorBlockInfoStack, 0 ) ; // -1 : remember - stack is zero based ; stack[0] is top
    if ( ( ! bi->JccCode ) && ( ! bi->TttnCode ) && ( ! bi->CmpCode ) )
    {
        _Compile_GetCmpLogicFromTOSwithJcc ( bi ) ;
        compiledAtAddress = bi->JccCode ;
    }
    else compiledAtAddress = BI_Compile_JccAdjustN ( bi, jmpToAddress, flipN ) ;
    return compiledAtAddress ;
}

int32
BI_CalculateOffsetForCallOrJumpOrJcc ( BlockInfo * bi )
{
    if ( ! bi->Insn )
    {
        if ( bi->InsnAddress )
        {
            bi->Insn = ( * bi->InsnAddress ) ;
            bi->Uinsn16 = ( * ((uint16 *) bi->InsnAddress)) ;
        }
        else Error ( "\nBI_CalculateOffsetForCallOrJumpOrJcc : No insn or InsnAddress", QUIT ) ;
    }
    if ( ! bi->InsnType )
    {
        if ( ( (bi->Uinsn16 & (0x800f)) == JCC32_2 ) || ( bi->Insn == JCC8 ) ) bi->InsnType = T_JCC ;
        //if ( ( bi->Uinsn16 == JCC32_2 ) || ( bi->Insn == JCC8 ) ) bi->InsnType = T_JCC ;
        //if ( ( bi->Insn == JCC32 ) || ( bi->Insn == JCC8 ) ) bi->InsnType = T_JCC ;
        else bi->InsnType = ( T_JMP | T_CALL ) ;
    }
    bi->InsnSize = ( ( (bi->Uinsn16 & (0x800f)) == JCC32_2 ) ) ? 2 : 1 ;
    //bi->InsnSize = ( ( bi->Uinsn16 == JCC32_2 ) ) ? 2 : 1 ;
    //bi->InsnSize = ( bi->Insn == JCC32 ) ? 2 : 1 ;
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
                bi->OffsetSize = 1 ;
                bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + bi->OffsetSize ) ;
                if ( ( bi->Disp >= - 127 ) && ( bi->Disp <= 128 ) ) bi->OffsetSize = 1, bi->Insn = JMP8 ;
                else bi->OffsetSize = 4, bi->Insn = JMP32 ;
            }
        }
    }
    else if ( bi->InsnType & ( T_JCC ) )
    {
        if ( ( (bi->Uinsn16 & (0x800f)) == JCC32_2 ) ) bi->OffsetSize = 4 ;
        //if ( bi->Uinsn16 == JCC32_2 ) bi->OffsetSize = 4 ;
        //if ( bi->Insn == JCC32 ) bi->OffsetSize = 4 ;
        else bi->OffsetSize = 1 ;
    //bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + bi->OffsetSize ) ;
    //if ( ( bi->Disp >= - 123 ) && ( bi->Disp <= 124 ) ) bi->OffsetSize = 1 ; //, bi->Insn = JMP8 ;
    //else bi->OffsetSize = 4 ;
    }
    bi->Disp = bi->JmpToAddress - ( bi->InsnAddress + bi->InsnSize + bi->OffsetSize ) ;
#if 0      
    if ( ( bi->Disp >= - 127 ) && ( bi->Disp <= 128 ) && ( bi->OffsetSize == 1 ) ) bi->IiFlags |= COULD_BE_8 ;
    else bi->IiFlags |= SHOULD_BE_32 ;
    //else if ( ( bi->Disp >= ( - ( 65535 ) ) ) && ( bi->Disp <= ( 65536 ) ) ) bi->IiFlags |= COULD_BE_16 ;
#endif        
    return bi->Disp ;
}

int32
SetOffsetForCallOrJump ( BlockInfo * bi ) //byte * insnAddr, byte * jmpToAddr, byte insnType, byte insn )
{
    int32 disp = bi->Disp = BI_CalculateOffsetForCallOrJumpOrJcc ( bi ) ; //( insnAddr, jmpToAddr, insn, insnType ) ;
    byte * offsetAddress = bi->InsnAddress + bi->InsnSize ; //insnAddr + insnSize ;
    if ( IS_INSN_JCC8 ( bi->Insn ) || ( bi->Insn == JMP8 ) ) //|| ( insn == JCC8 ) )
    {
        if ( ( disp > - 127 ) && ( disp < 128 ) )
        {
            * ( ( int8* ) offsetAddress ) = ( byte ) disp ;
            return disp ;
        }
    }
    * ( ( int32* ) ( offsetAddress ) ) = disp ;
    return disp ;
}

int32
CalculateOffsetForCallOrJump ( byte * insnAddr, byte * jmpToAddr, byte insnType, byte insn )
{
    BlockInfo *bi = BlockInfo_Allocate ( ) ; // just COMPILER_TEMP
    bi->InsnAddress = insnAddr ;
    bi->JmpToAddress = jmpToAddr ;
    bi->Insn = insn ; // correct previous useages
    bi->Uinsn16 = ( * ((uint16 *) insnAddr)) ;
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
        int64 insnSize = ( insn == JCC32 ) ? 2 : 1 ;
        byte * offsetAddress = insnAddr + insnSize ;
        if ( insn == JCC32 ) disp = * ( int32* ) offsetAddress ;
        else disp = ( int8 ) * ( byte* ) offsetAddress ;
        return disp ;
    }
    return 0 ;
}

void
CSL_If_TttN_0Branch_Jcc ( )
{
    if ( CompileMode )
    {
        //DBI_ON ;
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        byte * compiledAtAddress = Compiler_TestJccAdjustN ( _Compiler_, 0, 1 ) ; // whatever logic < > <= >= == this jmps if not true -'AdjustN'
        //DBI_OFF ;
        // N, ZERO : use inline|optimize logic which needs to get flags immediately from a 'cmp', jmp if the zero flag is not set
        // for non-inline|optimize ( reverse polarity : cf. _Compile_Jcc comment ) : jmp if cc is not true; cc is set by setcc after 
        // the cmp, or is a value on the stack. 
        // We cmp that value with zero and jmp if this second cmp sets the flag to zero else do the immediately following block code
        Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
    }
    else
    {
        byte *str = _Context_->ReadLiner0->InputLine ;
        int64 index = _Context_->Lexer0->TokenStart_ReadLineIndex - 1 ;
        if ( String_IsPreviousCharA_ ( str, index, '}' ) ) CSL_If2Combinator ( ) ;
        else if ( String_IsPreviousCharA_ ( str, index, '#' ) ) CSL_If_ConditionalInterpret ( ) ;
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

void
CSL_Else ( )
{
    if ( CompileMode )
    {
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        byte * compiledAtAddress = Compile_UninitializedJump ( ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ; // if 0 branch to here after the jmp to endif
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
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
    }
    //else Compiler_Init ( _Compiler_, 0 ) ;
}

//: tr ( x ) begin x @ 10 < while x x @ 1 + = x @ p repeat ; pwi tr 0 tr 
//: tu ( x ) begin x @ p x x @ 1 + = x @ 10 > until ;  pwi tu 0 tu
void
CSL_Begin ( )
{
    if ( CompileMode )
    {
        //Stack_Push_PointerToJmpOffset ( Here ) ;
        //_Compiler_->BeginAddress = Here ;
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        Stack_Push ( _Compiler_->BeginAddressStack, ( int64 ) Here ) ;
    }
}

//: tr ( x ) begin x @ 10 < while x x @ 1 + = x @ p repeat ; pwi tr 0 tr 
//: tfor ( n | x ) x n @ =  begin x @ 0 > while x @ p x x @ 1 - = repeat ; pwi tfor 10 tfor        
void
CSL_While ( )
{
    if ( CompileMode )
    {
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        byte * compiledAtAddress = Compiler_TestJccAdjustN ( _Compiler_, 0, 1 ) ; // whatever logic < > <= >= == this jmps if not true -'AdjustN'
        Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
    }
}

#if 0
: factorial3 ( REG n | REG rec ) 
    rec 1 = 
    begin rec rec @ n @ * = n n @ 1 - =  n @ 1 > doWhile  
    return rec @ 
;
pwi factorial3 
7 factorial3 dup p 5040 _assert //pause
: factorial4n ( REG n | REG rec ) 
    rec 1 = 
    n @ 1 > if begin rec rec @ n @ * = n n @ 1 - = n @ 1 > doWhile endif
    return rec @ 
;
pwi factorial4n 
7 factorial4n dup p 5040 _assert //pause
#endif
void
CSL_DoWhile ( )
{
    if ( CompileMode )
    {
        byte * beginAddress = ( byte* ) Stack_Pop ( _Compiler_->BeginAddressStack ) ;
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 1 ) ;
        byte * compiledAtAddress = Compiler_TestJccAdjustN ( _Compiler_, beginAddress, 0 ) ; // whatever logic < > <= >= == this jmps if not true -'AdjustN'
    }
}

//: tr ( x ) begin x @ 10 < while x x @ 1 + = x @ p repeat ; pwi tr 0 tr 
void
CSL_Repeat ( )
{
    if ( CompileMode )
    {
        byte * beginAddress = ( byte* ) Stack_Pop ( _Compiler_->BeginAddressStack ) ;
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        Compile_JumpToAddress ( beginAddress, 0 ) ;
        //d1 ( Debugger_Dis ( _Debugger_ ) ) ;
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
    }
}

//: tu ( x ) begin x @ p x x @ 1 + = x @ 10 > until ;  pwi tu 0 tu
void
CSL_Until ( )
{
    if ( CompileMode )
    {
        byte * beginAddress = ( byte* ) Stack_Pop ( _Compiler_->BeginAddressStack ) ;
        Compiler_Word_SCHCPUSCA ( _Context_->CurrentEvalWord, 0 ) ;
        Compiler_TestJccAdjustN ( _Compiler_, beginAddress, 1 ) ;
    }
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
    int32 disp ; //= bi->Disp ;
    if ( jmpToAddr ) //&& (! disp ))
    {
        disp = CalculateOffsetForCallOrJump ( Here, jmpToAddr, T_JCC, insn ) ;
#if 0       
        if ( ( disp >= - 127 ) && ( disp <= 128 ) )
        //if ( ( disp >= - 123 ) && ( disp <= 124 ) )
        {
            _Compile_Jcc ( JCC8, ttt, n, disp ) ;
            return compiledAtAddress ;
        }
#endif        
    }
    else disp = 0 ; // allow this function to be used to have a delayed compile of the actual address
    //SetCompilerField ( CurrentTopBlockInfo, JccCode, Here ) ;
    _Compile_Jcc ( JCC32, ttt, n, disp ) ;
    return compiledAtAddress ;
}

byte *
Compile_JccGotoInfo ( BlockInfo *bi, int64 type )
{
    byte * compiledAtAddress ;
    Word * word = CSL_WordList ( 0 ) ;
    if ( type ) bi->BI_Gi = GotoInfo_NewAdd ( bi, word, &( _CSL_->SC_Buffer [ _CSL_->SC_Index - 1 ] ), type, Here ) ;
    bi->JccCode = Here ;
    //compiledAtAddress = Compile_Jcc ( bi->Ttt, bi->N, 0, ( GetState ( _CSL_, JCC8_ON ) ? JCC8 : JCC32 ) ) ;
    compiledAtAddress = Compile_Jcc ( bi->Ttt, bi->N, 0, JCC32 ) ;
    return compiledAtAddress ;
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
    Word * word ;
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Word* wordr = CSL_WordList ( 0 ) ; // 'return
    compiler->ReturnWord = wordr ;
    byte * token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ) ;
    word = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
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
        if ( Is_DebugOn )
        {
            iPrintf ( "\nAdjustGotoInfo : %.24s : srcAddress = %lx : gi->CompiledAtAddress = %lx",
                gi->pb_LabelName, srcAddress, gi->CompiledAtAddress ) ;
            _Debugger_Disassemble ( _Debugger_, 0, ( byte* ) srcAddress, 6, 1 ) ;
            iPrintf ( "\nAdjustGotoInfo : %s", Context_Location ( ) ) ;
        }
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
            rtrn = CalculateOffsetForCallOrJump ( compiledAtAddress, jmpToAddress ? jmpToAddress : Here, T_JCC, 0 ) ; // ?? could recompile the insn to JCC8 if jump < 127 ??
        else return rtrn ; // insn move for some reason
        if ( ( rtrn == - 1 ) || Is_DebugOn )
        {
            char * format0 = ( gi->GI_CAttribute & GI_JCC_TO_TRUE ) ? "\nGotoInfo_ResetOffsetAndRemove : GI_JCC_TO_TRUE : %.24s : OriginalAddress = %lx : CompiledAtAddress = %lx" :
                "\nGotoInfo_ResetOffsetAndRemove : GI_JCC_TO_FALSE : %.24s : OriginalAddress = %lx : CompiledAtAddress = %lx" ;
            iPrintf ( format0, gi->pb_LabelName, gi->OriginalAddress, gi->CompiledAtAddress ) ;
            _Debugger_Disassemble ( _Debugger_, 0, compiledAtAddress, 6, 1 ) ;
            iPrintf ( "\nGotoInfo_ResetOffsetAndRemove : %s", Context_Location ( ) ) ;
        }
        //gi->AddressSet = true ;
    }
    else rtrn = CalculateOffsetForCallOrJump ( gi->CompiledAtAddress, address, 0, 0 ) ;

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
        CalculateOffsetForCallOrJump ( insnAddr ? insnAddr : Here, jmpToAddress, 0, 0 ) ;
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

#if 0

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

#endif