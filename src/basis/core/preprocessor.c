
#include "../../include/csl.h"

void
CSL_PreProcessor ( )
{
    Lexer * lexer = _Lexer_ ;
    Interpreter * interp = _Context_->Interpreter0 ;
    int64 svState = GetState ( lexer, ( ADD_TOKEN_TO_SOURCE | ADD_CHAR_TO_SOURCE ) ) ;
    Word * ppword ;
    Lexer_SourceCodeOff ( lexer ) ;
    _CSL_UnAppendFromSourceCode_NChars ( _CSL_, 1 ) ; // 1 : '#'
    SetState ( interp, PREPROCESSOR_MODE, true ) ;
    uint64 *svStackPointer = GetDataStackPointer ( ) ;
    byte * token = Lexer_ReadToken ( lexer ) ;
    ppword = _Finder_FindWord_InOneNamespace ( _Finder_, Namespace_Find ( ( byte* ) "PreProcessor" ), token ) ;
    Interpreter_DoWord ( interp, ppword, - 1, - 1 ) ;
    SetState ( interp, PREPROCESSOR_MODE, false ) ;
    if ( Compiling ) SetState ( lexer, ( ADD_TOKEN_TO_SOURCE | ADD_CHAR_TO_SOURCE ), svState ) ;
    CSL_TypeStackReset ( ) ;
    SetDataStackPointer ( svStackPointer ) ;
}

Boolean
_GetCondStatus ( )
{
    Context * cntx = _Context_ ;
    Boolean status, svcm = GetState ( cntx->Compiler0, COMPILE_MODE ), svcs = GetState ( cntx, C_SYNTAX ) ;
    SetState ( cntx->Compiler0, COMPILE_MODE, false ) ;
    SetState ( cntx, C_SYNTAX, false ) ;
    SetState ( cntx, CONTEXT_PREPROCESSOR_MODE, true ) ;
    uint64 *svStackPointer = GetDataStackPointer ( ) ;
    Interpret_ToEndOfLine ( cntx->Interpreter0 ) ;
    SetState ( cntx, CONTEXT_PREPROCESSOR_MODE, false ) ;
    SetState ( cntx->Compiler0, COMPILE_MODE, svcm ) ;
    SetState ( cntx, C_SYNTAX, svcs ) ;
    status = DataStack_Pop ( ) ;
    if ( status > 0 ) status = true ;
    SetDataStackPointer ( svStackPointer ) ;
    return status ;
}

#if 0

Ppibs *
Ppibs_Init ( Ppibs * ppibs )
{
    memset ( ppibs, 0, sizeof (Ppibs ) ) ;
    return ppibs ;
}
#endif

Ppibs *
Ppibs_New ( )
{
    Ppibs * ppibs = ( Ppibs * ) Mem_Allocate ( sizeof (Ppibs ), CONTEXT ) ;
    ppibs->Filename = String_New ( _ReadLiner_->Filename, CONTEXT ) ;
    ppibs->LineNumber = _ReadLiner_->LineNumber ;
    //Ppibs_Init ( ppibs ) ;
    return ppibs ;
}

Ppibs *
GetLogicNode ( int64 index )
{
    int64 llen = List_Length ( _Context_->PreprocessorStackList ) ;
    if ( index > llen ) index = llen ; // -1 gets 'top/first/original' node
    if ( llen >= index )
    {
        Ppibs *node = ( Ppibs * ) List_Pick_Value ( _Context_->PreprocessorStackList, index ) ; //List_Top_Value ( _Context_->PreprocessorStackList ) ;
        return node ; //->IfNodeStatus ;
    }
    else 0 ; //return 1 ; //default status ;
}

// top of the _Context_->PreprocessorStackList array/list

Ppibs *
GetTopLogicNode ( )
{
    Ppibs * top = GetLogicNode ( 0 ) ;
    return top ;
}

Boolean
GetTopLogicNodeStatus ( )
{
    Ppibs *top = GetTopLogicNode ( ) ;
    if ( top ) return top->Status ;
    else return 1 ; // default true 
}

Boolean
PP_IfStatus ( int64 evalFlag, int64 condStatus )
{
    Ppibs * newTop = Ppibs_New ( ), *top = GetTopLogicNode ( ) ;
    Boolean cond, topAccumStatus = top ? top->AccumStatus : 1, topStatus = top ? top->Status : 1 ;
    //newTop->Filename = _ReadLiner_->Filename ;
    //newTop->LineNumber = _ReadLiner_->LineNumber ;
    if ( evalFlag )
    {
        cond = _GetCondStatus ( ) ;
        newTop->IfCond = cond ;
        newTop->Status = cond && topStatus ;
    }
    else
    {
        newTop->IfCond = ( condStatus ? 1 : 0 ) ;
        newTop->Status = condStatus && topStatus ;
        CSL_CommentToEndOfLine ( ) ;
    }
    newTop->AccumStatus = topAccumStatus ; // = topStatus ; //accStatus ;
    _List_PushNew_1Value ( _Context_->PreprocessorStackList, CONTEXT, T_PREPROCESSOR, ( int64 ) newTop ) ;
    return newTop->Status ; //&& newTop->AccumStatus ;
}

int64
GetElxxStatus ( int64 cond, int64 type )
{
    Ppibs *top = GetTopLogicNode ( ) ;
    Boolean status ;
    if ( top )
    {
        if ( type == PP_ELIF )
        {
            if ( ( ! top->IfCond ) && ( ! top->ElifCond ) && cond )
            {
                top->ElifCond = cond ;
                status = top->AccumStatus && cond ;
            }
            else
            {
                if ( ! top->IfCond ) top->SvIfCond = 1 ;
                status = 0 ;
            }
        }
        else if ( type == PP_ELSE )
        {
            status = ( ( top->AccumStatus || top->SvIfCond ) && ( ! top->IfCond ) ) ;
        }
        top->Status = status ;
        return status ;
    }
    else _SyntaxError ( ( byte* ) "#elxx without #if", 1 ) ;
    return 0 ;
}

Boolean
GetElifStatus ( )
{
    int64 cond = _GetCondStatus ( ) ;
    return GetElxxStatus ( cond, PP_ELIF ) ;
}

Boolean
GetElseStatus ( )
{
    return GetElxxStatus ( 0, PP_ELSE ) ;
}

Boolean
_GetEndifStatus ( )
{
    Boolean status = GetTopLogicNodeStatus ( ) ;
    return status ;
}

Boolean
GetEndifStatus ( )
{
    Ppibs *node = ( Ppibs* ) List_Pop ( _Context_->PreprocessorStackList ) ;
    if ( Is_DebugOn && node ) oPrintf ( "\t%s.%d", node->Filename, node->LineNumber ) ;
    else if ( ! node ) Error ( "\nError : extra 'endif'", PAUSE ) ;
    Boolean status = _GetEndifStatus ( ) ;
    return status ;
}

// dragons here ... ??

void
SkipPreprocessorCode ( )
{
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    byte * token, * token1 = 0 ;
    int64 ifLevel = 0, svDebugLevel = _CSL_->DebugLevel ;
    int64 svState = GetState ( lexer, ( ADD_TOKEN_TO_SOURCE | ADD_CHAR_TO_SOURCE ) ) ;
    SetState ( _CSL_, DEFINES_MACROS_ON, false ) ;

    //if ( Is_DebugModeOn && Is_DebugShowOn ) _CSL_->DebugLevel = 1 ;
    Lexer_SourceCodeOff ( lexer ) ;
    do
    {
        int64 inChar = ReadLine_PeekNextChar ( cntx->ReadLiner0 ) ;
        if ( ( inChar == - 1 ) || ( inChar == eof ) )
        {
            SetState ( lexer, END_OF_LINE, true ) ;
            goto done ;
        }
        token = Lexer_ReadToken ( lexer ) ;
        if ( Is_DebugOn && token1 )
        {
            iPrintf ( "\nSPC : skipping token1 = %s : token = %s : at %s", token1, token, Context_Location ( ) ) ;
        }
        if ( token )
        {
            if ( ! token1 ) token1 = token ;
            //if ( Is_DebugOn ) iPrintf ( "\nSPC : token = %s :: at %s", token, Context_Location ( ) ) ;
            if ( String_Equal ( token, "//" ) )
            {
                CSL_CommentToEndOfLine ( ) ;
                Lexer_SourceCodeOff ( lexer ) ;
            }
            else if ( String_Equal ( token, "/*" ) )
            {
                CSL_ParenthesisComment ( ) ;
                Lexer_SourceCodeOff ( lexer ) ;
            }
            else if ( String_Equal ( token, "#" ) )
            {
                token1 = Lexer_ReadToken ( lexer ) ;
                //if ( Is_DebugOn && token1 ) iPrintf ( "\nSPC : token1 = %s : token = %s : at %s", token1, token, Context_Location ( ) ) ;
                if ( token1 )
                {
                    if ( ( String_Equal ( token1, "if" ) ) )
                    {
                        if ( PP_IfStatus ( 1, 0 ) ) goto done ;
                        ifLevel ++ ;
                    }
#if 1 // untested                  
                    else if ( String_Equal ( token1, "ifdef" ) )
                    {
                        int64 defined = _CSL_Defined ( ) ;
                        if ( PP_IfStatus ( 0, defined ) ) goto done ;
                        ifLevel ++ ;
                    }
                    else if ( String_Equal ( token1, "ifndef" ) )
                    {
                        int64 defined = _CSL_Defined ( ) ;
                        if ( PP_IfStatus ( 0, ! defined ) ) goto done ;
                        ifLevel ++ ;
                    }
#endif                    
                    else if ( String_Equal ( token1, "else" ) )
                    {
                        if ( ifLevel ) continue ;
                        if ( GetElseStatus ( ) ) goto done ;
                    }
                    else if ( String_Equal ( token1, "elif" ) )
                    {
                        if ( ifLevel ) continue ;
                        if ( GetElifStatus ( ) ) goto done ;
                    }
                    else if ( String_Equal ( token1, "endif" ) )
                    {
                        if ( GetEndifStatus ( ) ) goto done ;
                        ifLevel -- ;
                    }
                    else if ( String_Equal ( token1, "define" ) ) continue ;
                    else if ( String_Equal ( token1, "defined" ) ) continue ;
                    else if ( String_Equal ( token1, "undef" ) ) continue ;
                    else if ( String_Equal ( token1, "include" ) ) continue ;
                    else if ( String_Equal ( token1, "error" ) ) continue ;
                    else if ( String_Equal ( token1, "warning" ) ) continue ;
                    else _SyntaxError ( ( byte* ) "Stray '#' in code!", 1 ) ;
                }
                else goto done ;
            }
        }
    }
    while ( token ) ;
done:
    if ( Compiling ) SetState ( lexer, ( ADD_TOKEN_TO_SOURCE | ADD_CHAR_TO_SOURCE ), svState ) ;
    SetState ( _CSL_, SOURCE_CODE_ON | DEFINES_MACROS_ON, true ) ;
    //SetState ( _CSL_, SOURCE_CODE_ON, true ) ;
    //DebugOff ;
    _CSL_->DebugLevel = svDebugLevel ;
}

void
CSL_If_ConditionalInterpret ( )
{
    if ( ! PP_IfStatus ( 1, 0 ) ) SkipPreprocessorCode ( ) ;
    //if ( ! PP_IfStatus ( 1, 0 ) ) { SetState ( _CSL_, SOURCE_CODE_ON, false ) ; SkipPreprocessorCode () ; }
}

void
CSL_Elif_ConditionalInterpret ( )
{
    if ( ! GetElifStatus ( ) ) SkipPreprocessorCode ( ) ;
    //if ( ! GetElifStatus ( ) ) { SetState ( _CSL_, SOURCE_CODE_ON, false ) ; SkipPreprocessorCode () ; }
}

void
CSL_Else_ConditionalInterpret ( )
{
    if ( ! GetElseStatus ( ) ) SkipPreprocessorCode ( ) ;
    //if ( ! GetElseStatus ( ) ) { SetState ( _CSL_, SOURCE_CODE_ON, false ) ; SkipPreprocessorCode () ; }
}

void
CSL_Endif_ConditionalInterpret ( )
{
    if ( ! GetEndifStatus ( ) ) SkipPreprocessorCode ( ) ;
}

byte *
PP_ReadToken ( )
{
    int64 svppd = GetState ( _Interpreter_, PREPROCESSOR_DEFINE ) ;
    SetState ( _Interpreter_, PREPROCESSOR_DEFINE, true ) ;
    byte * token = Lexer_ReadToken ( _Lexer_ ) ;
    SetState ( _Interpreter_, PREPROCESSOR_DEFINE, svppd ) ;
    return token ;
}

void
CSL_PP_Define ( )
{
    Context * cntx = _Context_ ;
    Interpreter * interp = cntx->Interpreter0 ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    Word * word ;
    byte * macroStr ;
    Namespace * svns = CSL_In_Namespace ( ) ;
    Namespace * ns = Namespace_FindOrNew_SetUsing ( "Defines", Namespace_Find ( "Root" ), 1 ) ;
    Boolean locals ;
    Context_SetSpecialTokenDelimiters ( cntx, " \r\t\n", CONTEXT ) ;

    CSL_RightBracket ( ) ;
    CSL_SourceCode_Init ( ) ;
    CSL_Token ( ) ;
    CSL_Word_New ( ) ;
    CSL_BeginBlock ( ) ;

    byte c = _ReadLine_PeekOffsetChar ( rl, 0 ), c1 = _ReadLine_PeekOffsetChar ( rl, 1 ) ;
    if ( ( c == '(' ) )
    {
        Lexer_ReadToken ( _Lexer_ ) ;
        CSL_LocalsAndStackVariablesBegin ( ) ;
        locals = true ;
    }
    else locals = false ;
    if ( ( c != '\n' ) &&( ! ( ( c == '/' ) && ( c1 == '/' ) ) ) ) //Lexer_NextPrintChar ( _Lexer_ ) == '/' ) ) ) ) // newline or comment
    {
        SetState ( interp, PREPROCESSOR_DEFINE, true ) ;
        if ( ! locals )
        {
            byte * b = Buffer_DataCleared ( _CSL_->Preprocessor ) ;
            strncpy ( b, &rl->InputLine [ rl->ReadIndex ], BUFFER_SIZE ) ;
            String_RemoveFinalNewline ( b ) ;
            if ( b[0] == 0 ) strncat ( ( char* ) b, "1 \n", BUFFER_SIZE ) ; // 1 : set the default value as true (1) : eg. #define _ARRAY_H (namespaces/test/arrayTest.csl line 7)
            else strncat ( ( char* ) b, " \n", BUFFER_SIZE ) ;
            macroStr = String_New ( b, DICTIONARY ) ;
        }
        Interpret_ToEndOfLine ( interp ) ;
        SetState ( interp, PREPROCESSOR_DEFINE, false ) ;
    }
    CSL_SemiColon ( ) ;
    if ( locals ) CSL_Prefixable ( ) ; // if we have local variables make it a prefix word ; maybe : if ( GetState ( _Context_, C_SYNTAX ) ) 
    else
    {
        SetState ( _CSL_, DEFINES_MACROS_ON, true ) ;
        word = _CSL_->LastFinished_Word ;
        if ( word )
        {
            word->W_ObjectAttributes |= ( LITERAL | CONSTANT ) ;
            word->TextMacroValue = macroStr ;
            word->W_ObjectAttributes |= TEXT_MACRO ;
            Namespace_DoAddWord ( ns, word ) ;
        }
    }
    CSL_Inline ( ) ;
    CSL_SaveDebugInfo ( _CSL_->LastFinished_Word ) ; // how would this kind of thing work with an inline word??
    CSL_SourceCode_Init ( ) ; //don't leave the define in sc
    _CSL_Namespace_InNamespaceSet ( svns ) ;
    Context_SetSpecialTokenDelimiters ( cntx, 0, CONTEXT ) ;
}

int64
_CSL_Defined ( )
{
    byte * token = PP_ReadToken ( ) ; // Lexer_ReadToken ( _Lexer_ ) ;
    //byte * token = Lexer_Peek_NextToken ( _Lexer_, 0 ) ; //Lexer_ReadToken ( _Lexer_ ) ;
    Word * w = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
    //d1 ( DataStack_Check ( ) ) ;
    return ( int64 ) w ;
}

void
CSL_Undef ( )
{
    Word * w = ( Word* ) _CSL_Defined ( ) ;
    if ( w ) Word_Recycle ( w ) ; //dlnode_Remove ( ( dlnode* ) w ) ;
}

int64
CSL_Defined ( )
{
    int64 d = _CSL_Defined ( ) ;
    DataStack_Push ( d ) ;
}

void
CSL_IfDef_Preprocessor ( )
{
    int64 defined = _CSL_Defined ( ) ;
    if ( ! ( PP_IfStatus ( 0, defined ) ) ) SkipPreprocessorCode ( ) ;
}

// ? untested 

void
CSL_Ifndef_Preprocessor ( )
{
    int64 defined = _CSL_Defined ( ) ;
    if ( ! ( PP_IfStatus ( 0, ! defined ) ) ) SkipPreprocessorCode ( ) ; //PP_IFDEF ) ;
}

#if 1 // include preprocessor

void
AddFilenameTo_C_Preprocessor_IncludedList ( byte * name )
{
    Namespace * ns = _CSL_->C_Preprocessor_IncludedList ;
    if ( ! ns ) ns = _CSL_->C_Preprocessor_IncludedList = _Namespace_New ( "cpidsl", 0 ) ;
    _Word_New ( name, 0, 0, 0, 0, ns, DICTIONARY ) ;
}

void
AddDirectoriesToPreprocessor_IncludeDirectory_SearchList ( )
{
    Namespace * ns = _CSL_->C_Preprocessor_IncludeDirectory_SearchList ;
    if ( ! ns ) ns = _CSL_->C_Preprocessor_IncludeDirectory_SearchList = _Namespace_New ( "cpidsl", 0 ) ;
    _Word_New ( "/usr/local/include/", 0, 0, 0, 0, ns, DICTIONARY ) ;
    _Word_New ( "/usr/include/", 0, 0, 0, 0, ns, DICTIONARY ) ;
}

byte *
CheckFilenameExists ( Symbol * symbol, byte * fname )
{
    byte * fn = ( char* ) Buffer_DataCleared ( _CSL_->ScratchB5 ) ;
    strncpy ( fn, symbol->Name, BUFFER_SIZE ) ;
    strncat ( fn, fname, BUFFER_SIZE ) ;
    FILE * file = fopen ( ( char* ) fn, "r" ) ;
    if ( file ) return fn ;
    else return 0 ;
}

void
CSL_C_Include_PreProcessor ( )
{
    char * _filename = ( char* ) _CSL_FilenameToken ( ), *fn = ( char* ) Buffer_DataCleared ( _CSL_->ScratchB5 ), *filename, *afn ;
    //char * _filename = ( char* ) _CSL_Token ( ), *fn = ( char* ) Buffer_Data_Cleared ( _CSL_->ScratchB5 ), *filename, *afn ;
    strncpy ( fn, _filename, BUFFER_SIZE ) ;
    if ( fn [0] == '<' )
    {
        if ( ! _CSL_->C_Preprocessor_IncludeDirectory_SearchList ) AddDirectoriesToPreprocessor_IncludeDirectory_SearchList ( ) ;
        fn [strlen ( ( char* ) fn ) - 1] = 0 ;
        filename = & fn[1] ;
        dllist * dirl = _CSL_->C_Preprocessor_IncludeDirectory_SearchList->W_List ;
        afn = ( byte* ) dllist_Map1_WReturn ( dirl, ( MapFunction1 ) CheckFilenameExists, ( int64 ) filename ) ;
    }
    else if ( fn[0] = '\"' )
    {
        Interpreter_InterpretAToken ( _Interpreter_, fn, - 1, - 1 ) ;
        afn = ( char* ) DataStack_Pop ( ) ;
        CSL_CommentToEndOfLine ( ) ; // shouldn't be anything after the filename but if there is ignore it
    }
    else afn = fn ;
    CSL_C_Syntax_On ( ) ;
    if ( afn )
    {
        uint64 *svStackPointer = GetDataStackPointer ( ) ;
        if ( GetState ( _CSL_, PP_INCLUDE_FILES_ONLY ) )
        {
            if ( ! DLList_FindName_InOneNamespace ( _CSL_->C_Preprocessor_IncludedList, afn ) )
            {
                if ( _O_->Verbosity > 1 ) iPrintf ( ( byte* ) "\nEntering : %s at %s", afn, Context_Location ( ) ) ;
                AddFilenameTo_C_Preprocessor_IncludedList ( afn ) ;
                _CSL_Contex_NewRun_3 ( _CSL_, _Context_IncludeFile, afn, 2, 0 ) ;
            }
        }
        else CSL_ContextNew_IncludeFile ( afn, 0 ) ;
        SetDataStackPointer ( svStackPointer ) ;
    }
}
#endif
#define DEBUG_SAVE_DONT_DELETE 0
#if DEBUG_SAVE_DONT_DELETE
#define dbg(0, x, 0) x

void
Ppibs_Print ( Ppibs * ppibs, byte * prefix )
{
    int64 depth = List_Length ( _Context_->PreprocessorStackList ) ;
    iPrintf ( "\n\nInputLineString = %s", _ReadLiner_->InputLineString ) ;
    iPrintf ( "\n%s : Ppibs = 0x%016lx : depth = %d : Filename = %s : Line = %d", prefix, ppibs, depth, ppibs->Filename, ppibs->LineNumber ) ;
    iPrintf ( "\nIfBlockStatus = %d", ppibs->Status ) ;
    iPrintf ( "\nElifStatus = %d", ppibs->Status ) ;
    iPrintf ( "\nElseStatus = %d", ppibs->Status ) ;
    //Pause () ;
}
#else 
//#define dbg( x )
#endif
/* preprocessor BNF :
 *  ppBlock      =:=     #if (elifBlock)* (elseBlock)? #endif
 *  elifBlock    =:=     #elif (ppBlock)*
 *  elseBlock    =:=     #else (ppBlock)*
 */
// "#if" stack pop is 'true' interpret until "#else" and this does nothing ; if stack pop 'false' skip to "#else" token skip those tokens and continue interpreting

