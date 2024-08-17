
#include "../../include/csl.h"
#define IS_CALL_INSN(address) ( ( * address == CALL32 ) || ( ( ( * ( uint16* ) address ) == 0xff49 ) && ( *( address + 2 ) == 0xd1 ) ) )

void
Debugger_StepOneInstruction ( Debugger * debugger )
{
    debugger->SaveTOS = TOS ;
    debugger->SaveStackDepth = DataStack_Depth ( ) ;
    DefaultColors ; // nb. : so text output will be in default colors
    ( ( VoidFunction ) debugger->StepInstructionBA->BA_Data ) ( ) ;
    AdjustR14WithDsp ( ) ;
    DebugColors ;
}

void
Debugger_COI_Append_Insn ( Debugger * debugger )
{
    Debugger_Udis_GetInstructionSize ( debugger ) ;
    ByteArray_AppendCopy ( debugger->StepInstructionBA, debugger->InsnSize, debugger->DebugAddress ) ;
}

// Debugger_CompileOneInstruction ::
// this function should not affect the C registers at all 
// we save them before we call our stuff and restore them after

void
Debugger_CompileOneInstruction ( Debugger * debugger, Boolean showFlag )
{
    ByteArray * svcs = Get_CompilerSpace ( ) ;
    _ByteArray_Init ( debugger->StepInstructionBA ) ; // we are only compiling one insn here so clear our BA before each use
    //byte * disasmStart = debugger->StepInstructionBA->StartIndex ;
    Set_CompilerSpace ( debugger->StepInstructionBA ) ; // now compile to this space
    _Compile_Save_C_CpuState ( _CSL_, showFlag > 1 ) ; //&& ( Verbosity () >= 3 ) ) ; // save our c compiler cpu register state
    _Compile_Restore_Debugger_CpuState ( debugger, showFlag > 1 ) ; //&& ( Verbosity () >= 3 ) ) ; // restore our runtime state before the current insn
    Debugger_COI_Append_Insn ( debugger ) ;
    _Compile_Save_Debugger_CpuState ( debugger, showFlag > 1 ) ; //showRegsFlag ) ; //&& ( Verbosity () >= 3 ) ) ; // save our runtime state after the instruction : which we will restore before the next insn
    _Compile_Restore_C_CpuState ( _CSL_, showFlag > 1 ) ; //&& ( Verbosity () >= 3 ) ) ; // save our c compiler cpu register state
    _Compile_Return ( ) ;
    Set_CompilerSpace ( svcs ) ; // restore compiler space pointer before "do it" in case "do it" calls the compiler
    //_Debugger_Disassemble ( _Debugger_, 0, ( byte* ) disasmStart, 1024, 1 ) ;
    debugger->DebugAddress += debugger->InsnSize ;
}

void
Debugger_CompileAndStepOneInstruction ( Debugger * debugger )
{
    if ( debugger->DebugAddress )
    {
        Boolean showExtraFlag = _CSL_->DebugLevel > 1 ; //true ; //false ;
        byte * svHere = Here ; // for Debug_ExtraShow 
        Debugger_CompileOneInstruction ( debugger, showExtraFlag ) ; // compile the insn here
        Debugger_StepOneInstruction ( debugger ) ;
        if ( showExtraFlag ) Debug_ExtraShow ( Here - svHere, 0 ) ; //showExtraFlag ) ;
        if ( GetState ( debugger, DBG_AUTO_MODE ) && ( ! GetState ( debugger, DBG_CONTINUE_MODE ) ) )
            SetState ( debugger, DBG_SHOW_STACK_CHANGE, false ) ;
        else
        {
            int64 codeSize = debugger->PreHere ? Here - debugger->PreHere : 0 ;
            if ( codeSize ) _Debugger_ShowEffects ( debugger, debugger->w_Word, 1, 0, showExtraFlag ) ;
            else Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
        }
        if ( Compiling ) _Debugger_DisassembleWrittenCode ( debugger ) ;
    }
}

void
Debugger_StepLoop ( Debugger * debugger )
{
    SetState ( debugger, ( DBG_UDIS ), false ) ; // turn off disassembly
    Word * word ;
    while ( debugger->DebugAddress )
    {
        word = Word_GetFromCodeAddress ( debugger->DebugAddress ) ;
        if ( word )
        {
            if ( ( word == debugger->OutWord ) && ( ( * debugger->DebugAddress ) == _RET ) && ( ! Stack_Depth ( debugger->ReturnStack ) ) )
            {
                iPrintf ( "\n ... returning from word : %s at 0x%-16lx",
                    ( word ? ( char* ) c_gd ( word->Name ) : ( char* ) "<dbg>" ), debugger->DebugAddress ) ;
                debugger->OutWord = 0 ;
                Debugger_Step ( debugger ) ;
                break ;
            }
            Debugger_Step ( debugger ) ;
        }
        else
        {
            Debugger_ReturnStack_DepthZero ( debugger ) ; // not perfect but works
            break ;
        }
    }
    SetState ( debugger, ( DBG_UDIS ), true ) ;
}

void
Do_JcAddress ( byte * jcAddress )
{
    Debugger * debugger = _Debugger_ ;
    if ( jcAddress )
    {
        debugger->DebugAddress = jcAddress ;
    }
    else
    {
        debugger->DebugAddress += debugger->InsnSize ;
    }
}

Boolean
Debugger_CheckSkipDebugOrCallThruWord ( Debugger * debugger, byte * jcAddress )
{
    Word *word, * word0 = Word_GetFromCodeAddress ( jcAddress ) ;
    word = Word_UnAlias ( word0 ) ;
    if ( word && String_Equal ( word->Name, "<dbg>" ) )
    {
        //Debugger_UdisOneInstruction ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
        SetState ( debugger, ( DBG_AUTO_MODE | DBG_AUTO_MODE_ONCE | DBG_CONTINUE_MODE ), false ) ;
        debugger->Key = 's' ; //turn off autorepeat
        iPrintf ( "\nskipping over a debug word : %s : at 0x%-16lx", "<dbg>", debugger->DebugAddress ) ;
        debugger->DebugAddress += debugger->InsnSize ;
        return 4 ;
    }
    else if ( ( word && ( ! Debugger_CanWeStep ( debugger, word ) ) ) || ( word && ( word->W_MorphismAttributes & ( DEBUG_WORD | RT_STEPPING_DEBUG ) ) ) )
    {
        if ( word->W_MorphismAttributes & ( RT_STEPPING_DEBUG ) )
            SetState_TrueFalse ( _CSL_, DBG_UDIS_ONE, ( DBG_AUTO_MODE | DBG_INTERPRET_LOOP_DONE ) ) ;
        // we are already stepping here and now, so skip
        if ( word->W_MorphismAttributes & ( DEBUG_WORD | RT_STEPPING_DEBUG ) )
        {
            Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
            if ( GetState ( _CSL_, DBG_UDIS ) ) iPrintf ( "\nskipping over a debug word : %s : at 0x%-16lx",
                ( word ? ( char* ) c_gd ( word->Name ) : ( char* ) "<dbg>" ), debugger->DebugAddress ) ;
            debugger->DebugAddress += debugger->InsnSize ; // 3 : sizeof call reg insn
            return 4 ;
        }
        else
        {
            if ( word->W_MorphismAttributes & ( CSL_ASM_WORD ) )
            {
                debugger->DebugAddress = jcAddress ;
                _Stack_Push ( debugger->ReturnStack, ( int64 ) debugger->ReturnAddress ) ; // the return address
            }
            else
            {
                if ( Is_DebugShowOn ) iPrintf ( "\ncalling thru and over a C word : %s : at 0x%-16lx", 
                    ( word ? ( char* ) c_gd ( word->Name ) : ( char* ) "<dbg>" ), jcAddress ) ;
                _Block_Eval ( word->Definition ) ;
                AdjustR14WithDsp ( ) ;
                //CSL_PrintDataStack () ; 
                debugger->DebugAddress += debugger->InsnSize ; //debugger->InsnSize ; // 3 : sizeof call reg insn
                if ( GetState ( debugger, DBG_EVAL_MODE ) ) SetState ( debugger, ( DBG_CONTINUE_MODE ), false ) ;
            }
            return 4 ;
        }
    }
    else
    {
        //Debugger_UdisOneInstruction ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
        debugger->DebugAddress = jcAddress ;
        debugger->w_Word = word ;
        if ( Is_DebugShowOn && GetState ( debugger, ( DBG_UDIS ) ) ) iPrintf ( "\n ... calling word : %s at 0x%-16lx",
            ( word ? ( char* ) c_gd ( word->Name ) : ( char* ) "<dbg>" ), debugger->DebugAddress ) ;
        _Stack_Push ( debugger->ReturnStack, ( int64 ) debugger->ReturnAddress ) ; // the return address
        return 0 ;
    }
}

void
Debugger_StepInstructionType ( Debugger * debugger )
{
    byte *jcAddress = 0, *dadr = debugger->DebugAddress, updateFlag = 0 ;
    if ( dadr )
    {
        Debugger_Udis_GetInstructionSize ( debugger ) ;
        if ( ( * dadr ) == _RET )
        {
#if 0         
            SetState ( debugger, ( DBG_AUTO_MODE | DBG_CONTINUE_MODE ), false ) ;
            debugger->Key = 's' ; //turn off autorepeat
#endif          
            debugger->Insn = _RET ;
            updateFlag = Debugger_CASOI_Do_Return_Insn ( debugger ) ;
            return ;
        }
        else if ( ( * dadr == JMP32 ) || ( * dadr == JMP8 ) )
        {
            if ( * dadr == JMP32 ) debugger->Insn = JMP32 ;
            else if ( * dadr == JMP8 ) debugger->Insn = JMP8 ;
            jcAddress = JumpCallInstructionAddress ( dadr ) ;
            Do_JcAddress ( jcAddress ) ;
            //Debugger_CheckSkipDebugOrCallThruWord ( debugger, jcAddress ) ;
            return ;
        }
        else if ( * dadr == CALL32 )
        {
            debugger->Insn = CALL32 ;
            debugger->ReturnAddress = dadr + debugger->InsnSize ;
            debugger->DebugAddress = JumpCallInstructionAddress ( dadr ) ;
            Debugger_CheckSkipDebugOrCallThruWord ( debugger, debugger->DebugAddress ) ;
            return ;
        }
        else if ( ( ( ( * ( uint16* ) dadr ) == 0xff49 ) && ( ( *( dadr + 2 ) == 0xd2 ) || ( *( dadr + 2 ) == 0xd3 ) ) ) ) //CALL/JMP reg : MOD_RM
        {
            debugger->Insn = CALL_REG ;
            debugger->ReturnAddress = dadr + debugger->InsnSize ;
            if ( ( *( dadr + 2 ) == 0xd2 ) ) jcAddress = ( byte* ) debugger->cs_Cpu->R10d ;
            else if ( ( *( dadr + 2 ) == 0xd3 ) ) jcAddress = ( byte* ) debugger->cs_Cpu->R11d ;
            else jcAddress = JumpCallInstructionAddress_X64ABI ( dadr ) ;
            Debugger_CheckSkipDebugOrCallThruWord ( debugger, jcAddress ) ;
            return ; 
        }
        else if ( ( * ( dadr ) >> 4 ) == 0x7 ) // JCC8
        {
            debugger->Insn = JCC8 ;
            jcAddress = Debugger_DoJcc ( debugger ) ;
            Do_JcAddress ( jcAddress ) ;
            return ;
        }
        else if ( * dadr == 0x0f )
        {
            if ( ( ( uint32 ) ( * ( uint32 * ) dadr ) ) == NOOP4 )
            {
                debugger->DebugAddress += debugger->InsnSize ;
                return ;
            }
            else if ( ( ( * ( dadr + 1 ) >> 4 ) == 0x8 ) ) // JCC32
            {
                debugger->Insn = JCC32 ;
                jcAddress = Debugger_DoJcc ( debugger ) ;
                Do_JcAddress ( jcAddress ) ;
                return ;
            }
            return ;
        }
        else
        {
            Debugger_CompileAndStepOneInstruction ( debugger ) ;
        }
    }
}

void
Debugger_ReturnStack_DepthZero ( Debugger * debugger )
{
    //if ( GetState ( debugger, ( DBG_AUTO_MODE | DBG_CONTINUE_MODE ) ) )
    //    Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
    //SetState ( debugger, DBG_STACK_OLD, true ) ;
    debugger->CopyRSP = 0 ;
    if ( GetState ( debugger, DBG_BRK_INIT ) ) SetState_TrueFalse ( debugger, DBG_STACK_OLD | DBG_INTERPRET_LOOP_DONE | DBG_STEPPED,
        DBG_ACTIVE | DBG_BRK_INIT | DBG_STEPPING | DBG_SETUP_ADDRESS ) ;
    else SetState_TrueFalse ( debugger, DBG_STACK_OLD | DBG_INTERPRET_LOOP_DONE | DBG_STEPPED,
        DBG_ACTIVE | DBG_STEPPING | DBG_SETUP_ADDRESS ) ;
    debugger->DebugAddress = 0 ;
    SetState ( debugger->cs_Cpu, CPU_SAVED, false ) ;
    //SetState ( debugger, DBG_SETUP_ADDRESS, false ) ;
}

int64
Debugger_CASOI_Do_Return_Insn ( Debugger * debugger )
{
    int64 rtrn = 0, sd ;
    //int64 cws = Debugger_CanWeStep ( debugger, debugger->w_Word ) ;
    if ( ( sd = Stack_Depth ( debugger->ReturnStack ) ) ) //&& cws ) // GetState ( debugger, DBG_CAN_STEP ) ) ) //&& ( ! GetState ( debugger, DBG_BRK_INIT|DBG_STEPPING ) ) )
    {
        //_Debugger_Disassemble ( _Debugger_, 0, ( byte* ) debugger->DebugAddress, 0, 1 ) ;
        if ( Verbosity ( ) > 3 ) CSL_PrintReturnStack ( ) ;
        //_Debugger_ShowInfo ( debugger, "ret:", 0, 1 ) ;
        //Debugger_UdisOneInstruction ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
        debugger->DebugAddress = ( byte* ) Stack_Pop ( debugger->ReturnStack ) ;
        rtrn = 4 ;
    }
    else
    {
        Debugger_ReturnStack_DepthZero ( debugger ) ;
        rtrn = 4 ;
    }
#if 0    
    if ( GetState ( debugger, DBG_SHOW ) && ( byte* ) debugger->SaveDsp != ( byte* ) _DspReg_ )
    {
        iPrintf ( "\ndebugger->SaveDsp != _DspReg_" ) ;
        CSL_PrintDataStack ( ) ;
        iPrintf ( "\n_Debugger_->cs_Cpu->R14d = %lx : _Dsp_Reg_ = %lx : debugger->SaveDsp = %lx", _Debugger_->cs_Cpu->R14d, _DspReg_, debugger->SaveDsp ) ;
        _DspReg_ = debugger->SaveDsp ; //??
    }
#endif    
    return rtrn ;
}

void
Debugger_Step ( Debugger * debugger )
{
    if ( kbhit ( ) )
    {
        SetState_TrueFalse ( debugger, DBG_MENU, DBG_CONTINUE_MODE ) ; //Pause ( ) ;
        Debugger_InterpreterLoop ( debugger ) ;
    }
    if ( ! GetState ( debugger, DBG_STEPPING ) ) Debugger_PreStartStepping ( debugger ) ;
    else
    {
        SetState ( debugger, DBG_CAN_STEP, false ) ;
        Debugger_StepInstructionType ( debugger ) ;
        Debugger_AfterStep ( debugger ) ;
    }
}

int64
Debugger_CanWeStep ( Debugger * debugger, Word * word )
{
    int64 result = true ;
    if ( ! word ) result = false ;
    else if ( ! word->CodeStart ) result = false ;
    else if ( ! NamedByteArray_CheckAddress ( _O_CodeSpace, word->CodeStart ) ) result = false ;
    else if ( word->W_MorphismAttributes & ( CSL_ASM_WORD | CPRIMITIVE | DLSYM_WORD | C_PREFIX_RTL_ARGS ) ) result = false ;
    else if ( word->W_MorphismAttributes & ( CSL_WORD ) ) result = true ;
    SetState ( debugger, DBG_CAN_STEP, result ) ;
    return result ;
}

void
Debugger_Stepping_Off ( Debugger * debugger )
{
    if ( Debugger_IsStepping ( debugger ) )
    {
        Debugger_SetStepping ( debugger, false ) ;
        debugger->DebugAddress = 0 ;
    }
}

byte *
Debugger_DoJcc ( Debugger * debugger )
{
    int64 numOfBytes = ( debugger->InsnSize == 6 ) ? 2 : 1 ; //( debugger->InsnSize == 4 ) ? 2 : ( debugger->InsnSize == 2 ) ? 1 : 0 ;
    byte * jcAddress = ( numOfBytes == 2 ) ? JccInstructionAddress_2Byte ( debugger->DebugAddress ) :
        JccInstructionAddress_1Byte ( debugger->DebugAddress ) ;
    int64 tttn, ttt, n ;
    tttn = ( numOfBytes == 2 ) ? ( debugger->DebugAddress[1] & 0xf ) : ( debugger->DebugAddress[0] & 0xf ) ;
    ttt = ( tttn & 0xe ) >> 1 ;
    n = tttn & 1 ;

    // cf. Intel Software Developers Manual v1. (253665), Appendix B
    // ?? nb. some of this may not be (thoroughly) tested as of 11-14-2011 -- but no known problems after months of usual testing ??
    // setting jcAddress to 0 means we don't branch and just continue with the next instruction
    if ( ttt == TTT_BELOW ) // ttt 001
    {
        if ( ( n == 0 ) && ! ( ( uint64 ) debugger->cs_Cpu->RFlags & CARRY_FLAG ) ) jcAddress = 0 ;
        else if ( ( n == 1 ) && ( ( uint64 ) debugger->cs_Cpu->RFlags & CARRY_FLAG ) ) jcAddress = 0 ;
    }
    else if ( ttt == TTT_ZERO ) // ttt 010
    {
        if ( ( n == 0 ) && ! ( ( uint64 ) debugger->cs_Cpu->RFlags & ZERO_FLAG ) ) jcAddress = 0 ;
        else if ( ( n == 1 ) && ( ( uint64 ) debugger->cs_Cpu->RFlags & ZERO_FLAG ) ) jcAddress = 0 ;
    }
    else if ( ttt == TTT_BE ) // ttt 011 :  below or equal
        // the below code needs to be rewritten :: re. '|' and '^' :: TODO ??
    {
        if ( ( n == 0 ) && ! ( ( ( uint64 ) debugger->cs_Cpu->RFlags & CARRY_FLAG ) | ( ( uint64 ) debugger->cs_Cpu->RFlags & ZERO_FLAG ) ) )
        {
            jcAddress = 0 ;
        }
        else if ( ( n == 1 ) && ( ( ( uint64 ) debugger->cs_Cpu->RFlags & CARRY_FLAG ) | ( ( uint64 ) debugger->cs_Cpu->RFlags & ZERO_FLAG ) ) )
        {
            jcAddress = 0 ;
        }
    }
    else if ( ttt == TTT_LESS ) // ttt 110
    {
        if ( ( n == 0 ) && ! ( ( ( uint64 ) debugger->cs_Cpu->RFlags & SIGN_FLAG ) ^ ( ( uint64 ) debugger->cs_Cpu->RFlags & OVERFLOW_FLAG ) ) )
        {
            jcAddress = 0 ;
        }
        else if ( ( n == 1 ) && ( ( ( uint64 ) debugger->cs_Cpu->RFlags & SIGN_FLAG )
            ^ ( ( uint64 ) debugger->cs_Cpu->RFlags & OVERFLOW_FLAG ) ) )
        {
            jcAddress = 0 ;
        }
    }
    else if ( ttt == TTT_LE ) // ttt 111
    {
        if ( ( n == 0 ) &&
            ! ( ( ( ( uint64 ) debugger->cs_Cpu->RFlags & SIGN_FLAG )
            ^ ( ( uint64 ) debugger->cs_Cpu->RFlags & OVERFLOW_FLAG ) ) | ( ( uint64 ) debugger->cs_Cpu->RFlags & ZERO_FLAG ) ) )
        {
            jcAddress = 0 ;
        }
        if ( ( n == 1 ) &&
            ( ( ( ( uint64 ) debugger->cs_Cpu->RFlags & SIGN_FLAG )
            ^ ( ( uint64 ) debugger->cs_Cpu->RFlags & OVERFLOW_FLAG ) ) | ( ( uint64 ) debugger->cs_Cpu->RFlags & ZERO_FLAG ) ) )
        {

            jcAddress = 0 ;
        }
    }
    return jcAddress ;
}

#if 0

void
Debugger_DoJccType ( Debugger * debugger, byte * jcAddress )
{
    if ( ( ! debugger->ReturnAddress ) && Stack_Depth ( debugger->ReturnStack ) )
        debugger->ReturnAddress = ( byte* ) Stack_Top ( debugger->ReturnStack ) ;
    Word * word = Word_UnAlias ( Word_GetFromCodeAddress ( jcAddress ) ) ;
    if ( IS_CALL_INSN ( debugger->DebugAddress ) ) _Word_ShowSourceCode ( word ) ;
    if ( ( ! word ) || ( ! Debugger_CanWeStep ( debugger, word ) ) )
    {
        if ( GetState ( debugger, DBG_SHOW ) ) if ( Is_DebugShowOn ) iPrintf ( "\ncalling thru - a non-native (C) subroutine : %s : .... :> %s ",
                word ? ( char* ) c_gd ( word->Name ) : "", Context_Location ( ) ) ;
        // here we attempt to solve the problem encountered when we are stepping thru code that 
        // calls words that are then also calling the debugger ... ; eg. interpreter.csl 'doWord'
        uint64 svState = debugger->State ;
        sigjmp_buf svJmpBuf ;
        memcpy ( svJmpBuf, _Context_->JmpBuf0, sizeof (sigjmp_buf ) ) ;
        byte * svDebugAddress = debugger->DebugAddress + debugger->InsnSize ;
        Debugger_CompileAndStepOneInstruction ( debugger ) ;
        debugger->DebugAddress = svDebugAddress ; // can be reset by Debugger_CompileAndStepOneInstruction
        debugger->State = svState ; // it can be turned off if this word involves debugger eg. 'doWord'
        memcpy ( _Context_->JmpBuf0, svJmpBuf, sizeof (sigjmp_buf ) ) ;
        // and thus we can continue stepping ; nb! something is not quite right yet??

    }
    else Debugger_COI_StepKey ( debugger, word ) ;
}

void
_Debugger_COI_StepInto ( Debugger * debugger, Word * word )
{
    while ( word->W_MorphismAttributes & ( ALIAS ) ) word = word->W_AliasOf ;
    if ( ( * debugger->DebugAddress == CALL32 ) || ( ( ( * ( uint16* ) debugger->DebugAddress ) == 0xff49 )//CALL/JMP reg : MOD_RM
        && ( *( debugger->DebugAddress + 2 ) == 0xd2 ) ) ) // call r10
    {
        if ( GetState ( debugger, DBG_SHOW ) && GetState ( _CSL_, DBG_UDIS ) )
            iPrintf ( "\nstepping into a csl compiled function : %s : .... :> %s ",
            word ? ( char* ) c_gd ( word->Name ) : "", Context_Location ( ) ) ;
        _Debugger_Disassemble ( _Debugger_, 0, ( byte* ) debugger->DebugAddress, 0, 1 ) ;
    }
    Debugger_CompileAndStepOneInstruction ( debugger ) ;
}

void
Debugger_COI_StepKey ( Debugger * debugger, Word * word )
{
    if ( ( ( debugger->Key == 'o' ) || ( debugger->Key == 'h' ) ) ) //|| ( debugger->Key == 'c' ) || ( debugger->Key == 'z' ) ) && ( debugger->ReturnAddress ) )
    {
        debugger->Key = 0 ;
        Debugger_StepLoop ( debugger ) ;
    }
    else _Debugger_COI_StepInto ( debugger, word ) ;
}
#endif

