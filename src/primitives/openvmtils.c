
#include "../include/csl.h"

void
OpenVmTil_Verbosity ( )
{
    Do_C_Pointer_StackAccess ( (byte* )& _O_->Verbosity );
}

void
OpenVmTil_ShowMachineCodeInstructions ( )
{
    Do_C_Pointer_StackAccess ( (byte* ) & _O_->Dbi );
}

void
Ovt_Optimize ( )
{
    DataStack_Push (( int64 ) GetState ( _CSL_, CO_ON ) ? 1 : 0) ;
}

void
Ovt_Inlining ( )
{
    DataStack_Push (( int64 ) GetState ( _CSL_, INLINE_ON ) ? 1 : 0) ;
}

// allows variables to be created on first use without a "var" declaration
#if 0
void
Ovt_AutoVar ( )
{
    DataStack_Push (( int64 ) GetState ( _O_, AUTO_VAR ) ? 1 : 0) ;
}

void
Ovt_AutoVarOff ( )
{
    SetState ( _O_, AUTO_VAR, false ) ;
}

// allows variables to be created on first use without a "var" declaration

void
Ovt_AutoVarOn ( )
{
    SetState ( _O_, AUTO_VAR, true ) ;
}
#endif

void
OpenVmTil_DataStackSize ( )
{
    DataStack_Push (( int64 ) & _O_->DataStackSize) ;
}

void
OpenVmTil_CodeSize ( )
{
    DataStack_Push (( int64 ) & _O_->MachineCodeSize) ;
}

void
OpenVmTil_SessionObjectsSize ( )
{
    DataStack_Push (( int64 ) & _O_->SessionObjectsSize) ;
}

void
OpenVmTil_CompilerTempObjectsSize ( )
{
    DataStack_Push (( int64 ) & _O_->CompilerTempObjectsSize) ;
}

void
OpenVmTil_ObjectsSize ( )
{
    DataStack_Push (( int64 ) & _O_->ObjectSpaceSize) ;
}

void
OpenVmTil_DictionarySize ( )
{
    DataStack_Push (( int64 ) & _O_->DictionarySize) ;
}

void
OpenVmTil_Print_DataSizeofInfo ( int64 flag )
{
    if ( flag || ( Verbosity () > 1 ) )
    {
        iPrintf ( "\ndlnode size : %d bytes, ", sizeof (dlnode ) ) ;
        iPrintf ( "dllist size : %d bytes, ", sizeof (dllist ) ) ;
        iPrintf ( "dobject size : %d bytes, ", sizeof ( dobject ) ) ;
        iPrintf ( "DLNode size : %d bytes, ", sizeof ( DLNode ) ) ;
        iPrintf ( "AttributeInfo size : %d bytes, ", sizeof (AttributeInfo ) ) ;
        //iPrintf ( "\nObject size : %d bytes, ", sizeof (Object ) ) ;
        iPrintf ( "\nSymbol size : %d bytes, ", sizeof (Symbol ) ) ;
        iPrintf ( "Word size : %d bytes, ", sizeof (Word ) ) ;
        iPrintf ( "ListObject size : %d bytes, ", sizeof ( ListObject ) ) ;
        iPrintf ( "WordData size : %d bytes, ", sizeof (WordData ) ) ;
        iPrintf ( "Context size : %d bytes, ", sizeof (Context ) ) ;
        iPrintf ( "\nSystem size : %d bytes, ", sizeof (System ) ) ;
        iPrintf ( "Debugger size : %d bytes, ", sizeof (Debugger ) ) ;
        iPrintf ( "MemorySpace size : %d bytes, ", sizeof (MemorySpace ) ) ;
        iPrintf ( "ReadLiner size : %d bytes, ", sizeof (ReadLiner ) ) ;
        iPrintf ( "Lexer size : %d bytes, ", sizeof (Lexer ) ) ;
        iPrintf ( "\nInterpreter size : %d bytes, ", sizeof (Interpreter ) ) ;
        iPrintf ( "Finder size : %d bytes, ", sizeof (Finder ) ) ;
        iPrintf ( "Compiler size : %d bytes, ", sizeof (Compiler ) ) ;
        iPrintf ( "ByteArray size : %d bytes, ", sizeof (ByteArray ) ) ;
        iPrintf ( "NamedByteArray size : %d bytes, ", sizeof (NamedByteArray ) ) ;
        iPrintf ( "\nMemChunk size : %d bytes, ", sizeof ( MemChunk ) ) ;
        iPrintf ( "CSL size : %d bytes, ", sizeof (CSL ) ) ;
        iPrintf ( "OpenVimTil size : %d bytes, ", sizeof (OpenVmTil ) ) ;
        iPrintf ( "OVT_Static size : %d bytes, ", sizeof (OVT_StaticMemSystem ) ) ;
        iPrintf ( "OVT_MemSystem size : %d bytes, ", sizeof (OVT_MemSystem ) ) ;
        iPrintf ( "\nStack size : %d bytes", sizeof (Stack ) ) ;
    }
}

void
OVT_Exit ( )
{
    if ( Verbosity () > 0 ) iPrintf ( "bye\n" ) ;
    exit ( 0 ) ;
}

void
OVT_StartupMessage ( Boolean promptFlag )
{
    if ( Verbosity () > 0 )
    {
        DefaultColors ;
        //if ( CSL->InitSessionCoreTimes > 1 ) CSL_NewLine () ;
        if ( promptFlag && ( _O_->Restarts < 2 ) )
        {
            System_Time ( _CSL_->Context0->System0, 0, ( char* ) "\nSystem Startup", 1 ) ;
            OVT_Time ( ( char* ) "OVT Startup", 1 ) ;

            _CSL_Version ( promptFlag ) ;
        }
        if ( Verbosity () > 1 )
        {
            iPrintf ( "\nOpenVmTil : csl comes with ABSOLUTELY NO WARRANTY; for details type `license' in the source directory." ) ;
            iPrintf ( "\nType 'tc' 'demo' for starters" ) ;
            iPrintf ( "\nType 'bye' to exit" ) ;
        }
    }
    else if ( promptFlag && ( _O_->Restarts < 2 ) ) _O_->Verbosity = 1 ;
}

void
_OVT_Ok ( Boolean control )
{
    if ( Verbosity () > 3 )
    {
        _CSL_SystemState_Print ( 0 ) ;
        iPrintf ( "\n<Esc> - break, <Ctrl-C> - quit, <Ctrl-D> - restart, \'bye\'/\'exit\' - leave." ) ;
    }
    _Context_Prompt (_O_->OVT_CSL->Context0, control) ;
}

void
OVT_Ok ( )
{
    _OVT_Ok ( 1 ) ;
    //_CSL_Prompt ( Verbosity () && ( ( _O_->RestartCondition < RESET_ALL ) || _O_->StartTimes > 1 ) ) ;
}


Boolean cli ;
int lic, csl_returnValue = 0 ;

#if 0
int s9_main ( int argc, char **argv ) ;
void
doPrompt ( )
{
    if ( ( lic == '\r' ) || ( lic == '\n' ) ) printf ( "s9> " ) ;
    else printf ( "\ns9> " ) ;
    fflush ( stdout ) ;
}

int
s9_getChar ( FILE * f )
{
    if ( cli ) lic = _Lexer_NextChar ( _Lexer_ ) ;
    else lic = fgetc ( f ) ;
    return lic ;
}

void
s9_ungetChar ( int c, FILE * f )
{
    if ( cli ) ReadLine_UnGetChar ( _ReadLiner_ ) ;
    else ungetc ( c, f ) ;
}

char * csl_buffer ;
void
CSL_S9fes ( )
{
    ReadLiner * rl = _ReadLiner_ ;
    FILE * svFile = rl->InputFile ;
    rl->InputFile = stdin ;
    ReadLine_Init ( rl, _CSL_Key ) ;
    SetState ( _Context_->System0, ADD_READLINE_TO_HISTORY, true ) ;
    byte * snp = rl->NormalPrompt, *sap = rl->AltPrompt ;
    rl->AltPrompt = ( byte* ) "l9< " ;
    rl->NormalPrompt = ( byte* ) "l9> " ;
    csl_buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ;

    s9_main ( 1, ( char*[] ) { "s9" } ) ;

    _Lexer_->TokenBuffer [0] = 0 ;
    rl->NormalPrompt = snp ;
    rl->AltPrompt = sap ;
    rl->InputFile = svFile ;
    iPrintf ( "s9fes : s9 : exiting ... \n" ) ;
    if ( csl_returnValue == 2 ) DataStack_Push ((int64) csl_buffer) ;
}
#endif

#if 0

#if 0
"Ar9" class :{ byte ar [64][64][64] ; };
#if 1
: a9 ( Ar9 a | x y z ) 
    x 0 = y 0 = z 0 = 
#else
: a9 ( Ar9 a ) 
    byte x 0 = y 0 = z 0 = 
#endif
    { z @ 64 < }
    {
        { y @ 64 < }
        {
            { x @ 64 < }
            {
                a.ar [z @][y @][x @ ] x @ =
                x ++  
            }
            while 
            x 0 =
            //adump ( x @ y @ z @ a ) 
            y ++ 
        }
        while
        x 0 =
        y 0 =
        z ++
    }
    while
    adump ( 0 246 67 a )
;
#endif
    
void
C_a9 (  Ar9 * a ) 
{
    int64 x, y, z ;
    x = 0, y = 0, z = 0  ;
    while  ( z < 256  )
    {
        while ( y < 256 )
        {
            while ( x < 256 )
            {
                a->ar [z][y][x] = x ;
                x ++  ;
            }
            x = 0,  y ++ ;
        }
        x = 0, y = 0,  z ++ ;
    }
    printf ( "\na->ar[0][246][67] = %d", a->ar[0][246][67] ) ;
}
#endif    
    
