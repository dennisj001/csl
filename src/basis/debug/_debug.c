
#include "../../include/csl.h"

#if DEBUG
int64 dsDepth = 0 ;

void
_dbg ( ) //int64 index, int64 one, int64 two )
{
    //Debugger * debugger = _Debugger_ ;
    AdjustR14WithDsp ( ) ;
    int64 diff = 0, dsd = DataStack_Depth ( ) ;
    if ( ! dsDepth ) 
    {
        dsDepth = dsd ;
        oPrintf ( "\n dbg: ... %s : %ld.%ld : %s ...", "+++", diff, dsd, "+++" ) ;
    }
    else if ( dsd > dsDepth )
    {
        diff = dsd - dsDepth ;
        dsDepth = dsd ;
        oPrintf ( "\n dbg: ... %s : %ld.%ld : %s ...", "+++", diff, dsd, "+++" ) ;
        //CSL_PrintDataStack () ;
        //if ( ! GetState ( debugger, DBG_STEPPING ) ) 
        //_Debugger_ShowInfo ( debugger, "dbg", 0, 1 ) ;
        //CSL_PrintDataStack () ; 
        //Pause () ;
    }
    else if ( dsd < dsDepth )
    {
        diff = dsDepth - dsd ;
        dsDepth = dsd ;
        oPrintf ( "\n dbg: ... %s : %ld.%ld : %s ...", "---", diff, dsd, "---" ) ;
        //CSL_PrintDataStack () ;
        //if ( ! GetState ( debugger, DBG_STEPPING ) ) 
        //_Debugger_ShowInfo ( debugger, "dbg", 0, 1 ) ;
        //CSL_PrintDataStack () ;
        //Pause () ;
    }
}

#endif

// we have the address of a jcc insn 
// get the address it jccs to

byte *
JccInstructionAddress_2Byte ( byte * address )
{
    int32 offset = * ( int32* ) ( address + 2 ) ; // 2 : 2 byte opCode
    byte * jcAddress = address + offset + 6 ; // 6 : sizeof 0f jcc insn - 0x0f8x - includes 2 byte opCode
    return jcAddress ;
}

byte *
JccInstructionAddress_1Byte ( byte * address )
{
    //int32 offset = ( int8 ) * ( byte* ) ( address + 1 ) ; // 1 : 1 byte opCode
    int8 offset = GetDispForCallOrJumpFromInsnAddr ( address ) ;
    byte * jcAddress = address + offset + 2 ; // 2 : sizeof 0f jcc insn - 0x7x - includes 1 byte opCode
    return jcAddress ;
}

// we have the address of a jmp/call insn 
// get the address it jmp/calls to

byte *
JumpCallInstructionAddress ( byte * address )
{
    byte *jcAddress ;
    byte insn = * address ;

    if ( ( insn == JMP32 ) || ( insn == JCC32 ) )
    {
        int32 offset32 = * ( int32* ) ( address + 1 ) ; // 1 : 1 byte opCode
        jcAddress = address + offset32 + 5 ; // 5 : sizeof jmp insn - includes 1 byte opcode
    }
    else
    {
        int8 offset8 = * ( byte* ) ( address + 1 ) ;
        if ( ( abs ( offset8 ) < 127 ) && ( IS_INSN_JCC8 ( * address ) || ( insn == JMP8 ) ) ) //GetState ( _CSL_, JCC8_ON ) )
        {
            jcAddress = address + offset8 + 2 ;
        }
        else
        {
            int32 offset32 = * ( int32* ) ( address + 1 ) ; // 1 : 1 byte opCode
            jcAddress = address + offset32 + 5 ; // 5 : sizeof jmp insn - includes 1 byte opcode
        }
    }
    return jcAddress ;
}

byte *
JumpCallInstructionAddress_X64ABI ( byte * address )
{
    int64 offset ;
    if ( ( ( * ( address - 20 ) ) == 0x49 ) && ( ( * ( address - 19 ) ) == 0xbb ) ) offset = 18 ; // need a comment here ??
    else offset = 8 ;
    byte * jcAddress = * ( byte** ) ( address - offset ) ; //JumpCallInstructionAddress ( debugger->DebugAddress ) ;
    return jcAddress ;
}

void
_CSL_ACharacterDump ( char aChar )
{
    if ( isprint ( aChar ) ) iPrintf ( "%c", aChar ) ;
    else iPrintf ( "." ) ;
}

void
CSL_CharacterDump ( byte * address, int64 number )
{
    int64 i ;
    for ( i = 0 ; i < number ; i ++ ) _CSL_ACharacterDump ( address [ i ] ) ;
    iPrintf ( " " ) ;
}

void
_CSL_AByteDump ( byte aByte )
{
    iPrintf ( "%02x ", aByte ) ;
}

void
CSL_NByteDump ( byte * address, int64 number )
{
    int64 i ;
    for ( i = 0 ; i < number ; i ++ ) _CSL_AByteDump ( address [ i ] ) ;
    iPrintf ( " " ) ;
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
        if ( ( ( * ( uint16* ) ( address - 10 ) ) == 0xbb49 ) ) 
        {
            if ( ( ( * ( uint16* ) ( address - 20 ) ) == 0xba49 ) ) iaddress = ( byte* ) ( * ( ( uint64* ) ( address - 18 ) ) ) ; //mov r11, 0xxx in Compile_Call_TestRSP  
            else iaddress = ( byte* ) ( * ( ( uint64* ) ( address - 8 ) ) ) ; //mov r11, 0xxx in Compile_Call_TestRSP  
        }
        else if ( ( ( * ( uint16* ) ( address - 20 ) ) == 0xba49 ) ) iaddress = ( byte* ) ( * ( ( uint64* ) ( address - 18 ) ) ) ; //mov r11, 0xxx in Compile_Call_TestRSP  
        else iaddress = ( byte* ) ( * ( ( uint64* ) ( address - CELL ) ) ) ;
    }
    else if ( ( ( * ( uint16* ) address ) == 0xff48 ) && ( *( address + 2 ) == 0xd3 ) ) // call rax
    {
        iaddress = ( byte* ) ( * ( ( uint64* ) ( address - CELL ) ) ) ;
    }
    return iaddress ;
}

byte *
GetPostfix ( byte * address, byte* postfix, byte * buffer )
{
    byte * iaddress = 0, *str ;
    Word * word = 0 ;
    char * prePostfix = ( char* ) "  \t" ;
    if ( iaddress = Calculate_Address_FromOffset_ForCallOrJump ( address ) )
    {
        if ( ( word = Word_GetFromCodeAddress_NoAlias ( iaddress ) ) )
        {
            byte * name = ( byte* ) c_gd ( word->Name ) ;
            byte *postfx = c_u ( " >" ) ;
            byte *containingNamespace = word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "" ;
            if ( ( byte* ) word->CodeStart == iaddress )
            {
                snprintf ( ( char* ) buffer, 128, "%s< %s.%s : " UINT_FRMT "%s%s",
                    prePostfix, containingNamespace, name, ( uint64 ) iaddress, postfx, postfix ) ;
            }
            else
            {
                snprintf ( ( char* ) buffer, 128, "%s< %s.%s+%ld%s",
                    prePostfix, containingNamespace, name, iaddress - ( byte* ) word->CodeStart, postfx ) ; //, postfix ) ;
                //strcat ( buffer, c_u ( " >" ) ) ;
            }
        }
        if ( ! word ) snprintf ( ( char* ) buffer, 128, "%s< %s >", prePostfix, ( char * ) "C compiler code" ) ;
        postfix = buffer ;
    }
    else
    {
        str = String_CheckForAtAdddress ( *( ( byte ** ) ( address + 2 ) ), &_O_->Default, &_O_->User ) ;
        if ( str )
        {
            snprintf ( ( char* ) buffer, 128, "%s%s", prePostfix, str ) ;
            postfix = buffer ;
        }
    }
    return postfix ;
}

void
Compile_Debug_GetRSP ( ) // where we want the acquired pointer
{
    _Compile_PushReg ( ACC ) ;
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & _Debugger_->DebugRSP, RSP, ACC ) ; // esp 
    _Compile_PopToReg ( ACC ) ;
    _Compile_Stack_PushReg ( DSP, ACC, 0 ) ;
}

void
CSL_SetRtDebugOn ( )
{
    SetState ( _CSL_, RT_DEBUG_ON, true ) ;
    SetState ( _Debugger_, ( DBG_INTERPRET_LOOP_DONE ), true ) ;
}

void
Compile_DebugRuntimeBreakpointFunction ( block function ) // where we want the acquired pointer
{
    Compile_Call_TestRSP ( ( byte* ) CSL_SetRtDebugOn ) ;
    Compile_Call ( ( byte* ) _Debugger_->SaveCpuState ) ;
    Compile_Call_TestRSP ( ( byte* ) function ) ;
    Compile_Call ( ( byte* ) _Debugger_->RestoreCpuState ) ; // ?? why didn't we have this earlier
}

void
_CSL_DebugRuntimeBreakpoint ( ) // where we want the acquired pointer
{
    Compile_DebugRuntimeBreakpointFunction ( CSL_DebugRuntimeBreakpoint ) ;
}

void
_CSL_DebugRuntimeBreakpoint_IsDebugShowOn ( ) // where we want the acquired pointer
{
    Compile_DebugRuntimeBreakpointFunction ( CSL_DebugRuntimeBreakpoint_IsDebugShowOn ) ;
}

void
_CSL_DebugRuntimeBreakpoint_IsDebugOn ( ) // where we want the acquired pointer
{
    Compile_DebugRuntimeBreakpointFunction ( CSL_DebugRuntimeBreakpoint_IsDebugOn ) ;
}

void
_CSL_DebugRuntimeBreakpoint_IsDebugLevel1 ( ) // where we want the acquired pointer
{
    Compile_DebugRuntimeBreakpointFunction ( CSL_DebugRuntimeBreakpoint_IsDebugLevel1 ) ;
}

void
_CSL_DebugRuntimeBreakpoint_IsDebugLevel2 ( ) // where we want the acquired pointer
{
    Compile_DebugRuntimeBreakpointFunction ( CSL_DebugRuntimeBreakpoint_IsDebugLevel2 ) ;
}

