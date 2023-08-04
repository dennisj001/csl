
#include "../../include/csl.h"

void
_ByteArray_UnAppendSpace ( ByteArray * ba, int64 size ) // size in bytes
{
    // ?? no error checking ??
    ba->EndIndex -= size ;
    ba->StartIndex -= size ;
}

void
_ByteArray_DataClear ( ByteArray * ba )
{
    Mem_Clear ( ba->BA_Data, ba->BA_DataSize ) ;
}

void
_ByteArray_Init ( ByteArray * ba )
{
    ba->BA_Data = ( byte* ) ( ba + 1 ) ;
    ba->StartIndex = ba->BA_Data ;
    ba->EndIndex = ba->StartIndex ;
    ba->bp_Last = & ba->BA_Data [ ba->BA_DataSize - 1 ] ;
    ba->MemRemaining = ba->BA_DataSize ;
    _ByteArray_DataClear ( ba ) ;
}

int64
ByteArray_IsAddressWwitinTheArray ( ByteArray * ba, byte * address )
{
    if ( ( address >= ( byte* ) ba->BA_Data ) && ( address <= ( byte* ) ba->bp_Last ) ) return true ; // ?!? not quite accurate
    return false ;
}

ByteArray *
ByteArray_Init ( ByteArray * ba, int64 size, uint64 type )
{
    // we want to keep track of how much data for each type separate from MemChunk accounting
    ba->BA_MemChunk.mc_AllocType = type ; 
    ba->BA_DataSize = size ;
    ba->BA_AllocSize = size + sizeof (ByteArray ) ;
    ba->BA_AAttribute = type ;
    Set_BA_Symbol_To_BA ( ba ) ; // nb! : ByteArray has two nodes, a MemChunk and a Symbol, each on different lists 
    _ByteArray_Init ( ba ) ;
    return ba ;
}

ByteArray *
ByteArray_AllocateNew ( int64 size, uint64 allocType )
{
    ByteArray * ba = ( ByteArray* ) MemChunk_AllocateListAdd ( size + sizeof ( ByteArray ), allocType ) ;
    ByteArray_Init ( ba, size, allocType ) ;
    return ba ;
}

byte *
_ByteArray_GetEndIndex ( ByteArray * ba )
{
    return ba->EndIndex ;
}

byte *
_ByteArray_Here ( ByteArray * ba )
{
    return ba->EndIndex ;
}

void
_ByteArray_SetEndIndex ( ByteArray * ba, byte * index )
{
    ba->EndIndex = index ;
}

void
_ByteArray_SetHere ( ByteArray * ba, byte * index )
{
    ba->EndIndex = index ;
}

void
_SetPreHere_ForDebug ( byte * address )
{
    _Debugger_->PreHere = address ;
}

void
SetPreHere_ForDebug ( byte * address )
{
    if ( _Debugger_ ) _SetPreHere_ForDebug ( address ) ;
}

void
ByteArray_SetHere_AndForDebug (ByteArray * ba, byte * address)
{
    if ( address )
    {
        _ByteArray_SetHere ( ba, address ) ;
        //if ( forDebugFlag ) 
        SetPreHere_ForDebug ( address ) ;
    }
}

void
SetHere (byte * address)
{
    if ( address ) ByteArray_SetHere_AndForDebug (_O_->CodeByteArray, address) ;
}

byte *
_ByteArray_GetStartIndex ( ByteArray * ba )
{
    return ba->StartIndex ;
}

void
_ByteArray_SetStartIndex ( ByteArray * ba, byte * address )
{
    ba->StartIndex = address ;
}

// ! TODO : should be macros here !

void
ByteArray_AppendCopyInteger ( ByteArray * ba, int64 size, int64 data ) // size in bytes
{
    _ByteArray_AppendSpace ( ba, size ) ; // size in bytes
    byte * address = ba->StartIndex ;
    if ( address )
    {
        switch ( size )
        {
            case 1:
            {
                *( ( int8* ) address ) = ( byte ) data ;
                break ;
            }
            case 2:
            {
                *( ( int16* ) address ) = ( int16 ) data ;
                break ;
            }
            case 4:
            {
                *( ( int32* ) address ) = ( int32 ) data ;
                break ;
            }
            case 8:
            {
                *( ( int64* ) address ) = ( int64 ) data ;
                break ;
            }
        }
    }
    else Error ( "ByteArray_AppendCopyItem : Out of memory", ABORT ) ;
}

void
ByteArray_AppendCopy ( ByteArray * ba, int64 size, byte * data ) // size in bytes
{
    byte *memSpace = _ByteArray_AppendSpace ( ba, size ) ; // size in bytes
    MemCpy ( memSpace, data, size ) ;
}

void
ByteArray_AppendCopyUpToRET ( ByteArray * ba, byte * data ) // size in bytes
{
    int64 i ;
    for ( i = 0 ; 1 ; i ++ )
    {
        if ( data [ i ] == _RET ) break ;
    }
    ByteArray_AppendCopy ( ba, i, data ) ; // ! after we find out how big 'i' is
}

void
_NBA_SetCompilingSpace_MakeSureOfRoom ( NamedByteArray * nba, int64 room )
{
    if ( nba )
    {
        ByteArray * ba = _ByteArray_AppendSpace_MakeSure ( nba->ba_CurrentByteArray, room ) ;
        if ( ! ba ) Error_Abort ( "\n_NBA_SetCompilingSpace_MakeSureOfRoom :", "no ba?!\n" ) ;
        Set_CompilerSpace ( ba ) ;
    }
}

ByteArray *
_NamedByteArray_AddNewByteArray ( NamedByteArray *nba, int64 size )
{
    if ( size < nba->NBA_DataSize )
    {
        size = nba->NBA_DataSize ;
    }
    nba->MemAllocated += size ;
    nba->MemRemaining += size ;
    nba->ba_CurrentByteArray = ByteArray_AllocateNew ( size, nba->NBA_AAttribute ) ; // the whole ba itself is allocated as a chunk then we can allocate with its specific type
    dllist_AddNodeToHead ( &nba->NBA_BaList, ( dlnode* ) & nba->ba_CurrentByteArray->BA_Symbol ) ; // ByteArrays are linked here in the NBA with their BA_Symbol node. BA_MemChunk is linked in PermanentMemList
    nba->ba_CurrentByteArray->BA_Symbol.S_Value = ( uint64 ) nba->ba_CurrentByteArray ; // for FreeNbaList
    nba->ba_CurrentByteArray->OurNBA = nba ;
    nba->TotalAllocSize += nba->ba_CurrentByteArray->BA_AllocSize ; //BA_MemChunk.S_ChunkSize ; //+ sizeof ( ByteArray );

    nba->NumberOfByteArrays ++ ;
    return nba->ba_CurrentByteArray ;
}

NamedByteArray *
_NamedByteArray_Allocate ( int64 allocType )
{
    NamedByteArray * nba = ( NamedByteArray* ) MemChunk_AllocateListAdd ( sizeof ( NamedByteArray ), allocType ) ;
    return nba ;
}

NamedByteArray *
NamedByteArray_Allocate ( )
{
    NamedByteArray * nba = _NamedByteArray_Allocate ( OPENVMTIL ) ; // allocate the nba structure OPENVMTIL but nba->NBA_AAttribute has it's own different type
    return nba ;
}

void
_NamedByteArray_Init ( NamedByteArray * nba, byte * name, int64 size, int64 atype )
{
    _Symbol_NameInit ( ( Symbol* ) & nba->NBA_Symbol, name ) ;
    nba->NBA_MemChunk.mc_Name = name ;
    nba->NBA_AAttribute = atype ;
    dllist_Init ( &nba->NBA_BaList, &nba->NBA_ML_HeadNode, &nba->NBA_ML_TailNode ) ;
    nba->NBA_DataSize = nba->OriginalSize = size ;
    nba->MemInitial = size ;
    nba->SmallestRemaining = size ;
    nba->LargestRemaining = size ; 
    nba->TotalAllocSize = sizeof ( NamedByteArray ) ;
    Set_NbaSymbolNode_To_NBA ( nba ) ;
    nba->NBA_MemChunk.mc_unmap = nba->NBA_MemChunk.mc_unmap ;
    nba->NumberOfByteArrays = 0 ;
    nba->Allocations = 1 ;
    _NamedByteArray_AddNewByteArray ( nba, size ) ;
}

NamedByteArray *
NamedByteArray_New ( byte * name, int64 size, int64 atype )
{
    NamedByteArray * nba ;
    //if ( String_Equal ( "HistorySpace", name ) ) _Printf ( "\nNamedByteArray_New : name = %s", name ) ;
    //if (atype == OVT_STATIC) nba = (NamedByteArray * ) Mem_Allocate ( sizeof ( OpenVmTil ), OVT_STATIC ) ; 
    //else 
    nba = NamedByteArray_Allocate ( ) ; //NamedByteArray_Allocate ( ) ; // else the nba would be deleted with MemList_FreeExactType ( nba->NBA_AAttribute ) ;
    _NamedByteArray_Init ( nba, name, size, atype ) ;
    return nba ;
}
// returns true if address is in this nba memory space

int64
NamedByteArray_CheckAddress ( NamedByteArray * nba, byte * address )
{
    ByteArray * ba ;
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
    {
        nodeNext = dlnode_Next ( node ) ;
        ba = Get_BA_Symbol_To_BA ( node ) ;
        if ( ba && ByteArray_IsAddressWwitinTheArray ( ba, address ) == true ) return true ;
        //if ( ( address >= ( byte* ) ba->BA_Data ) && ( address <= ( byte* ) ba->bp_Last ) ) return true ; // ?!? not quite accurate
    }
    return false ;
}

ByteArray *
_ByteArray_AppendSpace_MakeSure ( ByteArray * ba, int64 size ) // size in bytes
{
    NamedByteArray * nba = ba->OurNBA ;
    if ( nba )
    {
        while ( ba->MemRemaining < size )
        {
            int64 largestRemaining = 0 ;
            // check the other bas in the nba list to see if any have enough remaining
            {
                dlnode * node, *nodeNext ;
                for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
                {
                    if ( node == nodeNext ) break ; // ?? TODO : should need this
                    nodeNext = dlnode_Next ( node ) ;
                    ba = Get_BA_Symbol_To_BA ( node ) ;
                    if ( ba )
                    {
                        if ( ba->MemRemaining > largestRemaining ) largestRemaining = ba->MemRemaining ;
                        if ( ba->MemRemaining >= size ) goto done ;
                    }
                }
            }
            _O_->ReAllocations ++ ;
            //size = ( size > ( 100 * K ) ) ? size : ( 100 * K ) ;
            nba->NBA_DataSize *= ++ nba->Allocations ;
            if ( Verbosity () > 2 ) NBA_PrintInfo ( nba ) ;
            ba = _NamedByteArray_AddNewByteArray ( nba, nba->NBA_DataSize ) ;
        }
    }
    else Error_Abort ( "_ByteArray_AppendSpace_MakeSure", ( byte* ) "\n_ByteArray_AppendSpace_MakeSure : no nba?!\n" ) ;
done:
    nba->ba_CurrentByteArray = ba ;
    return ba ;
}

//macros.h :  _Allocate( size, nba ) _ByteArray_AppendSpace ( nba->ba_CurrentByteArray, size ) 

byte *
_ByteArray_AppendSpace ( ByteArray * ba, int64 size ) // size in bytes
{
    while ( ba->MemRemaining < size )
    {
        ba = _ByteArray_AppendSpace_MakeSure ( ba, size ) ;
    }
    ba->StartIndex = ba->EndIndex ; // move index to end of the last append
    ba->EndIndex += size ;
    if ( ba->OurNBA ) ba->OurNBA->MemRemaining -= size ; //nb. debugger->StepInstructionBA doesn't have an nba
    ba->MemRemaining -= size ;
    return ba->StartIndex ;
}

NamedByteArray *
NBA_MemSpace_New ( MemorySpace * memSpace, byte * name, int64 size, int64 allocType )
{
    NamedByteArray *nba = NamedByteArray_New ( name, size, allocType ) ;
    dllist_AddNodeToHead ( memSpace->NBAs, ( dlnode* ) & nba->NBA_Symbol ) ;
    return nba ;
}

NamedByteArray *
NBA_OvtNew ( byte * name, int64 size, int64 allocType )
{
    OpenVmTil * ovt = _O_ ;
    NamedByteArray *nba = NamedByteArray_New ( name, size, allocType ) ;
    dllist_AddNodeToHead ( ovt->NBAs, ( dlnode* ) & nba->NBA_Symbol ) ;
    return nba ;
}

void
FreeNba_BaNode ( NamedByteArray * nba, dlnode * node )
{
    ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
    MemChunk* mchunk = ( MemChunk* ) ba ; //( ( Symbol * ) node )->S_Value ;
    int64 size = ba->BA_DataSize ; //mchunk->S_ChunkSize ;
    dlnode_Remove ( node ) ; // remove BA_Symbol from nba->NBA_BaList cf. _NamedByteArray_AddNewByteArray
    _Mem_ChunkFree ( mchunk ) ;
    nba->MemRemaining -= size ;
    nba->MemAllocated -= size ;
    nba->TotalAllocSize -= size ;
}

void
FreeNba_BaList ( NamedByteArray * nba )
{
    dllist * list = & nba->NBA_BaList ;
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nodeNext )
    {
        nodeNext = dlnode_Next ( node ) ;
        FreeNba_BaNode ( nba, node ) ;
    }
}

void
OVT_FreeTempMem ( )
{
    OVT_MemListFree_CompilerTempObjects ( ) ;
    OVT_MemListFree_TempObjects ( ) ;
    OVT_MemListFree_Objects ( ) ;
    OVT_MemListFree_LispTemp ( ) ;
}

NamedByteArray *
_OVT_Find_NBA ( byte * name )
{
    // needs a Word_Find that can be called before everything is initialized
    Symbol * s = DLList_FindName_InOneNamespaceList ( _O_->MemorySpace0->NBAs, ( byte * ) name ) ;
    if ( s ) return Get_NbaSymbolNode_To_NBA ( s ) ;
    else return 0 ;
}

// fuzzy still but haven't yet needed to adjust this one

void
_OVT_MemList_FreeNBAMemory ( NamedByteArray *nba, uint64 moreThan, int64 always )
{
    if ( nba && ( always || ( nba->MemAllocated > ( nba->MemInitial + moreThan ) ) ) ) // this logic is fuzzy ?? what is wanted is a way to fine tune mem allocation 
    {
        dlnode * node, *nodeNext ;
        int64 size, flag ;
        for ( flag = 0, node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
            if ( ba )
            {
                FreeNba_BaNode ( nba, node ) ;
                nba->NumberOfByteArrays -- ;
                if ( ! nodeNext )
                {
                    nba->MemAllocated = 0 ;
                    nba->MemRemaining = 0 ;
                    nba->NBA_DataSize = nba->OriginalSize ;
                    _NamedByteArray_AddNewByteArray ( nba, nba->OriginalSize ) ;
                }
            }
        }
        nba->InitFreedRandMarker = rand ( ) ;
    }
}

void
NamedByteArray_Delete ( NamedByteArray * nba, Boolean reinitFlag )
{
    ByteArray * ba ;
    dlnode * node, *nodeNext ;
    if ( nba )
    {
        dlnode_Remove ( ( dlnode* ) & nba->NBA_Symbol ) ;
        for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ba = Get_BA_Symbol_To_BA ( node ) ;
            _Mem_ChunkFree ( ( MemChunk * ) ba ) ;
        }
        if ( ! reinitFlag ) _Mem_ChunkFree ( ( MemChunk * ) nba ) ; 
        else _NamedByteArray_Init ( nba, nba->NBA_MemChunk.mc_Name, nba->NBA_DataSize, nba->NBA_AAttribute ) ;
    }
}

