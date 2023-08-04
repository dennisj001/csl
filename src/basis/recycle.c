
#include "../include/csl.h"

// only check a first node, if no first node the list is empty

byte *
DLList_CheckRecycledForAllocation ( dllist * list, int64 size )
{
    DLNode * node = 0 ;
    if ( list ) node = ( DLNode* ) dllist_First ( ( dllist* ) list ) ;
    if ( node )
    {
        dlnode_Remove ( ( dlnode* ) node ) ; // necessary else we destroy the list!
        Mem_Clear ( ( byte* ) node, size ) ;
        node->n_DLNode.n_InUseFlag = N_IN_USE ;
        return ( byte* ) node ;
    }
    else return 0 ;
}

void
OVT_RecyclingAccounting ( int64 flag )
{
    if ( flag == OVT_RA_RECYCLED )
    {
        _O_->MemorySpace0->RecycledWordCount ++ ;
        _O_->MemorySpace0->WordsInRecycling -- ;
    }
    else if ( flag == OVT_RA_ADDED ) _O_->MemorySpace0->WordsInRecycling ++ ;

}

void
OVT_Recycle ( dlnode * anode )
{
    if ( anode ) dllist_AddNodeToTail ( _O_->RecycledWordList, anode ) ;
    OVT_RecyclingAccounting ( OVT_RA_ADDED ) ;
}

// put a word on the recycling list

void
Word_Recycle ( Word * w )
{
    if ( w->S_WAllocType == DICTIONARY )
    {
        OVT_Recycle ( ( dlnode * ) w ) ;
        w->W_ObjectAttributes &= ~ ( RECYCLABLE_COPY | RECYCLABLE_LOCAL ) ;
    }
}

void
_CheckRecycleWord ( Node * node )
{
    Word * w = ( Word * ) node ;
    if ( w && ( w->W_ObjectAttributes & ( RECYCLABLE_COPY | RECYCLABLE_LOCAL ) ) )
    {
        if ( Verbosity ( ) > 2 ) _Printf ( "\n_CheckRecycleWord : recycling : %s%s%s",
            w->S_ContainingNamespace ? w->S_ContainingNamespace->Name : ( byte* ) "", w->S_ContainingNamespace ? ( byte* ) "." : ( byte* ) "", w->Name ) ; //, Pause () ;
        Word_Recycle ( w ) ;
    }
}

// check a compiler word list for recycleable words and add them to the recycled word list : _O_->MemorySpace0->RecycledWordList

void
DLList_Recycle_NamespaceList ( dllist * list )
{
    dllist_Map ( list, ( MapFunction0 ) _CheckRecycleWord ) ;
}

