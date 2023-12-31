
#include "../../include/csl.h"

OVT_StaticMemSystem *_OSMS_ ;
OVT_Static * _OS_ ;

byte*
_Mmap ( int64 size )
{
    byte * mem = ( byte* ) mmap ( NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, - 1, 0 ) ;
    memset ( mem, 0, size ) ;
    return mem ;
}

byte*
Mmap ( int64 size )
{
    byte * mem = _Mmap ( size ) ;
    _OSMS_->OVT_MmapAllocated += size ;
    return mem ;
}

void
Munmap ( byte * chunk, int64 size )
{
    munmap ( chunk, size ) ;
    _OSMS_->OVT_MmapAllocated -= size ;
}

void
Munmap_OS_Chunk ( byte *data )
{
    OS_Chunk * chunk = ( OS_Chunk* ) ( data - sizeof (int64 ) ) ;
    Munmap ( ( byte* ) chunk, chunk->osc_Size ) ;
}

byte *
mmap_AllocMem ( int64 size )
{
    byte * mem = ( byte* ) Mmap ( size ) ;
    if ( ( mem == MAP_FAILED ) )
    {
        perror ( "_Mem_Mmap" ) ;
        OVT_ShowMemoryAllocated ( ) ;
        //OVT_Exit ( ) ;
        CSL_FullRestart ( ) ;
    }
    //Kbhit_Pause ( ) ;
    return mem ;
}

void
MemChunk_Show ( MemChunk * mchunk )
{
    Printf ( "\nMemChunk : address : 0x%08x : allocType = %8lu : size = %8d", ( uint64 ) mchunk, ( uint64 ) mchunk->S_WAllocType, ( int64 ) mchunk->S_ChunkSize ) ;
}

void
_MemChunk_WithSymbol_Show ( MemChunk * mchunk, int64 flag )
{
    Symbol * sym = ( Symbol * ) ( mchunk + 1 ) ;
    Printf ( "\n%s : %s : 0x%lld : %d, ", ( flag == MEM_ALLOC ) ? "Alloc" : "Free",
        ( ( int64 ) ( sym->S_Name ) > 0x80000000 ) ? ( char* ) sym->S_Name : "(null)", mchunk->S_WAllocType, mchunk->S_ChunkSize ) ;
}

void
_MemChunk_Account ( MemChunk * mchunk, int64 flag )
{
    if ( flag == MEM_ALLOC )
    {
        _OS_->TotalMemAllocated += mchunk->S_ChunkSize ;
        _OS_->Mmap_RemainingMemoryAllocated += mchunk->S_ChunkSize ;
    }
    else
    {
        _OS_->TotalMemFreed += mchunk->S_ChunkSize ;
        _OS_->Mmap_RemainingMemoryAllocated -= mchunk->S_ChunkSize ;
    }
}

void
_Mem_ChunkFree ( MemChunk * mchunk )
{
    _MemChunk_Account ( mchunk, MEM_FREE ) ;
    dlnode_Remove ( ( dlnode* ) mchunk ) ;
    Munmap ( mchunk->S_unmap, mchunk->S_ChunkSize ) ;
}

MemChunk *
_Mem_ChunkAllocate ( int64 size, uint64 allocType )
{
    // a MemChunk is already a part of the size being part of the ByteArray and NamedByteArray : cf. types.h
    MemChunk * mchunk = ( MemChunk * ) mmap_AllocMem ( size ) ;
    mchunk->S_unmap = ( byte* ) mchunk ;
    mchunk->S_ChunkSize = size ; // S_ChunkSize is the total size of the chunk including any prepended accounting structure in that total
    mchunk->S_WAllocType = allocType ;
    return mchunk ;
}

byte *
Mem_ChunkAllocate ( int64 size, uint64 allocType )
{
    MemChunk * mchunk = ( MemChunk * ) _Mem_ChunkAllocate ( size, allocType ) ;
    _MemChunk_Account ( ( MemChunk* ) mchunk, MEM_ALLOC ) ;
    if ( allocType == HISTORY ) dllist_AddNodeToHead ( _OS_->HistorySpace_MemChunkStringList, ( dlnode* ) mchunk ) ;
    else dllist_AddNodeToHead ( _OS_->OvtMemChunkList, ( dlnode* ) mchunk ) ;
    return ( byte* ) mchunk ;
}

byte *
OVT_Static_AllocMem ( int64 size )
{
    byte *mem ;
    OS_Node * osNode ;
    osNode = ( OS_Node * ) Mmap ( size += sizeof ( OS_Node ) ) ; //mmap_AllocMem ( size += sizeof ( OS_Node ) ) ;
    osNode->osc_Size = size ;
    mem = osNode->osc_b_Chunk ;
    dllist_AddNodeToHead ( ( dllist* ) & _OSMS_->OVT_StaticMemList.osl_List, &osNode->osc_Node ) ;
    return mem ;
}

OVT_StaticMemSystem *
OVT_Static_MemSystem_New ( )
{
    OS_Node * osNode ;
    int64 size ;
    osNode = ( OS_Node * ) _Mmap ( size = sizeof ( OS_Node ) + sizeof ( OVT_StaticMemSystem ) ) ;
    OVT_StaticMemSystem * osms = _OSMS_ = ( OVT_StaticMemSystem * ) osNode->osc_b_Chunk ;
    osms->OVT_MmapAllocated = osNode->osc_Size = size ;
    OS_List_Init ( &osms->OVT_StaticMemList ) ;
    dllist_AddNodeToHead ( ( dllist* ) & _OSMS_->OVT_StaticMemList.osl_List, &osNode->osc_Node ) ;
    return osms ;
}

void
OVT_Static_FreeMem ( byte * mem )
{
    OS_Node * osNode = ( OS_Node * ) ( mem - sizeof ( OS_Node ) ) ;
    dlnode_Remove ( &osNode->osc_Node ) ;
    Munmap ( mem, osNode->osc_Size ) ;
}

void
Free_OVT_Static_Mem ( )
{
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) & _OSMS_->OVT_StaticMemList ) ; node ; node = nodeNext )
    {
        nodeNext = dlnode_Next ( node ) ;
        OVT_Static_FreeMem ( ( byte* ) node ) ;
    }
}

void
OVT_FullRestartCompleteDelete ( )
{
    Free_OVT_Static_Mem ( ) ;
    if ( _O_ && ( _O_->SigSegvs ++ < 2 ) ) _OpenVmTil_Delete ( _O_ ) ;
    _OSMS_ = 0 ;
    _OS_ = 0 ;
    _O_ = 0 ;
}

OVT_Static *
OVT_Static_New ( )
{
    OVT_Static * os = 0 ;
    if ( ! _OSMS_ ) _OSMS_ = OVT_Static_MemSystem_New ( ) ;
    _OS_ = os = ( OVT_Static * ) Mem_Allocate ( sizeof (OVT_Static ), _STATIC_ ) ;
    os->HistorySpace_MemChunkStringList = _dllist_New ( OVT_STATIC ) ;
    os->OvtMemChunkList = _dllist_New ( OVT_STATIC ) ;
    return os ;
}

byte *
Mem_Allocate ( int64 size, uint64 allocType )
{
    OpenVmTil * ovt = _O_ ;
    MemorySpace * ms = _O_ ? _O_->MemorySpace0 : 0 ;
    switch ( allocType )
    {
        case DICTIONARY: return _Allocate ( size, ms->DictionarySpace ) ;
        case CODE: return _Allocate ( size, ms->CodeSpace ) ;
        case OBJECT_MEM: return _Allocate ( size, ms->ObjectSpace ) ;
        case STRING_MEMORY: return _Allocate ( size, ms->StringSpace ) ;
        case LISP: return _Allocate ( size, ms->LispSpace ) ;

        case CONTEXT: return _Allocate ( size, ms->ContextSpace ) ;
        case BUFFER: return _Allocate ( size, ms->BufferSpace ) ;
        case LISP_TEMP: return _Allocate ( size, ms->LispTempSpace ) ;
            //case LISP_COPY: return _Allocate ( size, ms->LispCopySpace ) ;
        case TEMPORARY: return _Allocate ( size, ms->TempObjectSpace ) ; // used for SourceCode
        case SESSION: return _Allocate ( size, ms->SessionObjectsSpace ) ;
        case COMPILER_TEMP: return _Allocate ( size, ms->CompilerTempObjectSpace ) ;
        case WORD_RECYCLING: return _Allocate ( size, ms->WordRecylingSpace ) ;

        case T_CSL: case DATA_STACK: return _Allocate ( size, ovt->CSLInternalSpace ) ;
        case INTERNAL_OBJECT_MEM: return _Allocate ( size, ovt->InternalObjectSpace ) ;
        case OPENVMTIL: return _Allocate ( size, ovt->OpenVmTilSpace ) ;
        case _STATIC_: case OVT_STATIC: case HISTORY:
        {
            if ( allocType == _STATIC_ ) return Mmap ( size ) ;
            else if ( allocType & ( OVT_STATIC ) ) return OVT_Static_AllocMem ( size ) ; //_OS_->StaticAllocation += size ;
            else if ( allocType & HISTORY ) _OS_->HistoryAllocation += size ;
            return mmap_AllocMem ( size ) ;
        }
        default: CSL_Exception ( MEMORY_ALLOCATION_ERROR, 0, QUIT ) ;
    }
    return 0 ;
}

void *
sl9_malloc ( int size )
{
    return (void*) Mem_Allocate ( size, LISP_TEMP ) ;
}

void
MemorySpace_Init ( MemorySpace * ms )
{
    OpenVmTil * ovt = _O_ ;
    MemorySpace * oldMs = ovt->MemorySpace0 ;

    ms->DictionarySpace = ( oldMs && oldMs->DictionarySpace ) ? oldMs->DictionarySpace : MemorySpace_NBA_New ( ms, ( byte* ) "DictionarySpace", ovt->DictionarySize, DICTIONARY ) ;
    ms->CodeSpace = ( oldMs && oldMs->CodeSpace ) ? oldMs->CodeSpace : MemorySpace_NBA_New ( ms, ( byte* ) "CodeSpace", ovt->MachineCodeSize, CODE ) ;

    ms->LispSpace = MemorySpace_NBA_New ( ms, ( byte* ) "LispSpace", ovt->LispSpaceSize, LISP ) ;
    ms->ObjectSpace = MemorySpace_NBA_New ( ms, ( byte* ) "ObjectSpace", ovt->ObjectSpaceSize, OBJECT_MEM ) ;
    ms->StringSpace = MemorySpace_NBA_New ( ms, ( byte* ) "StringSpace", ovt->StringSpaceSize, STRING_MEMORY ) ;
    ms->BufferSpace = MemorySpace_NBA_New ( ms, ( byte* ) "BufferSpace", ovt->BufferSpaceSize, BUFFER ) ;

    ms->TempObjectSpace = MemorySpace_NBA_New ( ms, ( byte* ) "TempObjectSpace", ovt->TempObjectsSize, TEMPORARY ) ;
    ms->LispTempSpace = MemorySpace_NBA_New ( ms, ( byte* ) "LispTempSpace", ovt->LispTempSize, LISP_TEMP ) ;
    //ms->LispCopySpace = MemorySpace_NBA_New ( ms, ( byte* ) "LispCopySpace", ovt->LispCopySize, LISP_COPY ) ;
    ms->CompilerTempObjectSpace = MemorySpace_NBA_New ( ms, ( byte* ) "CompilerTempObjectSpace", ovt->CompilerTempObjectsSize, COMPILER_TEMP ) ;
    ms->WordRecylingSpace = MemorySpace_NBA_New ( ms, ( byte* ) "WordRecylingSpace", ovt->WordRecylingSize, WORD_RECYCLING ) ;
    ms->SessionObjectsSpace = MemorySpace_NBA_New ( ms, ( byte* ) "SessionObjectsSpace", ovt->SessionObjectsSize, SESSION ) ;

    if ( ovt->OVT_CSL && ovt->OVT_CSL->Context0 )
    {
        NBA * cnba = ovt->OVT_CSL->Context0->ContextNba ;
        ms->ContextSpace = cnba ;
    }
    _O_CodeByteArray = ms->CodeSpace->ba_CurrentByteArray ; //init CompilerSpace ptr
    if ( _O_->Verbosity > 2 ) Printf ( "\nSystem Memory has been initialized.  " ) ;
}

void
Mem_FreeItem ( dllist * mList, byte * item )
{
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) mList ) ; node ; node = nodeNext )
    {
        MemChunk * mchunk = ( MemChunk* ) node ;
        nodeNext = dlnode_Next ( node ) ;
        if ( ( byte* ) mchunk->S_pb_Data2 == item )
        {
            _Mem_ChunkFree ( mchunk ) ;
            return ;
        }
    }
}

void
FreeChunkList ( dllist * list )
{
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nodeNext )
    {
        nodeNext = dlnode_Next ( node ) ;
        _Mem_ChunkFree ( ( MemChunk* ) node ) ;
    }
}

void
FreeNba_BaNode ( NamedByteArray * nba, dlnode * node )
{
    ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
    MemChunk* mchunk = ( MemChunk* ) ( ( Symbol * ) node )->S_Value ;
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
NBA_FreeChunkType ( Symbol * s, uint64 allocType, int64 exactFlag )
{
    NamedByteArray * nba = Get_NbaSymbolNode_To_NBA ( s ) ;
    if ( exactFlag )
    {
        if ( nba->NBA_AAttribute != allocType ) return ;
    }
    else if ( ! ( nba->NBA_AAttribute & allocType ) ) return ;
    FreeNba_BaList ( nba ) ;
    nba->MemRemaining = 0 ;
    nba->MemAllocated = 0 ;
    _NamedByteArray_AddNewByteArray ( nba, nba->NBA_DataSize ) ;
}

MemorySpace *
MemorySpace_Find ( byte * name )
{
    MemorySpace * ms = ( MemorySpace* ) _DLList_FindName_InOneNamespaceList ( _O_->MemorySpaceList, name ) ;
    return ms ;
}

MemorySpace *
MemorySpace_StaticMem_Allocate ( )
{
    MemorySpace *ms = ( MemorySpace* ) Mem_Allocate ( sizeof ( MemorySpace ), OPENVMTIL ) ; //mmap_AllocMem ( sizeof ( MemorySpace ) ) ;
    ms->NBAs = _dllist_New ( OPENVMTIL ) ;
    return ms ;
}

void
_MemorySpace_Do_MemorySpace ( OpenVmTil * ovt, MemorySpace * ms )
{
    ovt->MemorySpace0 = ms ;
    if ( ovt->OVT_CSL && ovt->OVT_CSL->Context0 )
    {
        NBA * cnba = ovt->OVT_CSL->Context0->ContextNba ;
        ovt->MemorySpace0->ContextSpace = cnba ;
    }
}

MemorySpace *
MemorySpace_New ( OpenVmTil * ovt, byte * name )
{
    MemorySpace * ms = MemorySpace_Find ( name ) ;
    if ( ! ms )
    {
        ms = MemorySpace_StaticMem_Allocate ( ) ;
        MemorySpace_Init ( ms ) ;
        ( ( Symbol* ) ms )->S_Name = name ;
        dllist_AddNodeToHead ( ovt->MemorySpaceList, ( dlnode* ) ms ) ;
        //if (_CSL_) CSL_Context_PushNew ( _CSL_ ) ;
        _MemorySpace_Do_MemorySpace ( ovt, ms ) ;
    }
    return ms ;
}

// not so useful now maybe future developments will change things

void
MemorySpace_Delete ( byte * name )
{
    NamedByteArray *nba ;
    dlnode * node, *nodeNext ;
    MemorySpace * ms = MemorySpace_Find ( name ) ;
    if ( ms && ( node = dllist_First ( ( dllist* ) ms->NBAs ) ) )
    {
        MemorySpace * defaultMs = MemorySpace_Find ( "DefaultMemorySpace" ) ;
        defaultMs->WordsInRecycling = ms->WordsInRecycling ;
        defaultMs->RecycledWordCount = ms->RecycledWordCount ;
        for ( ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            nba = Get_NbaSymbolNode_To_NBA ( node ) ;
            _OVT_MemList_FreeNBAMemory ( nba, 0, 1 ) ;
        }
        dlnode_Remove ( ( dlnode * ) ms ) ;
        //if (_CSL_) CSL_Context_PopDelete ( _CSL_ ) ;
        _MemorySpace_Do_MemorySpace ( _O_, defaultMs ) ;
    }
}

void
CSL_MemorySpace_Delete ( )
{
    MemorySpace_Delete ( ( byte* ) DataStack_Pop ( ) ) ;
}

void
CSL_MemorySpace_New ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    MemorySpace * ms = MemorySpace_New ( _O_, name ) ;
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
#if 0     // 15 * M       
            nba->NBA_DataSize = nba->NBA_DataSize > ( 100 * K ) ? nba->NBA_DataSize : ( 100 * K ) ;
            nba->NBA_DataSize *= ( ++ nba->Allocations ) ;
            nba->NBA_DataSize += size ;
#elif 0  // 14 * M           
            nba->NBA_DataSize += ( ( ( ++ nba->Allocations ) * ( 100 * K ) ) + size ) ;
#elif 0  // 16 * M          
            nba->NBA_DataSize = ( ( ++ nba->Allocations ) * ( nba->NBA_DataSize ? nba->NBA_DataSize : ( 100 * K ) ) + size ) ;
#else   // 13 * M 
            size = ( size > ( 100 * K ) ) ? size : ( 100 * K ) ;
            nba->NBA_DataSize += ( ( ( ++ nba->Allocations ) * size ) + ( 100 * K ) ) ;
            //nba->NBA_DataSize =  ((nba->NBA_DataSize > (100 * K) ? nba->NBA_DataSize : (100 * K) ) * (++nba->Allocations) ) + size ;
#endif            
            if ( _O_->Verbosity > 1 )
                NBA_PrintInfo ( nba ) ;
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
MemorySpace_NBA_New ( MemorySpace * memSpace, byte * name, int64 size, int64 allocType )
{
    NamedByteArray *nba = NamedByteArray_New ( name, size, allocType ) ;
    dllist_AddNodeToHead ( memSpace->NBAs, ( dlnode* ) & nba->NBA_Symbol ) ;
    return nba ;
}

NamedByteArray *
MemorySpace_NBA_OvtNew ( byte * name, int64 size, int64 allocType )
{
    OpenVmTil * ovt = _O_ ;
    NamedByteArray *nba = NamedByteArray_New ( name, size, allocType ) ;
    dllist_AddNodeToHead ( ovt->NBAs, ( dlnode* ) & nba->NBA_Symbol ) ;
    return nba ;
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
        if ( ! reinitFlag ) _Mem_ChunkFree ( ( MemChunk * ) nba ) ; // mchunk )

        else _NamedByteArray_Init ( nba, nba->NBA_MemChunk.Name, nba->NBA_DataSize, nba->NBA_AAttribute ) ;
    }
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

void
NBA_Show ( NamedByteArray * nba, int64 flag )
{
    NBA_PrintInfo ( nba ) ;
    if ( flag && ( _O_->Verbosity > 1 ) )
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
    if ( _O_->Verbosity > 1 )
    {
        Printf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d : OrigAllocSize = %8d",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining,
            nba->SmallestRemaining, nba->NBA_DataSize, nba->OriginalSize ) ;
    }
    else if ( _O_->Verbosity > 2 )
    {
        Printf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d : OrigAllocSize = %8d : alloctype = %8lu ",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining,
            nba->SmallestRemaining, nba->NBA_DataSize, nba->OriginalSize, ( uint64 ) nba->NBA_AAttribute ) ;
    }
    else
    {
        Printf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d",
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
_OVT_ShowPermanentMemList ( OpenVmTil * ovt )
{
    int64 size ;
    if ( ovt )
    {
        int64 diff ;
        dlnode * node, *nodeNext ;
        if ( ovt->Verbosity > 2 ) printf ( "\nMemChunk List :: " ) ;
        for ( size = 0, node = dllist_First ( ( dllist* ) _OS_->OvtMemChunkList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->S_ChunkSize ;
        }
        diff = _OS_->Mmap_RemainingMemoryAllocated - size ;
        if ( diff ) //|| ovt->Verbosity > 2 )
        {
            printf ( "\nTotal Size = %9ld : _OS_->Mmap_RemainingMemoryAllocated = %9ld :: diff = %6ld", size, _OS_->Mmap_RemainingMemoryAllocated, diff ) ;
        }
        Printf ( "\nNon-Static Mem Allocated      = %9ld", ovt->TotalNbaAccountedMemAllocated ) ;
        //printf ( "\n_OS_->Mmap_RemainingMemoryAllocated                     = %9ld : <=: _OS_->Mmap_RemainingMemoryAllocated", _OS_->Mmap_RemainingMemoryAllocated ) ;
        fflush ( stdout ) ;
    }
    ovt->PermanentMemListRemainingAccounted = size ;

    return size ;
}

void
_OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( OpenVmTil * ovt, Boolean showFlag )
{
    if ( ovt )
    {
        dlnode * node, * nextNode, *msNode, *nextMsNode ;
        NamedByteArray * nba ;
        MemorySpace * ms ;
        Boolean flag = 0 ;
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
                if ( ! flag ++ )
                {
                    for ( node = dllist_First ( ( dllist* ) & ovt->NBAs ) ; node ; node = nextNode )// nb. NBAs is the NBA Symbol list
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
        }
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
        Printf ( "\nData Stack              InUse = %9d : Unused = %9d", dsu, dsa ) ;
    }
    printf ( "\nTotal Accounted Mem     InUse = %9ld : Unused = %9ld",
        ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining, ovt->TotalNbaAccountedMemRemaining ) ;
    fflush ( stdout ) ;
}

void
_OVT_ShowMemoryAllocated ( OpenVmTil * ovt )
{
    Boolean vf = ( ovt->Verbosity > 1 ) ;
    if ( ! vf ) Printf ( c_gu ( "\nIncrease the verbosity setting to 2 or more for more info here. ( Eg. : verbosity 2 = )" ) ) ;
    OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( ovt, 1 ) ;
    _OVT_ShowPermanentMemList ( ovt ) ;
    int64 leak = ovt->PermanentMemListRemainingAccounted - ( _OS_->TotalMemAllocated - _OS_->TotalMemFreed ) ; //- _OS_->HistoryAllocation ; //sizeof (OVT_Static ) ;
    //int64 leak = ovt->PermanentMemListRemainingAccounted - _OSMS_->OVT_MmapAllocated - (_OS_->TotalMemAllocated - _OS_->TotalMemFreed ) - _OS_->HistoryAllocation ; //sizeof (OVT_Static ) ;
    //int64 leak = (_OSMS_->OVT_MmapAllocated - ovt->PermanentMemListRemainingAccounted) - (_OS_->TotalMemAllocated - _OS_->TotalMemFreed ) - _OS_->HistoryAllocation ; //sizeof (OVT_Static ) ;
    int64 memDiff2 = _OS_->Mmap_RemainingMemoryAllocated - ovt->PermanentMemListRemainingAccounted ;
    byte * memDiff2s = ( byte* ) "\nCurrent Unaccounted Diff (leak?) = %9d : <=: _OS_->Mmap_RemainingMemoryAllocated - ovt->PermanentMemListRemainingAccounted" ;
    byte * leaks = ( byte* ) "\nleak?                            = %9d : <=  ( mmap_TotalMemAllocated - mmap_TotalMemFreed ) - ( _OS_->TotalMemAllocated - _OS_->TotalMemFreed ) - _OS_->HistoryAllocation -_OS_->StaticAllocation" ;
    if ( memDiff2 ) Printf ( c_ad ( memDiff2s ), memDiff2 ) ;
    else if ( vf ) Printf ( c_ud ( memDiff2s ), memDiff2 ) ;
    if ( leak ) Printf ( c_ad ( leaks ), leak ) ;
    else if ( vf ) Printf ( c_ud ( leaks ), leak ) ;
    if ( memDiff2 || leak || vf )
    {
        Printf ( "\nTotalNbaAccountedMemAllocated      = %9d : <=: ovt->TotalNbaAccountedMemAllocated", ovt->TotalNbaAccountedMemAllocated ) ;
        Printf ( "\nMem Used - Categorized  = %9d : <=: ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining", ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotalNbaAccountedMemRemaining      = %9d : <=: ovt->TotalNbaAccountedMemRemaining", ovt->TotalNbaAccountedMemRemaining ) ;
        Printf ( "\nMmap_RemainingMemoryAllocated      = %9d : <=: _OS_->Mmap_RemainingMemoryAllocated", _OS_->Mmap_RemainingMemoryAllocated ) ;
        Printf ( "\nPermanentMemListRemainingAccounted = %9d : <=: ovt->PermanentMemListRemainingAccounted", ovt->PermanentMemListRemainingAccounted ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Remaining = %9d : <=: _OS_->TotalMemAllocated - _OS_->TotalMemFreed", _OS_->TotalMemAllocated - _OS_->TotalMemFreed ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Allocated = %9d : <=: _OS_->TotalMemAllocated", _OS_->TotalMemAllocated ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Freed     = %9d : <=: _OS_->TotalMemFreed", _OS_->TotalMemFreed ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Remaining = %9d : <=: _OS_->TotalMemAllocated - _OS_->TotalMemFreed", _OS_->TotalMemAllocated - _OS_->TotalMemFreed ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nOVT_MmapAllocated   = %9d", _OSMS_->OVT_MmapAllocated ) ;
    }
    Printf ( "\nHistoryAllocation             = %9d", _OS_->HistoryAllocation ) ;
    Printf ( "\nTotal Memory leaks            = %9d", leak ) ;

    Printf ( "\nNBA ReAllocations             = %9d", _O_->ReAllocations ) ;
    int64 wordSize = ( sizeof ( Word ) + sizeof ( WordData ) ) ;
    Printf ( "\nRecycledWordCount             = %9d : %-5d x %3d bytes", _O_->MemorySpace0->RecycledWordCount * wordSize, _O_->MemorySpace0->RecycledWordCount, wordSize ) ;
    Printf ( "\nWordsInRecycling              = %9d : %-5d x %3d bytes\n", _O_->MemorySpace0->WordsInRecycling * wordSize, _O_->MemorySpace0->WordsInRecycling, wordSize ) ;
    Buffer_PrintBuffers ( ) ;
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
    _O_CodeByteArray = _ByteArray_AppendSpace_MakeSure ( _O_CodeByteArray, memDesired ) ;
}

void
OVT_CheckCodeSpaceForRoom ( )
{
    _OVT_CheckCodeSpaceForRoom ( 4 * K ) ;
}

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
    OVT_Recycle ( ( dlnode * ) w ) ;
    w->W_ObjectAttributes &= ~ ( RECYCLABLE_COPY | RECYCLABLE_LOCAL ) ;
}

void
dbg_new ( Word * w )
{
    if ( ( uint64 ) w == 0x7ffff4928020 )
        Printf ( "\n got it at %s", Context_Location ( ) ) ;
}

void
_CheckRecycleWord ( Node * node )
{
    Word * w = ( Word * ) node ;
    if ( w && ( w->W_ObjectAttributes & ( RECYCLABLE_COPY | RECYCLABLE_LOCAL ) ) )
    {
        if ( _O_->Verbosity > 2 ) _Printf ( "\n_CheckRecycleWord : recycling : %s%s%s",
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

