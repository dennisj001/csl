
#include "../../include/csl.h"

int64
GetTerminalWidth ( )
{
#ifdef TIOCGSIZE
    struct ttysize ts ;
    ioctl ( STDIN_FILENO, TIOCGSIZE, &ts ) ;
    //cols = ts.ts_cols ;
    //lines = ts.ts_lines;
    return ts.ts_cols ;
#elif defined(TIOCGWINSZ)
    struct winsize ts ;
    //fd = open("/dev/tty", O_RDWR);
    //if(fd < 0 || ioctl(fd, TIOCGWINSZ, &ts) < 0) Error ("/dev/tty", 0);
    //close(fd) ;
    //ioctl ( STDIN_FILENO, TIOCGWINSZ, &ts ) ;
    ioctl ( STDOUT_FILENO, TIOCGWINSZ, &ts ) ;
    //cols = ts.ws_col ;
    //lines = ts.ws_row;
    return ts.ws_col ;
#endif /* TIOCGSIZE */
}

char
_kbhit ( int64 key )
{
    int64 oldf ;
    oldf = fcntl ( STDIN_FILENO, F_GETFL, 0 ) ;
    fcntl ( STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK ) ;
    char ch = getc (stdin) ;
    fcntl ( STDIN_FILENO, F_SETFL, oldf ) ;
    if ( key == CHAR_PRINT ) return ( ch >= ' ' ) ;
    else if ( key == CHAR_ANY ) return ( ch ) ;
    else return (ch == key) ; //return ch ;
}

char
kbhit ( void )
{
    return _kbhit ( ESC ) ;
}

#define KEY() getc ( stdin )

void
Clear_Terminal_Line ( )
{
    // try, try, again!?
    fflush (stdout) ;
    iPrintf ( "\r%c[J", ESC ) ; // clear from cursor to end of screen -- important if we have (mistakenly) gone up an extra line
    fflush (stdout) ;
    iPrintf ( "\r%c[J", ESC ) ; // clear from cursor to end of screen -- important if we have (mistakenly) gone up an extra line
    fflush (stdout) ;
}

void
_CursorUp ( int n )
{
    iPrintf ( ( byte* ) "%c[%dA", ESC, n ) ;
}

int64
_Key ( FILE * f )
{
    int64 key = fgetc ( f ) ; // GetC () ;
    //if ( _O_->LogFlag && ( isalnum ( key ) || ispunct ( key ) ) ) CSL_LogChar ( key ) ;
    if ( _O_->LogFlag && ( isgraph ( key ) || isspace ( key ) ) ) CSL_LogChar ( key ) ;
    return key ;
}

void
Kbhit_Pause ( )
{
    if ( kbhit ( ) ) OpenVmTil_Pause ( ) ;
}

int64
Key_Kbhit ( FILE * f )
{
    int64 key = _Key ( f ) ;
    if ( _O_ && ( ! _O_->SigSegvs ) && ( ! GetState ( _Debugger_, DBG_STEPPING ) ) ) Kbhit_Pause ( ) ;
    return key ;
}

int64
Key ( )
{
    return Key_Kbhit ( stdin ) ;
}

byte
_CSL_Key ( ReadLiner * rl )
{
    int key = _Key ( rl->InputFile ) ;
    return ( byte ) key ;
}

void
_CSL_PrintString ( byte * string ) //  '."'
{
    oPrintf ( "%s", string ) ;
    //fflush ( stdout ) ;
}

void
CSL_PrintChar ( byte c ) //  '."'
{
    oPrintf ( "%c", c ) ;
}

void
CSL_LogChar ( byte c ) //  '."'
{
    Printf_Log ( "%c", c ) ;
    //fflush ( stdout ) ;
}

void
Emit ( byte c )
{
    CSL_PrintChar ( c ) ;
}

void
Context_DoPrompt ( Context * cntx )
{
    byte lc = _ReadLiner_->InputKeyedCharacter ;
    Boolean se = String_Equal ( &_O_->Pbf8[0], cntx->ReadLiner0->NormalPrompt ), lcnl = ( lc == '\n' ), lcbnl = _O_->Pblc == '\n' ;

    if ( ( ! _O_->Pbf8[0] ) || ( ! lcnl ) || ( ( ! se ) && ( ! lcbnl ) && ( _O_->Pbf8[0] != '\r' ) ) ) CSL_PrintChar ( '\n' ) ;
    if ( ( ! se ) || ( ( ! lcbnl ) && ( ! csl_returnValue ) ) ) iPrintf ( "%s", ( char* ) cntx->ReadLiner0->NormalPrompt ) ;
    else csl_returnValue = 0 ;
}

// all output comes thru here

void
_Printf ( char *format, ... )
{
    va_list args ;
    va_start ( args, ( char* ) format ) ;
    vprintf ( ( char* ) format, args ) ;
    va_end ( args ) ;
    fflush ( stdout ) ;
    if ( _O_->LogFlag )
    {
        va_start ( args, ( char* ) format ) ;
        vfprintf ( _CSL_->LogFILE, ( char* ) format, args ) ;
        va_end ( args ) ;
        fflush ( _CSL_->LogFILE ) ;
    }
}

void
Printf_Log ( char *format, ... )
{
    if ( _O_->LogFlag )
    {
        va_list args ;
        va_start ( args, ( char* ) format ) ;
        vfprintf ( _CSL_->LogFILE, ( char* ) format, args ) ;
        va_end ( args ) ;
        fflush ( _CSL_->LogFILE ) ;
    }
}

//iPrintf : to be used with internal output
void
iPrintf ( char *format, ... )
{
    va_list args ;
    if ( kbhit ( ) ) OpenVmTil_Pause ( ) ;
    //if ( kbhit ( ) ) OpenVmTil_Pause ( ) ;
    if ( Verbosity ( ) ) //GetState ( _ReadLiner_, CHAR_ECHO ) )
    {
        va_start ( args, ( char* ) format ) ;
        if ( IS_INCLUDING_FILES ) vprintf ( ( char* ) format, args ) ;
        else
        {
            vsprintf ( ( char* ) Buffer_DataCleared ( _O_->PrintBuffer ), ( char* ) format, args ) ;
            //strncpy ( Buffer_Data ( _O_->PrintBufferCopy ), Buffer_Data ( _O_->PrintBuffer ), BUFFER_SIZE ) ;
            printf ( "%s", Buffer_Data ( _O_->PrintBuffer ) ) ;
            _O_->Pblc = String_LastChar ( _Buffer_Data ( _O_->PrintBuffer ) ) ;
            strncpy ( ( char* ) &_O_->Pbf8[0], ( char* ) Buffer_Data ( _O_->PrintBuffer ), 7 ) ;
            //strncpy ( (char*) Buffer_Data ( _O_->PrintBufferCopy ), (char*) Buffer_Data ( _O_->PrintBuffer ), BUFFER_SIZE ) ;
        }
        va_end ( args ) ;
        fflush ( stdout ) ;
    }
}
#if 0
void
dPrintf ( char *format, ... )
{
    va_list args ;
    if ( _O_->DebugOutputFlag & 3 ) //CSL_DebugOutputOn
    {
        va_start ( args, ( char* ) format ) ;
        if ( _O_->DebugOutputFlag |= 4 ) //CSL_DebugOutputConcatOn
        {
            vsprintf ( ( char* ) Buffer_DataCleared ( _O_->PrintBufferConcatCopy ), ( char* ) format, args ) ;
            strncat ( _O_->PrintBufferCopy->B_Data, _O_->PrintBufferConcatCopy->B_Data, BUFFER_SIZE ) ;
        }
        else vsprintf ( ( char* ) Buffer_DataCleared ( _O_->PrintBufferCopy ), ( char* ) format, args ) ;
        va_end ( args ) ;
    }
}
void
lPrintf ( char *format, ... )
{
    va_list args ;
    //if ( _O_->LogFlag )
    {
        va_start ( args, ( char* ) format ) ;
        vfprintf ( _CSL_->LogFILE, ( char* ) format, args ) ;
        va_end ( args ) ;
        fflush ( _CSL_->LogFILE ) ;
    }
}
#endif
// oPrintf : printf with PrintBufferCopy added logic
// printf for user output not for internal output
void
oPrintf ( char *format, ... )
{
    va_list args ;
    if ( kbhit ( ) ) OpenVmTil_Pause ( ) ;
    if ( Verbosity ( ) ) //GetState ( _ReadLiner_, CHAR_ECHO ) )
    {
        va_start ( args, ( char* ) format ) ;
        if ( IS_INCLUDING_FILES ) vprintf ( ( char* ) format, args ) ;
        else
        {
            vsprintf ( ( char* ) Buffer_DataCleared ( _O_->PrintBuffer ), ( char* ) format, args ) ;
            //strncpy ( Buffer_Data ( _O_->PrintBufferCopy ), Buffer_Data ( _O_->PrintBuffer ), BUFFER_SIZE ) ;
            printf ( "%s", Buffer_Data ( _O_->PrintBuffer ) ) ;
            _O_->Pblc = String_LastChar ( _Buffer_Data ( _O_->PrintBuffer ) ) ;
            strncpy ( ( char* ) &_O_->Pbf8[0], ( char* ) Buffer_Data ( _O_->PrintBuffer ), 7 ) ;
            //strncpy ( (char*) Buffer_Data ( _O_->PrintBufferCopy ), (char*) Buffer_Data ( _O_->PrintBuffer ), BUFFER_SIZE ) ;
        }
        va_end ( args ) ;
        fflush ( stdout ) ;
    }
    if ( _O_->DebugOutputFlag & 3 ) //CSL_DebugOutputOn // better logic here ??
#if 1      
    {
        va_start ( args, ( char* ) format ) ;
        if ( _O_->DebugOutputFlag & 4 ) //CSL_DebugOutputConcatOn
        {
            byte *data = Buffer_DataCleared ( _O_->PrintBufferConcatCopy ) ;
            vsprintf ( ( char* ) data, ( char* ) format, args ) ;
            strncat ( _O_->PrintBufferCopy->B_Data, data, BUFFER_SIZE ) ;
        }
        else vsprintf ( ( char* ) Buffer_DataCleared ( _O_->PrintBufferCopy ), ( char* ) format, args ) ;
        va_end ( args ) ;
    }
#else // doesn' work ??
        {
            dPrintf ( format ) ;
        }
#endif    
    if ( _O_->LogFlag )
#if 1        
    {
        va_start ( args, ( char* ) format ) ;
        vfprintf ( _CSL_->LogFILE, ( char* ) format, args ) ;
        va_end ( args ) ;
        fflush ( _CSL_->LogFILE ) ;
    }
#else
        {
            lPrintf ( format ) ;
        }
#endif
}



