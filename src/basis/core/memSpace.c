#include "../../include/csl.h"

OVT_MemSystem *_OMS_ ; // initialized by OVT_Static_MemSystem_New
OVT_StaticMemSystem * _OSMS_ ;
int64 mmaped = 0 ;
//byte * Mem = 0 ;
#define STATIC_MEM_SIZE (2 * K)

void
MemChunk_Account ( int64 size, int64 allocType, int64 flag )
{
    //return ; // debug
    if ( ( allocType & ( MMAP | PRE_STATIC_MEM | STATIC | HISTORY ) ) )
    {
        if ( _OSMS_ )
        {
            if ( flag == MEM_ALLOC ) // OVT_StaticMemSystem
            {
                _OSMS_->Allocated += size ;
                _OSMS_->RemainingAllocated += size ;
            }
            else
            {
                _OSMS_->Freed += size ;
                _OSMS_->RemainingAllocated -= size ;
            }
        }
        //else printf ( "\nMemChunk_Account failure : _OSMS_" ), fflush ( stdout ) ;
    }
    else
    {
        if ( _OMS_ )
        {
            if ( flag == MEM_ALLOC ) // OVT_MemSystem
            {
                _OMS_->Allocated += size ;
                _OMS_->RemainingAllocated += size ;
            }
            else
            {
                _OMS_->RemainingAllocated -= size ;
                _OMS_->Freed += size ;
            }
        }
        //else printf ( "\nMemChunk_Account failure : _OMS_" ), fflush ( stdout ) ;
    }
}

void
_Munmap ( byte * ptr, int64 size, int64 allocType )
{
    MemChunk_Account ( size, allocType, MEM_FREE ) ;
    munmap ( ptr, size ) ;
    mmaped -= size ;
}


#define PRE_STATIC_MEM_Munmap( ptr, size ) _Munmap ( ptr, size, PRE_STATIC_MEM ) 

void
Munmap ( MemChunk * mchunk )
{
#if 0 // DEBUG    
    if ( ! ( mchunk->mc_Type & MEM_CHUNK ) )
    {
        printf ( "\nMunmap : incorrect Type" ) ;
        fflush ( stdout ) ;
    }
#endif    
    _Munmap ( ( byte* ) mchunk, mchunk->mc_ChunkSize, mchunk->mc_AllocType ) ;
}

byte*
_Mmap ( int64 size )
{
    //byte * mem = Mem = ( byte* ) mmap ( Mem, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, - 1, 0 ) ;
    byte * mem = ( byte* ) mmap ( NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, - 1, 0 ) ;
    if ( mem == MAP_FAILED ) // MAP_FAILED == -1
    {
        perror ( "_Mem_Mmap" ) ;
        OVT_ShowMemoryAllocated ( ) ;
        //OVT_Exit ( ) ;
        CSL_FullRestart ( ) ;
    }
    memset ( mem, 0, size ) ; // ??! is this necessary 
    mmaped += size ;
    return mem ;
}

byte*
Mmap_AndAccount ( int64 size, int64 allocType )
{
    byte * mem = _Mmap ( size ) ;
    MemChunk_Account ( size, allocType, MEM_ALLOC ) ;
    return mem ;
}

// MemChunk struct is at the beginning of structures being allocated 

MemChunk *
MemChunk_Allocate ( int64 size, uint64 allocType )
{
    // a MemChunk is already a part of the size being part of the ByteArray and NamedByteArray : cf. types.h
    MemChunk * mchunk = ( MemChunk * ) Mmap_AndAccount ( size, allocType ) ; //Mmap ( size ) ;
    mchunk->mc_unmap = ( byte* ) mchunk ;
    mchunk->mc_ChunkSize = size ; // S_ChunkSize is the total size of the chunk including any prepended accounting structure in that total
    mchunk->mc_AllocType = allocType ;
    mchunk->mc_Data = ( byte* ) ( mchunk + 1 ) ;
    mchunk->mc_Type = MEM_CHUNK ;
    return mchunk ;
}

MemChunk *
MemChunk_AllocateListAdd ( int64 size, int64 allocType )
{
    MemChunk * mchunk = ( MemChunk * ) MemChunk_Allocate ( size, allocType ) ;
    switch ( allocType )
    {
        case STATIC:
        {
            dllist_AddNodeToHead ( _OSMS_->OVT_StaticMemList, ( dlnode* ) mchunk ) ;
            break ;
        }
#if 1        
        case HISTORY:
        {
            dllist_AddNodeToHead ( _OSMS_->HistorySpace_MemChunkStringList, ( dlnode* ) mchunk ) ;
            break ;
        }
#endif        
        case PRE_STATIC_MEM: return mchunk ;
        default:
        {
            dllist_AddNodeToHead ( _OMS_->OvtMemChunkList, ( dlnode* ) mchunk ) ;
            break ;
        }
    }
    return mchunk ;
}

byte *
MemChunk_AllocateMem ( int64 size, int64 allocType )
{
    MemChunk * mchunk = MemChunk_AllocateListAdd ( size, allocType ) ;
    return mchunk->mc_Data ;
}

void
OVT_Static_New ( )
{
    OVT_StaticMemSystem * osms = _OSMS_ = ( OVT_StaticMemSystem * ) MemChunk_Allocate ( sizeof ( OVT_StaticMemSystem ), MMAP ) ; //_OVT_Static_AllocMem ( sizeof ( OVT_StaticMemSystem ) ) ;
    // don't know about this ?? if this makes sense here or its setup or will ever be used or needed but
    MemChunk_Account ( sizeof ( OVT_StaticMemSystem ), PRE_STATIC_MEM, MEM_ALLOC ) ; // couldn't be accounted before _OSMS_ was created
    osms->OVT_StaticMemList = _dllist_New ( PRE_STATIC_MEM ) ;
    OVT_MemSystem * oms = _OMS_ = ( OVT_MemSystem * ) MemChunk_Allocate ( sizeof ( OVT_MemSystem ), PRE_STATIC_MEM ) ;

    osms->StaticMemSpace = _NamedByteArray_Allocate ( STATIC ) ;
    _NamedByteArray_Init ( osms->StaticMemSpace, ( byte* ) "StaticSpace", STATIC_MEM_SIZE, STATIC ) ;
    osms->HistorySpace_MemChunkStringList = _dllist_New ( STATIC ) ;
    oms->OvtMemChunkList = _dllist_New ( STATIC ) ;
    if ( ! sigsetjmp ( osms->JmpBuf0, 0 ) ) ; //?? // nb. siglongjmp always comes to beginning of the block 
}

void
PRE_STATIC_MEM_Munmap_List ( dllist * list )
{
    PRE_STATIC_MEM_Munmap ( ( byte * ) & list->l_List.n_Head, sizeof ( dlnode ) ) ;
    PRE_STATIC_MEM_Munmap ( ( byte * ) & list->l_List.n_Tail, sizeof ( dlnode ) ) ;
    PRE_STATIC_MEM_Munmap ( ( byte * ) list, sizeof ( dllist ) ) ;
}

void
OVT_Static_Delete ( OVT_StaticMemSystem * osms )
{
    FreeChunkList ( osms->HistorySpace_MemChunkStringList ) ;
    NamedByteArray_Delete ( osms->StaticMemSpace, 0 ) ;
    FreeChunkList ( osms->OVT_StaticMemList ) ;
    PRE_STATIC_MEM_Munmap_List ( osms->OVT_StaticMemList ) ;
    PRE_STATIC_MEM_Munmap ( ( byte * ) _OMS_, sizeof ( OVT_MemSystem ) ) ;
    PRE_STATIC_MEM_Munmap ( ( byte * ) osms, sizeof ( OVT_StaticMemSystem ) ) ;
    printf ( "\nmmaped = %ld", mmaped ), fflush ( stdout ) ;
}

void
OpenVmTil_Delete ( OpenVmTil * ovt )
{
    if ( ovt )
    {
        if ( ovt->Verbosity > 2 ) iPrintf ( "\nAll allocated, non-static memory is being freed.\nRestart : verbosity = %d.", ovt->Verbosity ) ;
        FreeChunkList ( _OMS_->OvtMemChunkList ) ;
    }
    _O_ = 0 ;
}

OpenVmTil *
OpenVmTil_Allocate ( OpenVmTil * ovt )
{
    if ( ! _OSMS_ ) OVT_Static_New ( ) ;
    OpenVmTil_Delete ( ovt ) ; // first delete a previous ovt - it could have been corrupted by a software error
    ovt = _O_ = ( OpenVmTil* ) MemChunk_AllocateListAdd ( sizeof ( OpenVmTil ), OPENVMTIL ) ;
    ovt->OpenVmTilSpace = NBA_OvtNew ( ( byte* ) "OpenVmTilSpace", ( ovt->OpenVmTilSize ? ovt->OpenVmTilSize : ( 6 * K ) ), OPENVMTIL ) ;
    ovt->NBAs = _dllist_New ( OPENVMTIL ) ;
    ovt->MemorySpaceList = _dllist_New ( OPENVMTIL ) ;
    return ovt ;
}

void
OVT_FullRestartCompleteDelete ( )
{
    //if ( _O_ && ( _O_->SigSegvs ++ < 3 ) ) 
    OpenVmTil_Delete ( _O_ ) ;
    OVT_Static_Delete ( _OSMS_ ) ;
    _OMS_ = 0 ;
    _OSMS_ = 0 ;
    _O_ = 0 ;
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
        case TEMPORARY: return _Allocate ( size, ms->TempObjectSpace ) ; // used for SourceCode
        case SESSION: return _Allocate ( size, ms->SessionObjectsSpace ) ;
        case COMPILER_TEMP: return _Allocate ( size, ms->CompilerTempObjectSpace ) ;
        case WORD_RECYCLING: return _Allocate ( size, ms->WordRecylingSpace ) ;

        case T_CSL: case DATA_STACK: return _Allocate ( size, ovt->CSL_InternalSpace ) ;
        case INTERNAL_OBJECT_MEM: return _Allocate ( size, ovt->InternalObjectSpace ) ;
        case OPENVMTIL: return _Allocate ( size, ovt->OpenVmTilSpace ) ;

        case STATIC: case STATIC_MEM: return _Allocate ( size, _OSMS_->StaticMemSpace ) ;

        case PRE_STATIC_MEM: return Mmap_AndAccount ( size, allocType ) ;

        default: CSL_Exception ( MEMORY_ALLOCATION_ERROR, 0, QUIT ) ;
    }
    return 0 ;
}

void *
sl9_malloc ( int size )
{
    return ( void* ) Mem_Allocate ( size, LISP_TEMP ) ;
}

void
MemorySpace_Init ( MemorySpace * ms )
{
    OpenVmTil * ovt = _O_ ;
    MemorySpace * oldMs = ovt->MemorySpace0 ;

    ms->DictionarySpace = ( oldMs && oldMs->DictionarySpace ) ? oldMs->DictionarySpace :
        NBA_MemSpace_New ( ms, ( byte* ) "DictionarySpace", ovt->DictionarySize, DICTIONARY ) ;
    ms->CodeSpace = ( oldMs && oldMs->CodeSpace ) ? oldMs->CodeSpace :
        NBA_MemSpace_New ( ms, ( byte* ) "CodeSpace", ovt->MachineCodeSize, CODE ) ;

    ms->LispSpace = NBA_MemSpace_New ( ms, ( byte* ) "LispSpace", ovt->LispSpaceSize, LISP ) ;
    ms->ObjectSpace = NBA_MemSpace_New ( ms, ( byte* ) "ObjectSpace", ovt->ObjectSpaceSize, OBJECT_MEM ) ;
    ms->StringSpace = NBA_MemSpace_New ( ms, ( byte* ) "StringSpace", ovt->StringSpaceSize, STRING_MEMORY ) ;
    ms->BufferSpace = NBA_MemSpace_New ( ms, ( byte* ) "BufferSpace", ovt->BufferSpaceSize, BUFFER ) ;

    ms->TempObjectSpace = NBA_MemSpace_New ( ms, ( byte* ) "TempObjectSpace", ovt->TempObjectsSize, TEMPORARY ) ;
    ms->LispTempSpace = NBA_MemSpace_New ( ms, ( byte* ) "LispTempSpace", ovt->LispTempSize, LISP_TEMP ) ;
    //ms->LispCopySpace = NBA_MemSpace_New ( ms, ( byte* ) "LispCopySpace", ovt->LispCopySize, LISP_COPY ) ;
    ms->CompilerTempObjectSpace = NBA_MemSpace_New ( ms, ( byte* ) "CompilerTempObjectSpace", ovt->CompilerTempObjectsSize, COMPILER_TEMP ) ;
    ms->WordRecylingSpace = NBA_MemSpace_New ( ms, ( byte* ) "WordRecylingSpace", ovt->WordRecylingSize, WORD_RECYCLING ) ;
    ms->SessionObjectsSpace = NBA_MemSpace_New ( ms, ( byte* ) "SessionObjectsSpace", ovt->SessionObjectsSize, SESSION ) ;

    if ( ovt->OVT_CSL && ovt->OVT_CSL->Context0 )
    {
        NBA * cnba = ovt->OVT_CSL->Context0->ContextNba ;
        ms->ContextSpace = cnba ;
    }
    _O_CodeByteArray = ms->CodeSpace->ba_CurrentByteArray ; //init CompilerSpace ptr
    if ( Verbosity ( ) > 2 ) iPrintf ( "\nSystem Memory has been initialized.  " ) ;
}

void
_Mem_ChunkFree ( MemChunk * mchunk )
{
    dlnode_Remove ( ( dlnode* ) mchunk ) ;
    Munmap ( mchunk ) ;
}

void
Mem_FreeItem ( dllist * mList, byte * item )
{
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) mList ) ; node ; node = nodeNext )
    {
        MemChunk * mchunk = ( MemChunk* ) node ;
        nodeNext = dlnode_Next ( node ) ;
        if ( item && ( ( byte* ) mchunk->mc_Data == item ) )
        {
            _Mem_ChunkFree ( mchunk ) ;
            return ;
        }
        else _Mem_ChunkFree ( mchunk ) ;

    }
}

void
FreeChunkList ( dllist * list )
{
    Mem_FreeItem ( list, 0 ) ;
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

void
MemChunk_Show ( MemChunk * mchunk )
{
    iPrintf ( "\nMemChunk : address : 0x%08x : allocType = %8lu : size = %8d", ( uint64 ) mchunk, ( uint64 ) mchunk->mc_AllocType, ( int64 ) mchunk->mc_ChunkSize ) ;
}

void
_MemChunk_WithSymbol_Show ( MemChunk * mchunk, int64 flag )
{
    Symbol * sym = ( Symbol * ) ( mchunk + 1 ) ;
    iPrintf ( "\n%s : %s : 0x%lld : %d, ", ( flag == MEM_ALLOC ) ? "Alloc" : "Free",
        ( ( int64 ) ( sym->S_Name ) > 0x80000000 ) ? ( char* ) sym->S_Name : "(null)", mchunk->mc_AllocType, mchunk->mc_ChunkSize ) ;
}

