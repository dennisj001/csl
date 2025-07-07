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

void
CSL_Jcc8_On ( )
{
    //SetState ( _CSL_, JCC8_ON, true ) ;
}

void
CSL_Jcc8_Off ( )
{
    //SetState ( _CSL_, JCC8_ON, false ) ;
}
#endif

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
    System_Time (_Context_->System0, timer, ( char* ) "Timer") ;
}

void
CSL_Throw ( )
{
    byte * msg = ( byte * ) DataStack_Pop ( ) ;
    Throw ( "", msg, 0 ) ;
}

void
ShellEscape ( byte * str )
{
    int status ;
    status = system ( str ) ;
    if ( Verbosity ( ) > 1 ) printf ( ( char* ) c_gd ( "\n_ShellEscape : command = \"%s\" : returned %d.\n" ), str, status ) ;
    Lexer_Init ( _Lexer_, 0, 0, CONTEXT ) ;
    _O_->Pbf8[0] = _ReadLiner_->NormalPrompt[0] ;
    _DoPrompt ( ) ;
}

#if 0

void
ShellEscape ( byte * str )
{
    ShellEscape ( ( char* ) str ) ;
    //SetState ( _Context_->Lexer0, LEXER_DONE, true ) ;
    //_OVT_Ok ( true ) ;
}
#endif

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
#if 0 // experimenting here ...
// the problem here is bash -i doesn't echo to input window

void
shell ( )
{
    Context * cntx = _Context_ ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * buffer = Buffer_New_pbyte ( BUFFER_SIZE ), *token ;
    byte * svPrompt = ReadLine_GetPrompt ( rl ) ;
    ReadLine_SetPrompt ( rl, "$ " ) ;
    iPrintf ( "\n type \'.\' to exit" ) ;
    DoPrompt ( ) ;
    system ( "bash -i" ) ;
    //execl ("/bin/bash", "bash", "-i", "-c", str, (char *) NULL) ;

    ReadLine_SetPrompt ( rl, svPrompt ) ;
    iPrintf ( "\n leaving shell ..." ) ;
    //free (str) ;
}

#elif 0 //current working simple mode

void
shell ( )
{
    Context * cntx = _Context_ ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * buffer = Buffer_New_pbyte ( BUFFER_SIZE ), *token ;
    byte * svPrompt = ReadLine_GetPrompt ( rl ) ;
    ReadLine_SetPrompt ( rl, "$ " ) ;
    iPrintf ( "\n type \'.\' to exit" ) ;
    DoPrompt ( ) ;
    while ( 1 )
    {
        _ReadLine_GetLine ( rl, 0 ) ;
        if ( String_Equal ( & rl->InputLine [rl->ReadIndex], ".\n" ) ) break ;
        //byte * str = String_New ( & rl->InputLine [rl->ReadIndex], TEMPORARY ) ;
        //buffer[0] = 0 ;
        //strcat ( buffer, "ksh " ) ;
        //strcat ( buffer, "set BASH=/usr/bin/bash -i" ) ;
        strcpy ( buffer, & rl->InputLine [rl->ReadIndex] ) ;
        ShellEscape ( buffer ) ; //buffer ) ; // prompt is included in ShellEscape
        //exece ("/bin/bash", "bash", "-i", "-c", str, (char *) 0) ;
    }
    ReadLine_SetPrompt ( rl, svPrompt ) ;
    iPrintf ( "\n leaving shell ..." ) ;
}

#endif

void
CSL_ChangeDirectory ( )
{
    byte * filename = _Lexer_LexNextToken_WithDelimiters ( _Lexer_, 0, 1, 0, 1, LEXER_ALLOW_DOT ) ; 
    if ( String_Equal ( filename, ".." ) )
    {
        byte * b = Buffer_New_pbyte ( BUFFER_SIZE ) ;
        byte * directory = getcwd ( b, BUFFER_SIZE ) ;
        int i, l = strlen ( directory ) ;
        for ( i = l ; i > 0 ; i -- )
        {
            if ( directory[i] == '/' )
            {
                directory [i] = 0 ;
                break ;
            }
        }
        filename = directory ;
    }
    chdir ( filename ) ;
}

void
CSL_Filename ( )
{
    byte * filename = _Context_->ReadLiner0->Filename ;
    if ( ! filename ) filename = ( byte* ) "command line" ;
    DataStack_Push ( ( int64 ) filename ) ;
}

void
CSL_Location ( )
{
    iPrintf ( _Context_Location ( _Context_ ) ) ;
}

void
CSL_LineNumber ( )
{
    DataStack_Push ( ( int64 ) _Context_->ReadLiner0->LineNumber ) ;
}

void
CSL_LineCharacterNumber ( )
{
    DataStack_Push ( ( int64 ) _Context_->ReadLiner0->ReadIndex ) ;
}

void
_CSL_Version ( Boolean flag )
{
    Exception *e = _O_->OVT_Exception ;
    //if ( flag || ( ( Verbosity () ) && ( _O_->StartedTimes == 1 ) ) && (CSL->InitSessionCoreTimes == 1) )
    if ( flag || ( e->Restarts < 2 ) )
    {
        //_iPrintf ( "\ncsl %s", _O_->VersionString ) ;
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
    NUMBER_BASE_SET ( 2 ) ;
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
    _CSL_Dump ( DUMP_MOD ) ;
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
    ShellEscape ( ( char* ) DataStack_Pop ( ) ) ;
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
