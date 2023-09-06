#include "../../include/csl.h"
#define ESC 27

#define _ReadLine_SetFinalCharacter( rl, chr ) rl->InputBuffer [ rl->EndPosition ] = chr 

void
CSL_ReadTables_Setup ( CSL * csl )
{
    int64 i ;
    for ( i = 0 ; i < 256 ; i ++ )
    {
        csl->ReadLine_CharacterTable [ i ] = 0 ;
    }
    csl->ReadLine_CharacterTable [ 0 ] = 21 ;
    csl->ReadLine_CharacterTable [ 127 ] = 1 ;
    csl->ReadLine_CharacterTable [ '\b' ] = 1 ;
    csl->ReadLine_CharacterTable [ '\t' ] = 2 ;
    csl->ReadLine_CharacterTable [ 3 ] = 3 ; // Ctrl-C
    csl->ReadLine_CharacterTable [ 4 ] = 4 ; // Ctrl-D
    csl->ReadLine_CharacterTable [ '\r' ] = 20 ; // remember this is raw char input : the lexer also handles some of these
    csl->ReadLine_CharacterTable [ '\n' ] = 5 ;
    csl->ReadLine_CharacterTable [ eof ] = 6 ;
    csl->ReadLine_CharacterTable [ ESC ] = 7 ;
    csl->ReadLine_CharacterTable [ '[' ] = 8 ;
    csl->ReadLine_CharacterTable [ '1' ] = 9 ;
    csl->ReadLine_CharacterTable [ '4' ] = 10 ;
    csl->ReadLine_CharacterTable [ 'A' ] = 11 ;
    csl->ReadLine_CharacterTable [ 'B' ] = 12 ;
    csl->ReadLine_CharacterTable [ 'C' ] = 13 ;
    csl->ReadLine_CharacterTable [ 'D' ] = 14 ;
    csl->ReadLine_CharacterTable [ '3' ] = 15 ;
    csl->ReadLine_CharacterTable [ 'H' ] = 16 ;
    csl->ReadLine_CharacterTable [ 'O' ] = 17 ;
    csl->ReadLine_CharacterTable [ 'F' ] = 18 ;
    csl->ReadLine_CharacterTable [ '~' ] = 19 ;
    csl->ReadLine_CharacterTable [ '(' ] = 22 ;
    //cfrl->ReadLine_CharacterTable [ '!' ] = 23 ; // the lexer handles this

    csl->ReadLine_FunctionTable [ 0 ] = ReadTable_Default ;
    csl->ReadLine_FunctionTable [ 1 ] = ReadTable_BackSpace ;
    csl->ReadLine_FunctionTable [ 2 ] = ReadTable_Tab ;
    csl->ReadLine_FunctionTable [ 3 ] = ReadTable_0x03 ; // Ctrl-C
    csl->ReadLine_FunctionTable [ 4 ] = ReadTable_0x04 ; // Ctrl-D
    csl->ReadLine_FunctionTable [ 5 ] = ReadTable_Newline ;
    csl->ReadLine_FunctionTable [ 6 ] = ReadTable_EOF ;
    csl->ReadLine_FunctionTable [ 7 ] = ReadTable_ESC ;
    csl->ReadLine_FunctionTable [ 8 ] = ReadTable_LeftBracket ;
    csl->ReadLine_FunctionTable [ 9 ] = ReadTable_1 ; // ESC[1 Home ;
    csl->ReadLine_FunctionTable [ 10 ] = ReadTable_4 ; // ESC[4 End ;
    csl->ReadLine_FunctionTable [ 11 ] = ReadTable_A ; // ESC[A Up Arrow ;
    csl->ReadLine_FunctionTable [ 12 ] = ReadTable_B ; // ESC[B Down Arrow ;
    csl->ReadLine_FunctionTable [ 13 ] = ReadTable_C ; // ESC[C Right Arrow ;
    csl->ReadLine_FunctionTable [ 14 ] = ReadTable_D ; // ESC[D Left Arrow ;
    csl->ReadLine_FunctionTable [ 15 ] = ReadTable_3 ; // part of Delete escape sequence ;
    csl->ReadLine_FunctionTable [ 16 ] = ReadTable_H ; // ;
    csl->ReadLine_FunctionTable [ 17 ] = ReadTable_O ; // ESC[O Home ;
    csl->ReadLine_FunctionTable [ 18 ] = ReadTable_F ; // End ;
    csl->ReadLine_FunctionTable [ 19 ] = ReadTable_Tilde ; // ESC[~ Delete ;
    csl->ReadLine_FunctionTable [ 20 ] = ReadTable_CarriageReturn ; // '\r'
    csl->ReadLine_FunctionTable [ 21 ] = ReadTable_Zero ;
    csl->ReadLine_FunctionTable [ 22 ] = ReadTable_LParen ;
    //cfrl->ReadLine_FunctionTable [ 23 ] = ReadTable_Exclam ;
}

void
ReadTable_Default ( ReadLiner * rl )
{
    rl->EscapeModeFlag = 0 ;
    ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_LParen ( ReadLiner * rl )
{
    ReadTable_Default ( rl ) ;
}

void
ReadTable_Tab ( ReadLiner * rl ) // '\t':
{
    if ( AtCommandLine ( rl ) )
    {
        ReadLine_TabWordCompletion ( rl ) ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_0x03 ( ReadLiner * rl ) //  <CTRL-C>
{
    if ( _LC_ && GetState ( _LC_, LC_REPL ) )
    {
        ReadTable_Zero ( rl ) ;
    }
    else CSL_Quit ( ) ;
}

void
ReadTable_0x04 ( ReadLiner * rl ) // <CTRL-D>
{
    if ( _LC_ && GetState ( _LC_, LC_REPL ) )
    {
        ReadTable_Zero ( rl ) ;
    }
    else CSL_FullRestart ( ) ; //CSL_RestartInit ( ) ;
}

void
ReadTable_CarriageReturn ( ReadLiner * rl ) // '\r' 
{
    rl->InputKeyedCharacter = '\n' ; // convert '\r' to '\n'
    ReadTable_Newline ( rl ) ;
}

void
ReadTable_Newline ( ReadLiner * rl ) // '\n'
{
    if ( GetState ( _Context_->System0, ADD_READLINE_TO_HISTORY ) || GetState ( rl, ADD_TO_HISTORY ) )
    {
        _OpenVmTil_AddStringToHistoryList ( rl->InputLine ) ;
    }
    rl->LineNumber ++ ;
    //if ( ! GetState ( _Debugger_, DBG_COMMAND_LINE ) )
    {
        _ReadLine_AppendCharacter_Actual ( rl ) ;
        ReadLine_ShowCharacter ( rl ) ;
    }
    ReadLiner_Done ( rl ) ;
}

void
ReadTable_Zero ( ReadLiner * rl ) // eof
{
    //_ReadLine_NullDelimitInputBuffer ( rl ) ;
    _ReadLine_AppendCharacter_Actual ( rl ) ;
    ReadLiner_Done ( rl ) ;
    SetState ( rl, END_OF_STRING, true ) ;
}

//ReadTable_0 255:

void
ReadTable_EOF ( ReadLiner * rl ) // eof
{
    _ReadLine_AppendCharacter_Actual ( rl ) ;
    ReadLiner_Done ( rl ) ;
    SetState ( rl, END_OF_FILE, true ) ;
}

void
ReadTable_ESC ( ReadLiner * rl ) // 27 - ESC '^'
{
    rl->EscapeModeFlag = 1 ;
    ReadLine_ShowCharacter ( rl ) ; // does the escape action
}

void
ReadTable_LeftBracket ( ReadLiner * rl ) // '['  91 - second char of standard escape sequence
{
    if ( rl->EscapeModeFlag == 1 )
    {
        rl->EscapeModeFlag = 2 ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

// end of char sequence - 'ESC [ A'

void
ReadTable_A ( ReadLiner * rl ) // 'A' - back in history - UP arrow - ESC[A toward Head of list
{
    if ( rl->EscapeModeFlag == 2 )
    {
        dlnode * node = dllist_SetCurrentNode_Before ( _OSMS_->HistorySpace_MemChunkStringList ) ;
        if ( node )
        {
            rl->HistoryNode = ( HistoryStringNode* ) node ;
        }
        else rl->HistoryNode = 0 ;
        ReadLine_ShowHistoryNode ( rl ) ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_B ( ReadLiner * rl )// 'B' - forward, toward end of history - DOWN arrow - ESC[B - ^[[B - towards Tail of List
{
    if ( rl->EscapeModeFlag == 2 )
    {
        dlnode * node = dllist_SetCurrentNode_After ( _OSMS_->HistorySpace_MemChunkStringList ) ;
        if ( node )
        {
            rl->HistoryNode = ( HistoryStringNode* ) node ;
        }
        else rl->HistoryNode = 0 ;
        ReadLine_ShowHistoryNode ( rl ) ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_C ( ReadLiner * rl ) // 'C' - ^[C = right arrow
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 0 ;
        if ( rl->CursorPosition >= rl->EndPosition )
        {
            __ReadLine_AppendCharacter ( rl, ( byte ) ' ' ) ;
        }
        ReadLine_DoCursorMoveInput ( rl, rl->CursorPosition + 1 ) ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_D ( ReadLiner * rl ) // 'D' - ^[D = left arrow
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 0 ;
        if ( rl->CursorPosition > 0 )
        {
            ReadLine_DoCursorMoveInput ( rl, rl->CursorPosition - 1 ) ;
        }
        else return ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_F ( ReadLiner * rl ) // 'F' - End key
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 0 ;
        ReadLine_DoCursorMoveInput ( rl, rl->EndPosition ) ;
    }
    else if ( rl->EscapeModeFlag == 1 )
    {
        rl->EscapeModeFlag = 2 ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_H ( ReadLiner * rl ) // 'H' - Home Delete almost - see '~'
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 0 ;
        ReadLine_DoCursorMoveInput ( rl, 0 ) ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_4 ( ReadLiner * rl ) // ESC[4~ End
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 7 ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_3 ( ReadLiner * rl ) // ESC[3~ - Delete almost - see '~'
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 6 ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_1 ( ReadLiner * rl ) // ESC[1~ Home
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 4 ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_O ( ReadLiner * rl ) // 'O' - End key
{
    if ( rl->EscapeModeFlag == 2 )
    {
        rl->EscapeModeFlag = 3 ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_Tilde ( ReadLiner * rl ) // '~' - Delete
{
    if ( rl->EscapeModeFlag == 3 ) // end ??
    {
        rl->EscapeModeFlag = 0 ;
        ReadLine_DoCursorMoveInput ( rl, rl->EndPosition ) ;
    }
    else if ( rl->EscapeModeFlag == 4 ) // home
    {
        rl->EscapeModeFlag = 0 ;
        ReadLine_DoCursorMoveInput ( rl, 0 ) ;
    }
    else if ( rl->EscapeModeFlag == 6 ) // delete
    {
        rl->EscapeModeFlag = 0 ;
        ReadLine_DeleteChar ( rl ) ;
    }
    else if ( rl->EscapeModeFlag == 7 ) // end
    {
        rl->EscapeModeFlag = 0 ;
        ReadLine_DoCursorMoveInput ( rl, rl->EndPosition ) ;
    }
    else ReadLine_SaveCharacter ( rl ) ;
}

void
ReadTable_BackSpace ( ReadLiner * rl )
#if 0
{
    byte * il = _ReadLine_String_FormattingRemoved ( rl ) ;
    ReadLine_InputLine_Clear ( rl ) ;
    strncpy ( rl->InputLine, il, BUFFER_IX_SIZE ) ;
    rl->InputLineString = rl->InputLine ;
    ReadLine_DeleteChar ( rl ) ;
}
#else // experimental
{
    if ( rl->CursorPosition > rl->EndPosition ) rl->CursorPosition = rl->EndPosition ;
    if ( GetState ( rl, TAB_WORD_COMPLETION ) )
    {
        if ( rl->InputLine[rl->CursorPosition - 1] != '.' )
        {
            int64 cp = rl->CursorPosition, slids ;
            TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
            memset ( tci, 0, sizeof ( TabCompletionInfo ) ) ;
            byte *ids, * id = TabCompletionInfo_GetAPreviousIdentifier ( rl, _ReadLine_CursorPosition ( rl ) ) ;
            if ( tci->TokenFirstChar ) tci->PreviousIdentifier = TabCompletionInfo_GetAPreviousIdentifier ( rl, tci->TokenFirstChar - 1 ) ; // TokenStart refers to start of 'Identifier'
            Word * w = CSL_FindInAnyNamespace ( String_FormattingRemoved ( tci->PreviousIdentifier ) ) ;
            if ( w )
            {
                //ReadLine_InputLine_Clear ( rl ) ;
                byte * fnql = ReadLiner_GenerateFullNamespaceQualifiedName ( rl, w ) ;
                strcpy ( &rl->InputLine[tci->StringFirstChar], fnql ) ;
                strcat ( &rl->InputLine[tci->StringFirstChar], "." ) ;
                ids = String_FormattingRemoved ( id ) ;
                rl->CursorPosition = cp + ( slids = strlen ( ids ) ) ; // don't add   "."
                strncat ( rl->InputLine, ids, slids - 1 ) ;
            }
            else
            {
                byte * il = _ReadLine_String_FormattingRemoved ( rl ) ;
                strncpy ( &rl->InputLine[tci->StringFirstChar], il, BUFFER_IX_SIZE ) ;
                rl->CursorPosition = strlen ( il ) ;
            }
            rl->InputLineString = rl->InputLine ;
        }
    }
    ReadLine_DeleteChar ( rl ) ;
}
#endif

