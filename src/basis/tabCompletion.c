
#include "../include/csl.h"

void
RL_TabCompletion_Run ( ReadLiner * rl, Word * rword )
{
    TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
    tci->StartFlag = 0 ;
    Word * nextWord = TC_Tree_Map ( tci, ( MapFunction ) _TabCompletion_Compare, rword ) ; // new
    if ( nextWord ) tci->NextWord = nextWord ; // wrap around
    else tci->NextWord = (Word *) dllist_First ( tci->OriginalContainingNamespace->S_SymbolList ) ; //tci->NextWord->S_ContainingNamespace->S_SymbolList ) ;
}

TabCompletionInfo *
TabCompletionInfo_New ( uint64 allocType )
{
    TabCompletionInfo *tci = ( TabCompletionInfo * ) Mem_Allocate ( sizeof (TabCompletionInfo ), allocType ) ;
    return tci ;
}

byte *
ReadLiner_GenerateFullNamespaceQualifiedName ( ReadLiner * rl, Word * w )
{
    byte * b0 = Buffer_DataCleared ( _CSL_->TabCompletionBuf ) ;
    Stack_Init ( rl->TciNamespaceStack ) ;
    Stack * nsStk = rl->TciNamespaceStack ;
    Namespace *ns ;
    byte * nsName, *c_gdDot = 0 ; //( CString ) c_dd ( "." ) ;
    int64 i, dot = 0, notUsing = 0 ; //, strlenUnAdorned = 0 ; //, ow = 0 ;

    String_Init ( b0 ) ;
    for ( ns = ( Is_NamespaceType ( w ) ? w : w->ContainingNamespace ) ; ns ; ns = ns->ContainingNamespace ) // && ( tw->ContainingNamespace != _O_->CSL->Namespaces ) )
    {
        if ( ns->State & NOT_USING )
        {
            notUsing = 1 ;
        }
        _Stack_Push ( nsStk, ( int64 ) ( ( ns->State & NOT_USING ) ? c_gd ( ns->Name ) : ( ns->Name ) ) ) ;
        //strlenUnAdorned += strlen ( ns->Name ) ;
    }
    if ( notUsing ) c_gdDot = ( byte* ) c_gd ( "." ) ;
    for ( i = Stack_Depth ( nsStk ) ; i ; i -- )
    {
        nsName = ( byte* ) _Stack_Pop ( nsStk ) ;
        if ( nsName )
        {
            strcat ( ( char* ) b0, ( char* ) nsName ) ; //( CString ) notUsing ? c_dd ( nsName ) : nsName ) ;
            if ( i > 1 )
            {
                strcat ( ( char* ) b0, "." ) ; //( CString ) notUsing ? ( CString ) c_ddDot : ( CString ) "." ) ;
            }
        }
    }
    //if ( ( ! Is_NamespaceType ( w ) ) && ( ! String_Equal ( nsName, w->Name ) ) )
    if ( ! Is_NamespaceType ( w ) ) //&& ( ! String_Equal ( nsName, w->Name ) ) )
    {
        if ( ! dot ) strncat ( ( CString ) b0, ( CString ) notUsing ? ( CString ) c_gdDot : ( CString ) ".", BUFFER_IX_SIZE ) ;
        strncat ( ( char* ) b0, notUsing ? ( char* ) c_gd ( w->Name ) : ( char* ) w->Name, BUFFER_IX_SIZE ) ; // namespaces are all added above
        //strlenUnAdorned += strlen ( w->Name ) ;
    }
    //ReadLine_SetCursorPosition ( rl, strlenUnAdorned ) ;
    return b0 ;
}

// added 0.756.541
// problem here is that with the Class word '.' (dot) it loops and doesn't return

int64
_TC_FindPrevious_NamespaceQualifiedIdentifierStart ( TabCompletionInfo * tci, byte * s, int64 pos )
{
    int64 f, l, last = 0, dot ; // these refer to positions in the string s
    do
    {
        l = String_LastCharOfLastToken_FromPos ( s, pos ) ;
        if ( ! last ) tci->TokenLastChar = last = l ;
        if ( ( last == pos ) && ( s [last] <= ' ' ) && ( last != _ReadLine_CursorPosition ( _Context_->ReadLiner0 ) ) ) return last ;
        f = String_FirstCharOfToken_FromPosOfLastChar ( s, l ) ;
        if ( f > 0 ) dot = String_IsThereADotSeparatorBackFromPosToLastNonDelmiter ( s, f - 1 ) ;
        else break ;
    }
    while ( pos = dot ) ;
    return f ;
}

void
RL_TC_StringInsert_AtCursor ( ReadLiner * rl, byte * strToInsert )
{
    TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
    int64 stiLen, newCursorPos, startCursorPos = _ReadLine_CursorPosition ( rl ), slotEnd ;
    int64 slotStart = _TC_FindPrevious_NamespaceQualifiedIdentifierStart ( tci, rl->InputLineString, startCursorPos ) ;
    stiLen = Strlen ( strToInsert ) ;
    newCursorPos = slotStart + stiLen ;
    slotEnd = newCursorPos ;
    //if ( (slotStart == 0) || (newCursorPos < stiLen ))
    //if ( newCursorPos < stiLen )
    {
        Clear_Terminal_Line ( ) ;
        ReadLine_InputLine_Clear ( rl ) ;
        Strncpy ( rl->InputLineString, String_FormattingRemoved (_CSL_->OriginalInputLine, COMPILER_TEMP ), BUFFER_SIZE ) ;
    }
    ReadLine_SetCursorPosition ( rl, newCursorPos ) ;
    //_ReadLine_InsertStringIntoInputLineSlotAndShow ( rl, slotStart, startCursorPos, ( byte* ) strToInsert ) ; // 1 : TokenLastChar is the last char of the identifier
    _ReadLine_InsertStringIntoInputLineSlotAndShow ( rl, slotStart, slotEnd, ( byte* ) strToInsert ) ; // 1 : TokenLastChar is the last char of the identifier
    ReadLine_Set_ReadIndex ( rl, rl->CursorPosition ) ; 
}

byte *
_TabCompletionInfo_GetAPreviousIdentifier ( ReadLiner *rl, int64 start )
{
    byte * b = Buffer_DataCleared ( _CSL_->TabCompletion ) ;
    TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
    tci->TokenLastChar = ReadLine_LastCharOfLastToken_FromPos ( rl, start ) ;
    tci->TokenFirstChar = ReadLine_FirstCharOfToken_FromLastChar ( rl, tci->TokenLastChar ) ;
    tci->TokenLength = tci->TokenLastChar - tci->TokenFirstChar + 1 ; // zero array start
    Strncpy ( b, & rl->InputLine [ tci->TokenFirstChar ], tci->TokenLength ) ;
    b [ tci->TokenLength ] = 0 ;
    return TemporaryString_New ( b ) ;
}

// nb. the notation (function names) around parsing in tab completion is based in 'reverse parsing' - going back in the input line from the cursor position

void
RL_TabCompletionInfo_Init ( ReadLiner * rl )
{
    Namespace * piw ;
    Word * wf ;
    TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
    memset ( tci, 0, sizeof ( TabCompletionInfo ) ) ;
    SetState ( rl, TAB_WORD_COMPLETION, true ) ;
    strcpy ( ( CString ) _CSL_->OriginalInputLine, ( CString ) rl->InputLineString ) ; // we use this extra buffer at ReadLine_TC_StringInsert_AtCursor
    tci->Identifier = _TabCompletionInfo_GetAPreviousIdentifier ( rl, _ReadLine_CursorPosition ( rl ) ) ;
    tci->DotSeparator = ReadLine_IsThereADotSeparator ( rl, tci->TokenFirstChar - 1 ) ;
    if ( tci->TokenFirstChar ) tci->PreviousIdentifier = _TabCompletionInfo_GetAPreviousIdentifier ( rl, tci->TokenFirstChar - 1 ) ; // TokenStart refers to start of 'Identifier'
    if ( ( tci->EndDottedPos = ReadLine_IsLastCharADot ( rl, _ReadLine_CursorPosition ( rl ) ) ) ) //ReadLine_IsDottedToken ( rl ) )
    {
        tci->SearchToken = ( byte * ) "" ; // everything matches
        rl->CursorPosition = tci->EndDottedPos ;
        rl->InputLine [ tci->EndDottedPos ] = ' ' ; // overwrite the final '.' with ' ' and move cursor pos back to that space 
    }
    else tci->SearchToken = tci->Identifier ? tci->Identifier : ( byte* ) "" ;
    if ( tci->DotSeparator )
    {
        tci->PreviousIdentifier = _TabCompletionInfo_GetAPreviousIdentifier ( rl, tci->DotSeparator - 1 ) ; // TokenStart refers to start of 'Identifier'
        if ( tci->PreviousIdentifier && ( piw = CSL_FindInAnyNamespace ( tci->PreviousIdentifier ) ) )
        {
            if ( Is_NamespaceType ( piw ) )
            {
                if ( ( tci->OriginalWord = _Finder_FindWord_InOneNamespace ( _Finder_, piw, tci->Identifier ) ) ) tci->RunWord = tci->OriginalWord ;
                else if ( wf = Finder_FindWord_AnyNamespace ( _Finder_, tci->Identifier ), ( wf && ( wf->ContainingNamespace == piw ) ) ) tci->RunWord = tci->OriginalWord = wf ;
                else tci->RunWord = ( Word* ) dllist_First ( ( dllist* ) piw->Lo_List ) ;
                tci->OriginalContainingNamespace = piw ;
            }
            if ( tci->OriginalWord && Is_NamespaceType ( tci->OriginalWord ) && ( tci->EndDottedPos ) )
            {
                tci->RunWord = ( Word* ) dllist_First ( ( dllist* ) tci->OriginalWord->Lo_List ) ;
                tci->OriginalContainingNamespace = tci->OriginalWord ;
            }
        }
    }
    else
    {
        if ( ( tci->OriginalWord = _Finder_FindWord_InOneNamespace ( _Finder_, _CSL_Namespace_InNamespaceGet ( ), tci->Identifier ) ) ||
            ( tci->OriginalWord = CSL_FindInAnyNamespace ( tci->Identifier ) ) )
        {
            if ( Is_NamespaceType ( tci->OriginalWord ) && ( tci->EndDottedPos ) )
            {
                tci->RunWord = ( Word* ) dllist_First ( ( dllist* ) tci->OriginalWord->Lo_List ) ;
                tci->OriginalContainingNamespace = tci->OriginalWord ;
            }
            else
            {
                tci->EndDottedPos = 0 ;
                tci->OriginalContainingNamespace = tci->OriginalWord->ContainingNamespace ? tci->OriginalWord->ContainingNamespace : _CSL_->Namespaces ;
                tci->RunWord = tci->OriginalWord ;
            }
        }
    }
    if ( ! tci->OriginalContainingNamespace ) tci->OriginalContainingNamespace = _CSL_->Namespaces ;
    tci->OriginalRunWord = tci->RunWord ;
    tci->WordWrapCount = 0 ;
    tci->WordCount = 0 ;
    tci->StartFlag = 0 ;
    //srand ( time (0) ) ;
    tci->FoundMarker = rand ( ) ;
    tci->FoundWrapCount = 0 ;
    _Context_->NlsWord = 0 ;
    tci->NextWord = tci->RunWord ;
}

Word *
_TabCompletion_Compare ( Word * word )
{
    ReadLiner * rl = _Context_->ReadLiner0 ;
    TabCompletionInfo * tci = rl->TabCompletionInfo0 ;
    byte * searchToken ;
    int64 gotOne = 0, slst, sltwn, strOpRes = - 1 ;
    tci->WordCount ++ ;
    if ( word )
    {
        searchToken = tci->SearchToken ;
        Word * tw = tci->TrialWord = word ;
        byte * twn = tw->Name, *fqn ;
        if ( twn )
        {
            d0 ( if ( String_Equal ( twn, "rsp" ) ) iPrintf ( "got it" ) ) ;
            slst = Strlen ( ( CString ) searchToken ), sltwn = Strlen ( twn ) ;
            if ( ! slst ) // we match anything when user ends with a dot ( '.' ) ...
            {
                // except .. We don't want to jump down into a lower namespace here. ??
                if ( ( tw->ContainingNamespace != tci->OriginalContainingNamespace ) || ( tw->ContainingNamespace == _CSL_->Namespaces ) )
                {
                    gotOne = true ; //1 ;
                }
                else return false ;
            }
            else
            //if ( ! gotOne )
            {
                //tci->WordWrapCount = 8 ;
                switch ( tci->WordWrapCount )
                {
                        // this arrangement allows us to see some word matches before others
                    case 0: //case 1:
                    {
                        strOpRes = String_Equal ( twn, searchToken ) ;
                        if ( ( strOpRes ) && ( slst == sltwn ) ) gotOne = true ;
                        break ;
                    }
                    case 1: case 2:
                    {
                        strOpRes = Stringn_Equal ( twn, searchToken, slst ) ;
                        if ( strOpRes ) gotOne = true ;
                        break ;
                    }
                    case 3: // in case of uppercase tokens (?)
                    {
                        strOpRes = ( int64 ) strstr ( ( CString ) twn, ( CString ) searchToken ) ;
                        if ( strOpRes ) gotOne = true ;
                        break ;
                    }
                    case 4:
                    {
                        strOpRes = Stringni_Equal ( twn, searchToken, slst ) ;
                        if ( strOpRes ) gotOne = true ;
                        break ;
                    }
                    case 5:
                    {
                        strOpRes = ( int64 ) strstr ( ( CString ) twn, ( CString ) searchToken ) ;
                        if ( strOpRes ) gotOne = true ;
                        break ;
                    }
                    case 6: default:
                    {
                        byte bufw [128], bufo[128] ;
                        strToLower ( bufw, twn ) ;
                        strToLower ( bufo, searchToken ) ;
                        strOpRes = ( int64 ) strstr ( ( CString ) bufw, ( CString ) bufo ) ;
                        if ( strOpRes ) gotOne = true ;
                        break ;
                    }
                }
            }
            if ( gotOne )
            {
                if ( word->W_FoundMarker == tci->FoundMarker )
                {
#if 0                   
                    tci->FoundWrapCount ++ ;
                    tci->FoundCount = 0 ;
                    if ( ++ tci->WordWrapCount > 6 ) // 6 : there are 6 cases in the switch in _TabCompletion_Compare
                    {
                        tci->FoundMarker = rand ( ) ;
                        tci->WordWrapCount = 0 ;
                    }
#endif                    
                    return false ;
                }
                word->W_FoundMarker = tci->FoundMarker ;
                fqn = ReadLiner_GenerateFullNamespaceQualifiedName ( rl, tw ) ;
                RL_TC_StringInsert_AtCursor ( rl, fqn ) ;
                tci->FoundCount ++ ;
                if ( tci->FoundCount > tci->MaxFoundCount ) tci->MaxFoundCount = tci->FoundCount ;
                if ( Verbosity () > 4 )
                {
                    //if ( tci->FoundWrapCount )
                    {
                        iPrintf ( " [ Search = \'%s\' : FoundWrapCount = %d : WordWrapCount = %d : WordCount = %d : Found List Length = %d : Max List Length = %d ]",
                            ( tci->RunWord ? tci->RunWord->Name : tci->Identifier ), tci->FoundWrapCount, tci->WordWrapCount, tci->WordCount, tci->FoundCount, tci->MaxFoundCount ) ;
                    }
                }
                return word ; //true ;
            }
        }
    }
    return false ;
}

Word *
TC_Tree_Map ( TabCompletionInfo * tci, MapFunction mf, Word * wordi )
{
    Word *word = wordi, *rword = 0 ; //return word
    while ( 1 )
    {
        Word *nextWord, *ns, *nextNs ; //return word
        if ( word )
        {
            ns = word->S_ContainingNamespace ;
            nextNs = ( Word* ) dlnode_Next ( ( node* ) ns ) ;
        }
        else ns = ( Word * ) dllist_First ( _CSL_->Namespaces->W_List ) ;
        while ( ns )
        {
            nextNs = ( Word* ) dlnode_Next ( ( node* ) ns ) ;
            for ( word = ( Word * ) dllist_First ( ns->S_SymbolList ) ; word ; word = nextWord )
            {
                nextWord = ( Word* ) dlnode_Next ( ( node* ) word ) ;
                if ( kbhit ( ) ) return 0 ; //must be here else could loop forever !?
                if ( rword = (Word*) mf ( ( Symbol* ) word ) ) goto doReturn ;
            }
            if ( rword = (Word*) mf ( ( Symbol* ) ns ) ) goto doReturn ;
            ns = nextNs ;
        }
doReturn:
        if ( ! rword )
        {
            if ( nextWord ) rword = nextWord ;
            else if ( nextNs ) rword = nextNs ; //( Word* ) dllist_First ( nextNs->S_SymbolList ) ;
            if ( ( ! rword ) && ++ tci->WordWrapCount > 6 ) // 6 : there are six cases in the switch in _TabCompletion_Compare
            {
                tci->FoundMarker = rand ( ) ;
                tci->WordWrapCount = 0 ;
            }
        }
        if ( rword != tci->LastFoundWord ) break ;
    }
    //if ( (int64) rword == 1 ) return 0 ;
        //_Printf ("") ;
    //if ( rword ) tci->LastFoundWord = rword ;
    return rword ;
}


