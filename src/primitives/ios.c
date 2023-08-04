
#include "../include/csl.h"

void
Fflush ( )
{
    fflush ( stdout ) ;
}

void
CSL_Kbhit ( void )
{
    DataStack_Push (( int64 ) kbhit ( )) ;
}

void
CSL_PrintString ( )
{
    _CSL_PrintString ( ( byte* ) DataStack_Pop ( ) ) ;
}

void
CSL_NewLine ( )
{
    CSL_PrintChar ( '\n' ) ;
}

void
CSL_CarriageReturn ( )
{
    CSL_PrintChar ( '\r' ) ;
}

void
CSL_SPACE ( ) // '.'
{
    CSL_PrintChar ( ' ' ) ;
}

void
CSL_TAB ( ) // '.'
{
    CSL_PrintChar ( '\t' ) ;
}

void
_ConvertToBinary ( uint64 n, byte* buffer, int16 size )
{
    uint64 i ; // 8 - bits/byte ; 4 - spacing
    byte * ptr ;
    for ( i = 0, ptr = & buffer [ size - 2 ] ; i < ( CELL_SIZE * 8 ) ; ptr -- )
    {
        if ( n & ( ( ( uint64 ) 1 ) << i ) )
        {
            *ptr = '1' ;
        }
        else
        {
            *ptr = '0' ;
        }
        i ++ ;
        if ( ! ( i % 4 ) ) ptr -- ;
        if ( ! ( i % 8 ) ) ptr -- ;
        if ( ! ( i % 16 ) ) ptr -- ;
        if ( ! ( i % 32 ) ) ptr -- ;
    }
}

byte *
_Print_Binary ( uint64 n )
{
    byte *ptr ;
    if ( n )
    {
        int16 i, size = 128 ;
        byte *buffer = Buffer_New_pbyte ( size ) ; //[ size ] ; // 8 - bits/byte ; 4 - spacing
        //buffer [ size - 1 ] = 0 ;
        for ( i = 0 ; i < size ; i ++ ) buffer [ i ] = ' ' ; // fill buffer with spaces
        _ConvertToBinary ( n, buffer, size ) ;
        for ( ptr = & buffer[0], i = 0 ; i < size ; ptr ++ )
        {
            if ( ( *ptr == 0 ) ) break ;
            if ( ( * ptr == '1' ) )
            {
                // go back to 8 bit boundary
                while ( *-- ptr != ' ' ) ;
                // go back to 16 bit boundary
                while ( *-- ptr != ' ' ) ;
                ptr += 2 ;
                break ;
            }
        }
        return ptr ;
    }
    else return ( byte* ) "" ;
}

void
Print_Binary ( uint64 n )
{
    oPrintf ( "\n %s", _Print_Binary ( n ) ) ;
}

void
PrintfInt ( int64 n )
{
    byte * buffer = Buffer_DataCleared ( _CSL_->ScratchB1 ) ;
    if ( NUMBER_BASE_GET() == 10 ) sprintf ( ( char* ) buffer, INT_FRMT, n ) ;
    else if ( NUMBER_BASE_GET() == 2 )
    {
        Print_Binary ( n ) ;
        return ;
    }
    else /* if ( _Context->System0->NumberBase == 16 ) */ sprintf ( ( char* ) buffer, UINT_FRMT_0x016, n ) ; // hex
    // ?? any and all other number bases ??
    oPrintf ( buffer ) ;
}

void
CSL_PrintInt ( )
{
    PrintfInt ( DataStack_Pop ( ) ) ;
}

void
CSL_HexPrintInt ( )
{
    int64 svb = NUMBER_BASE_GET() ;
    NUMBER_BASE_SET ( 16 ) ;
    PrintfInt ( DataStack_Pop ( ) ) ;
    NUMBER_BASE_SET ( svb ) ;
}

void
CSL_Emit ( )
{
    int64 c = DataStack_Pop ( ) ;
    if ( ( c >= 0 ) && ( c < 256 ) ) CSL_PrintChar ( c ) ;
    else CSL_PrintChar ( c ) ; //_Printf ( "%c", ( ( CString ) c )[0] ) ;
}

void
CSL_Key ( )
{
#if 0    
    ReadLine_Get_Key ( _Context_->ReadLiner0 ) ;
    DataStack_Push (_Context_->ReadLiner0->InputKeyedCharacter) ;
#else
    DataStack_Push (Key ( )) ;
#endif    
}

void
CSL_LogOn ( )
{
    _O_->LogFlag = true ;
    if ( ! _CSL_->LogFILE ) _CSL_->LogFILE = fopen ( ( char* ) "csl.log", "w" ) ;
}

void
CSL_PrintBufferCopy ( )
{
    byte *buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ; //[ size ] ; // 8 - bits/byte ; 4 - spacing
    strncpy ( buffer, _O_->PrintBufferCopy->B_Data, BUFFER_SIZE ) ;
    DataStack_Push ( (int64) buffer ) ;
}


byte *
_CSL_PrintBufferCopy ( )
{
    return _O_->PrintBufferCopy->B_Data ;
}

void
CSL_PrintBufferClear ( )
{
    Buffer_DataCleared (_O_->PrintBufferCopy ) ; //[0] = 0 ;
    Buffer_DataCleared (_O_->PrintBufferConcatCopy ) ; //[0] = 0 ;
    //_O_->PrintBufferCopy->B_Data [0] = 0 ;
}

void
CSL_DebugOutputOn ( )
{
    _O_->DebugOutputFlag |= 2 ;
}
void
CSL_DebugOutputOff ( )
{
    _O_->DebugOutputFlag &= ~2 ;
}

void
CSL_DebugOutputConcatOn ( )
{
    _O_->DebugOutputFlag |= 4 ;
}
void
CSL_DebugOutputConcatOff ( )
{
    _O_->DebugOutputFlag &= ~6 ;
}

void
CSL_LogAppend ( )
{
    byte * logFilename = ( byte* ) DataStack_Pop ( ) ;
    _CSL_->LogFILE = fopen ( ( char* ) logFilename, "a" ) ;
    CSL_LogOn ( ) ;
}

void
CSL_LogWrite ( )
{
    byte * logFilename = ( byte* ) DataStack_Pop ( ) ;
    _CSL_->LogFILE = fopen ( ( char* ) logFilename, "w" ) ;
    CSL_LogOn ( ) ;
}

void
CSL_LogOff ( )
{
    CSL * csl = _CSL_ ;
    if ( csl )
    {
        fflush ( csl->LogFILE ) ;
        if ( csl->LogFILE ) fclose ( csl->LogFILE ) ; // ? not needed  ?
        csl->LogFILE = 0 ;
        _O_->LogFlag = false ;
    }
}

