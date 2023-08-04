#include "../include/csl.h"

// void getStdin(void) {Chr = getc(InFile), Env.put(Chr) ; }
// void putStdout(int64 c) {putc(c, OutFile);}
#if PICOLISP
extern int64 Chr ;

void
key ( )
{
    Chr = _Context_->Lexer0->NextChar ( ) ;
    //putc ( Chr, stdout ) ;
    //emit ( Chr ) ;
}

void
emit ( int64 c )
{
    putc ( Chr, stdout ) ;
    //_Printf ( (byte*)"%c", (char) c ) ;
}
#endif

void
CSL_Jcc8_On ( )
{
    SetState ( _CSL_, JCC8_ON, true ) ;
}

void
CSL_Jcc8_Off ( )
{
    SetState ( _CSL_, JCC8_ON, false ) ;
}

void
CSL_InitTime ( )
{
    System_InitTime ( _Context_->System0 ) ;
}

void
CSL_TimerInit ( )
{
    int64 timer = DataStack_Pop ( ) ;
    if ( timer < 8 )
    {
        _System_TimerInit ( _Context_->System0, timer ) ;
    }
    else Throw ( "CSL_TimerInit : ", "Error: timer index must be less than 8", QUIT ) ;
}

void
CSL_Time ( )
{
    int64 timer = DataStack_Pop ( ) ;
    System_Time ( _Context_->System0, timer, ( char* ) "\nTimer", 1 ) ;
}

void
CSL_Throw ( )
{
    byte * msg = ( byte * ) DataStack_Pop ( ) ;
    Throw ( "", msg, 0 ) ;
}

void
_ShellEscape ( char * str )
{
    int status ;
#if 1  
    //signal ( SIGCHLD, SIG_IGN ) ;
    status = system ( str ) ;
#elif 0
    pid_t pid1 ;
    pid_t pid2 ;

    if ( pid1 = fork ( ) )
    {
        /* parent process A */
        //waitpid ( pid1, &status, 0) ; //NULL ) ;
        waitpid ( pid1, &status, WNOHANG ) ;
    }
    else if ( ! pid1 )
    {
        /* child process B */
        if ( pid2 = fork ( ) )
        {
            exit ( 0 ) ;
        }
        else if ( ! pid2 )
        {
            /* child process C */
            //execvp("something");
            status = system ( str ) ;
        }
        else
        {
            /* error */
        }
    }
    else
    {
        /* error */
    }
#elif 0
    char *cmd[] = { str, ( char * ) 0 } ; //{ "ls", "-l", ( char * ) 0 } ;
    char *env[] = { ( char * ) 0 } ; //{ "HOME=/usr/home", "LOGNAME=home", ( char * ) 0 } ;
    status = execve ( "", cmd, env ) ;
#elif 0
    status = execlp ( "str", "str", "", ( char * ) 0 ) ;
#elif 0
    status = execl ( "", "sh", "-c", str, ( char * ) 0 ) ;
#elif 0 // bad!!??
    {
        pid_t pid ;

        pid = fork ( ) ;
        if ( pid == 0 )
        {
            status = system ( str ) ;
        }
        else return ;
    }
#elif 0
    {
        extern char **environ ;
        pid_t pid ;
        char *argv[] = { "sh", "-c", str, NULL } ;
        d0 ( Verbosity () = 2 ) ;
        if ( Verbosity () > 1 ) printf ( "\nposix_spawn :: command = %s\n", str ) ;
        //else printf ("\n") ;
#if 0
        posix_spawn ( pid_t * __restrict __pid,
            const char *__restrict __path,
            const posix_spawn_file_actions_t * __restrict
            __file_actions,
            const posix_spawnattr_t * __restrict __attrp,
            char *const __argv[__restrict_arr],
            char *const __envp[__restrict_arr] ) ;
#endif        
        status = posix_spawn ( &pid, "/bin/sh", NULL, NULL, argv, environ ) ;
        //status = system ( str ) ;
#if 1        
        if ( status == 0 )
        {
            if ( Verbosity () > 1 ) printf ( "\nposix_spawn : child : pid = %d\n", pid ) ;
            //if ( wait ( &status ) != -1 ) //( waitpid ( pid, &status, 0 ) != - 1 )
            if ( waitpid ( pid, &status, WNOHANG ) != - 1 )
            {
                if ( Verbosity () > 1 ) printf ( "\nposix_spawn : child : pid = %d : %s :: exited with status %d\n", pid, ( char* ) String_ConvertToBackSlash ( ( byte* ) str ), status ) ;
            }
            else
            {
                if ( Verbosity () > 0 ) perror ( "\nwaitpid" ) ;
            }
        }
        else
        {
            if ( Verbosity () > 1 ) printf ( "\nposix_spawn: %s\n", strerror ( status ) ) ;
        }
#endif        
    }
#endif    
    if ( Verbosity () > 1 ) printf ( ( char* ) c_gd ( "\n_ShellEscape : command = \"%s\" : returned %d.\n" ), str, status ) ;
    fflush ( stdout ) ;
}

void
ShellEscape ( byte * str )
{
    _ShellEscape ( ( char* ) str ) ;
    SetState ( _Context_->Lexer0, LEXER_DONE, true ) ;
    _OVT_Ok ( true ) ;
}

void
ShellEscape_Postfix ( )
{
    byte * str = ( byte* ) DataStack_Pop ( ) ;
    ShellEscape ( ( byte* ) str ) ;
}

void
_shell ( )
{
    byte * str = _String_Get_ReadlineString_ToEndOfLine ( ) ;
    ShellEscape ( str ) ;
}

void
shell ( )
{
    Context * cntx = _Context_ ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * svPrompt = ReadLine_GetPrompt ( rl ) ;
    ReadLine_SetPrompt ( rl, "$ " ) ;
    iPrintf ( "\n type \'exit\' to exit" ) ;
    Context_DoPrompt (cntx) ;
    while ( 1 )
    {
        _ReadLine_GetLine ( rl, 0 ) ; 
        byte * str = String_New ( & rl->InputLine [rl->ReadIndex], TEMPORARY ) ;
        if ( String_Equal ( str, "exit\n" ) ) break ;
        ShellEscape ( str ) ; // prompt is included in ShellEscape
    }
    ReadLine_SetPrompt ( rl, svPrompt ) ;
    iPrintf ( "\n leaving shell ..." ) ;
}

void
CSL_Filename ( )
{
    byte * filename = _Context_->ReadLiner0->Filename ;
    if ( ! filename ) filename = ( byte* ) "command line" ;
    DataStack_Push (( int64 ) filename) ;
}

void
CSL_Location ( )
{
    iPrintf ( _Context_Location ( _Context_ ) ) ;
}

void
CSL_LineNumber ( )
{
    DataStack_Push (( int64 ) _Context_->ReadLiner0->LineNumber) ;
}

void
CSL_LineCharacterNumber ( )
{
    DataStack_Push (( int64 ) _Context_->ReadLiner0->ReadIndex) ;
}

void
_CSL_Version ( Boolean flag )
{
    //if ( flag || ( ( Verbosity () ) && ( _O_->StartedTimes == 1 ) ) && (CSL->InitSessionCoreTimes == 1) )
    if ( flag || ( _O_->Restarts < 2 ) )
    {
        //_Printf ( "\ncsl %s", _O_->VersionString ) ;
        iPrintf ( "\nversion %s", _O_->VersionString ) ;
    }
}

void
CSL_Version ( )
{
    _CSL_Version ( 1 ) ;
}

void
CSL_SystemState_Print ( )
{
    _CSL_SystemState_Print ( 0 ) ;
}

void
CSL_SystemState_PrintAll ( )
{
    _CSL_SystemState_Print ( 1 ) ;
}

void
_SetEcho ( int64 boolFlag )
{
    SetState ( _Context_->ReadLiner0, CHAR_ECHO, boolFlag ) ;
    SetState ( _CSL_, READLINE_ECHO_ON, boolFlag ) ;
}

void
CSL_Echo ( )
{
    // toggle flag
    if ( GetState ( _CSL_, READLINE_ECHO_ON ) )
    {
        _SetEcho ( false ) ;
    }
    else
    {
        _SetEcho ( true ) ;
    }
}

void
CSL_EchoOn ( )
{
    _SetEcho ( true ) ;
}

void
CSL_EchoOff ( )
{
    _SetEcho ( false ) ;
}

// ?? optimize state should be in either CSL or OpenVmTil not System structure

void
CSL_NoOp ( void )
{
    //if ( CompileMode ) _Compile_Return ( ) ;
}

void
CSL_Hex ( )
{
    NUMBER_BASE_SET ( 16 ) ;
}

void
CSL_Binary ( ) 
{
    NUMBER_BASE_SET ( 2 );
}

void
CSL_Decimal ( ) 
{

    NUMBER_BASE_SET ( 10 ) ;
}

void
CSL_Dump ( )
{
    byte * location = Context_IsInFile ( _Context_ ) ? Context_Location ( ) : ( byte* ) "" ;
    iPrintf ( "\nDump at : %s :", location ) ;
    _CSL_Dump ( 16 ) ;
    iPrintf ( "\n" ) ;
}

#if 1 
void
CSL_Source_AddToHistory ( )
{
    Word *word = ( Word* ) DataStack_Pop ( ) ;
    if ( word )
    {
        _CSL_Source ( word, 1 ) ;
    }
    //else CSL_Exception ( NOT_A_KNOWN_OBJECT, QUIT ) ;
}

void
CSL_Source_DontAddToHistory ( )
{
    Word *word = ( Word* ) DataStack_Pop ( ) ;
    if ( word )
    {
        _CSL_Source ( word, 0 ) ;
    }
    //else CSL_Exception ( NOT_A_KNOWN_OBJECT, QUIT ) ;
}
#endif

void
CSL_AllocateNew ( )
{
    DataStack_Push ( ( int64 ) Mem_Allocate ( DataStack_Pop ( ), OBJECT_MEM ) ) ;
}

void
CSL_ReturnFromFile ( )
{
    _EOF ( _Context_->Lexer0 ) ;
}

void
CSL_ShellEscape ( )
{
    _ShellEscape ( ( char* ) DataStack_Pop ( ) ) ;
    NewLine ( _Context_->Lexer0 ) ;
}

#if 0
void foxWindow ( int64 argc, char **argv ) ;

void
CSL_Window ( )
{
    int64 argc = 0 ;
    char ** argv = 0 ;
    foxWindow ( argc, argv ) ;
}
#endif
