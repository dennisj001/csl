#include "../../include/csl.h"

void
_dlnode_Init ( dlnode * node )
{
    node->n_After = 0 ;
    node->n_Before = 0 ;
}

dlnode *
_dlnode_New ( uint64 allocType )
{
    dlnode * node = ( dlnode* ) Mem_Allocate ( sizeof (dlnode ), allocType ) ;
    return node ;
}

inline dlnode *
_dlnode_Before ( dlnode * anode )
{
    return anode->n_Before ;
}

inline dlnode *
_dlnode_After ( dlnode * anode )
{
    return anode->n_After ;
}

inline dlnode *
Is_NotHeadOrTailNode ( dlnode * anode )
{
    if ( anode && _dlnode_After ( anode ) && _dlnode_Before ( anode ) ) return anode ;
    else return 0 ;
}
// with error checking anode

inline dlnode *
Is_NotAHeadNode ( dlnode * anode )
{
    if ( anode && _dlnode_Before ( anode ) ) return anode ;
    else return 0 ;
}

// with error checking anode

inline dlnode *
Is_NotATailNode ( dlnode * anode )
{
    if ( anode && _dlnode_After ( anode ) ) return anode ;
    return 0 ;
}

// toward the TailNode

dlnode *
dlnode_Next ( dlnode * node )
{
    // don't return TailNode, return 0
    if ( node )
    {
        dlnode * nextNode = node->n_After ;
        if ( nextNode && nextNode->n_After ) return nextNode ;
    }
    return 0 ;
}

// toward the HeadNode

dlnode *
dlnode_Previous ( dlnode * node )
{
    // don't return HeadNode return 0
    if ( node )
    {
        dlnode * prevNode = node->n_Before ;
        if ( prevNode && prevNode->n_Before ) return prevNode ;
    }
    return 0 ;
}

void
dlnode_InsertThisAfterANode ( dlnode * thisNode, dlnode * aNode ) // Insert thisNode After aNode : toward the tail of the list - "after" the Head
{
    if ( thisNode && Is_NotATailNode ( aNode ) )
    {
        //if ( aNode->afterNode ) 
        aNode->n_After->n_Before = thisNode ; // don't overwrite a Head or Tail node 
        thisNode->n_After = aNode->n_After ;
        aNode->n_After = thisNode ; // necessarily after the above statement ! 
        thisNode->n_Before = aNode ;
    }
}

void
dlnode_InsertThisBeforeANode ( dlnode * thisNode, dlnode * aNode ) // Insert thisNode Before aNode : toward the head of the list - "before" the Tail
{
    if ( thisNode && Is_NotAHeadNode ( aNode ) )
    {
        //if ( aNode->beforeNode ) 
        aNode->n_Before->n_After = thisNode ; // don't overwrite a Head or Tail node
        thisNode->n_Before = aNode->n_Before ;
        aNode->n_Before = thisNode ; // necessarily after the above statement ! 
        thisNode->n_After = aNode ;
    }
}

void
dlnode_Remove ( dlnode * node )
{
    if ( Is_NotHeadOrTailNode ( node ) )
    {
        node->n_Before->n_After = node->n_After ;
        node->n_After->n_Before = node->n_Before ;
        node->n_After = 0 ;
        node->n_Before = 0 ;
    }
}

void
dlnode_ReplaceNodeWithANode ( dlnode * node, dlnode * anode )
{
    if ( node && anode )
    {
        dlnode * n_After = node->n_After ;
        dlnode_Remove ( node ) ;
        dlnode_InsertThisBeforeANode ( anode, n_After ) ;
    }
}

void
dlnode_Replace ( dlnode * replacedNode, dlnode * replacingNode )
{
    if ( replacedNode && replacingNode )
    {
        if ( replacedNode->n_Before ) replacedNode->n_Before->n_After = replacingNode ;
        if ( replacedNode->n_After ) replacedNode->n_After->n_Before = replacingNode ;
    }
}

void
_dllist_Init ( dllist * list )
{
    if ( list && list->Head && list->Tail )
    {
        list->Head->n_After = ( dlnode * ) list->Tail ;
        list->Head->n_Before = ( dlnode * ) 0 ;
        list->Tail->n_After = ( dlnode* ) 0 ;
        list->Tail->n_Before = ( dlnode * ) list->Head ;
        list->l_CurrentNode = 0 ;
    }
}

void
dllist_Init ( dllist * list, dlnode * ahead, dlnode * atail )
{
    list->Head = ahead ;
    list->Tail = atail ;
    _dllist_Init ( list ) ;
}

dllist *
_dllist_New ( uint64 allocType )
{
    dllist * list = ( dllist* ) Mem_Allocate ( sizeof ( dllist ), allocType ) ;
    list->Head = _dlnode_New ( allocType ) ;
    list->Tail = _dlnode_New ( allocType ) ;
    _dllist_Init ( list ) ;
    return list ;
}

dllist *
dllist_New ( )
{
    return _dllist_New ( DICTIONARY ) ;
}

int64
dllist_Depth ( dllist * list )
{
    int64 length ;
    dlnode * node, * nextNode ;
    for ( length = 0, node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        length ++ ;
    }
    return length ;
}

void
_dllist_AddNodeToHead ( dllist *list, dlnode * node )
{
    if ( list && node ) dlnode_InsertThisAfterANode ( node, list->Head ) ; // after Head toward Tail
}

void
dllist_AddNodeToHead ( dllist *list, dlnode * node )
{
    if ( list && node )
    {
        dlnode_Remove ( node ) ; // if the node is already on a list it will be first removed
        _dllist_AddNodeToHead ( list, node ) ;
        list->l_CurrentNode = 0 ;
    }
}

void
_dllist_AddNodeToTail ( dllist *list, dlnode * node )
{
    if ( list && node ) dlnode_InsertThisBeforeANode ( node, list->Tail ) ; // before Tail toward Head
}

void
dllist_AddNodeToTail ( dllist *list, dlnode * node )
{
    if ( list && node )
    {
        dlnode_Remove ( node ) ;
        _dllist_AddNodeToTail ( list, node ) ;
        list->l_CurrentNode = node ;
    }
}

inline dlnode *
_dllist_Head ( dllist * list )
{
    return list->Head ;
}

inline dlnode *
_dllist_Tail ( dllist * list )
{
    return list->Tail ;
}

dlnode *
dllist_Head ( dllist * list )
{
    //if ( ! list ) return 0 ;
    //return ( dlnode * ) list->head ;
    dlnode * ahead = 0 ;
    if ( list ) ahead = _dllist_Head ( list ) ;
    return ahead ;
}

dlnode *
dllist_Tail ( dllist * list )
{
    dlnode * atail ;
    if ( list ) atail = _dllist_Tail ( list ) ;
    else return 0 ;
    return atail ;
}

inline Boolean
Is_NotTheHeadNode ( dlnode * anode )
{
    if ( anode->n_Before ) return true ;
    else return false ;
}

inline Boolean
Is_NotTheTailNode ( dlnode * anode )
{
    if ( anode->n_After ) return true ;
    else return false ;
}

// 'after' is toward the tail

dlnode *
_dllist_First ( dllist * list )
{
    dlnode * head = _dllist_Head ( list ), * headAfter ;
    if ( head && ( headAfter = _dlnode_After ( head ) ) )
        if ( headAfter->n_After ) return headAfter ; //Is_NotTheTailNode
    return 0 ;
}

// 'before' is toward the head

dlnode *
_dllist_Last ( dllist * list )
{
    dlnode *tail = _dllist_Tail ( list ), * tailBefore ;
    if ( tail && ( tailBefore = _dlnode_Before ( tail ) ) )
        if ( tailBefore->n_Before ) return tailBefore ; // Is_NotTheHeadNode
    return 0 ;
}

dlnode *
dllist_First ( dllist * list )
{
    dlnode * first ;
    if ( list && ( first = _dllist_First ( list ) ) ) return first ;
    else return 0 ;
}

dlnode *
dllist_Last ( dllist * list )
{
    dlnode * last ;
    if ( list && ( last = _dllist_Last ( list ) ) ) return last ;
    else return 0 ;
}

dlnode *
dllist_NodePrevious ( dllist * list, dlnode * node )
{
    if ( node )
    {
        node = _dlnode_Before ( node ) ;
    }
    if ( ! node ) node = dllist_Head ( list ) ;
    return node ;
}

// toward the HeadNode

dlnode *
_dllist_Before ( dllist * list )
{
    return dlnode_Previous ( list->l_CurrentNode ) ;
}

dlnode *
dllist_SetCurrentNode_Before ( dllist * list )
{
    list->l_CurrentNode = _dllist_Before ( list ) ;
    if ( list->l_CurrentNode == 0 )
    {
        list->l_CurrentNode = dllist_Head ( list ) ;
        return 0 ;
    }
    return list->l_CurrentNode ;
}
// toward the TailNode

dlnode *
_dllist_CurrentNodeAfter ( dllist * list )
{
    return dlnode_Next ( list->l_CurrentNode ) ;
}
// toward the TailNode

dlnode *
dllist_SetCurrentNode_After ( dllist * list )
{
    list->l_CurrentNode = _dllist_CurrentNodeAfter ( list ) ;
    if ( list->l_CurrentNode == 0 )
    {
        list->l_CurrentNode = dllist_Tail ( list ) ;
        return 0 ;
    }
    return ( dlnode* ) list->l_CurrentNode ;
}

void
_dllist_AddNamedValue ( dllist * list, byte * name, int64 value, uint64 allocType )
{
    Symbol * sym = _Symbol_New ( name, allocType ) ;
    sym->W_Value = value ;
    _dllist_AddNodeToHead ( list, ( dlnode* ) sym ) ;
}

void
_dllist_PushNode ( dllist* list, dlnode * node )
{
    _dllist_AddNodeToHead ( list, node ) ;
}

// use list like a endless stack

dlnode *
_dllist_PopNode ( dllist * list )
{
    dlnode *node = dllist_First ( ( dllist* ) list ) ;
    if ( node )
    {
        dlnode_Remove ( node ) ;
        return node ;
    }
    else return 0 ; // LIST_EMPTY
}

void
_dllist_DropN ( dllist * list, int64 n )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node && ( -- n >= 0 ) ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ; // before Remove
        dlnode_Remove ( node ) ;
    }
}

//_dllist_RemoveNodes including 'last'

void
_dllist_RemoveNodes ( dlnode *first, dlnode * last )
{
    dlnode * node, *nextNode ;
    for ( node = first ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ; // before Remove
        dlnode_Remove ( node ) ;
        if ( node == last ) break ;
    }
}

dlnode *
_dllist_Get_N_Node ( dllist * list, int64 n )
{
    dlnode * node ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node && ( -- n >= 0 ) ; node = dlnode_Next ( node ) ) ; // nb. this is a little subtle
    return node ;
}

int64
_dllist_Get_N_Node_M_Slot ( dllist * list, int64 n, int64 m )
{
    dlnode * node ;
    //for ( node = dllist_First ( ( dllist* ) list ) ; node && ( -- n >= 0 ) ; node = dlnode_Next ( node ) ) ; // nb. this is a little subtle
    node = _dllist_Get_N_Node ( list, n ) ;
    return node ? dobject_Get_M_Slot ( ( dobject* ) node, m ) : 0 ; // LIST_EMPTY
}

void
_dllist_Set_N_Node_M_Slot ( dllist * list, int64 n, int64 m, int64 value )
{
    dlnode * node ;
    node = _dllist_Get_N_Node ( list, n ) ;
    if ( node ) dobject_Set_M_Slot ( ( dobject* ) node, m, value ) ;
}

void
_dllist_MapNodes_UntilWord ( dlnode *first, VMapNodeFunction mf, Word * word )
{
    dlnode * node, *nextNode ;
    for ( node = first ; node ; node = nextNode )
    {
        Word * word1 = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) node, SCN_T_WORD ) ;
        if ( word1 == word ) break ;
        nextNode = dlnode_Next ( node ) ; // before Remove
        mf ( node ) ;
    }
}

void
dllist_Map4_FromFirstFlag_Indexed ( dllist * list, Boolean fromFirst, MapFunction4 mf, int64 one, int64 two, int64 three )
{
    dlnode * node, *nextNode, *prevNode ;
    int64 index ;
    if ( fromFirst )
    {
        for ( index = 0, node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode, index ++ )
        {
            // get nextNode before map function (mf) in case mf changes list by a Remove of current node
            // problems could arise if mf removes Next node
            nextNode = dlnode_Next ( node ) ;
            mf ( node, index, one, two, three ) ;
        }
    }
    else
    {
        for ( index = dllist_Depth ( list ), node = dllist_Last ( ( dllist* ) list ) ; node ; node = prevNode, index -- )
        {
            prevNode = dlnode_Previous ( node ) ;
            mf ( node, index, one, two, three ) ;
        }
    }
}

void
dllist_Map2_FromFirstFlag ( dllist * list, MapFunction2 mf, int64 one, int64 two, int64 fromFirstFlag )
{
    dlnode * node, *nextNode, *prevNode ;
    if ( fromFirstFlag )
    {
        for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
        {
            // get nextNode before map function (mf) in case mf changes list by a Remove of current node
            // problem could arise if mf removes Next node
            nextNode = dlnode_Next ( node ) ;
            mf ( node, one, two ) ;
        }
    }
    else
    {
        for ( node = dllist_Last ( ( dllist* ) list ) ; node ; node = prevNode )
        {
            prevNode = dlnode_Previous ( node ) ;
            mf ( node, one, two ) ;
        }
    }
}

void
dllist_Map ( dllist * list, MapFunction0 mf )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        // get nextNode before map function (mf) in case mf changes list by a Remove of current node
        // problem could arise if mf removes Next node
        nextNode = dlnode_Next ( node ) ;
        mf ( node ) ;
    }
}

void
dllist_Map1_FromEnd ( dllist * list, MapFunction1 mf, int64 one )
{
    dlnode *last, * node, *previousNode ;
    for ( last = dllist_Last ( ( dllist* ) list ), node = last ; node ; node = previousNode )
    {
        previousNode = dlnode_Previous ( node ) ;
        mf ( node, one ) ;
    }
}

void
dllist_Map1 ( dllist * list, MapFunction1 mf, int64 one )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        mf ( node, one ) ;
    }
}

void
dllist_Map1_Break ( dllist * list, MapFunction1 mf, int64 one )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        if ( mf ( node, one ) ) break ;
    }
}

void
dllist_Map1_FromNode ( dlnode * node0, MapFunction1 mf, int64 one )
{
    dlnode *node, *nextNode, *previousNode ;
    CSL_NewLine ( ) ;
    for ( node = node0 ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        mf ( node, one ) ;
    }
    CSL_NewLine ( ) ;
    for ( node = node0 ; node ; node = previousNode )
    {
        previousNode = dlnode_Previous ( node ) ;
        mf ( node, one ) ;
    }
}

int64
dllist_Map1_WReturn ( dllist * list, MapFunction1 mf, int64 one )
{
    int64 rtrn = 0 ;
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        if ( rtrn = mf ( node, one ) ) break ;
    }
    return rtrn ;
}

void
dllist_Map2_FromEnd ( dllist * list, MapFunction2 mf, int64 one, int64 two )
{
    dlnode * node, *previousNode ;
    for ( node = dllist_Last ( ( dllist* ) list ) ; node ; node = previousNode )
    {
        previousNode = dlnode_Previous ( node ) ;
        mf ( node, one, two ) ;
    }
}

void
dllist_Map2 ( dllist * list, MapFunction2 mf, int64 one, int64 two )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        mf ( node, one, two ) ;
    }
}

void
dllist_State_Map2 ( dllist * list, int64 state, VMapSymbol2 mf, int64 one, int64 two )
{
    dlnode * node, *nextNode ;
    Word * word ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        word = ( Word * ) node ;
        if ( word->State & state )
        {
            mf ( word, one, two ) ;
        }
    }
}

void
dllist_Map2_WithBreak ( dllist * list, MapFunction2 mf, int64 one, int64 two )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        if ( mf ( node, one, two ) ) break ;
    }
}

void
dllist_Map2_FromLast ( dllist * list, MapFunction2 mf, int64 one, int64 two )
{
    dlnode * node, *prevNode ;
    for ( node = dllist_Last ( ( dllist* ) list ) ; node ; node = prevNode )
    {
        prevNode = dlnode_Previous ( node ) ;
        if ( mf ( node, one, two ) ) break ;
    }
}

int64
dllist_Map4 ( dllist * list, MapFunction4wReturn mf, int64 one, int64 two, int64 three, int64 four )
{
    int64 rtrn = 0 ;
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        if ( rtrn = mf ( node, one, two, three, four ) ) break ;
    }
    return rtrn ;
}

void
dllist_Map_OnePlusStatus ( dllist * list, MapFunction2 mf, int64 one, int64 * status )
{
    dlnode * node, *nextNode ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node && ( *status != DONE ) ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        mf ( node, one, ( int64 ) status ) ;
    }
}

void
Tree_Map_Namespaces_State_2Args ( dllist * list, uint64 state, MapSymbolFunction2 mf, int64 one, int64 two )
{
    dlnode * node, *nextNode ;
    Word * word ;
    d0 ( _CSL_->FindWordCount = 0 ) ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        word = ( Word * ) node ;
        d0 ( _CSL_->FindWordCount ++ ) ;
        //if ( Is_DebugOn ) _Printf ( "\nTree_Map_Namespaces_State_2Args : %s", word->Name ) ;
        if ( Is_NamespaceType ( word ) )
        {
            if ( word->State & state )
            {
                //if ( Is_DebugOn ) _Printf ( "\nTree_Map_Namespaces_State_2Args : pre-mf : %s", word->Name ) ;
                mf ( ( Symbol* ) word, one, two ) ;
            }
            Tree_Map_Namespaces_State_2Args ( word->W_List, state, mf, one, two ) ;
        }
    }
    d0 ( CSL_WordAccounting ( ( byte* ) "Tree_Map_State_2" ) ) ;
}

void
Tree_Map_NamespacesTree ( dllist * list, MapSymbolFunction mf )
{
    dlnode * node, *nextNode ;
    Word * word ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        word = ( Word * ) node ;
        if ( Is_NamespaceType ( word ) ) Tree_Map_NamespacesTree ( word->W_List, mf ) ;
        else mf ( ( Symbol* ) word ) ;
    }
}

void
Tree_Map_Namespaces ( dllist * list, MapSymbolFunction mf )
{
    dlnode * node, *nextNode ;
    Word * word ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        word = ( Word * ) node ;
        if ( Is_NamespaceType ( word ) )
        {
            mf ( ( Symbol* ) word ) ;
            Tree_Map_Namespaces ( word->W_List, mf ) ;
        }
    }
}

Word *
Tree_Map_OneNamespace_OneArg  ( Word * word, MapFunction_1 mf1, int64 one )
{
    Word *nextWord ;
    for ( ; word ; word = nextWord )
    {
        nextWord = ( Word* ) dlnode_Next ( ( node* ) word ) ;
        //if ( Is_DebugOn ) _Printf ( " %s.%s", word->ContainingNamespace ? word->ContainingNamespace->Name : (byte*) "", word->Name ) ;
        //d0 ( CSL->FindWordCount ++ ) ;
        if ( mf1 ( ( Symbol* ) word, one ) ) return word ;
        //if ( word == nextWord ) return (Word *) -1 ; // circular list ?!
    }
    return 0 ;
}

void
Tree_Map_OneNamespace ( Namespace * ns, MapFunction mf )
{
    Word * word, *nextWord ;
    for ( word = ( Word * ) dllist_First ( ( dllist* ) ns->W_List ) ; word ; word = nextWord )
    {
        nextWord = ( Word* ) dlnode_Next ( ( node* ) word ) ;
        mf ( word ) ;
    }
}


void
Tree_Map_OneList ( dllist* list, MapFunction mf )
{
    Word * word, *nextWord ;
    for ( word = ( Word * ) dllist_First ( list ) ; word ; word = nextWord )
    {
        nextWord = ( Word* ) dlnode_Next ( ( node* ) word ) ;
        mf ( word ) ;
    }
}


void
Tree_Map_NamespacesTree1 ( dllist * list, MapSymbolFunction1 mf1, int64 one  )
{
    dlnode * node, *nextNode ;
    Word * word ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nextNode )
    {
        nextNode = dlnode_Next ( node ) ;
        word = ( Word * ) node ;
        if ( Is_NamespaceType ( word ) ) Tree_Map_NamespacesTree1 ( word->W_List, mf1, one ) ;
        mf1 ( ( Symbol* ) word, one ) ;
    }
}

Word *
Tree_Map_OneNamespace_TwoArgs ( Namespace * ns, MapFunction_2 mf2, int64 one, int64 two )
{
    Word * word, *nextWord ;
    for ( word = ( Word * ) dllist_First ( ( dllist* ) ns->W_List ) ; word ; word = nextWord )
    {
        nextWord = ( Word* ) dlnode_Next ( ( node* ) word ) ;
        if ( mf2 ( ( Symbol* ) word, one, two ) ) return word ;
    }
    return 0 ;
}

Word *
Tree_Map_State_OneArg ( uint64 state, MapFunction_1 mf, int64 one )
{
    if ( _CSL_->Namespaces )
    {
        Word * word, * word2, *nextWord ;
        _CSL_->FindWordCount = 1 ;
        if ( mf ( ( Symbol* ) _CSL_->Namespaces, one ) ) return _CSL_->Namespaces ;
        for ( word = ( Word * ) dllist_First ( ( dllist* ) _CSL_->Namespaces->W_List ) ; word ; word = nextWord )
        {
            nextWord = ( Word* ) dlnode_Next ( ( node* ) word ) ;
            _CSL_->FindWordCount ++ ;
            if ( mf ( ( Symbol* ) word, one ) ) return word ;
            else if ( ( Is_NamespaceType ( word ) ) && ( word->State & state ) )
            {
                //if ( Is_DebugOn && String_Equal (word->Name, "Defines") ) Pause () ;
                if ( ( word2 = Tree_Map_OneNamespace_OneArg ( ( Word* ) dllist_First ( ( dllist* ) word->W_List ), mf, one ) ) )
                    return word2 ;
            }
        }
    }
    return 0 ;
}

void
List_N_M_Node_PrintWords ( dllist * alist )
{
    dllist_Map1_FromEnd ( alist, ( MapFunction1 ) Word_N_M_Node_Print, 0 ) ;
}

// i don't like not to have C access to a structure but i want dobject allocation in init to 
// be robustly tested for csl use

dobject *
_dllist_PushNew_M_Slot_Node ( dllist* list, int64 allocType, int64 typeCode, int64 m_slots, ... )
{
    int64 i, value ;
    va_list args ;
    va_start ( args, m_slots ) ;
    dobject * dobj = dobject_Allocate ( typeCode, m_slots, allocType ) ;
    for ( i = 0 ; i < m_slots ; i ++ )
    {
        value = va_arg ( args, int64 ) ;
        dobj->do_iData[i] = value ;
    }
    va_end ( args ) ;
    //dobject * dobj = dobject_New_M_Slot ( allocType, typeCode, m_slots, ... )

    _dllist_PushNode ( list, ( dlnode* ) dobj ) ;

    return dobj ;
}

inline
void
List_Push ( dllist *list, dlnode * node )
{
    _dllist_AddNodeToHead ( list, ( dlnode* ) node ) ;
}

inline
dlnode *
List_Pop ( dllist *list )
{
    return _dllist_PopNode ( list ) ;
}

inline
dlnode *
List_Top ( dllist *list )
{
    return dllist_First ( list ) ;
}

inline
int64
dobject_Get_M_Slot ( dobject* dobj, int64 m )
{
    if ( dobj ) return dobj->do_iData [m] ;
    else return 0 ;
}

inline
void
dobject_Set_M_Slot ( dobject* dobj, int64 m, int64 value )
{
    if ( dobj ) dobj->do_iData [m] = value ;
}

inline
void
List_Set_N_Node_M_Slot ( dllist *list, int64 n, int64 m, int64 value )
{
    _dllist_Set_N_Node_M_Slot ( list, n, m, value ) ;
}

inline
int64
List_Get_N_Node_M_Slot ( dllist *list, int64 n, int64 m )
{
    return _dllist_Get_N_Node_M_Slot ( list, n, m ) ;
}

// first slot of the n node

inline
int64
List_GetN_Value ( dllist *list, int64 n )
{
    return _dllist_Get_N_Node_M_Slot ( list, n, 0 ) ;
}

inline
int64
List_Pick_Value ( dllist *list, int64 n )
{
    return List_GetN_Value ( list, n ) ;
}

inline
int64
List_Pop_1Value ( dllist *list )
{
    //return List_GetN_Value ( list, n ) ;
    dobject* dobj = ( dobject * ) _dllist_PopNode ( list ) ;
    return dobj->do_iData [0] ;
}

inline
dlnode *
List_Pick ( dllist *list, int64 n )
{
    return _dllist_Get_N_Node ( list, n ) ;
}

inline
void
List_SetN_Value ( dllist *list, int64 n, int64 value )
{
    dobject * dobj = ( dobject* ) _dllist_Get_N_Node ( list, n ) ;
    dobject_Set_M_Slot ( dobj, 0, value ) ;
}

inline
void
List_SetTop_Value ( dllist *list, int64 value )
{
    List_SetN_Value ( list, 0, value ) ;
}

inline
int64
List_Top_Value ( dllist *list )
{
    return List_GetN_Value ( list, 0 ) ;
}

inline
int64
List_Depth ( dllist *list )
{
    return dllist_Depth ( list ) ;
}

inline
int64
List_Length ( dllist *list )
{
    return dllist_Depth ( list ) ;
}

inline
void
List_Push_1Value_NewNode_T_WORD ( dllist *list, int64 value, int64 allocType )
{
    _dllist_PushNew_M_Slot_Node ( list, allocType, T_WORD, 1, value ) ;
}

inline
void
_List_PushNew_ForWordList ( dllist *list, Word * word, int64 inUseFlag )
{
    _dllist_PushNew_M_Slot_Node ( list, WORD_RECYCLING, T_WORD, SCN_NUMBER_OF_SLOTS, ( ( int64 ) word ), word->W_SC_Index, inUseFlag ) ;
}

inline
void
_List_PushNew_1Value ( dllist *list, int64 allocType, int64 typeCode, int64 value )
{
    _dllist_PushNew_M_Slot_Node ( list, allocType, typeCode, 1, value ) ;
}

inline
void
List_PushNew_T_WORD ( dllist *list, int64 value, int64 allocType )
{
    _List_PushNew_1Value ( list, value, T_WORD, allocType ) ;
}

