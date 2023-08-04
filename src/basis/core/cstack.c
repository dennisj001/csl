
#include "../../include/csl.h"

int64
_Stack_Overflow ( Stack * stack )
{
    return ( stack->StackPointer >= stack->StackMax ) ;
}

int64
_Stack_IsEmpty ( Stack * stack )
{
    return ( stack->StackPointer < stack->StackMin ) ;
}

void
_Stack_Push ( Stack * stack, int64 value )
{
    stack->StackPointer ++ ;
    *( stack->StackPointer ) = value ;
}

void
_Stack_Dup ( Stack * stack )
{
    _Stack_Push ( stack, *( stack->StackPointer ) ) ;
}

int64
_Stack_Pop ( Stack * stack )
{
    return *( stack->StackPointer -- ) ;
}

int64
Stack_Pop_WithExceptionFlag ( Stack * stack, int64 exceptionOnEmptyFlag )
{
    if ( _Stack_IsEmpty ( stack ) )
    {
        if ( exceptionOnEmptyFlag == 1 ) CSL_Exception ( STACK_UNDERFLOW, 0, QUIT ) ;
        else return 0 ;
    }
    return _Stack_Pop ( stack ) ;
}

int64
Stack_Pop_WithZeroOnEmpty ( Stack * stack )
{
    return Stack_Pop_WithExceptionFlag ( stack, 0 ) ;
}

int64
Stack_Pop_WithExceptionOnEmpty ( Stack * stack )
{
    return Stack_Pop_WithExceptionFlag ( stack, 1 ) ;
}

int64
Stack_Pop ( Stack * stack )
{
#if STACK_CHECK_ERROR
    return Stack_Pop_WithExceptionFlag ( stack, 1 ) ;
#else
    return _Stack_Pop ( stack ) ;
#endif    
}

int64
_Stack_PopOrTop ( Stack * stack )
{
    int64 sd = Stack_Depth ( stack ) ;
    if ( sd <= 0 ) CSL_Exception ( STACK_UNDERFLOW, 0, QUIT ) ;
    else if ( sd == 1 ) return Stack_Top ( stack ) ;
    return _Stack_Pop ( stack ) ;
}

void
_Stack_DropN ( Stack * stack, int64 n )
{
    if ( n ) stack->StackPointer -= n ;
}

void
_Stack_Drop ( Stack * stack )
{
    stack->StackPointer -- ;
}

int64
Stack_Top ( Stack * stack )
{
    int64 sd = Stack_Depth ( stack ) ;
    if ( sd == 0 ) return 0 ;
    else return *stack->StackPointer ;
}

int64
_Stack_Pick ( Stack * stack, int64 offset )
{
    return * ( stack->StackPointer - offset ) ;
}

int64
_Stack_PickFromBottom ( Stack * stack, int64 offset )
{
    return * ( stack->StackMin + offset ) ;
}

int64
_Stack_Bottom ( Stack * stack )
{
    return * ( stack->StackMin ) ;
}

void
_Stack_SetBottom ( Stack * stack, int64 value )
{
    *stack->StackMin = value ;
}

void
_Stack_SetTop ( Stack * stack, int64 value )
{
    *stack->StackPointer = value ;
}

void
Stack_SetTop ( Stack * stack, int64 value )
{
    if ( Stack_Depth ( stack ) ) *stack->StackPointer = value ;
}

inline int64
_Stack_N ( Stack * stack, int64 n )
{
    return *( stack->StackPointer - n ) ;
}

int64
_Stack_NOS ( Stack * stack )
{
    return _Stack_N ( stack, 1 ) ;
}

void
Stack_Push ( Stack * stack, int64 value )
{
#if STACK_CHECK_ERROR
    if ( _Stack_Overflow ( stack ) )
    {
        AlertColors ;
        iPrintf ( "\nStack_Push : _Stack_Overflow - StackDepth = %d\n", Stack_Depth ( stack ) ) ;
        CSL_Exception ( STACK_OVERFLOW, 0, QUIT ) ;
    }
    _Stack_Push ( stack, value ) ;
#else
    _Stack_Push ( stack, value ) ;
#endif
}

void
Stack_Dup ( Stack * stack )
{
#if STACK_CHECK_ERROR
    if ( _Stack_Overflow ( stack ) ) CSL_Exception ( STACK_OVERFLOW, 0, QUIT ) ;
    _Stack_Dup ( stack ) ;
#else
    _Stack_Dup ( stack ) ;
#endif
}

int64
_Stack_IntegrityCheck ( Stack * stack )
{
    byte * errorString ;
    int64 flag ;
    // first a simple integrity check of the stack info struct
    if ( ( stack->StackMin == & ( stack->StackData [ 0 ] ) ) &&
        ( stack->StackMax == & ( stack->StackData [ stack->StackSize - 1 ] ) ) && // nb! stack changes size in stepping ??? :: -1 : zero based array
        ( stack->InitialTosPointer == & ( stack->StackData [ - 1 ] ) ) )
    {
        return true ;
    }
    if ( _Stack_Overflow ( stack ) ) flag = STACK_OVERFLOW, errorString = ( byte* ) "\nStack Integrity Error : Stack Onverflow" ;
    else flag = STACK_UNDERFLOW, errorString = ( byte* ) "\nStack Integrity Error : Stack Underflow" ;
    CSL_Exception ( flag, c_da ( errorString ), QUIT ) ; //errorString = "\nStack Integrity Error : Stack Underflow" ;
    return false ;
}

int64
_Stack_Depth ( Stack * stack )
{
    int64 depth = 0 ;
    if ( stack ) depth = stack->StackPointer - stack->InitialTosPointer ;
    return ( depth ) ;
}

int64
Stack_Depth ( Stack * stack )
{
    // first a simple integrity check of the stack info struct
    if ( stack && _Stack_IntegrityCheck ( stack ) ) return _Stack_Depth ( stack ) ;
    return ( 0 ) ;
}

void
Stack_InitQuick ( Stack * stack )
{
    if ( stack )
    {
        stack->InitialTosPointer = & stack->data [ - 1 ] ; // first push goes to stack->StackData [ 0 ]
        stack->StackPointer = stack->InitialTosPointer ;
    }
}

void
_Stack_Init ( Stack * stack, int64 slots )
{
    if ( stack )
    {
        memset ( stack, 0, sizeof ( Stack ) + ( slots * sizeof (int64 ) ) ) ;
        stack->StackSize = slots ; // re-init size after memset above cleared it
        stack->StackMin = & stack->data [ 0 ] ; // 
        stack->StackMax = & stack->data [ stack->StackSize - 1 ] ;
        stack->StackData = & stack->data [ 0 ] ;
        Stack_InitQuick ( stack ) ;
    }
}

#if 0

void
Stack_Delete ( Stack * stack )
{
    Mem_FreeItem ( _O_->_OS_->OvtMemChunkList, ( byte* ) stack ) ;
}
#endif

void
Stack_Init ( Stack * stack )
{
    _Stack_Init ( stack, stack->StackSize ) ;
}

Stack *
_Stack_Allocate ( int64 slots, uint64 allocType )
{
    Stack * stack = ( Stack* ) Mem_Allocate ( sizeof ( Stack ) + ( slots * sizeof (int64 ) ), allocType ) ;
    stack->StackSize = slots ;
    return stack ;
}

Stack *
Stack_New ( int64 slots, uint64 allocType )
{
    Stack * stack = _Stack_Allocate ( slots, allocType ) ;
    _Stack_Init ( stack, slots ) ;
    return stack ;
}

Stack *
Stack_Copy ( Stack * stack, uint64 type )
{
    Stack * nstack = Stack_New ( stack->StackSize, type ) ;
    MemCpy ( nstack->data, stack->data, stack->StackSize * sizeof (int64 ) ) ;

    // ?? -> preserve relative stack pointer
    int64 depth = Stack_Depth ( stack ) ;
    //depth = stack->StackPointer - stack->InitialTosPointer ;
    nstack->StackPointer = nstack->InitialTosPointer + depth ;

    return nstack ;
}

void
Stack_Print_AValue ( uint64 * stackPointer, int64 i, byte * stackName, byte * buffer, Boolean isWordAlreadyFlag )
{
    Word * word = 0 ;
    byte * string = 0, tsb [32], *ts = 0 ; // typeSignature
    int64 tsl = 0 ;
    tsb[0] = 0 ;
    if ( isWordAlreadyFlag ) // 
    {
        word = ( Word* ) ( stackPointer [ i ] ) ;
#if 0 // not yet       
        if ( word && word->Name )
        {
            tsl = Word_TypeSignatureLength ( word, 0 ) ;
            if ( tsl > 1 ) ts = Word_ExpandTypeLetterSignature ( word, 0 ) ;
            else ts = Word_TypeSignature ( word, tsb ) ;
        }
#endif        
    }
    else word = Word_GetFromCodeAddress ( ( byte* ) ( stackPointer [ i ] ) ) ;
    if ( word )
    {
        if ( IS_NON_MORPHISM_TYPE ( word ) && ( ! ( word->W_MorphismAttributes & CSL_WORD ) ) && ( ! tsl ) )
        {
            sprintf ( ( char* ) buffer, "< word : %s.%s : value = 0x%016lx >",
                word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "<literal>", c_gd ( String_ConvertToBackSlash ( word->Name, 0 ) ),
                ( uint64 ) word->S_Value ) ;
        }
        else
        {
            sprintf ( ( char* ) buffer, "< word : %s.%s : code = 0x%016lx >", // : type %s- %s",
                ( word->ContainingNamespace ? ( char* ) word->ContainingNamespace->Name : "" ), c_gd ( word->Name ), ( uint64 ) word->Definition ) ;
            //,( tsl ? "signature " : "" ), c_gd ( ts ) ) ;
        }
    }
    else string = String_CheckForAtAdddress ( ( byte* ) stackPointer[i], &_O_->Debug, &_O_->User ) ;
    iPrintf ( "\n  %s   [ %3ld ] < " UINT_FRMT " > = " UINT_FRMT "\t%s",
        stackName, i, ( uint64 ) & stackPointer [ i ], stackPointer [ i ], word ? buffer : string ? string : ( byte* ) "" ) ;
}

void
_Stack_PrintHeader ( Stack * stack, byte * name )
{
    int64 depth = Stack_Depth ( stack ) ;
    uint64 * sp = stack->StackPointer ; // 0 based stack
    byte * location = c_gd ( Context_Location ( ) ) ;
    iPrintf ( "\n%s at : %s :\n%s depth =%4d : %s = Top = " UINT_FRMT ", InitialTos = " UINT_FRMT ", Size = " UINT_FRMT,
        name, location, name, depth, stack == _DataStack_ ? "Dsp (R14)" : _ReturnStack_ ? "CSLRsp (Rbx)" : "", ( int64 ) sp,
        ( int64 ) stack->InitialTosPointer, stack->StackMax - stack->StackMin + 1 ) ;
}

void
_Stack_PrintValues ( byte * name, uint64 * stackPointer, int64 depth0, Boolean isWordAlreadyFlag )
{
    if ( depth0 )
    {
        int64 i, depth1 = ( depth0 > K ) ? K : depth0 ;
        byte * buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ;
        //if ( ( depth >= 0 ) && ( depth < 256 ) ) // 256 : don't let this function run to far with stack related errors
        {
            for ( i = 0 ; depth1 -- > 0 ; i -- ) Stack_Print_AValue ( stackPointer, i, name, buffer, isWordAlreadyFlag ) ;
        }
        if ( depth0 > 64 ) iPrintf ( "\n%s : actual depth is %ld ...", name, depth0 ) ;
        //else CSL_Exception ( STACK_UNDERFLOW, 0, QUIT ) ;
    }
}

void
_Stack_Print ( Stack * stack, byte * name, int64 depth, Boolean isWordAlreadyFlag )
{
    _Stack_PrintHeader ( stack, name ) ;
    _Stack_PrintValues ( name, stack->StackPointer, depth, isWordAlreadyFlag ) ;
    CSL_NewLine ( ) ;
}

void
Stack_Print ( Stack * stack, byte * name, Boolean isWordAlreadyFlag )
{
    _Stack_Print ( stack, name, Stack_Depth ( stack ), isWordAlreadyFlag ) ;
}

void
_PrintNStackWindow ( uint64 * reg, byte * name, byte * regName, int64 size )
{
    // Intel SoftwareDevelopersManual-253665.pdf section 6.2 : a push decrements ESP, a pop increments ESP
    // therefore TOS is in lower mem addresses, bottom of stack is in higher memory addresses
    byte * buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ;
    int64 saveSize = size ;
    if ( reg )
    {
        iPrintf ( "\n%s   :%3i  : %s = " UINT_FRMT " : Top = " UINT_FRMT "", name, size, regName, ( uint64 ) reg, ( uint64 ) reg ) ;
        // print return stack in reverse of usual order first
        while ( size -- > 1 ) Stack_Print_AValue ( reg, size, name, buffer, 0 ) ;
        _Stack_PrintValues ( ( byte* ) name, reg, saveSize, 0 ) ;
    }
}

void
_CSL_PrintNReturnStack ( int64 size, Boolean useExistingFlag )
{
    Debugger * debugger = _Debugger_ ;
    Cpu * cpu = 0 ;
    iPrintf ( "\n_CSL_PrintNReturnStack : %s", Context_Location ( ) ) ;
    if ( useExistingFlag )
    {
        if ( GetState ( debugger, DBG_STEPPING ) && debugger->CopyRSP )
        {
            _PrintNStackWindow ( ( uint64* ) debugger->CopyRSP, ( byte * ) "ReturnStackCopy", ( byte * ) "RSCP", size ) ;
            //_PrintNStackWindow ( ( uint64* ) debugger->cs_Cpu->Rsp, ( byte * ) "Return Stack", ( byte * ) "Rsp (RSP)", size ) ;
            _Stack_PrintValues ( ( byte* ) "ReturnStack ", debugger->ReturnStack->StackPointer, Stack_Depth ( debugger->ReturnStack ), 0 ) ;
        }
        else
        {
            if ( ! debugger->cs_Cpu->Rsp )  CSL_GetCpuState ( ), cpu = _CSL_->cs_Cpu, _CpuState_Show ( cpu ) ;
            else cpu = debugger->cs_Cpu ;

            _PrintNStackWindow ( ( uint64* ) cpu->Rsp, ( byte * ) "C Return Stack", ( byte * ) "Rsp (RSP)", size ) ;
            _Stack_PrintValues ( ( byte* ) "ReturnStack ", debugger->ReturnStack->StackPointer, Stack_Depth ( debugger->ReturnStack ), 0 ) ;
        }
    }
    else
    {
        _CSL_WordName_Run ( ( byte* ) "getRsp" ) ;
        uint64 * rsp = ( uint64 * ) DataStack_Pop ( ) ;
        _PrintNStackWindow ( rsp, ( byte* ) "Return Stack", ( byte* ) "Rsp (RSP)", size ) ;
    }
}

void
_CSL_PrintNDataStack ( int64 size )
{
    _PrintNStackWindow ( _DspReg_, ( byte* ) "Data Stack", ( byte* ) "Dsp (DSP:R14)", size ) ;
}

