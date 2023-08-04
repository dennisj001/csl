
#include "../include/csl.h"

void
_Interpret_ListNode ( dlnode * node )
{
    Word * word = ( Word * ) dobject_Get_M_Slot ( (dobject*) node, SCN_T_WORD ) ;
    _Word_Interpret ( word ) ;
}

void
List_Interpret ( dllist * list )
{
    dllist_Map ( list, _Interpret_ListNode ) ;
    //List_Init ( list ) ;
}

// list : a list of lists of postfix operations needing to be interpreted

void
List_InterpretLists ( dllist * olist )
{
    Compiler * compiler = _Compiler_ ;
    if ( ! ( GetState ( compiler, INFIX_LIST_INTERPRET ) ) ) // prevent recursive call here
    {
        int64 svs = GetState ( compiler, C_INFIX_EQUAL ) ;
        SetState ( compiler, C_INFIX_EQUAL, false ) ;
        SetState ( compiler, INFIX_LIST_INTERPRET, true ) ;
        dlnode * node, *nextNode ;
        for ( node = dllist_First ( ( dllist* ) olist ) ; node ; node = nextNode )
        {
            // get nextNode before map function (mf) in case mf changes list by a Remove of current node
            // problem could arise if mf removes Next node
            nextNode = dlnode_Next ( node ) ;
            dllist * ilist = ( dllist * ) dobject_Get_M_Slot ( (dobject*) node, SCN_T_WORD ) ;
            List_Interpret ( ilist ) ;
            //dlnode_Remove ( node ) ;
            Word_Recycle ( (Word * ) node ) ;
        }
        List_Init ( olist ) ;
        SetState ( compiler, INFIX_LIST_INTERPRET, false ) ;
        SetState ( compiler, C_INFIX_EQUAL, svs ) ;
    }
}

// list : a list of lists of postfix operations needing to be interpreted

void
List_CheckInterpretLists_OnVariable ( dllist * list, byte * token )
{
    if ( list )
    {
        dlnode * node, *nextNode ;
        for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
        {
            // get nextNode before map function (mf) in case mf changes list by a Remove of current node
            // problem could arise if mf removes Next node
            nextNode = dlnode_Next ( node ) ;
            dllist * plist = ( dllist * ) dobject_Get_M_Slot ( (dobject*) node, SCN_T_WORD ) ; // plist created in CSL_IncDec
            Word * word = ( Word * ) List_Top_Value ( plist ) ;
            byte *checkPostfixToken = word ? word->Name : 0 ;
            if ( checkPostfixToken && String_Equal ( checkPostfixToken, token ) )
            {
                List_Interpret ( plist ) ;
                dlnode_Remove ( node ) ;
                //Word_Recycle ( (Word * ) node ) ;
            }
        }
        //List_Init ( list ) ;
    }
}

void
List_PrintNames ( dllist * list, int64 count, int64 flag )
{
    dlnode * node, *nextNode ;
    Word * nodeWord, *beforeNode, *n_After ;
    byte * thisName, *beforeName, *afterName, *bt = Buffer_New_pbyte ( 64 ), *ba = Buffer_New_pbyte ( 64 ), *bb = Buffer_New_pbyte ( 64 ) ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node && count -- ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        if ( flag )
        {
            nodeWord = ( node->n_After && node->n_After->n_After ? ( Word* ) dobject_Get_M_Slot ( (dobject*) node, 0 ) : 0 ) ;
            if ( ! nodeWord ) break ;
            thisName = nodeWord ? sconvbs ( bt, nodeWord->Name ) : ( byte* ) " ", node ;
            beforeNode = ( node->n_Before == list->Head ? 0 : ( Word * ) dobject_Get_M_Slot ( (dobject*) node->n_Before, SCN_T_WORD ) ) ;
            n_After = ( node->n_After == list->Tail ? 0 : ( Word* ) dobject_Get_M_Slot ( (dobject*) node->n_After, SCN_T_WORD ) ) ;
            afterName = n_After ? sconvbs ( ba, n_After->Name ) : ( byte* ) " ", node->n_After ;
            beforeName = beforeNode ? sconvbs ( bb, ( beforeNode )->Name ) : ( byte* ) " ", node->n_Before ;
            iPrintf ( "\n\tName : %s 0x%08x \t\tBefore : %s 0x%08x : \t\tAfter : %s 0x%08x,",
                thisName, node, beforeName, node->n_Before, afterName, node->n_After ) ;
        }
        else iPrintf ( "\n\tName : %s", ( ( Word* ) ( node ) )->Name ) ; //thisName ) ;
    }
}

void
List_Show_N_Word_Names ( dllist * list, uint64 n, int64 showBeforeAfterFlag, int64 dbgFlag )
{
    if ( dbgFlag ) NoticeColors ;
    List_PrintNames ( list, n, showBeforeAfterFlag ) ;
    if ( dbgFlag ) DefaultColors ;
}

void
List_DupList ( )
{
    //LC_Init_Runtime ( ) ;
    ListObject * l0 = ( ListObject * ) TOS, *l1 ;
    l1 = LO_CopyOne ( l0 ) ;
    DataStack_Push (( int64 ) l1) ;
}

void
List_PrintWithValue ( )
{
    ListObject * l0 = ( ListObject * ) DataStack_Pop ( ) ;
    LO_PrintWithValue ( l0 ) ;
}

