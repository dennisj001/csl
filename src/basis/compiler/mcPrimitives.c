
#include "../../include/csl.h"

void
Compile_WordRun ( )
{
    _Compile_Stack_PopToReg ( DSP, RAX ) ;
    //Compile_Move_Rm_To_Reg ( RAX, RAX, 0xa8, 0 ) ; // 0xa8 : Definition offset in Word structure in version 0.902.860
    Compile_Move_Rm_To_Reg ( RAX, RAX, 0x88, 0 ) ; // 0x88 : Definition offset in Word structure in version 0.918.550
    _Compile_Group5 ( CALL, REG, RAX, 0, 0, 0 ) ;
}

Word *
_CSL_MachineCodePrimitive_NewAdd ( const char * name, uint64 morphismAttributes, int64 objectAttributes, block * callHook, byte * function, int64 functionArg )
{
    Word * word = _DObject_New ( ( byte* ) name, ( uint64 ) function, ( morphismAttributes ), objectAttributes, 0, 0, function, functionArg, 0, 0, DICTIONARY ) ;
    if ( callHook ) *callHook = word->Definition ;
    return word ;
}

void
CSL_MachineCodePrimitive_NewAdd ( const char * name, uint64 morphismAttributes, int64 objectAttributes, 
    block * callHook, byte * function, int64 functionArg, const char *nameSpace, const char * superNamespace )
{
    Word * word = _CSL_MachineCodePrimitive_NewAdd ( name, morphismAttributes, objectAttributes, callHook, function, functionArg ) ;
    _CSL_InitialAddWordToNamespace ( word, ( byte* ) nameSpace, ( byte* ) superNamespace ) ;
}

void
CSL_MachineCodePrimitive_AddWords ( CSL * csl )
{
    Debugger * debugger = _Debugger_ ;
    // this form (below) can and should replace the loop because we need to have variables for some elements
    CSL_MachineCodePrimitive_NewAdd ( "call_ToAddressThruSREG_TestAlignRSP", CSL_ASM_WORD, 0, & csl->Call_ToAddressThruSREG_TestAlignRSP, ( byte* ) Compile_Call_ToAddressThruSREG_TestAlignRSP, - 1, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "restoreCpuState", CSL_ASM_WORD, 0, & debugger->RestoreCpuState, ( byte* ) Compile_CpuState_Restore, ( int64 ) debugger->cs_Cpu, "Debug", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "saveCpuState", CSL_ASM_WORD, 0, & debugger->SaveCpuState, ( byte* ) Compile_CpuState_Save, ( int64 ) debugger->cs_Cpu, "Debug", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "restoreCpuState", CSL_ASM_WORD, 0, & csl->RestoreCpuState, ( byte* ) Compile_CpuState_Restore, ( int64 ) csl->cs_Cpu, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "saveCpuState", CSL_ASM_WORD, 0, & csl->SaveCpuState, ( byte* ) Compile_CpuState_Save, ( int64 ) csl->cs_Cpu, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "restoreCpu2State", CSL_ASM_WORD, 0, & csl->RestoreCpu2State, ( byte* ) Compile_CpuState_Restore, ( int64 ) csl->cs_Cpu2, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "saveCpu2State", CSL_ASM_WORD, 0, & csl->SaveCpu2State, ( byte* ) Compile_CpuState_Save, ( int64 ) csl->cs_Cpu2, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "wrun", CSL_ASM_WORD, 0, & csl->WordRun, ( byte* ) Compile_WordRun, ( int64 ) - 1, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "<dbg>", CSL_ASM_WORD | RT_STEPPING_DEBUG, 0, 0, ( byte* ) _CSL_DebugRuntimeBreakpoint, - 1, "Debug", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "<dso>", CSL_ASM_WORD | RT_STEPPING_DEBUG, 0, 0, ( byte* ) _CSL_DebugRuntimeBreakpoint_IsDebugShowOn, - 1, "Debug", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "<d:dbg>", CSL_ASM_WORD | RT_STEPPING_DEBUG, 0, 0, ( byte* ) _CSL_DebugRuntimeBreakpoint_IsDebugOn, - 1, "Debug", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "<d1:dbg>", CSL_ASM_WORD | RT_STEPPING_DEBUG, 0, 0, ( byte* ) _CSL_DebugRuntimeBreakpoint_IsDebugLevel1, - 1, "Debug", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "<d2:dbg>", CSL_ASM_WORD | RT_STEPPING_DEBUG, 0, 0, ( byte* ) _CSL_DebugRuntimeBreakpoint_IsDebugLevel2, - 1, "Debug", "Root" ) ;
#if TCO    
    DBI_ON ;
    CSL_MachineCodePrimitive_NewAdd ( "lc_EvalList_restoreCpuState", CSL_WORD | CSL_ASM_WORD, 0, & csl->LC_Apply_RestoreCpuState, ( byte* ) Compile_CpuState_Restore, ( int64 ) csl->LC_Apply_Cpu, "System", "Root" ) ;
    DBI_OFF ;
    CSL_MachineCodePrimitive_NewAdd ( "lc_EvalList_saveCpuState", CSL_WORD | CSL_ASM_WORD, 0, & csl->LC_Apply_SaveCpuState, ( byte* ) Compile_CpuState_Save, ( int64 ) csl->LC_Apply_Cpu, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "lc_EvalList_restoreCpuState", CSL_WORD | CSL_ASM_WORD, 0, & csl->LC_EvalList_RestoreCpuState, ( byte* ) Compile_CpuState_Restore, ( int64 ) csl->LC_EvalList_Cpu, "System", "Root" ) ;
    CSL_MachineCodePrimitive_NewAdd ( "lc_EvalList_saveCpuState", CSL_WORD | CSL_ASM_WORD, 0, & csl->LC_EvalList_SaveCpuState, ( byte* ) Compile_CpuState_Save, ( int64 ) csl->LC_EvalList_Cpu, "System", "Root" ) ;
#endif    
}

