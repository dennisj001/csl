
#include "../include/csl.h"

HistoryStringNode *
HistoryStringNode_New ( byte * hstring )
{
    HistoryStringNode * hsn = ( HistoryStringNode * ) Mem_Allocate ( sizeof ( HistoryStringNode ), HISTORY ) ;
    _Symbol_Init_AllocName ( ( Symbol* ) hsn, hstring, HISTORY ) ; // use Name for history string
    return hsn ;
}

HistoryStringNode *
HistorySymbolList_Find ( byte * hstring )
{
    HistoryStringNode * hsn = 0 ;
    dlnode * node, * nextNode ;
    for ( node = dllist_First ( ( dllist* ) _OS_->HistorySpace_MemChunkStringList ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        hsn = ( HistoryStringNode* ) node ;
        if ( ( hsn->S_Name ) && ( String_Equal ( ( char* ) hsn->S_Name, ( char* ) hstring ) ) ) return hsn ;
    }
    return 0 ;
}

void
ReadLine_ShowHistoryNode ( ReadLiner * rl )
{
    rl->EscapeModeFlag = 0 ;
    if ( rl->HistoryNode && rl->HistoryNode->S_Name )
    {
        byte * dst = Buffer_Data ( _CSL_->ScratchB1 ) ;
        dst = _String_ConvertStringToBackSlash ( dst, rl->HistoryNode->S_Name, - 1 ) ;
        _ReadLine_PrintfClearTerminalLine ( ) ;
        __ReadLine_DoStringInput ( rl, String_FilterMultipleSpaces ( dst, TEMPORARY ), rl->AltPrompt ) ;
        ReadLine_SetCursorPosition ( rl, rl->EndPosition ) ;
    }
    else
    {
        ReadLine_ClearCurrentTerminalLine ( rl, rl->EndPosition ) ; // nb : this is also part of ShowString
        ReadLine_ShowNormalPrompt ( rl ) ;
    }
    _ReadLine_CursorToEnd ( rl ) ;
}

void
_OpenVmTil_AddStringToHistoryList ( byte * istring )
{
    HistoryStringNode * hsn ;
    if ( istring && strcmp ( ( char* ) istring, "" ) ) // don't add blank lines to history
    {
        byte * nstring = Buffer_Data ( _CSL_->ScratchB1 ) ;
        nstring = _String_ConvertStringToBackSlash ( nstring, istring, - 1 ) ;

        hsn = HistorySymbolList_Find ( nstring ) ;
        if ( ! hsn ) hsn = HistoryStringNode_New ( nstring ) ;
        else dlnode_Remove ( ( dlnode* ) hsn ) ; // make it last with dllist_AddNodeToTail
        dllist_AddNodeToTail ( _OS_->HistorySpace_MemChunkStringList, ( dlnode* ) hsn ) ; //
        d0 ( int64 ll = List_Length ( _OS_->HistorySpace_MemChunkStringList ) ) ;
        dllist_SetCurrentNode_After ( _OS_->HistorySpace_MemChunkStringList ) ; // ! properly set Object.dln_Node
    }
}

void
OpenVmTil_AddStringToHistory ( )
{
    byte * string = ( byte* ) DataStack_Pop ( ) ;
    _OpenVmTil_AddStringToHistoryList ( string ) ;
}

void
OpenVmTil_AddStringToHistoryOn ( )
{
    SetState ( _Context_->ReadLiner0, ADD_TO_HISTORY, true ) ;
}

void
OpenVmTil_AddStringToHistoryOff ( )
{
    SetState ( _Context_->ReadLiner0, ADD_TO_HISTORY, false ) ;
}

#if 0
void
History_Init ( )
{
//    _OS_->HistorySpace_MemChunkStringList = _dllist_New ( OVT_STATIC ) ;
    _OS_->HistorySpace_MemChunkStringList = _dllist_New ( HISTORY ) ;
}
#endif

void
History_Delete ( )
{
    FreeChunkList ( _OS_->HistorySpace_MemChunkStringList ) ;
    _dllist_Init ( _OS_->HistorySpace_MemChunkStringList ) ;
    _OS_->HistoryAllocation = 0 ;
}
