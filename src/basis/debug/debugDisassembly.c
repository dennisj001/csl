
#include "../../include/csl.h"

ud_t *
Debugger_UdisInit ( Debugger * debugger )
{
    if ( ! debugger->Udis ) debugger->Udis = ( ud_t* ) Mem_Allocate ( sizeof (ud_t ), T_CSL ) ;
    return _Udis_Init ( debugger->Udis ) ;
}

ud_t *
Debugger_UdisNew ( Debugger * debugger )
{
    debugger->Udis = ( ud_t* ) Mem_Allocate ( sizeof (ud_t ), T_CSL ) ;
    return _Udis_Init ( debugger->Udis ) ;
}

void
Debugger_Udis_GetInstructionSize ( Debugger * debugger )
{
    debugger->InsnSize = _Udis_GetInstructionSize ( debugger->Udis, debugger->DebugAddress ) ;
}

int64
_Debugger_Disassemble ( Debugger * debugger, Word * word, byte* address, int64 number, int64 cflag )
{
    int64 size = 0 ;
    if ( GetState ( _CSL_, DBG_UDIS|DBG_UDIS_ONE ) )
    {
        ud_t * udis = Debugger_UdisInit ( debugger ) ;
        size = _Udis_Disassemble (udis, word, address, ( number > ( 3 * K ) ) ? ( 3 * K ) : number, cflag ) ;
        debugger->LastDisAddress = address ;
        SetState ( _CSL_, DBG_UDIS_ONE, false ) ;
        return size ; 
    }
}

void
Udis1Insn ( )
{
    byte * address = ( byte* ) DataStack_Pop ( ) ;
    Debugger_UdisOneInstructionWithSourceCode ( _Debugger_, 0, address, "", "" ) ; //byte * prefix, byte * postfix )
}

void
Udis1InsnX ( )
{
    Debugger * debugger = _Debugger_ ;
    int64 svu = GetState ( _CSL_, DBG_UDIS|DBG_UDIS_ONE ) ;
    SetState ( _CSL_, DBG_UDIS|DBG_UDIS_ONE, true ) ;
    Udis1Insn ( ) ;
    SetState ( _CSL_, DBG_UDIS|DBG_UDIS_ONE, svu ) ;
}

void
Debugger_Disassemble ( Debugger * debugger, byte* address, int64 number, int64 cflag )
{
    CSL_NewLine ( ) ;
    int64 size = _Debugger_Disassemble ( debugger, 0, address, number, cflag ) ;
    iPrintf ( ( byte * ) "\n%d bytes disassembled", size ) ;
}

void
Debugger_Dis ( Debugger * debugger )
{
    Debugger_GetWordFromAddress ( debugger ) ;
    Word * word = debugger->w_Word ;
    if ( ( word ) && ( word->S_CodeSize ) )
    {
        iPrintf ( "\nDisassembly of : %s.%s", c_ud ( word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "" ), c_gd ( word->Name ) ) ;
        int64 codeSize = word->S_CodeSize ;
        SetState ( debugger, DBG_DISASM_ACC, true ) ;
        _Debugger_Disassemble ( debugger, word, ( byte* ) word->CodeStart, codeSize ? codeSize : 64, ( word->W_MorphismAttributes & ( CPRIMITIVE | DLSYM_WORD | DEBUG_WORD ) ? 1 : 0 ) ) ;
        SetState ( debugger, DBG_DISASM_ACC, false ) ;
#if 0        
        if ( debugger->DebugAddress )
        {
            iPrintf ( "\nNext instruction ..." ) ;
            Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "\n", ( byte* ) "" ) ; // the next instruction
        }
#endif        
    }
    else
    {
        //word = _Context_->CurrentlyRunningWord ;
        if ( word ) iPrintf ( "\nDisassembly of : %s.%s : has no code size! Disassembling accumulated ...", c_ud ( word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "" ), c_gd ( word->Name ) ) ;
        Debugger_DisassembleAccumulated ( debugger ) ;
        //SetState ( debugger, DBG_DISASM_ACC, false ) ;
        //Debugger_DisassembleTotalAccumulated ( debugger ) ;
    }
}

// a function of PreHere, OptimizedCodeAffected FirstDisAddress

void
_Debugger_DisassembleWrittenCode ( Debugger * debugger )
{
    Word * word = debugger->w_Word ;
    int64 codeSize = debugger->PreHere ? Here - debugger->PreHere : 0 ;
    byte * compiledToAddr = ( codeSize > 0 ) ? debugger->PreHere : Here ;
    if ( word )
    {
        if ( codeSize > 0 )
        {
            NamedByteArray * nba = Get_CompilerSpace ( )->OurNBA ;
            byte * csName = nba ? ( byte * ) c_gd ( nba->NBA_Name ) : ( byte* ) "" ;
            if ( GetState ( _CSL_, DBG_UDIS ) ) iPrintf ( "\nCode compiled to %s for word :> %s <: %4d bytes : at 0x%lx at %s", csName,
                c_gn ( String_CB ( word->Name ) ), codeSize, compiledToAddr, Context_Location ( ) ) ;
            _Debugger_Disassemble ( debugger, word, compiledToAddr, codeSize,
                ( word->W_MorphismAttributes & ( CPRIMITIVE | DLSYM_WORD | DEBUG_WORD ) ? 1 : 0 ) ) ;
        }
        else Debugger_DisassembleAccumulated ( debugger ) ;
        debugger->PreHere = Here ;
    }
}

// this needs work -- some of these options are not necessary ?! or useful at all

void
Debugger_DisassembleAccumulated ( Debugger * debugger )
{
    int64 size ;
    if ( size = Here - debugger->StartHere ) //((debugger->PreHere < debugger->StartHere) ? debugger->PreHere : debugger->StartHere ) ;
    {
        _Debugger_->EntryWord = _Context_->CurrentWordBeingCompiled ;
        if ( debugger->EntryWord ) iPrintf ( ( byte * ) "\nDisassembling %d bytes of code accumulated since start within word \'%s\' at : 0x%016lx ...",
            size, ( char* ) debugger->EntryWord->Name, debugger->StartHere ) ;
        SetState ( debugger, DBG_DISASM_ACC, true ) ;
        if ( size > 0 ) _Debugger_Disassemble ( debugger, debugger->EntryWord, debugger->StartHere, size, 0 ) ;
        SetState ( debugger, DBG_DISASM_ACC, false ) ;
    }
}

void
Debugger_DisassembleTotalAccumulated ( Debugger * debugger )
{
    byte * address = _Context_->Compiler0->InitHere ;
    Word * word = _Context_->CurrentWordBeingCompiled ;
    if ( ! ( Here - address ) )
    {
        address = ( byte* ) ( _CSL_->LastFinished_Word ? _CSL_->LastFinished_Word->Definition : _CSL_->LastFinished_DObject->Definition ) ;
    }
    int64 size = Here - address ;
    size = ( size > 0 ) ? size : 128 ;
    int64 svState = GetState ( _CSL_, GLOBAL_SOURCE_CODE_MODE ) ;
    SetState ( _CSL_, GLOBAL_SOURCE_CODE_MODE, true ) ;
    SetState ( debugger, DBG_DISASM_ACC, true ) ;
    iPrintf ( "\nDisassembling the current word : \'%s\' : %4d bytes : total accumulated code ...", word ? word->Name : ( byte* ) "", size ) ;
    Debugger_Disassemble ( debugger, address, size, ! Compiling ) ;
    SetState ( _CSL_, GLOBAL_SOURCE_CODE_MODE, svState ) ;
    SetState ( debugger, DBG_DISASM_ACC, false ) ;
}

