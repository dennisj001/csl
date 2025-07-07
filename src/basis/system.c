#include "../include/csl.h"

// lib : full library path

#define RTLD_DEFAULT ((void *) 0)

void *
_dlsym ( byte * sym, byte * lib )
{
    void *fp, * hLibrary = dlopen ( ( char* ) lib, RTLD_GLOBAL | RTLD_LAZY ) ;
    if ( hLibrary )
    {
        fp = ( void* ) dlsym ( hLibrary, ( char* ) sym ) ;
        //fp = ( void* ) dlsym ( RTLD_DEFAULT, ( char* ) sym ) ; // either work after dlopen
    }
    if ( ( ! hLibrary ) || ( ! fp ) )
    {
        iPrintf ( c_ad ( "\ndlsym : dlerror = %s\n" ), dlerror ( ) ) ;
        return 0 ;
    }
    return fp ;
}

void *
_Dlsym ( byte * sym, byte * lib )
{
    void *functionPointer = _dlsym ( sym, lib ) ;
    if ( ( ! functionPointer ) )
    {
        char buffer [256], *sharedLib = ( char* ) lib ;
        int64 ll ;
        for ( ll = Strlen ( sharedLib ) ; sharedLib [ ll ] != '/' ; ll -- ) ;
        strcpy ( buffer, "./lib32" ) ;
        strcat ( buffer, &sharedLib [ll] ) ;
        functionPointer = _dlsym ( sym, ( byte* ) buffer ) ;
        if ( ! functionPointer )
        {
            iPrintf ( c_ad ( "\ndlsym : dlerror = %s\n" ), dlerror ( ) ) ;
            return 0 ;
        }
    }
    return functionPointer ;
}

Word *
Dlsym ( byte * sym, byte * lib )
{
    block b = ( block ) _Dlsym ( sym, lib ) ;
    Word * word = DataObject_New ( CSL_WORD, 0, sym, CPRIMITIVE | DLSYM_WORD | C_PREFIX | C_PREFIX_RTL_ARGS, 0, 0, 0, ( int64 ) b, 0, 0, 0, - 1 ) ;
    word->W_TypeAttributes |= WT_C_PREFIX_RTL_ARGS ;
    return word ;
}

void
CSL_Dlsym ( )
{
    byte * sym = Lexer_ReadToken ( _Context_->Lexer0 ) ;
    byte * lib = _Lexer_LexNextToken_WithDelimiters ( _Context_->Lexer0, 0, 1, 0, 1, LEXER_ALLOW_DOT ) ;
    if ( lib[0] == '\"' ) lib = Lexer_ParseTerminatingMacro (_Lexer_, '\"', 1) ; ; //Interpreter_InterpretAToken ( _Interpreter_, lib, -1, -1 ) ;
    byte * semi = Lexer_ReadToken ( _Context_->Lexer0 ) ; // drop the semi
    Word * word = Dlsym ( sym, lib ) ;
    _Word_Finish ( word ) ;
}

// callNumber | errno

//%rdi, %rsi, %rdx, %rcx, %r8, and %r9
// this is ??
void
CSL_system0 ( )
{
    _Compile_Stack_PopToReg ( DSP, ACC ) ;
    _Compile_INT80 ( ) ;
    _Compile_Stack_PushReg ( DSP, ACC, 0 ) ;
}

void
CSL_system1 ( )
{
    _Compile_Stack_PopToReg ( DSP, ACC ) ;
    _Compile_Stack_PopToReg ( DSP, RDI ) ;
    _Compile_INT80 ( ) ;
    _Compile_Stack_PushReg ( DSP, ACC, 0 ) ;
}
void
CSL_system2 ( )
{
    _Compile_Stack_PopToReg ( DSP, ACC ) ;
    _Compile_Stack_PopToReg ( DSP, RDI ) ;
    _Compile_Stack_PopToReg ( DSP, RSI ) ;
    _Compile_INT80 ( ) ;
    _Compile_Stack_PushReg ( DSP, ACC, 0 ) ;
}

void
CSL_system3 ( )
{
    _Compile_Stack_PopToReg ( DSP, ACC ) ;
    _Compile_Stack_PopToReg ( DSP, RDI ) ;
    _Compile_Stack_PopToReg ( DSP, RSI ) ;
    _Compile_Stack_PopToReg ( DSP, RDX ) ;
    _Compile_INT80 ( ) ;
    _Compile_Stack_PushReg ( DSP, ACC, 0 ) ;
}

void
_CSL_WordAccounting_Print ( byte * functionName )
{
    iPrintf ( "\n%s :: DObjectCreateCount = %d : WordCreateCount = %d : WordsAdded = %d : FindWordCount = %d : FindWordMaxCount = %d",
        functionName, _CSL_->DObjectCreateCount, _CSL_->WordCreateCount, _CSL_->WordsAdded, _CSL_->FindWordCount, _CSL_->FindWordMaxCount ) ;
    iPrintf ( "\nRecycledWordCount : %d", _O_->MemorySpace0->RecycledWordCount ) ;
    iPrintf ( "\nWordsInRecycling : %d", _O_->MemorySpace0->WordsInRecycling ) ;
    Buffer_PrintBuffers ( ) ;
}

void
CSL_WordAccounting ( byte * functionName )
{
    if ( _CSL_->FindWordCount > _CSL_->FindWordMaxCount ) _CSL_->FindWordMaxCount = _CSL_->FindWordCount ;
    if ( Verbosity ( ) > 4 )
        _CSL_WordAccounting_Print ( functionName ) ;
}

byte *
_CSL_GetSystemState_String0 ( byte * buf )
{
    strcpy ( ( char* ) buf, "\ntypeChecking is " ) ;
    if ( GetState ( _CSL_, TYPECHECK_ON ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    strcat ( ( char* ) buf, "optimize is " ) ;
    if ( GetState ( _CSL_, CO_ON ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    strcat ( ( char* ) buf, "inlining is " ) ;
    if ( GetState ( _CSL_, INLINE_ON ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    strcat ( ( char* ) buf, "infixMode is " ) ;
    if ( GetState ( _Context_, INFIX_MODE ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    strcat ( ( char* ) buf, "\nprefixMode is " ) ;
    if ( GetState ( _Context_, PREFIX_MODE ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    strcat ( ( char* ) buf, "c_syntax is " ) ;
    if ( GetState ( _Context_, C_SYNTAX ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    if ( ! GetState ( _Context_, PREFIX_MODE | INFIX_MODE ) ) strcat ( ( char* ) buf, "postfixMode is on" ) ;
    return buf ;
}

byte *
_CSL_GetSystemState_String1 ( byte *buf )
{
    strcat ( ( char* ) buf, "\nReadLine echo is " ) ;
    if ( GetState ( _CSL_, READLINE_ECHO_ON ) ) strcat ( ( char* ) buf, "on. " ) ;
    else strcat ( ( char* ) buf, "off. " ) ;
    strcpy ( ( char* ) buf, "\nDebug is " ) ;
    if ( GetState ( _CSL_, DEBUG_MODE ) ) strcat ( ( char* ) buf, "on, " ) ;
    else strcat ( ( char* ) buf, "off, " ) ;
    sprintf ( ( char* ) &buf[Strlen ( ( char* ) buf )], "Verbosity = %ld, ", Verbosity ( ) ) ;
    sprintf ( ( char* ) &buf[Strlen ( ( char* ) buf )], "Console = %ld, ", _O_->Console ) ;
    sprintf ( ( char* ) &buf[Strlen ( ( char* ) buf )], "NumberBase = %ld.", NUMBER_BASE_GET ( ) ) ;
    return buf ;
}

const char *
Convert_RestartCondtion ( int64 restartCondition )
{
    switch ( restartCondition )
    {
        case ( ( uint64 ) 1 << 10 ): return "INITIAL_START" ;
        case ( ( uint64 ) 1 << 9 ): return "FULL_RESTART" ;
        case ( ( uint64 ) 1 << 8 ): return "RESTART" ;
        case ( ( uint64 ) 1 << 7 ): return "RESET_ALL" ;
        case ( ( uint64 ) 1 << 6 ): return "ABORT" ;
        case ( ( uint64 ) 1 << 5 ): return "QUIT" ;
        case ( ( uint64 ) 1 << 4 ): return "CSL_RUN_INIT" ;
        case ( ( uint64 ) 1 << 3 ): return "STOP" ;
        case ( ( uint64 ) 1 << 2 ): return "BREAK" ;
        case ( ( uint64 ) 1 << 1 ): return "CONTINUE" ;
        case ( ( uint64 ) 1 << 0 ): return "ON" ;
        case ( ( uint64 ) 0 ): return "OFF" ;
        default: return "Unknown Condition" ;
    }
}

void
_State_Print ( )
{
    Boolean dsc = GetState ( _CSL_, DEBUG_SOURCE_CODE_MODE ) ;
    iPrintf ( "\nDebug Source Code %s", dsc ? "on" : "off" ) ;
    Boolean bno = Namespace_IsUsing ( ( byte* ) "BigNum" ) ;
    iPrintf ( " : BigNum %s", bno ? "on" : "off" ) ;
    Boolean lo = Namespace_IsUsing ( ( byte* ) "Lisp" ) ;
    iPrintf ( " : Lisp %s", lo ? "on" : "off" ) ;
}

void
_CSL_SystemState_Print ( int64 pflag )
{
    iPrintf ( "\nSystem State :\n==================" ) ;
    Finder * finder = _Finder_ ;
    byte * buf = Buffer_DataCleared ( _CSL_->ScratchB1 ) ;
    buf = _CSL_GetSystemState_String0 ( buf ) ;
    iPrintf ( buf ) ;
    buf = _CSL_GetSystemState_String1 ( buf ) ;
    iPrintf ( buf ) ;
    _State_Print () ;
    iPrintf ( " : Lisp Debug : %s", GetState ( _LC_, LC_DEBUG_ON ) ? "on" : "off" ) ;
    iPrintf ( " : jcc8 %s", GetState ( _CSL_, JCC8_ON ) ? "on" : "off" ) ;
    iPrintf ( "\n%s : at %s", Compiling ? "compiling" : "interpreting", Context_Location ( ) ) ;
    OVT_ExceptionState_Print ( ) ;
    Namespace * ins = _CSL_Namespace_InNamespaceGet ( ) ;
    if ( ins ) iPrintf ( "\nInNamespace = %s.%s", ins->S_ContainingNamespace->Name, ins->Name ) ;
    if ( finder->QualifyingNamespace ) iPrintf ( "\nQualifyingNamespace = %s.%s",
        finder->QualifyingNamespace->S_ContainingNamespace ? finder->QualifyingNamespace->S_ContainingNamespace->Name : ( byte* ) "",
        finder->QualifyingNamespace->Name ) ;
    if ( _Context_->QidInNamespace ) iPrintf ( "\nQualifyingNamespace = %s.%s",
        _Context_->QidInNamespace->S_ContainingNamespace ? _Context_->QidInNamespace->S_ContainingNamespace->Name : ( byte* ) "",
        _Context_->QidInNamespace->Name ) ;
    if ( pflag || ( Verbosity ( ) > 1 ) )
    {
        OpenVmTil_Print_DataSizeofInfo ( pflag ) ;
        _CSL_WordAccounting_Print ( ( byte* ) "_CSL_SystemState_Print" ) ;
        BigNum_StateShow ( ) ;
    }
}

void
Dump ( byte * address, int64 number, int64 dumpMod )
{
    if ( address && number )
    {
        //byte * nformat ;
        //if ( ( NUMBER_BASE_GET () == 16 ) ) nformat = ( byte* ) "\nDump : Address = " UINT_FRMT " : Number = " UINT_FRMT " :: (little endian dump)" ;
        //else nformat = ( byte* ) "\nDump : Address = " UINT_FRMT " : Number = " INT_FRMT " :: (little endian dump)" ;
        int64 i, n ;
        iPrintf ( "\nDump : Address = " UINT_FRMT " : Number = " INT_FRMT " :: (little endian dump)", ( int64 ) address, number ) ;
        for ( i = 0 ; i < number ; )
        {
            iPrintf ( "\n" UINT_FRMT " : ", address + i ) ;
            if ( ! ( i % dumpMod ) )
            {
                iPrintf ( " " ) ;
                for ( n = 0 ; n < dumpMod ; n += CELL_SIZE ) CSL_NByteDump ( ( byte* ) ( address + i + n ), CELL_SIZE ) ;
                for ( n = 0 ; n < dumpMod ; n += CELL_SIZE ) CSL_CharacterDump ( ( byte* ) ( address + i + n ), CELL_SIZE ) ;
                i += dumpMod ;
            }
            else i ++ ;
        }
    }
}

void
_CSL_Source ( Word *word, int64 addToHistoryFlag )
{
    if ( word )
    {
        Word * aword = 0 ;
        byte * name = c_gd ( word->Name ) ;
        if ( word->ContainingNamespace ) iPrintf ( "\n%s.", word->ContainingNamespace->Name ) ;
        if ( word->W_ObjectAttributes & OBJECT )
        {
            iPrintf ( "%s <:> %s", name, "object" ) ;
        }
        if ( word->W_ObjectAttributes & STRUCTURE )
        {
            iPrintf ( "%s <:> %s : size = %d", name, "structure", word->CompiledDataFieldByteSize ) ;
        }
        else if ( word->W_ObjectAttributes & NAMESPACE )
        {
            iPrintf ( "%s <:> %s", name, "namespace" ) ;
        }
        else if ( word->W_ObjectAttributes & TEXT_MACRO )
        {
            iPrintf ( "%s <:> %s", name, "text macro" ) ;
        }
        else if ( word->W_ObjectAttributes & STRING_MACRO )
        {
            iPrintf ( "%s <:> %s", name, "string macro" ) ;
        }
        else if ( word->W_ObjectAttributes & DEFINES_MACRO )
        {
            iPrintf ( "%s <:> %s", name, "defines macro" ) ;
        }
        else if ( word->W_ObjectAttributes & LOCAL_VARIABLE )
        {
            iPrintf ( "%s <:> %s", name, "local variable" ) ;
        }
        else if ( word->W_ObjectAttributes & PARAMETER_VARIABLE )
        {
            iPrintf ( "%s <:> %s", name, "stack variable" ) ;
        }
        else if ( word->W_ObjectAttributes & NAMESPACE_VARIABLE )
        {
            iPrintf ( "%s <:> %s", name, "variable" ) ;
        }
        else if ( word->W_ObjectAttributes & CONSTANT )
        {
            iPrintf ( "%s <:> %s", name, "constant" ) ;
        }
        if ( word->W_MorphismAttributes & ALIAS )
        {
            aword = Word_UnAlias ( word ) ; //word->W_AliasOf ;
            //aword = Word_Root (word) ;
            if ( aword ) iPrintf ( "%s alias for %s", name, ( char* ) c_gd ( aword->Name ) ) ;
        }
        else if ( word->W_MorphismAttributes & CPRIMITIVE )
        {
            iPrintf ( "%s <:> %s", name, "C compiled primitive" ) ;
        }
        else if ( word->W_LispAttributes & T_LISP_COMPILED_WORD )
        {
            iPrintf ( "%s <:> %s", name, "lambdaCalculus compiled word" ) ;
        }
        else if ( word->W_MorphismAttributes & CSL_WORD )
        {
            iPrintf ( "%s <:> %s", name, "csl compiled word" ) ;
        }
        else if ( word->W_LispAttributes & T_LC_DEFINE )
        {
            iPrintf ( "%s <:> %s", name, "lambdaCalculus defined word" ) ;
        }
        else if ( word->W_MorphismAttributes & BLOCK )
        {
            iPrintf ( "%s <:> %s", name, "csl compiled code block" ) ;
        }
        if ( word->W_MorphismAttributes & INLINE ) iPrintf ( ", %s", "inline" ) ;
        if ( word->W_MorphismAttributes & IMMEDIATE ) iPrintf ( ", %s", "immediate" ) ;
        if ( word->W_MorphismAttributes & PREFIX ) iPrintf ( ", %s", "prefix" ) ;
        if ( word->W_MorphismAttributes & C_PREFIX ) iPrintf ( ", %s", "c_prefix" ) ;
        //if ( word->W_MorphismAttributes & C_RETURN ) iPrintf ( ", %s", "c_return" ) ;
        if ( word->W_MorphismAttributes & INFIXABLE ) iPrintf ( ", %s", "infixable" ) ;
        if ( word->W_WordData )
        {
            _Word_ShowSourceCode ( word ) ; // source code has newlines for multiline history
            if ( aword && ( aword != Word_UnAlias ( aword ) ) )
            {
                _Word_ShowSourceCode ( aword ) ;
            }
            if ( addToHistoryFlag ) _OpenVmTil_AddStringToHistoryList ( word->W_OriginalCodeText ) ;
            if ( word->W_WordData->Filename ) iPrintf ( "\nSource code file location of %s : \"%s\" : %d.%d :: we are now at : %s", name,
                word->W_WordData->Filename, word->W_WordData->LineNumber, word->W_TokenStart_LineIndex, Context_IsInFile ( _Context_ ) ? Context_Location ( ) : ( byte* ) "command line" ) ;
            if ( ( word->W_LispAttributes & T_LC_DEFINE ) && ( ! ( word->W_LispAttributes & T_LISP_COMPILED_WORD ) ) ) iPrintf ( "\nLambda Calculus word : interpreted not compiled" ) ; // do nothing here
            else if ( ! ( word->W_MorphismAttributes & CPRIMITIVE ) )
            {
                iPrintf ( "\nCompiled with : %s%s%s%s%s",
                    GetState ( word, COMPILED_OPTIMIZED ) ? "optimizeOn" : "optimizeOff", GetState ( word, COMPILED_INLINE ) ? ", inlineOn" : ", inlineOff",
                    ( ( word->W_TypeAttributes & WT_C_SYNTAX ) || GetState ( word, W_C_SYNTAX ) ) ? ", c_syntaxOn" : "",
                    GetState ( word, W_INFIX_MODE ) ? ", infixOn" : "", GetState ( word, W_JCC8_ON ) ? ", Jcc8 on" : ", Jcc8 off" ) ;
                Boolean wsc = GetState ( word, W_SOURCE_CODE_MODE ) ;
                iPrintf ( " : Word Source Code %s", wsc ? "on" : "off" ) ;
                _State_Print ( ) ;
            }
            if ( word->Definition && word->S_CodeSize ) iPrintf ( "\nstarting at address : 0x%x -- code size = %d bytes", word->Definition, word->S_CodeSize ) ;
            if ( word->W_TypeSignatureString[0] ) iPrintf ( "\nTypeSignature : %s", Word_ExpandTypeLetterSignature ( word, 0 ) ) ;
        }
    }
}

void
_CSL_Dump ( int64 dumpMod )
{
    //CSL_PrintDataStack () ;
    int64 number = DataStack_Pop ( ) ;
    byte * address = ( byte* ) DataStack_Pop ( ) ;
    Dump ( address, number, dumpMod ) ;
}

Boolean
_AtCommandLine ( )
{
    return ( ! IS_INCLUDING_FILES ) ;
}

Boolean
AtCommandLine ( ReadLiner *rl )
{
    return ( ( GetState ( _Debugger_, DBG_COMMAND_LINE ) || GetState ( _Context_, AT_COMMAND_LINE ) ) ||
        ( GetState ( rl, CHAR_ECHO ) && ( _AtCommandLine ( ) ) ) ) ;
}
#if 0
// example from : http://www.kernel.org/doc/man-pages/online/pages/man3/dlsym.3.html

Load the math library, and _print the cosine of 2.0 :

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int64
main ( int64 argc, char **argv )
{
    void *handle ;
    double (*cosine )( double ) ;
    char *error ;

    handle = dlopen ( "libm.so", RTLD_LAZY ) ;
    if ( ! handle )
    {
        fprintf ( stderr, "%s\n", dlerror ( ) ) ;
        exit ( EXIT_FAILURE ) ;
    }

    dlerror ( ) ; /* Clear any existing error */

    /* Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
       would seem more natural, but the C99 standard leaves
       casting from "void *" to a function pointer undefined.
       The assignment used below is the POSIX.1-2003 (Technical
       Corrigendum 1) workaround; see the Rationale for the
       POSIX specification of dlsym(). */

    *( void ** ) ( &cosine ) = dlsym ( handle, "cos" ) ;

    if ( ( error = dlerror ( ) ) != NULL )
    {
        fprintf ( stderr, "%s\n", error ) ;
        exit ( EXIT_FAILURE ) ;
    }

    printf ( "%f\n", ( *cosine )( 2.0 ) ) ;
    dlclose ( handle ) ;
    exit ( EXIT_SUCCESS ) ;
}
/*
       If this program were in a file named "foo.c", you would build the program with
       the following command:

           gcc -rdynamic -o foo foo.c -ldl

       Libraries exporting _init() and _fini() will want to be compiled as follows,
       using bar.c as the example name:

           gcc -shared -nostartfiles -o bar bar.c
 */

#endif
// lib sym | addr
#if 0

void
_CSL_Dlsym ( )
{
    byte * sym = ( byte* ) DataStack_Pop ( ) ;
    byte * lib = ( byte* ) DataStack_Pop ( ) ;
    DataStack_Push ( ( int64 ) _Dlsym ( sym, lib ) ) ;
}

void
CSL_DlsymWord ( )
{
    byte * lib = ( byte* ) DataStack_Pop ( ) ;
    byte * sym = ( byte* ) DataStack_Pop ( ) ;
    Dlsym ( sym, lib ) ;
}
#endif
// takes semi - ";" - after the definition

#if 0

void *
dlOpen_Dlsym ( char * lib, char * sym )
{
    void * hLibrary, *fp ;
    char * error, buffer [1024] ;

    sprintf ( buffer, "./%s.so", lib ) ;
    hLibrary = dlopen ( buffer, RTLD_GLOBAL | RTLD_LAZY ) ;
    if ( ! hLibrary )
    {
        sprintf ( buffer, "/usr/lib32/%s.so", lib ) ;
        hLibrary = dlopen ( buffer, RTLD_GLOBAL | RTLD_LAZY ) ;
    }
    if ( ! hLibrary )
    {
        sprintf ( buffer, "/usr/local/lib/%s.so", lib ) ;
        hLibrary = dlopen ( buffer, RTLD_GLOBAL | RTLD_LAZY ) ;
    }
    if ( ! hLibrary )
    {
        sprintf ( buffer, "/usr/lib/%s.so", lib ) ;
        hLibrary = dlopen ( buffer, RTLD_GLOBAL | RTLD_LAZY ) ;
    }
    if ( ! hLibrary )
    {
        iPrintf ( "\nCannot open %s - cannot import library\n", buffer ) ;
        return 0 ;
    }
    fp = ( void* ) dlsym ( RTLD_DEFAULT /*hLibrary*/, ( char* ) sym ) ;
    //if ( ( error = dlerror ( ) ) != NULL )
    if ( ( ! fp ) || ( ( error = dlerror ( ) ) != NULL ) )
    {
        iPrintf ( "dlOpen_Dlsym : dlerror: %s\n", error ) ;
        return 0 ;
    }

    //void * hLibrary = dlopen ( lib, RTLD_DEFAULT |RTLD_GLOBAL | RTLD_LAZY ) ;
    void * fp = _Dlsym ( lib, sym ) ;

    return fp ;
}
#endif



