
#include "../../include/csl.h"

#define ReadLine_AppendPoint( rl ) &rl->InputBuffer [ rl->EndPosition ]

void
_ReadLine_NullDelimitInputBuffer ( ReadLiner * rl )
{
    rl->InputLine [ rl->EndPosition ] = 0 ;
}

void
_ReadLine_QuickAppendCharacter ( ReadLiner * rl, byte chr )
{
    rl->InputLine [ rl->EndPosition ++ ] = chr ;
    rl->InputLine [ rl->EndPosition ] = 0 ;
}

void
_ReadLine_SetOutputLineCharacterNumber ( ReadLiner * rl )
{
    rl->OutputLineCharacterNumber = Strlen ( ( char* ) rl->Prompt ) + rl->EndPosition ;
}

void
__ReadLine_AppendCharacter ( ReadLiner * rl, byte chr )
{
    _ReadLine_QuickAppendCharacter ( rl, chr ) ;
    rl->EndPosition ++ ;
    _ReadLine_SetOutputLineCharacterNumber ( rl ) ;
    _ReadLine_SetMaxEndPosition ( rl ) ;
}

void
__ReadLine_AppendCharacter_Actual ( ReadLiner * rl, byte chr )
{
    _ReadLine_QuickAppendCharacter ( rl, chr ) ;
    _ReadLine_SetOutputLineCharacterNumber ( rl ) ;
    _ReadLine_SetMaxEndPosition ( rl ) ;
}

void
_ReadLine_AppendCharacter ( ReadLiner * rl )
{
    __ReadLine_AppendCharacter ( rl, rl->InputKeyedCharacter ) ;
}

void
_ReadLine_AppendCharacter_Actual ( ReadLiner * rl )
{
    __ReadLine_AppendCharacter_Actual ( rl, rl->InputKeyedCharacter ) ;
}

void
ReadLine_DoCursorMoveInput ( ReadLiner * rl, int64 newCursorPosition )
{
    ReadLine_ClearCurrentTerminalLine ( rl, rl->CursorPosition ) ;
    ReadLine_SetCursorPosition ( rl, newCursorPosition ) ;
    ReadLine_ShowLine ( rl ) ;
    ReadLine_ShowCursor ( rl ) ;
}

void
ReadLine_SetCursorPosition ( ReadLiner * rl, int64 pos )
{
    if ( pos < 0 ) pos = 0 ; // prompt width 
    rl->CursorPosition = pos ;
}

void
ReadLiner_CommentToEndOfLine ( ReadLiner * rl )
{
    ReadLine_Set_ReadIndex ( rl, BUFFER_IX_SIZE ) ;
    ReadLiner_Done ( rl ) ;
}

int64
ReadLiner_PeekSkipSpaces ( ReadLiner * rl )
{
    int64 i ;
    for ( i = 0 ; _ReadLine_PeekOffsetChar ( rl, i ) == ' ' ; i ++ ) ;
    return i ;
}

void
ReadLiner_Done ( ReadLiner * rl )
{
    SetState ( rl, READLINER_DONE, true ) ;
}

Boolean
ReadLiner_IsDone ( ReadLiner * rl )
{
    return ( ( GetState ( rl, READLINER_DONE ) ) || ( rl->EndPosition >= BUFFER_IX_SIZE ) || ( rl->ReadIndex >= BUFFER_IX_SIZE ) ) ;
}

void
_ReadLine_MoveInputStartToLineStart ( int64 fromPosition ) //, int64 lineUpFlag )
{
    // nb. this is *necessary* when user scrolls up with scrollbar in eg. konsole and then hits up/down arrow
    //if ( lineUpFlag )
    {
        int64 n, columns = GetTerminalWidth ( ) ;
        if ( fromPosition && columns )
        {
            n = ( fromPosition ) / ( columns ) ;
            if ( ( fromPosition % columns ) < 2 ) n -- ; // nb : ?? -- i don't understand this but it works
            if ( n ) Cursor_Up ( n ) ; //_Printf ( "\r%c[%dA", ESC, n ) ; // move n lines up 
        }
        return ;
    }
    iPrintf ( "\r" ) ; // nb -- a workaround : ?? second sequence ( clear 2 eol ) not necessary but seems to reset things to work -- ??
    //_Printf ( "\r%c[2K", ESC ) ; // nb -- a workaround : ?? second sequence ( clear 2 eol ) not necessary but seems to reset things to work -- ??
}

void
ReadLine_ClearCurrentTerminalLine ( ReadLiner * rl, int64 fromPosition )
{
    _ReadLine_MoveInputStartToLineStart ( fromPosition + PROMPT_LENGTH + 1 ) ; // 1 : zero array indexing
    Clear_Terminal_Line ( ) ;
}

void
ReadLine_SetInputLine ( ReadLiner * rl, byte * buffer )
{
    strcpy ( ( char* ) rl->InputLine, ( char* ) buffer ) ;
}

void
ReadLine_InputLine_Clear ( ReadLiner * rl )
{
    Mem_Clear ( rl->InputLine, BUFFER_IX_SIZE ) ;
}

void
ReadLine_InputLine_Init ( ReadLiner * rl )
{
    ReadLine_InputLine_Clear ( rl ) ;
    //rl->InputLineCharacterNumber = 0 ;
    ReadLine_Set_ReadIndex ( rl, 0 ) ;
    rl->InputLineString = rl->InputLine ;
    SetState ( rl, READLINER_DONE, false ) ;
}

void
ReadLine_RunInit ( ReadLiner * rl )
{
    rl->HistoryNode = 0 ;
    rl->EscapeModeFlag = 0 ;
    _ReadLine_CursorToStart ( rl ) ;
    rl->EndPosition = 0 ;
    rl->MaxEndPosition = 0 ;
    SetState ( rl, END_OF_INPUT | END_OF_LINE | TAB_WORD_COMPLETION, false ) ; // ?? here ??
    ReadLine_InputLine_Init ( rl ) ;
}

void
ReadLine_Init ( ReadLiner * rl, ReadLiner_KeyFunction ipf )
{
    ReadLine_RunInit ( rl ) ;
    SetState ( rl, CHAR_ECHO, true ) ; // this is how we see our input at the command line!
    rl->LineNumber = 0 ;
    rl->InputFile = stdin ;
    rl->OutputFile = stdout ;
    rl->Filename = 0 ;
    rl->FileCharacterNumber = 0 ;
    rl->NormalPrompt = ( byte* ) "<: " ;
    rl->AltPrompt = ( byte* ) ":> " ;
    rl->DebugPrompt = ( byte* ) "==> " ;
    rl->DebugAltPrompt = ( byte* ) "<== " ;
    rl->Prompt = rl->NormalPrompt ;
    rl->InputStringOriginal = 0 ;
    if ( ipf ) ReadLine_SetRawInputFunction ( rl, ipf ) ;
}

ReadLiner *
ReadLine_New ( uint64 type )
{
    ReadLiner * rl = ( ReadLiner * ) Mem_Allocate ( sizeof (ReadLiner ), type ) ;
    rl->TabCompletionInfo0 = TabCompletionInfo_New ( type ) ;
    rl->TciNamespaceStack = Stack_New ( 64, type ) ;
    //rl->InputLine = Buffer_Data_Cleared ( _CSL_->InputLine ) ;
    ReadLine_Init ( rl, _CSL_Key ) ;
    return rl ;
}

void
_ReadLine_Copy ( ReadLiner * rl, ReadLiner * rl0, uint64 type )
{
    MemCpy ( rl, rl0, sizeof (ReadLiner ) ) ;
    rl->TabCompletionInfo0 = TabCompletionInfo_New ( type ) ;
    rl->TciNamespaceStack = Stack_New ( 64, COMPILER_TEMP ) ;
    strcpy ( ( char* ) rl->InputLine, ( char* ) rl0->InputLine ) ;
}

ReadLiner *
ReadLine_Copy ( ReadLiner * rl0, uint64 type )
{
    ReadLiner * rl = ( ReadLiner * ) Mem_Allocate ( sizeof (ReadLiner ), type ) ;
    _ReadLine_Copy ( rl, rl0, type ) ;
    return rl ;
}

void
ReadLine_TabWordCompletion ( ReadLiner * rl )
{
    if ( ! GetState ( rl, TAB_WORD_COMPLETION ) ) RL_TabCompletionInfo_Init ( rl ) ;
    RL_TabCompletion_Run ( rl, 0, rl->TabCompletionInfo0->NextWord ) ; //? rl->TabCompletionInfo0->NextWord : rl->TabCompletionInfo0->RunWord ) ; // the main workhorse here
}

void
__ReadLine_AppendCharacterAndCursoRight ( ReadLiner * rl, byte c )
{
    __ReadLine_AppendCharacter ( rl, c ) ;
    _ReadLine_CursorRight ( rl ) ;
    _ReadLine_SetEndPosition ( rl ) ;
}

void
_ReadLine_AppendCharacterAndCursoRight ( ReadLiner * rl )
{
    __ReadLine_AppendCharacterAndCursoRight ( rl, rl->InputKeyedCharacter ) ;
}

void
_ReadLine_AppendAndShowCharacter ( ReadLiner * rl, byte c )
{
    __ReadLine_AppendCharacterAndCursoRight ( rl, c ) ;
    _ReadLine_ShowCharacter ( rl, c ) ;
}

void
ReadLine_AppendAndShowCharacter ( ReadLiner * rl )
{
    _ReadLine_AppendAndShowCharacter ( rl, rl->InputKeyedCharacter ) ;
}

byte *
ReadLine_GetPrompt ( ReadLiner * rl )
{
    if ( rl->CursorPosition < rl->EndPosition ) rl->Prompt = ReadLine_GetAltPrompt ( rl ) ;
    else rl->Prompt = ReadLine_GetNormalPrompt ( rl ) ;
    return rl->Prompt ;
}

void
ReadLine_SetPrompt ( ReadLiner * rl, byte * newPrompt )
{
    rl->NormalPrompt = newPrompt ;
}

byte *
ReadLine_GetAltPrompt ( ReadLiner * rl )
{
    return (GetState ( _Debugger_, DBG_ACTIVE ) ? rl->DebugPrompt : rl->AltPrompt ) ;
}

byte *
ReadLine_GetNormalPrompt ( ReadLiner * rl )
{
    return (GetState ( _Debugger_, DBG_ACTIVE ) ? rl->DebugPrompt : rl->NormalPrompt ) ;
}

void
_ReadLine_Show ( ReadLiner * rl, byte * prompt )
{
    iPrintf ( "\r%s%s", prompt, rl->InputLine ) ;
}

void
_ReadLine_ShowLine ( ReadLiner * rl, byte * prompt )
{
    _ReadLine_Show ( rl, prompt ) ;
    _ReadLine_SetEndPosition ( rl ) ;
}

void
ReadLine_ShowLine ( ReadLiner * rl )
{
    _ReadLine_ShowLine ( rl, ReadLine_GetPrompt ( rl ) ) ;
}

void
_ReadLine_ClearAndShowLine ( ReadLiner * rl, byte * prompt )
{
    ReadLine_ClearCurrentTerminalLine ( rl, rl->CursorPosition ) ;
    _ReadLine_ShowLine ( rl, prompt ) ;
}

void
__ReadLine_DoStringInput ( ReadLiner * rl, byte * string, byte * prompt )
{
    ReadLine_ClearCurrentTerminalLine ( rl, rl->CursorPosition ) ;
    ReadLine_InputLine_Clear ( rl ) ;
    strcpy ( ( char* ) rl->InputLine, ( char* ) string ) ;
    _ReadLine_ShowLine ( rl, prompt ) ;
}

void
ReadLine_ClearAndShowLine ( ReadLiner * rl )
{
    ReadLine_ClearCurrentTerminalLine ( rl, rl->CursorPosition ) ;
    ReadLine_ShowLine ( rl ) ;
}

void
_ReadLine_ShowCursor ( ReadLiner * rl, byte * prompt )
{
    _ReadLine_MoveInputStartToLineStart ( rl->EndPosition + PROMPT_LENGTH + 1 ) ;
    byte saveChar = rl->InputLine [ rl->CursorPosition ] ; // set up to show cursor at end of new word
    rl->InputLine [ rl->CursorPosition ] = 0 ; // set up to show cursor at end of new word
    _ReadLine_Show ( rl, prompt ) ;
    rl->InputLine [ rl->CursorPosition ] = saveChar ; // set up to show cursor at end of new word
}

void
ReadLine_ShowCursor ( ReadLiner * rl )
{
    _ReadLine_ShowCursor ( rl, ReadLine_GetPrompt ( rl ) ) ;
}

void
_ReadLine_DoStringInput ( ReadLiner * rl, byte * string, byte * prompt )
{
    __ReadLine_DoStringInput ( rl, string, prompt ) ;
    _ReadLine_ShowCursor ( rl, prompt ) ;
}

void
ReadLine_ShowStringWithCursor ( ReadLiner * rl, byte * string )
{
    _ReadLine_DoStringInput ( rl, string, ReadLine_GetPrompt ( rl ) ) ;
}

void
ReadLine_ClearAndShowLineWithCursor ( ReadLiner * rl )
{
    ReadLine_ClearAndShowLine ( rl ) ;
    ReadLine_ShowCursor ( rl ) ;
}

void
ReadLine_ShowNormalPrompt ( ReadLiner * rl )
{
    //_ReadLine_ShowStringWithCursor ( rl, ( byte* ) "", rl->NormalPrompt ) ;
    Clear_Terminal_Line ( ) ;
    iPrintf ( "\r%s", rl->NormalPrompt ) ;
    rl->EndPosition = 0 ;
    rl->InputLine [ 0 ] = 0 ;
}

void
ReadLine_InsertCharacter ( ReadLiner * rl )
// insert rl->InputCharacter at cursorPostion
{
    String_InsertCharacter ( ( CString ) rl->InputLine, rl->CursorPosition, rl->InputKeyedCharacter ) ;
    ReadLine_ClearAndShowLineWithCursor ( rl ) ;
}

void
ReadLine_SaveCharacter ( ReadLiner * rl )
{
    if ( rl->CursorPosition < rl->EndPosition )
    {
        ReadLine_InsertCharacter ( rl ) ;
        _ReadLine_CursorRight ( rl ) ;
        ReadLine_ClearAndShowLineWithCursor ( rl ) ;
        return ;
    }
    ReadLine_AppendAndShowCharacter ( rl ) ;
}

void
_ReadLine_InsertStringIntoInputLineSlotAndShow ( ReadLiner * rl, int64 startOfSlot, int64 endOfSlot, byte * data )
{
    String_InsertStringIntoStringSlot ( rl->InputLineString, startOfSlot, endOfSlot, data, 0 ) ; // size in bytes
    ReadLine_ClearAndShowLineWithCursor ( rl ) ;
}

void
ReadLiner_InsertTextMacro ( ReadLiner * rl, Word * word )
{
    int64 nlen = ( Strlen ( ( char* ) word->Name ) + 1 ) ;
    String_InsertStringIntoStringSlot ( rl->InputLineString, rl->ReadIndex - nlen, rl->ReadIndex, ( byte* ) word->W_Value, 0 ) ; // size in bytes
    rl->ReadIndex -= nlen ;
    _CSL_UnAppendFromSourceCode_NChars ( _CSL_, nlen ) ;
}

void
ReadLine_DeleteChar ( ReadLiner * rl )
{
    byte * b = Buffer_DataCleared ( _CSL_->ScratchB2 ) ;
    if ( -- rl->EndPosition < 0 ) rl->EndPosition = 0 ;
    if ( rl->CursorPosition > rl->EndPosition )
    {
        if ( -- rl->CursorPosition < 0 ) _ReadLine_CursorToStart ( rl ) ;
    }
    rl->InputLine [ rl->CursorPosition ] = 0 ;
    // prevent string overwriting itself while coping ...
    strncpy ( ( char* ) b, ( char* ) & rl->InputLine [ rl->CursorPosition + 1 ], BUFFER_IX_SIZE ) ;
    if ( rl->CursorPosition < rl->EndPosition ) strncat ( ( char* ) rl->InputLine, ( char* ) b, BUFFER_IX_SIZE ) ;
    ReadLine_ClearAndShowLineWithCursor ( rl ) ;
}

int64
ReadLine_IsLastCharADot ( ReadLiner * rl, int64 pos )
{
    return String_IsLastCharADot ( rl->InputLine, pos ) ;
}

int64
ReadLine_FirstCharOfToken_FromLastChar ( ReadLiner * rl, int64 pos )
{
    return String_FirstCharOfToken_FromPos ( rl->InputLine, pos ) ;
}

int64
ReadLine_IsThereADotSeparator ( ReadLiner * rl, int64 pos )
{
    return String_IsThereADotSeparatorBackFromPosToLastNonDelmiter ( rl->InputLine, pos ) ;
}

int64
ReadLine_LastCharOfLastToken_FromPos ( ReadLiner * rl, int64 pos )
{
    return String_LastCharOfLastToken_FromPos ( rl->InputLine, pos ) ;
}

int64
ReadLine_EndOfLastToken ( ReadLiner * rl )
{
    return ReadLine_LastCharOfLastToken_FromPos ( rl, rl->CursorPosition ) ;
}

int64
ReadLine_BeginningOfLastToken ( ReadLiner * rl )
{
    return ReadLine_FirstCharOfToken_FromLastChar ( rl, ReadLine_EndOfLastToken ( rl ) ) ;
}

Boolean
ReadLine_IsReverseTokenQualifiedID ( ReadLiner * rl )
{
    return String_IsReverseTokenQualifiedID ( rl->InputLine, rl->ReadIndex ) ; //int64 pos ) ;
}

void
Readline_SaveInputLine ( ReadLiner * rl )
{
    rl->svLine = Buffer_DataCleared ( _CSL_->svLineB ) ;
    strcpy ( ( char* ) rl->svLine, ( char* ) rl->InputLine ) ;
}

void
Readline_Save_InputLine_State ( ReadLiner * rl )
{
    rl->svReadIndex = rl->ReadIndex ;
    rl->svState = rl->State ;
    Readline_SaveInputLine ( rl ) ;
}

void
Readline_RestoreInputLine ( ReadLiner * rl )
{
    strcpy ( ( char* ) rl->InputLine, ( char* ) rl->svLine ) ;
}

void
Readline_Restore_InputLine_State ( ReadLiner * rl )
{
    rl->ReadIndex = rl->svReadIndex ;
    rl->State = rl->svState ;
    Readline_RestoreInputLine ( rl ) ;
}

Boolean
_Readline_CheckArrayDimensionForVariables ( ReadLiner * rl )
{
    byte *p, * ilri = & rl->InputLine [ rl->ReadIndex ], * prb = ( byte* ) strchr ( ( char* ) &rl->InputLine [ rl->ReadIndex ], ']' ) ;
    if ( prb )
    {
        for ( p = ilri ; p != prb ; p ++ ) if ( isalpha ( * p ) ) return true ;
    }
    return false ;
}

Boolean
_Readline_Is_AtEndOfBlock ( ReadLiner * rl0 )
{
    ReadLiner * rl = ReadLine_Copy ( rl0, COMPILER_TEMP ) ;
    Word * word = CSL_WordList ( 0 ) ;
    if ( word )
    {
        int64 iz, ib, index = word->W_RL_Index + Strlen ( word->Name ), sd = _Stack_Depth ( _Context_->Compiler0->BlockStack ) ;
        byte c ;
        Boolean zf = false ; // zero flag
        for ( ib = false, iz = false ; 1 ; iz = false )
        {
            if ( ! zf ) c = rl->InputLine [ index ++ ] ;
            else c = rl->InputStringCurrent [ index ++ ] ;
            if ( ! c )
            {
                if ( zf ) return false ;
                else zf = true ;
                index = 0 ;
                continue ;
                //return false ;
            }
            if ( ( c == ';' ) && ( ! GetState ( _Context_, C_SYNTAX ) ) ) return true ;
            if ( c == '}' )
            {
                if ( -- sd <= 1 ) return true ;
                ib = 1 ; // b : bracket
                continue ;
            }
            if ( ( c == '/' ) && ( rl->InputLine [ index ] == '/' ) ) CSL_CommentToEndOfLine ( ) ;
            else if ( ib && ( c > ' ' ) && ( c != ';' ) ) return false ;
        }
    }
    return false ;
}

byte
ReadLine_Set_KeyedChar ( ReadLiner * rl, byte c )
{
    rl->InputKeyedCharacter = c ;
    rl->FileCharacterNumber ++ ;
    return rl->InputKeyedCharacter ;
}

byte
ReadLine_Get_Key ( ReadLiner * rl )
{
    if ( ! rl->Key )
    {
        ReadLiner_Done ( rl ) ;
        return 0 ;
    }
    else return ReadLine_Set_KeyedChar ( rl, rl->Key ? rl->Key ( rl ) : 0 ) ;
}

byte
ReadLine_GetNextCharFromString ( ReadLiner * rl )
{
    rl->InputStringIndex ++ ;
    if ( rl->InputStringIndex <= rl->InputStringLength ) return * rl->InputStringCurrent ++ ;
    else return 0 ;
}

void
ReadLine_SetRawInputFunction ( ReadLiner * rl, ReadLiner_KeyFunction ripf )
{
    rl->Key = ripf ;
}

void
ReadLine_SetInputString ( ReadLiner * rl, byte * string0, int64 size )
{
    byte * string ;
    int size1 ;
    if ( csl_returnValue == 3 )
    {
        string = & string0[1] ;
        size1 = Strlen ( ( char* ) string0 ) - 1 ;
        string0[size1] = 0 ;
        size = 0 ;
        //csl_returnValue = -1 ;
    }
    else string = string0 ;
    rl->InputStringOriginal = string ;
    rl->InputStringCurrent = rl->InputStringOriginal ;
    rl->InputStringIndex = 0 ;
    rl->InputStringLength = size ? size : Strlen ( ( char* ) string ) ;
}

// probably the next three functions should be integrated or reworked considering/using each other

void
ReadLine_ReadFileIntoAString ( ReadLiner * rl, FILE * file )
{
    int64 size, result ;
    size = _File_Size ( file ) ;
    byte * fstr = Mem_Allocate ( size, CONTEXT ) ; //COMPILER_TEMP ) ; // 2 : an extra so readline doesn't read into another area of allocated mem
    result = fread ( fstr, 1, size, file ) ;
    if ( result != size )
    {
        fstr = 0 ;
        size = 0 ;
    }
    ReadLine_SetInputString ( rl, fstr, size ) ;
}

void
_Readline_Setup_OneStringInterpret ( ReadLiner * rl, byte * str )
{
    ReadLine_Set_ReadIndex ( rl, 0 ) ;
    SetState ( rl, STRING_MODE, true ) ;
    ReadLine_SetInputLine ( rl, str ) ;
}

void
Readline_Setup_OneStringInterpret ( ReadLiner * rl, byte * str )
{
    Readline_Save_InputLine_State ( rl ) ;
    ReadLine_SetRawInputFunction ( rl, 0 ) ; //ReadLine_GetNextCharFromString ) ;
    ReadLine_SetInputString ( rl, str, 0 ) ;
    _Readline_Setup_OneStringInterpret ( rl, rl->InputStringOriginal ) ;
}

void
_ReadLine_TabCompletion_Check ( ReadLiner * rl )
{
    TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
    if ( rl->InputKeyedCharacter != '\t' )
    {
        if ( GetState ( rl, TAB_WORD_COMPLETION | TAB_COMPLETION_CHANGE_STATE ) )
        {
            SetState ( rl, TAB_COMPLETION_CHANGE_STATE, true ) ;
            if ( ( rl->InputKeyedCharacter == ' ' ) && ( tci->RunWord || tci->TrialWord ) )
            {
                if ( tci->StringFirstChar )
                {
                    byte * s = String_FirstEscapeCharFromPos ( &rl->InputLine[tci->StringFirstChar], 0 ) ;
                    byte * s1 = String_FormattingRemoved ( s ) ;
                    RL_TC_StringInsert_AtCursor ( rl, s1 ) ;
                }
                else
                {
                    Word * w ;
                    if ( tci->TrialWord ) w = tci->TrialWord ;
                    else w = tci->RunWord ;
                    RL_TC_StringInsert_AtCursor ( rl, w->Name ) ;
                    //ReadLiner_SetLastChar ( ' ' ) ; // _Printf does a ReadLiner_SetLastChar ( 0 )
                }
            }
            else if ( rl->InputKeyedCharacter == '\r' ) rl->InputKeyedCharacter = '\n' ; // leave line as is and append a space instead of '\r'
        }
    }
}

byte
_ReadLine_GetLine ( ReadLiner * rl, byte c )
// we're here until we get a newline char ( '\n' or '\r' ), a eof or a buffer overflow
// note : ReadLinePad [ 0 ] starts after the prompt ( "-: " | "> " ) and doesn't include them
{
    ReadLine_RunInit ( rl ) ;
    rl->LineStartFileIndex = rl->InputStringIndex ;
    while ( ! ReadLiner_IsDone ( rl ) )
    {
        if ( ! c ) ReadLine_Get_Key ( rl ) ;
        else ReadLine_Set_KeyedChar ( rl, c ), c = 0 ;
        if ( ( ! csl_returnValue ) && AtCommandLine ( rl ) ) _ReadLine_TabCompletion_Check ( rl ) ;
        _CSL_->ReadLine_FunctionTable [ _CSL_->ReadLine_CharacterTable [ rl->InputKeyedCharacter ] ] ( rl ) ;
        if ( GetState ( rl, TAB_COMPLETION_CHANGE_STATE ) ) SetState ( rl, ( TAB_COMPLETION_CHANGE_STATE | TAB_WORD_COMPLETION ), false ) ;
        SetState ( rl, ANSI_ESCAPE, false ) ;
    }
    return rl->InputKeyedCharacter ;
}

byte
ReadLine_GetLine ( )
{
    ReadLiner * rl = _ReadLiner_ ;

    return _ReadLine_GetLine ( rl, 0 ) ;
}

byte
ReadLine_NextChar ( ReadLiner * rl )
{
    byte nchar ;
    if ( ! ( nchar = _ReadLine_GetNextChar ( rl ) ) )
    {
        if ( GetState ( rl, STRING_MODE ) )
        {
            SetState ( rl, STRING_MODE, false ) ; // only good once
            return nchar ;
        }

        else _ReadLine_GetLine ( rl, 0 ) ; // get a line of characters
        //else rl->LineNumber ++, _ReadLine_GetLine ( rl, 0 ) ; // get a line of characters
        ReadLine_Set_ReadIndex ( rl, 0 ) ;
        nchar = _ReadLine_GetNextChar ( rl ) ;
    }
    return nchar ;
}

byte
ReadLine_NextNonPunctCharAfterEndOfString ( ReadLiner * rl )
{
    byte * rlp = & rl->InputLine [ rl->ReadIndex ] ;
    while ( *( rlp ++ ) != '"' ) ;
    while ( IsPunct ( *( rlp ++ ) ) ) ;

    return *rlp ;
}

Boolean
ReadLine_AreWeAtNewlineAfterSpaces ( ReadLiner * rl )
{
    int64 i = ReadLiner_PeekSkipSpaces ( rl ) ;
    if ( _ReadLine_PeekOffsetChar ( rl, i ) == '\n' ) return true ;

    return false ;
}

Boolean
ReadLine_CheckForLocalVariables ( ReadLiner * rl )
{
    byte c, c2 ;
    Word * word = _Context_->CurrentlyRunningWord ;
    Boolean result ;
    int64 i, si ;
    if ( ! word ) word = _Interpreter_->w_Word ;
    si = word->W_RL_Index + strlen ( ( char* ) word->Name ) ;
    i = 0 ;
    ReadLine_Set_ReadIndex ( rl, si ) ;
    do
    {
        c = _ReadLine_PeekOffsetChar ( rl, i ) ;
        if ( c == ')' )
        {
            ReadLine_Set_ReadIndex ( rl, si + i + 1 ) ;
            c2 = ReadLine_PeekNextNonWhitespaceChar ( rl ) ;
            if ( ( ! GetState ( _Interpreter_, PREPROCESSOR_DEFINE ) ) && ( c2 == '{' ) ) result = true ;
            else result = false ;
            break ;
        }
    }
    while ( i ++, c != '\n' ) ;
    ReadLine_Set_ReadIndex ( rl, si ) ;

    return result ;
}

void
ReadLine_ShowInfo ( ReadLiner * rl )
{

    if ( _ReadLiner_->Filename )
        iPrintf ( "\nReadLiner Show :: %s : %d.%d :: \n%s", rl->Filename, rl->LineNumber, rl->CursorPosition, rl->InputLine ) ;
}

byte *
_ReadLine_String_FormattingRemoved ( ReadLiner * rl, int64 start )
{
    byte *str = rl->InputLine, * bf = Buffer_DataCleared ( _CSL_->FormatRemoval ), *ns ;
    int64 i, j, ep = rl->EndPosition ;
    //rl->InputLineString = rl->InputLine ;
    for ( i = start, j = 0 ; str [i] ; i ++ )
    {
        if ( str[i] == ESC )
        {
            do
            {
                rl->CursorPosition -- ;
                rl->EndPosition -- ;
                if ( str[i] == 'm' ) break ;
            }
            while ( i ++ <= ep ) ;
        }
        else bf [j ++] = str[i] ;
    }
    ns = String_New ( bf, TEMPORARY ) ;
    return ns ;
}

// ESC char ( 0x27 ) marks the beginning of an color adjust line for NOT_USING namespaces

byte *
ReadLine_InputLine_FirstEscapeChar ( ReadLiner * rl, int64 start )
{
    byte *str = rl->InputLine ;
    return String_FirstEscapeCharFromPos ( str, start ) ;
}

