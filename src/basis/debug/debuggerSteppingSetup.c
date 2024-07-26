#include "../../include/csl.h"
// simply : copy the current insn to a ByteArray buffer along with
// prefix and postfix instructions that restore and
// save the cpu state; then run that ByteArray code buffer
// control flow logic is not good here !?

void
Debugger_PreStartStepping ( Debugger * debugger )
{
    Word * word = debugger->w_Word ;
    if ( word )
    {
        uint64 * svDsp = _DspReg_ ;
        debugger->WordDsp = _DspReg_ ; // by 'eval' we stop debugger->Stepping and //continue thru this word as if we hadn't stepped
        // we would at least need to save/restore our registers to step thru native c code
        Debugger_CanWeStep ( debugger, word ) ;
        if ( ! GetState ( debugger, DBG_CAN_STEP ) )
        {
            if ( word->Definition == CSL_BlockRun )
            {
                _Debugger_SetupStepping ( debugger, 0, ( byte* ) DataStack_Pop ( ), 0 ) ;
                SetState ( debugger, DBG_NEWLINE | DBG_PROMPT | DBG_AUTO_MODE, false ) ;
            }
            else
            {
                iPrintf ( "\nStepping turned off for this word : %s%s%s%s : debugger->DebugAddress = 0x%016lx : (e)valuating",
                    c_ud ( word->S_ContainingNamespace ? word->S_ContainingNamespace->Name : ( byte* ) "<literal> " ),
                    word->S_ContainingNamespace ? ( byte* ) "." : ( byte* ) "", c_gu ( word->Name ),
                    GetState ( debugger, DBG_AUTO_MODE ) ? " : automode turned off" : "",
                    debugger->DebugAddress ) ;
                debugger->DebugAddress = 0 ;
                SetState ( debugger->w_Word, W_STEPPED, false ) ;
                SetState ( debugger, DBG_STEPPED, false ) ;
                _Debugger_Eval ( debugger, 0 ) ;
                SetState ( _Debugger_, DBG_AUTO_MODE, false ) ;
                SetState ( debugger, DBG_CANT_STEP_EVAL, true ) ;
            }
            return ;
        }
        else
        {
            if ( Word_IsSyntactic ( word ) ) //&& ( ! GetState ( word, STEPPED ) ) )
            {
                Interpreter * interp = _Interpreter_ ;
                interp->w_Word = word ;
                SetState ( _Debugger_, DBG_INFIX_PREFIX, true ) ;
                DebugOff ; // so we don't debug the args
                Interpreter_DoInfixOrPrefixWord ( interp, word ) ; // 
                DebugOn ; // so debug is on for Word_Eval and we can step thru it
                SetState ( _Debugger_, DBG_INFIX_PREFIX, false ) ;
                Word_DbgBlock_Eval ( word, word->Definition ) ;
                DEBUG_SHOW ( word, 0, 0 ) ;
                SetState ( word, W_STEPPED, true ) ;
                SetState ( debugger, ( DBG_STEPPED | DBG_STEPPING ), false ) ; // no longjmp needed at end of Interpreter_Loop
                _Set_DataStackPointers ( svDsp ) ;
                return ;
            }
            else
            {
                debugger->WordDsp = _DspReg_ ;
                Debugger_SetupStepping ( debugger ) ;
            }
            //SetState ( debugger, DBG_NEWLINE | DBG_PROMPT | DBG_INFO | DBG_AUTO_MODE, false ) ;
            SetState ( debugger, DBG_NEWLINE | DBG_PROMPT | DBG_AUTO_MODE, false ) ;
        }
    }
    else SetState_TrueFalse ( debugger, DBG_NEWLINE, DBG_STEPPING ) ;
    return ;
}

void
Debugger_AfterStep ( Debugger * debugger )
{
    debugger->LastRsp = debugger->cs_Cpu->Rsp ;
    if ( ( int64 ) debugger->DebugAddress ) // set by StepOneInstruction
    {
        debugger->SteppedWord = debugger->w_Word ;
        SetState_TrueFalse ( debugger, DBG_STEPPING, ( DBG_INFO | DBG_MENU | DBG_PROMPT ) ) ;
    }
    else SetState_TrueFalse ( debugger, DBG_PRE_DONE | DBG_STEPPED | DBG_NEWLINE | DBG_PROMPT | DBG_INFO, DBG_STEPPING ) ;
}

void
_Debugger_SetupStepping ( Debugger * debugger, Word * word, byte * address, byte * name )
{
    iPrintf ( "\nSetting up stepping : location = %s : debugger->word = \'%s\' : ...",
        c_gd ( _Context_Location ( _Context_ ) ), word ? word->Name : ( name ? name : ( byte* ) "" ) ) ;
    if ( word )
    {
        if ( GetState ( debugger, DBG_SOURCE ) ) _CSL_Source ( debugger->w_Word, 0 ), SetState ( debugger, DBG_SOURCE, false ) ;
        if ( ( ! address ) || ( ! GetState ( debugger, ( DBG_BRK_INIT | DBG_SETUP_ADDRESS ) ) ) ) address = ( byte* ) word->Definition ;
    }
    SetState_TrueFalse ( debugger, DBG_STEPPING, DBG_NEWLINE | DBG_PROMPT | DBG_INFO | DBG_MENU ) ;
    debugger->DebugAddress = address ;
    debugger->w_Word = Word_UnAlias ( word ) ;

    if ( ! GetState ( debugger, ( DBG_BRK_INIT | DBG_RUNTIME_BREAKPOINT ) ) ) SetState ( debugger->cs_Cpu, CPU_SAVED, false ) ;
    SetState ( _CSL_->cs_Cpu, CPU_SAVED, false ) ;
    _Debugger_CpuState_CheckSave ( debugger ) ;
    _CSL_CpuState_CheckSave ( ) ;
    debugger->LevelBitNamespaceMap = 0 ;
    SetState ( debugger, DBG_START_STEPPING, true ) ;
    _Debugger_->LastSourceCodeWord = 0 ;
    CSL_NewLine ( ) ;
}

void
Debugger_SetupStepping ( Debugger * debugger )
{
    _Debugger_SetupStepping ( debugger, debugger->w_Word, debugger->DebugAddress, 0 ) ;
}

int64
_Debugger_SetupReturnStackCopy ( Debugger * debugger, int64 size, Boolean showFlag )
{
    if ( Verbosity ( ) > 3 ) _CSL_PrintNReturnStack ( 4, 1 ) ;
    uint64 * rsp = ( uint64* ) debugger->cs_Cpu->Rsp ; //debugger->DebugESP [- 1] ; //debugger->cs_Cpu->Rsp [1] ; //debugger->cs_Cpu->Rsp ;
    if ( rsp )
    {
        uint64 rsc, rsc0 ;
        int64 pushedWindow = 64 ;
        if ( ! debugger->CopyRSP )
        {
            rsc0 = ( uint64 ) Mem_Allocate ( size, TEMPORARY ) ;
            rsc = ( rsc0 + 0xf ) & ( uint64 ) 0xfffffffffffffff0 ; // 16 byte alignment
            debugger->CopyRSP = ( byte* ) rsc + size - pushedWindow ;
            if ( showFlag ) ( _PrintNStackWindow ( ( uint64* ) debugger->CopyRSP, ( byte* ) "ReturnStackCopy", ( byte* ) "RSCP", 4 ) ) ;
        }
        else rsc = ( uint64 ) ( debugger->CopyRSP - size + pushedWindow ) ;
        MemCpy ( ( byte* ) rsc, ( ( byte* ) rsp ) - size + pushedWindow, size ) ; // pushedWindow (32) : account for useful current stack
        if ( showFlag ) ( _PrintNStackWindow ( ( uint64* ) rsp, ( byte* ) "ReturnStack", ( byte* ) "RSP", 4 ) ) ; //pushedWindow ) ) ;
        if ( showFlag ) ( _PrintNStackWindow ( ( uint64* ) debugger->CopyRSP, ( byte* ) "ReturnStackCopy", ( byte* ) "RSCP", 4 ) ) ; //pushedWindow ) ) ;
        debugger->cs_Cpu->Rsp = ( uint64* ) debugger->CopyRSP ;
        SetState ( debugger, DBG_STACK_OLD, false ) ;
        return true ;
    }

    else return false ;
}

void
Debugger_PrintReturnStackWindow ( )
{
    _PrintNStackWindow ( ( uint64* ) _Debugger_->cs_Cpu->Rsp, ( byte* ) "Debugger ReturnStack (RSP)", ( byte* ) "RSP", 4 ) ;
}

// restore the 'internal running csl' cpu state which was saved after the last instruction : debugger->cs_CpuState is the 'internal running csl' cpu state

void
Debugger_SetupReturnStackCopy ( Debugger * debugger, int64 showFlag ) // restore the running csl cpu state
{
    // restore the 'internal running csl' cpu state which was saved after the last instruction : debugger->cs_CpuState is the 'internal running csl' cpu state
    // we don't have to worry so much about the compiler 'spoiling' our insn with restore 
    int64 stackSetupFlag = 0 ;
    if ( ( ! debugger->CopyRSP ) || GetState ( debugger, DBG_STACK_OLD ) )
        stackSetupFlag = _Debugger_SetupReturnStackCopy ( debugger, 8 * K, showFlag ) ;
    if ( showFlag ) Compile_Call_TestRSP ( ( byte* ) _Debugger_CpuState_Show ) ; // also dis insn
}

void
_Compile_Restore_Debugger_CpuState ( Debugger * debugger, int64 showFlag ) // restore the running csl cpu state
{
    // restore the 'internal running csl' cpu state which was saved after the last instruction : debugger->cs_CpuState is the 'internal running csl' cpu state
    // we don't have to worry so much about the compiler 'spoiling' our insn with restore 
    Debugger_SetupReturnStackCopy ( debugger, showFlag ) ; // restore the running csl cpu state
    _Compile_CpuState_Restore ( debugger->cs_Cpu, 1 ) ;
    if ( showFlag ) Compile_Call_TestRSP ( ( byte* ) _Debugger_CpuState_Show ) ; // also dis insn
}

void
_Compile_Restore_C_CpuState ( CSL * csl, int64 showFlag )
{
    _Compile_CpuState_Restore ( csl->cs_Cpu, 1 ) ;
    if ( showFlag ) Compile_Call_TestRSP ( ( byte* ) CSL_CpuState_Show ) ;
}

// restore the 'internal running csl' cpu state which was saved after the last instruction : debugger->cs_CpuState is the 'internal running csl' cpu state

void
_Compile_Save_C_CpuState ( CSL * csl, int64 showFlag )
{
    Compile_CpuState_Save ( csl->cs_Cpu ) ;
    if ( showFlag ) Compile_Call_TestRSP ( ( byte* ) _CSL_CpuState_CheckSave ) ;
}

void
_Compile_Save_Debugger_CpuState ( Debugger * debugger, int64 showFlag )
{
    Compile_CpuState_Save ( debugger->cs_Cpu ) ;
    if ( showFlag ) Compile_Call_TestRSP ( ( byte* ) CSL_Debugger_UdisOneInsn ) ;
    if ( ( Verbosity ( ) > 3 ) && ( debugger->cs_Cpu->Rsp != (uint64*) debugger->LastRsp ) ) Debugger_PrintReturnStackWindow ( ) ;
    if ( showFlag ) Compile_Call_TestRSP ( ( byte* ) CSL_Debugger_CheckSaveCpuStateShow ) ;
}

void
Debug_ExtraShow ( int64 size, Boolean force )
{
    Debugger * debugger = _Debugger_ ;
    if ( force || ( Verbosity ( ) > 3 ) )
    {
        if ( force || ( Verbosity ( ) > 4 ) )
        {
            iPrintf ( "\n\ndebugger->SaveCpuState" ) ;
            _Debugger_Disassemble ( debugger, 0, ( byte* ) debugger->SaveCpuState, 1000, 1 ) ; //137, 1 ) ;
            iPrintf ( "\n\ndebugger->RestoreCpuState" ) ;
            _Debugger_Disassemble ( debugger, 0, ( byte* ) debugger->RestoreCpuState, 1000, 2 ) ; //100, 0 ) ;
        }
        iPrintf ( "\n\ndebugger->StepInstructionBA->BA_Data" ) ;
        _Debugger_Disassemble ( debugger, 0, ( byte* ) debugger->StepInstructionBA->BA_Data, size, 0 ) ;
    }
}

Word *
Debugger_GetWordFromAddress ( Debugger * debugger )
{
    Word * word = 0 ;
    if ( debugger->DebugAddress ) word = Word_GetFromCodeAddress ( debugger->DebugAddress ) ;
    if ( ( ! word ) && debugger->Token ) word = _Finder_Word_Find ( _Finder_, USING, debugger->Token ) ; //Finder_FindWord_UsedNamespaces ( _Finder_, debugger->Token ) ;
    if ( word ) debugger->w_Word = word = Word_UnAlias ( word ) ;
    return word ;
}

