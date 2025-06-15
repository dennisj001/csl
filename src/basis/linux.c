
#include "../include/csl.h"

#ifdef LINUX
struct termios SavedTerminalAttributes ;

void
_DisplaySignal ( int64 signal )
{
    Exception *e = _O_->OVT_Exception ;
    if ( signal )
    {
        byte * location ; 
        if ( e->SigSegvs < 2 ) location = ( byte* ) Context_Location ( ) ;
        else location = (byte*) "" ;
        switch ( signal )
        {
            case SIGSEGV:
            {
                printf ( "\nSIGSEGV : memory access violation : address = 0x%016lx : %s", (uint64) e->SigAddress, location ) ;
                fflush ( stdout ) ;
                break ;
            }
            case SIGFPE:
            {
                iPrintf ( "\nSIGFPE : arithmetic exception - %s", location ) ;
                break ;
            }
            case SIGILL:
            {
                iPrintf ( "\nSIGILL : illegal instruction - %s", location ) ;
                break ;
            }
            case SIGTRAP:
            {
                iPrintf ( "\nSIGTRAP : int3/trap - %s", location ) ;
                break ;
            }
            default: break ;
        }
    }
}

void
Linux_SetupSignals ()
{
    struct sigaction signalAction ;
    // from http://www.linuxjournal.com/article/6483
    int64 i, result ;
    Mem_Clear ( ( byte* ) & signalAction, sizeof ( struct sigaction ) ) ;
    //Mem_Clear ( ( byte* ) _O_->OVT_Exception, sizeof ( Exception ) ) ;
    if ( _CSL_ ) Mem_Clear ( ( byte* ) &_CSL_->JmpBuf0, sizeof ( sigjmp_buf ) ) ; 
    Mem_Clear ( ( byte* ) &_O_->JmpBuf0, sizeof ( sigjmp_buf ) ) ;
    signalAction.sa_sigaction = OpenVmTil_SignalAction ;
    signalAction.sa_flags = SA_SIGINFO | SA_RESTART ; // restarts the set handler after being used instead of the default handler
    for ( i = SIGHUP ; i <= _NSIG ; i ++ )
    {
        result = sigaction ( i, &signalAction, NULL ) ;
        if ( result && ( _O_ && ( Verbosity () > 2 ) ) ) 
            printf ( "\nLinux_SetupSignals : signal number = " INT_FRMT_02 " : result = " INT_FRMT " : This signal can not have a handler.", 
                i, result ) ;
    }
    //signal ( SIGWINCH, SIG_IGN ) ; // a fix for a netbeans problem but causes crash with gcc 6.x -O2+
}

void
Linux_RestoreTerminalAttributes ( )
{
    tcsetattr ( STDIN_FILENO, TCSANOW, &SavedTerminalAttributes ) ;
}
struct termios term ;

void
Linux_SetInputMode ( struct termios * savedTerminalAttributes )
{
    struct termios term ; //terminalAttributes ;
    // Make sure stdin is a terminal. /
    if ( ! isatty ( STDIN_FILENO ) )
    {
        printf ( "Not a terminal.\n" ) ;
        fflush (stdout) ;
        exit ( EXIT_FAILURE ) ;
    }

    // Save the terminal attributes so we can restore them later. /
    memset ( savedTerminalAttributes, 0, sizeof ( struct termios ) ) ;
    tcgetattr ( STDIN_FILENO, savedTerminalAttributes ) ;
    atexit ( Linux_RestoreTerminalAttributes ) ;

    tcgetattr ( STDIN_FILENO, &term ) ; //&terminalAttributes ) ;
#if 0
    //terminalAttributes.c_iflag &= ~ ( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR
    //        | IGNCR | ICRNL | IXON ) ;
    //terminalAttributes.c_lflag &= ~ ( ICANON | ECHO | ECHONL | ISIG ) ; // | IEXTEN ) ;
    terminalAttributes.c_lflag &= ~ ( ICANON | ECHO | ECHONL ) ; // | ISIG ) ; // | IEXTEN ) ;
    //terminalAttributes.c_cflag &= ~ ( CSIZE | PARENB ) ;
    //terminalAttributes.c_cflag |= CS8 ;
    //terminalAttributes.c_cc [ VMIN ] = 1 ;
    //terminalAttributes.c_cc [ VTIME ] = 0 ;
    tcsetattr ( STDIN_FILENO, TCSANOW, &terminalAttributes ) ;
#else
    // from http://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
    term.c_iflag |= IGNBRK ;
    term.c_iflag &= ~ ( INLCR | ICRNL | IXON | IXOFF ) ;
    term.c_lflag &= ~ ( ICANON | ECHO | ECHOK | ECHOE | ECHONL | ISIG | IEXTEN ) ;
    //term.c_lflag &= ~ ( ICANON | ISIG | IEXTEN ) ;
    term.c_cc[VMIN] = 1 ;
    term.c_cc[VTIME] = 0 ;
    tcsetattr ( fileno ( stdin ), TCSANOW, &term ) ;
#endif    
}

void
_LinuxInit ( struct termios * savedTerminalAttributes )
{
    Linux_SetInputMode ( savedTerminalAttributes ) ; // nb. save first !! then copy to _O_ so atexit reset from global _O_->SavedTerminalAttributes
    //Linux_SetupSignals ( 1 ) ; //_O_ ? ! _O_->StartedTimes : 1 ) ;
}

void
LinuxInit ()
{
    _LinuxInit ( &SavedTerminalAttributes ) ; // nb. save first !! then copy to _O_ so atexit reset from global _O_->SavedTerminalAttributes
    //Linux_SetupSignals ( 1 ) ; //_O_ ? ! _O_->StartedTimes : 1 ) ;
}

#endif


