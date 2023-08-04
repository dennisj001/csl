#include "../../include/csl.h"

void
CSL_GetCpuState ( )
{
    Debugger * debugger = _Debugger_ ;
    ByteArray * svcs = Get_CompilerSpace ( ) ;
    _ByteArray_Init ( debugger->StepInstructionBA ) ; 
    Set_CompilerSpace ( debugger->StepInstructionBA ) ; // now compile to this space
    _Compile_Save_C_CpuState ( _CSL_, 0 ) ; 
    _Compile_Return ( ) ;
    Set_CompilerSpace ( svcs ) ; // restore compiler space pointer before "do it" in case "do it" calls the compiler
    ( ( VoidFunction ) debugger->StepInstructionBA->BA_Data ) ( ) ;
}

void
_CpuState_Show ( Cpu * cpu )
{
    byte * location = Context_Location ( ) ;
    if ( cpu == _Debugger_->cs_Cpu ) iPrintf ( "\nDebugger CpuState : at %s", location ) ;
    else if ( cpu == _CSL_->cs_Cpu2 ) iPrintf ( "\nCurrent CpuState : at %s", location ) ;
    else iPrintf ( "\nC Runtime (CSL) CpuState : at %s", location ) ;
    iPrintf ( "\nRAX " UINT_FRMT_0x016, cpu->Rax ) ;
    iPrintf ( " RCX " UINT_FRMT_0x016, cpu->Rcx ) ;
    iPrintf ( " RDX " UINT_FRMT_0x016, cpu->Rdx ) ;
    iPrintf ( " RBX " UINT_FRMT_0x016, cpu->Rbx ) ;
    iPrintf ( "\nRSP " UINT_FRMT_0x016, cpu->Rsp ) ;
    iPrintf ( " RBP " UINT_FRMT_0x016, cpu->Rbp ) ;
    iPrintf ( " RSI " UINT_FRMT_0x016, cpu->Rsi ) ;
    iPrintf ( " RDI " UINT_FRMT_0x016, cpu->Rdi ) ;
    iPrintf ( "\nR8  " UINT_FRMT_0x016, cpu->R8d ) ;
    iPrintf ( " R9  " UINT_FRMT_0x016, cpu->R9d ) ;
    iPrintf ( " R10 " UINT_FRMT_0x016, cpu->R10d ) ;
    iPrintf ( " R11 " UINT_FRMT_0x016, cpu->R11d ) ;
    iPrintf ( "\nR12 " UINT_FRMT_0x016, cpu->R12d ) ;
    iPrintf ( " R13 " UINT_FRMT_0x016, cpu->R13d ) ;
    iPrintf ( " R14 " UINT_FRMT_0x016, cpu->R14d ) ;
    iPrintf ( " R15 " UINT_FRMT_0x016, cpu->R15d ) ;
    if ( _Debugger_->DebugAddress ) iPrintf ( "\nRIP " UINT_FRMT_0x016, _Debugger_->DebugAddress ) ;
    else iPrintf ( "\nRIP " UINT_FRMT_0x016, cpu->Rip ) ;
    iPrintf ( " RFlags :: 0x%08x :: %s", cpu->RFlags, _Print_Binary ( ( uint64 ) cpu->RFlags ) ) ;
    //_Printf ( (byte*) "_Printf_Binary ( ( uint64 ) cpu->RFlags ) ; //, 14, 14 ) ;
    iPrintf ( " :: of:11:%d sf:7:%d, zf:6:%d, af:4:%d, pf:2:%d, cf:0:%d :: flag:bit:value",
        ( uint64 ) cpu->RFlags & OVERFLOW_FLAG ? 1 : 0, ( uint64 ) cpu->RFlags & SIGN_FLAG ? 1 : 0, ( uint64 ) cpu->RFlags & ZERO_FLAG ? 1 : 0,
        ( uint64 ) cpu->RFlags & AUX_FLAG ? 1 : 0, ( uint64 ) cpu->RFlags & PARITY_FLAG ? 1 : 0, ( uint64 ) cpu->RFlags & CARRY_FLAG ? 1 : 0
        ) ;
    d0 ( _PrintNStackWindow ( ( int64* ) cpu->Rsp, "ReturnStack", "ESP", 8 ) ) ;
}

// save the incoming current cpu register state to the C struct
// we are mainly concerned here about what goes into the C struct
// but we have to be careful not to negatively affect the C compiler 
// runtime in the process

void
_Compile_CpuState_SaveSelected ( Cpu * cpu )
{
    // push order for pushad
    // nb : intel stacks grow down toward lesser memory by 
    // subtracting from ESP to push and adding to pop
    // SoftwareDevelopersManual-253665.pdf, section 6.2
    // registers are pushed in this order ...
    // eax, ecx, edx, ebx, esp, ebp, esi, edi
    _Compile_PushReg ( RDX ) ;
    //_Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->CPU_OREG, OREG, ACC ) ; // save operand reg thru accum

    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rsp, RSP, RDX ) ; // Rsp //this won't be accurate for the runtime because it is called from C 
    //_Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rbp, RBP, OREG ) ; // edx
    //_Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rbx, RBX, OREG ) ; // ebx
    //_Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R10d, R10D, OREG ) ; // edx
    //_Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R11d, R11D, OREG ) ;// ebx
    //_Compile_Get_FromCAddress_ToReg ( OREG, ( byte* ) & cpu->CPU_OREG ) ; // our scratch reg
    _Compile_PopToReg ( RDX ) ; // restore rax
}

void
_Compile_CpuState_RestoreSelected ( Cpu * cpu )
{
    _Compile_PushReg ( RDX ) ;
    _Compile_Get_FromCAddress_ToReg_ThruReg ( RSP, ( byte * ) & cpu->Rsp, RDX ) ; // r12, r13, rsp, rbp : same problem
    //_Compile_Get_FromCAddress_ToReg_ThruReg ( RBP, ( byte * ) & cpu->Rbp, OREG ) ; // r12, r13, rsp, rbp : same problem
    //_Compile_Get_FromCAddress_ToReg ( RBX, ( byte* ) & cpu->Rbx ) ; // rbx
    //_Compile_Get_FromCAddress_ToReg ( R10D, ( byte* ) & cpu->R10d ) ; // r10d
    //_Compile_Get_FromCAddress_ToReg ( R11D, ( byte* ) & cpu->R11d ) ; // r11d
    _Compile_PopToReg ( RDX ) ; // restore rax
}

void
Compile_CpuState_Save ( Cpu * cpu )
{
    // push order for pushad
    // nb : intel stacks grow down toward lesser memory by 
    // subtracting from ESP to push and adding to pop
    // SoftwareDevelopersManual-253665.pdf, section 6.2
    // registers are pushed in this order ...
    // eax, ecx, edx, ebx, esp, ebp, esi, edi
    _Compile_PushReg ( ACC ) ;
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rdx, RDX, ACC ) ; // save operand reg thru accum
    _Compile_PopToReg ( ACC ) ; // restore rax

    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rax, RAX, RDX ) ; // eax
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rcx, RCX, RDX ) ; // edx
    //_Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rdx, RDX, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rbx, RBX, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rsp, RSP, RDX ) ; // Rsp //this won't be accurate for the runtime because it is called from C 
    cpu->Rsp = ( uint64* ) ( ( uint64 ) cpu->Rsp & ( uint64 ) 0xfffffffffffffff0 ) ;
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rbp, RBP, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rsi, RSI, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->Rdi, RDI, RDX ) ; // edx

    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R8d, R8D, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R9d, R9D, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R10d, R10D, RDX ) ; // edx
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R11d, R11D, RDX ) ; // ebx

    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R12d, R12D, RDX ) ; // esp //this won't be accurate for the runtime because it is called from C 
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R13d, R13D, RDX ) ; // ebp
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R14d, R14D, RDX ) ; // esi
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->R15d, R15D, RDX ) ; // edi

    //_Compile_MoveImm_To_Reg ( RAX, 0, CELL_SIZE ) ;
    _Compile_PushFD ( ) ; // save flags
    _Compile_PopToReg ( ACC ) ; // ebx now has eflags
    //_Compile_MoveImm_To_Reg ( RBX, 0xfffbffff, 4 ) ; // alignment bit
    //_Compile_Int8 ( 0x40 ) ;
    //Compile_AND ( REG, REG, RAX, RBX, 0, 0, CELL_SIZE ) ;
    _Compile_Set_CAddress_WithRegValue_ThruReg ( ( byte* ) & cpu->RFlags, ACC, RDX ) ; //flags
    _Compile_GetRValue_FromLValue_ToReg ( ACC, ( byte* ) & cpu->Rax ) ; // our scratch reg

    //_Compile_Set_C_LValue_WithImm_ThruReg ( ( int64 ) & cpu->State, CPU_STATE_SAVED, R9D, CELL ) ; // mark this CpuState as having been saved
    // restore our scratch regs so we leave things as we found them
    _Compile_Set_C_LValue_WithImm_ThruReg ( ( byte* ) & cpu->State, CPU_SAVED, RDX, CELL ) ;
    _Compile_GetRValue_FromLValue_ToReg ( RDX, ( byte* ) & cpu->Rdx ) ; // our scratch reg
}

// we have the cpu register state stored in a C struct 
// accurately restore that struct into the cpu register state 
// so that the cpu register state is as saved in the C struct when we leave

void
_Compile_CpuState_Restore ( Cpu * cpu, int64 cStackRegFlag )
{
    _Compile_GetRValue_FromLValue_ToReg ( RAX, ( byte* ) & cpu->Rax ) ; // rax
    //_Compile_GetRValue_FromLValue_ToReg ( RDX, ( byte* ) & cpu->Rdx ) ; // rdx
    _Compile_GetRValue_FromLValue_ToReg ( RCX, ( byte* ) & cpu->Rcx ) ; // rcx
    _Compile_GetRValue_FromLValue_ToReg ( RBX, ( byte* ) & cpu->Rbx ) ; // rbx
    //_Compile_GetRValue_FromLValue_ToReg ( RDX, ( byte* ) & cpu->CPU_OREG ) ; // restore thru - scratch reg

    if ( cStackRegFlag )
    {
        cpu->Rsp = ( uint64* ) ( ( uint64 ) cpu->Rsp & ( uint64 ) 0xfffffffffffffff0 ) ;
        _Compile_Get_FromCAddress_ToReg_ThruReg ( RSP, ( byte * ) & cpu->Rsp, RDX ) ; // r12, r13, rsp, rbp : same problem
        _Compile_Get_FromCAddress_ToReg_ThruReg ( RBP, ( byte * ) & cpu->Rbp, RDX ) ; // r12, r13, rsp, rbp : same problem
    }

    _Compile_GetRValue_FromLValue_ToReg ( RSI, ( byte* ) & cpu->Rsi ) ; // rsi
    _Compile_GetRValue_FromLValue_ToReg ( RDI, ( byte* ) & cpu->Rdi ) ; // rdi
    _Compile_GetRValue_FromLValue_ToReg ( R8D, ( byte* ) & cpu->R8d ) ; // r8d
    _Compile_GetRValue_FromLValue_ToReg ( R9D, ( byte* ) & cpu->R9d ) ; // r9d
    _Compile_GetRValue_FromLValue_ToReg ( R10D, ( byte* ) & cpu->R10d ) ; // r10d
    _Compile_GetRValue_FromLValue_ToReg ( R11D, ( byte* ) & cpu->R11d ) ; // r11d
    _Compile_Get_FromCAddress_ToReg_ThruReg ( R12D, ( byte* ) & cpu->R12d, RDX ) ; // r12, r13, rsp, rbp : same problem
    _Compile_Get_FromCAddress_ToReg_ThruReg ( R13D, ( byte* ) & cpu->R13d, RDX ) ; // r12, r13, rsp, rbp : same problem
    _Compile_GetRValue_FromLValue_ToReg ( R14D, ( byte* ) & cpu->R14d ) ; // r14d
    //if ( cpu->R15d == 0 ) cpu->R15d = cpu->R14d ;
    _Compile_GetRValue_FromLValue_ToReg ( R15D, ( byte* ) & cpu->R15d ) ; // r15d

    _Compile_GetRValue_FromLValue_ToReg ( ACC, ( byte* ) & cpu->RFlags ) ;
    //_Compile_MoveImm_To_Reg ( RBX, 0x00003ffff, 8 ) ; // turn off alignment bit
    //_Compile_Int8 ( 0x40 ) ;
    //Compile_AND ( REG, REG, RAX, RBX, 0, 0, CELL_SIZE ) ;
    _Compile_PushReg ( ACC ) ; // the flags
    _Compile_PopFD ( ) ; // pops the pushed flags in ebx to flags reg           // eflags
    //_Compile_Get_FromCAddress_ToReg ( RBX, ( byte* ) & cpu->Rbx ) ; // restore scratch reg rax
    _Compile_GetRValue_FromLValue_ToReg ( ACC, ( byte* ) & cpu->Rax ) ; // restore scratch reg rax

    _Compile_GetRValue_FromLValue_ToReg ( RDX, ( byte* ) & cpu->Rdx ) ; // restore thru - scratch reg
}

void
Compile_CpuState_Restore ( Cpu * cpu )
{
    _Compile_CpuState_Restore ( cpu, 1 ) ;
}

Cpu *
_CpuState_Copy ( Cpu *dst, Cpu * src )
{
    MemCpy ( dst, src, sizeof ( Cpu ) ) ;
    return dst ;
}

Cpu *
CpuState_Copy ( Cpu * cpu0, uint64 type )
{
    Cpu * cpu = CpuState_New ( type ) ;
    _CpuState_Copy ( cpu, cpu0 ) ;
    return cpu ;
}

Cpu *
CpuState_New ( uint64 type )
{
    Cpu * cpu ;
    cpu = ( Cpu * ) Mem_Allocate ( sizeof (Cpu ), type ) ;
    return cpu ;
}

Cpu *
CpuState_SaveCopyCurrent ( Cpu * cpu )
{
    Cpu * newCpu = CpuState_New ( TEMPORARY ) ;
    _CSL_->SaveCpu2State ( ) ;
    _CpuState_Copy ( newCpu, cpu ) ;
    return newCpu ;
}

Cpu *
CSL_CpuState_SaveCopyCurrent ( )
{
    return CpuState_SaveCopyCurrent ( _CSL_->cs_Cpu ) ;
}

Cpu *
Debugger_CpuState_SaveCopyCurrent ( )
{
    return CpuState_SaveCopyCurrent ( _Debugger_->cs_Cpu ) ;
}

void
_CpuState_Restore ( Cpu * cpu )
{
    Cpu *svCpu = _CSL_->cs_Cpu ; //CpuState_Copy ( CSL->cs_Cpu, TEMPORARY ) ;
    _CSL_->cs_Cpu = cpu ;
    _CSL_->RestoreCpuState ( ) ;
    _CSL_->cs_Cpu = svCpu ;
}

void
CpuState_Save ( )
{
    Cpu *newCpu = CSL_CpuState_SaveCopyCurrent ( ) ;
    DataStack_Push (( int64 ) newCpu) ;
}

void
CpuState_Restore ( )
{
    Cpu *cpu = ( Cpu * ) DataStack_Pop ( ) ;
    _CpuState_Restore ( cpu ) ;
}

char *
Get_Register_Name ( int64 reg )
{
    char * registerNames [ 16 ] = { ( char* ) "RAX", ( char* ) "RCX", ( char* ) "RDX", ( char* ) "RBX",
        ( char* ) "RBP", ( char* ) "RSP", ( char* ) "RSI", ( char* ) "RDI", ( char* ) "R8", ( char* ) "R9",
        ( char* ) "R10", ( char* ) "R11", ( char* ) "R12", ( char* ) "R13", ( char* ) "R14", ( char* ) "R15" } ;
    if ( reg < 16 ) return registerNames [reg] ;
    else return "" ;
}
