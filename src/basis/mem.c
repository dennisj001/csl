
#include "../include/csl.h"

void
NBA_Show ( NamedByteArray * nba, int64 flag )
{
    NBA_PrintInfo ( nba ) ;
    if ( flag && ( Verbosity () > 2 ) )
    {
        dlnode * node, *nodeNext ;
        for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
            MemChunk_Show ( &ba->BA_MemChunk ) ;
        }
    }
}

void
NBA_PrintInfo ( NamedByteArray * nba )
{
    byte * name = nba->NBA_Symbol.S_Name ;
    if ( Verbosity () > 2 )
    {
        iPrintf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d : OrigAllocSize = %8d",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining,
            nba->SmallestRemaining, nba->NBA_DataSize, nba->OriginalSize ) ;
    }
    else if ( Verbosity () > 2 )
    {
        iPrintf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d : OrigAllocSize = %8d : alloctype = %8lu ",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining,
            nba->SmallestRemaining, nba->NBA_DataSize, nba->OriginalSize, ( uint64 ) nba->NBA_AAttribute ) ;
    }
    else
    {
        iPrintf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining, nba->SmallestRemaining, nba->NBA_DataSize ) ;
    }
}

void
NBA_AccountRemainingAndShow ( NamedByteArray * nba, Boolean flag )
{
    if ( nba )
    {
        byte * name = nba->NBA_Symbol.S_Name ;
        int64 largest = 0, smallest = 0 ;
        nba->MemRemaining = 0 ;
        dlnode * node, *nodeNext ;
        for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
            nba->MemRemaining += ba->MemRemaining ;
            if ( ! largest ) largest = ba->MemRemaining ;
            else if ( ba->MemRemaining > largest ) largest = ba->MemRemaining ;
            if ( ! smallest ) smallest = ba->MemRemaining ;
            else if ( ba->MemRemaining < smallest ) smallest = ba->MemRemaining ;
        }
        nba->LargestRemaining = largest ;
        nba->SmallestRemaining = smallest ;
        if ( flag ) NBA_PrintInfo ( nba ) ;
    }
}

void
OVT_ShowNBAs ( OpenVmTil * ovt, int64 flag )
{
    if ( ovt )
    {
        dlnode * node, *nodeNext ;
        if ( ovt->MemorySpace0 && ( node = dllist_First ( ( dllist* ) ovt->MemorySpace0->NBAs ) ) )
        {
            for ( ; node ; node = nodeNext )
            {

                nodeNext = dlnode_Next ( node ) ;
                NamedByteArray * nba = Get_NbaSymbolNode_To_NBA ( node ) ;
                NBA_Show ( nba, flag ) ;
            }
        }
        printf ( "\n" ) ;
        fflush ( stdout ) ;
    }
}

int64
_OVT_CalculateShowMemList ( OpenVmTil * ovt )
{
    int64 size, diff ;
    if ( ovt )
    {
        dlnode * node, *nodeNext ;
        if ( ovt->Verbosity > 2 ) printf ( "\nMemChunk List :: " ) ;
        for ( size = 0, node = dllist_First ( ( dllist* ) _OMS_->OvtMemChunkList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->mc_ChunkSize ;
        }
        for ( node = dllist_First ( ( dllist* ) _OSMS_->HistorySpace_MemChunkStringList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->mc_ChunkSize ;
        }
        for ( node = dllist_First ( _OSMS_->OVT_StaticMemList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->mc_ChunkSize ;
        }
        ovt->TotalRemainingAccounted = size ;
        diff = abs ( _OMS_->RemainingAllocated + _OSMS_->RemainingAllocated - ovt->TotalRemainingAccounted ) ;
        diff -= (sizeof ( OVT_MemSystem ) + sizeof ( OVT_StaticMemSystem ) + ( 2 * sizeof ( dlnode ) + sizeof (dllist) ) ) ; // remaining STATIC allocated mem accounted in the StaticMemSystem
        if ( diff )
        {
            printf ( "\nTotalRemainingAccounted       = %9ld : RemainingAllocated = %9ld :: diff = %6ld :: mmaped = %9ld", 
                size, _OMS_->RemainingAllocated + _OSMS_->RemainingAllocated, diff, mmaped ) ;
            fflush ( stdout ) ;
        }
        iPrintf ( "\nNon-Static Mem Allocated      = %9ld", ovt->TotalNbaAccountedMemAllocated ) ;
    }
    return diff ;
}

void
_OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( OpenVmTil * ovt, Boolean showFlag )
{
    if ( ovt )
    {
        dlnode * node, * nextNode, *msNode, *nextMsNode ;
        NamedByteArray * nba ;
        MemorySpace * ms ;
        ovt->TotalNbaAccountedMemRemaining = 0 ;
        ovt->TotalNbaAccountedMemAllocated = 0 ;
        if ( ovt && ovt->MemorySpace0 )
        {
            for ( msNode = dllist_First ( ( dllist* ) ovt->MemorySpaceList ) ; msNode ; msNode = nextMsNode )
            {
                ms = ( MemorySpace * ) msNode ;
                nextMsNode = dlnode_Next ( msNode ) ;
                for ( node = dllist_First ( ( dllist* ) ms->NBAs ) ; node ; node = nextNode )// nb. NBAs is the NBA Symbol list
                {
                    nextNode = dlnode_Next ( node ) ;
                    nba = ( NBA* ) Get_NbaSymbolNode_To_NBA ( node ) ;
                    NBA_AccountRemainingAndShow ( nba, showFlag ) ;
                    ovt->TotalNbaAccountedMemAllocated += nba->TotalAllocSize ;
                    ovt->TotalNbaAccountedMemRemaining += nba->MemRemaining ;
                }
                for ( node = dllist_First ( ( dllist* ) ovt->NBAs ) ; node ; node = nextNode )// nb. NBAs is the NBA Symbol list
                {
                    nextNode = dlnode_Next ( node ) ;
                    nba = ( NBA* ) Get_NbaSymbolNode_To_NBA ( node ) ;
                    if ( nba ) // ?? should not need this
                    {
                        NBA_AccountRemainingAndShow ( nba, showFlag ) ;
                        ovt->TotalNbaAccountedMemAllocated += nba->TotalAllocSize ;
                        ovt->TotalNbaAccountedMemRemaining += nba->MemRemaining ;
                    }
                }
            }
        }
        nba = _OSMS_->StaticMemSpace ;
        NBA_AccountRemainingAndShow ( nba, showFlag ) ;
        ovt->TotalNbaAccountedMemAllocated += nba->TotalAllocSize ;
        ovt->TotalNbaAccountedMemRemaining += nba->MemRemaining ;
    }
}

void
OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( OpenVmTil * ovt, int64 flag )
{
    _OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( ovt, flag ) ;
    if ( _CSL_ && _DataStack_ ) // so we can use this function anywhere
    {
        int64 dsu = DataStack_Depth ( ) * sizeof (int64 ) ;
        int64 dsa = ( STACK_SIZE * sizeof (int64 ) ) - dsu ;
        iPrintf ( "\nData Stack              InUse = %9ld : Unused = %9ld", dsu, dsa ) ;
    }
    printf ( "\nTotal Accounted Mem     InUse = %9ld : Unused = %9ld",
        ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining, ovt->TotalNbaAccountedMemRemaining ) ;
    fflush ( stdout ) ;
}

void
_OVT_ShowMemoryAllocated ( OpenVmTil * ovt )
{
    Boolean vf = ( ovt->Verbosity > 1 ) ;
    if ( ! vf ) iPrintf ( c_gu ( "\nIncrease the verbosity setting to 2 or more for more info here. ( Eg. : verbosity 2 = )" ) ) ;
    OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( ovt, 1 ) ;
    int64 aleak = _OVT_CalculateShowMemList ( ovt ) ;
    int64 leak = mmaped - ( _OSMS_->RemainingAllocated + _OMS_->RemainingAllocated ) ;
    //leak = 10 ; // test
    byte * leaks = ( byte* ) "\nleak?                            = %9ld : ==  ( mmaped - ( _OSMS_->RemainingAllocated + _OMS_->RemainingAllocated )" ;
    if ( leak ) iPrintf ( c_ad ( leaks ), leak ) ;
    else if ( vf ) iPrintf ( c_ud ( leaks ), leak ) ;
    iPrintf ( "\nTotal Accounting errors       = %9ld", aleak ) ;
    iPrintf ( "\nTotal Actual leaks            = %9ld", leak ) ;
    iPrintf ( "\nNBA ReAllocations             = %9ld", _O_->ReAllocations ) ;
    int64 wordSize = ( sizeof ( Word ) + sizeof ( WordData ) ) ;
    iPrintf ( "\nWordsAllocation               = %9ld : %-5ld x %3ld bytes", _O_->MemorySpace0->WordsAllocated * wordSize, _O_->MemorySpace0->WordsAllocated, wordSize ) ;
    iPrintf ( "\nRecycled-Bytes:WordCount      = %9ld : %-5ld x %3ld bytes", _O_->MemorySpace0->RecycledWordCount * wordSize, _O_->MemorySpace0->RecycledWordCount, wordSize ) ;
    iPrintf ( "\nInRecycling-Bytes:Words       = %9ld : %-5ld x %3ld bytes\n", _O_->MemorySpace0->WordsInRecycling * wordSize, _O_->MemorySpace0->WordsInRecycling, wordSize ) ;
    Buffer_PrintBuffers ( ) ;
    if ( leak )
    {
        iPrintf ("\n_OSMS_->Allocated = %ld : _OSMS_->Freed = %ld : _OSMS_->RemainingAllocated = %ld", _OSMS_->Allocated, _OSMS_->Freed, _OSMS_->RemainingAllocated ) ;
        iPrintf ("\n_OMS_->Allocated = %ld : _OMS_->Freed = %ld : _OMS_->RemainingAllocated = %ld", _OMS_->Allocated, _OMS_->Freed, _OMS_->RemainingAllocated ) ;
        iPrintf ("\nmmaped = %ld : mmaped - ( _OSMS_->RemainingAllocated + _OMS_->RemainingAllocated ) = %ld", mmaped, 
            mmaped - ( _OSMS_->RemainingAllocated + _OMS_->RemainingAllocated ) ) ;
    }
}

// 'mem'

void
OVT_ShowMemoryAllocated ( )
{
    _OVT_ShowMemoryAllocated ( _O_ ) ;
}

void
_OVT_CheckCodeSpaceForRoom ( int64 memDesired )
{
    if ( ! _O_CodeByteArray->OurNBA ) _O_CodeByteArray->OurNBA = _O_->MemorySpace0->CodeSpace ;
    _O_CodeByteArray = _ByteArray_AppendSpace_MakeSure ( _O_CodeByteArray, memDesired ) ;
}

void
OVT_CheckCodeSpaceForRoom ( )
{
    _OVT_CheckCodeSpaceForRoom ( 4 * K ) ;
}

void
OVT_MemList_DeleteNBAMemory ( byte * name, Boolean reinitFlag )
{
    NamedByteArray *nba = _OVT_Find_NBA ( name ) ;
    NamedByteArray_Delete ( nba, reinitFlag ) ;
}

void
OVT_MemList_FreeNBAMemory ( byte * name, uint64 moreThan, int64 always )
{
    NamedByteArray *nba = _OVT_Find_NBA ( name ) ;
    _OVT_MemList_FreeNBAMemory ( nba, moreThan, always ) ;
}

void
OVT_MemListFree_ContextMemory ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "ContextSpace", 1 * M, 1 ) ;
}

void
OVT_MemListFree_TempObjects ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "TempObjectSpace", 1 * M, 1 ) ;
}

void
OVT_MemListFree_Objects ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "ObjectSpace", 1 * M, 1 ) ;
}

void
_OVT_MemListFree_LispSpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "LispSpace", 1 * M, 1 ) ;
}

void
OVT_MemListFree_LispSpace ( )
{
    _LC_Delete ( _LC_ ) ;
}

void
_OVT_MemListFree_WordRecyclingSpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "WordRecylingSpace", 0, 1 ) ;
}

void
_OVT_MemListFree_CompilerTempObjectSpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "CompilerTempObjectSpace", 0, 1 ) ;
}

void
OVT_MemListFree_CompilerTempObjects ( )
{
    _OVT_MemListFree_CompilerTempObjectSpace ( ) ;
}

void
OVT_MemListFree_LispTemp ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "LispTempSpace", 2 * M, 1 ) ;
    if ( _LC_ )
    {
        _dllist_Init ( _LC_->LispTempNamespace->W_List ) ;
        LC_Init_Variables ( _LC_ ) ;
    }
}

void
OVT_MemListFree_Session ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "SessionObjectsSpace", 2 * M, 1 ) ;
}

void
OVT_MemListFree_Buffers ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "BufferSpace", 2 * M, 0 ) ;
}

void
OVT_MemListFree_HistorySpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "HistorySpace", 1 * M, 0 ) ;
}

void
_OVT_MemListFree_CSLInternal ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "CSLInternalSpace", 0, 1 ) ;
}

void
_NBAsMemList_FreeTypes ( int64 allocType, int64 exactFlag )
{
    dllist_Map2 ( _O_->MemorySpace0->NBAs, ( MapFunction2 ) NBA_FreeChunkType, allocType, exactFlag ) ;
}

void
NBAsMemList_FreeExactType ( int64 allocType )
{
    _NBAsMemList_FreeTypes ( allocType, 1 ) ;
}

void
NBAsMemList_FreeVariousTypes ( int64 allocType )
{
    _NBAsMemList_FreeTypes ( allocType, 0 ) ;
}

