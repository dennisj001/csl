
#include "../../include/csl.h"
#if DEBUG
void
dbg ( int64 index, int64 one, int64 two )
{
#if 0    
    static int64 svLen = 0 ;

    void
    dbgp ( Word * ans, Word * scWord, int64 len )
    {
        _Namespace_PrintWords ( ans, scWord, 1 ) ;
        oPrintf ( "\nList_Length = %d", len ) ;
        if ( len == 2 ) Pause ( ) ;
        oPrintf ( "\n%s", Context_Location ( ) ) ;
    }

    if ( Is_DebugOn )
    {
        int64 len = 0 ;
        Namespace * ans = ( Word * ) 0x7ffff7b96ed8 ;
        Word *scWord = ( Word * ) 0x7ffff7b958e0 ;
        if ( ( len = List_Length ( ans->W_List ) ) == 2 )
        {
            if ( svLen == 3 ) dbgp ( ans, scWord, len ) ;
        }
        svLen = len ;
        if ( len == 3 ) dbgp ( ans, scWord, len ) ;
    }
    //if ( ns == (Word *)0x7ffff7b96d30)
    //    Pause () ;
#elif 0
    int64 sd = Stack_Depth ( _DataStack_ ) ; //Stack_Depth ( _CONTEXT_TDI_STACK ) ;
    return sd ;
#elif 0
    Context * cntx = _Context_ ;
    w = Compiler_PreviousNonDebugWord ( 0 ) ; // 0 : rem: we just popped the WordStack above
    oPrintf ( "\nword = Compiler_PreviousNonDebugWord ( 0 ) :: %s.%s", w->ContainingNamespace->Name, w->Name ) ;
    oPrintf ( "\nQualifyingNamespace = %s.%s", cntx->Finder0->QualifyingNamespace->ContainingNamespace->Name, cntx->Finder0->QualifyingNamespace->Name ) ;
    oPrintf ( "\nBaseObject = %s.%s", cntx->BaseObject->ContainingNamespace->Name, cntx->BaseObject->Name ) ;
    w = Compiler_PreviousNonDebugWord ( 1 ) ; // 0 : rem: we just popped the WordStack above
    oPrintf ( "\nword = Compiler_PreviousNonDebugWord ( 1 ) :: %s.%s", w->ContainingNamespace->Name, w->Name ) ;
    oPrintf ( "\nword = %s.%s", w->ContainingNamespace->Name, w->Name ) ;
    oPrintf ( "\nQualifyingNamespace = %s.%s", cntx->Finder0->QualifyingNamespace->ContainingNamespace->Name, cntx->Finder0->QualifyingNamespace->Name ) ;
    oPrintf ( "\nBaseObject = %s.%s", cntx->BaseObject->ContainingNamespace->Name, cntx->BaseObject->Name ) ;
#elif 1
    switch ( index )
    {
        case 1:
        {
            int64 endIndex = ( int64 ) ( ( ByteArray * ) ( 0x7ffff7a93000 ) )->EndIndex ;
            //oPrintf ( "\n%lx", endIndex ) ;
            if ( endIndex < 0 )
            {
                oPrintf ( "\n%lx", endIndex ) ;
                Pause ( ) ;
            }
            break ;
        }
        case 2:
        {
            Word * w = ( Word * ) one ;
            if ( w )
            {
                if ( String_Equal ( w->Name, "CodeSpace" ) )
                {
                    //oPrintf ( "\n%s", w->Name ) ;
                    ByteArray * ba = ( ByteArray * ) two ;
                    byte * endIndex = ba->EndIndex ;
                    //oPrintf ( "\n%lx", endIndex ) ;
                    if ( ( int64 ) ba->EndIndex < 0 )
                    {
                        oPrintf ( "\n%lx", endIndex ) ;
                        Pause ( ) ;
                    }
                }
            }
            break ;
        }
        case 3:
        {

            Word * w = ( Word * ) one ;
            if ( w )
            {
                if ( w->W_PtrToValue == ( uint64* ) 0x7fffecb7c2b1 )
                {
                    oPrintf ( "\n%lx", w->W_PtrToValue ) ;
                    Pause ( ) ;
                }
            }
            break ;
        }
        case 0:
        default:
        {
            Word * w = ( Word * ) one ;
            if ( w && Is_DebugOn )
            {
                Word * word = w ;
                iPrintf ( "\nword : \'%s\' : %s.%s", word->Name, word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "", word->Name ) ;
                if ( word->TypeNamespace ) iPrintf ( "\nword : \'%s\' : TypeNamespace = %s.%s", word->Name, word->TypeNamespace->ContainingNamespace->Name, word->TypeNamespace->Name ) ;
                else iPrintf ( "\nword : \'%s\' : TypeNamespace = 0", word->Name ) ;
                //Pause () ;
            }
            break ;
        }
        case 4:
        {
            Word * w = ( Word * ) one ;
            static Namespace * svClassNs ;
            oPrintf ( "\n%s : size = %ld", w->Name, two ) ;
            if ( ( ! two ) || String_Equal ( w->Name, "," ) ) Pause ( ) ;
            if ( svClassNs == w ) Pause ( ) ;
            svClassNs = w ;

        }
        case 5:
        {
            if ( Is_DebugOn )
            {
                TDI * tdi = ( TDI * ) one ;
                oPrintf ( "\nat %s : namespace name = %s : size = %ld", Context_Location ( ), tdi->Tdi_StructureUnion_Namespace ? tdi->Tdi_StructureUnion_Namespace->Name : ( byte* ) "", tdi->Tdi_StructureUnion_Size ) ;
            }
        }
    }
#endif    
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

