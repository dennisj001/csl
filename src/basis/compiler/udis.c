
#include "../../include/csl.h"

void
_Udis_PrintInstruction ( ud_t * ud, byte * address, byte * prefix, byte * postfix )
{
    Debugger * debugger = _Debugger_ ;
    //                                      //prefix <addr>      <code hex>  <code disassembly> <call/jmp naming>
    //                                        prefix ud_insn_off ud_insn_hex ud_insn_asm  postfix
    //                                        "%s    0x%-12x     \t% -17s    %-15s        \t-30%s"
    byte * buffer = Buffer_DataCleared ( _CSL_->StringInsertB3 ) ;
    char *format = ( byte* ) "\n%s0x%-16lx% -24s%-25s%-30s", *formats = ( byte* ) "\n%s0x%-16lx% -24s%-40s%-30s" ;

    postfix = GetPostfix ( address, postfix, buffer ) ; // buffer is returned as postfix by GetPostfix
    if ( address != debugger->DebugAddress )
    {
        format = ( char* ) c_ud ( format ) ;
        formats = ( char* ) c_ud ( formats ) ;
    }
    else
    {
        format = ( byte* ) c_gd ( format ) ;
        formats = ( byte* ) c_gd ( formats ) ;
    }
    if ( GetState ( _Debugger_, DBG_STEPPING ) ) iPrintf ( formats, prefix, ( int64 ) ud_insn_off ( ud ), 
        ud_insn_hex ( ud ), c_gu ( ud_insn_asm ( ud ) ), c_gu ( postfix ) ) ;
    else iPrintf ( format, prefix, ( int64 ) ud_insn_off ( ud ), ud_insn_hex ( ud ), ud_insn_asm ( ud ), postfix ) ;
}

int64
_Udis_GetInstructionSize ( ud_t * ud, byte * address )
{
    ud_set_input_buffer ( ud, address, 16 ) ;
    ud_set_pc ( ud, ( int64 ) address ) ;
    int64 isize = ud_disassemble ( ud ) ;
    return isize ;
}

ud_t *
_Udis_Init ( ud_t * ud )
{
#if 1
    ud_init ( ud ) ;
    ud_set_mode ( ud, 64 ) ;
    ud_set_syntax ( ud, UD_SYN_INTEL ) ;
#else  
    //pud_init = ( void(* ) ( ud_t * ) ) _Dlsym ( "libudis86", "ud_init" ) ;
    //pud_init ( ud ) ;
    ( ( void(* ) ( ud_t * ) ) _Dlsym ( "ud_init", "/usr/local/lib/libudis86.so" ) )( ud ) ;
    ( ( void (* )( struct ud*, uint8_t ) ) _Dlsym ( "ud_set_mode", "/usr/local/lib/libudis86.so" ) ) ( ud, 32 ) ;
    ( ( void (* )( struct ud*, void (* )( struct ud* ) ) ) _Dlsym ( "ud_set_syntax", "/usr/local/lib/libudis86.so" ) ) ( ud, UD_SYN_INTEL ) ;
#endif   
    return ud ;
}

int64
Debugger_UdisOneInstructionWithSourceCode (Debugger * debugger, Word * word, byte * address, byte * prefix, byte * postfix )
{
    ud_t * ud = debugger->Udis ;
    if ( address && GetState ( _CSL_, DBG_UDIS|DBG_UDIS_ONE ) )
    {
        int64 isize ;
        ud_set_input_buffer ( ud, address, 16 ) ;
        ud_set_pc ( ud, ( int64 ) address ) ;
        isize = ud_disassemble ( ud ) ;
        SC_ShowSourceCode_In_Word_At_Address (word, address ) ;
        _Udis_PrintInstruction ( ud, address, prefix, postfix ) ; //, debugger->DebugAddress ) ;
        debugger->LastDisAddress = address ;
        return isize ;
    }
    return 0 ;
}

int64
_Udis_Disassemble (ud_t *ud, Word * word, byte* iaddress, int64 number, byte * prefix, byte * postfix, int64 cflag )
{
    int64 isize, size = 0 ;
    if ( ( ((int64) iaddress) > 0 ) && GetState ( _CSL_, DBG_UDIS|DBG_UDIS_ONE ) )
    {
        char * iasm ;
        byte * address = 0 ;
        ud_set_input_buffer ( ud, ( byte* ) iaddress, number ? number : 12 ) ;
        ud_set_pc ( ud, ( uint64 ) iaddress ) ;
        do
        {
            isize = ud_disassemble ( ud ) ;
            iasm = ( char* ) ud_insn_asm ( ud ) ;
            address = ( byte* ) ( uint64 ) ud_insn_off ( ud ) ;
            //if ( word ) 
            SC_ShowSourceCode_In_Word_At_Address (word, address ) ;
            _Udis_PrintInstruction ( ud, address, ( byte* ) "", ( byte* ) "" ) ;
            if ( ( cflag && String_Equal ( ( byte* ) "ret", ( byte* ) iasm ) ) ) //|| String_Equal ( ( byte* ) "invalid", ( byte* ) iasm ) )
            {
                address ++ ;
                cflag -- ;
                if ( cflag <= 0 ) break ;
            }
            else if ( String_Equal ( ( byte* ) "invalid", ( byte* ) iasm ) ) break ;
            if ( ! number ) number = isize ;
            number -= isize ;
        }
        while ( ( isize && ( number > 0 ) ) ) ;
        size = address - iaddress + 1 ; // 1 : include last iaddress 
    }
    return (( size > 0 ) ? size : 0 ) ; 
}
