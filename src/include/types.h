// see readme.txt for a text description
// TODO : types, database, garbage collection : integration
typedef char int8 ;
typedef unsigned char uint8 ;
typedef uint8 ubyte ;
typedef uint8 byte ;
typedef short int16 ;
typedef unsigned short uint16 ;
typedef int int32 ;
typedef unsigned int uint32 ;
typedef long int int64 ;
typedef unsigned long int uint64 ;
typedef uint8 Boolean ;
typedef byte * Pointer ;
typedef Pointer Struct ;

typedef char * CString ;
typedef byte CharSet ;
typedef void (* VoidFunction ) (void) ;
typedef void (*vFunction_1_Arg ) ( int64 ) ;
typedef void (*vFunction_1_UArg ) ( uint64 ) ;
typedef void (*vFunction_2_Arg ) ( int64, int64 ) ;
typedef int64( *cFunction_0_Arg ) ( ) ;
typedef int64( *cFunction_1_Arg ) ( int64 ) ;
typedef
int64( *cFunction_2_Arg ) ( int64, int64 ) ;
typedef VoidFunction block ; // code block
typedef int ( *mpf2andOutFunc ) ( mpfr_ptr, mpfr_srcptr, mpfr_srcptr, mpfr_rnd_t ) ;
typedef struct
{
    int64 *InitialTosPointer ;
    int64 StackSize ;
    int64 *StackPointer ;
    int64 *StackMax ;
    int64 *StackMin ;
    int64 *StackData ;
    int64 data [0] ;
} Stack ;
typedef byte * function, * object, * type, * slot ;
typedef struct
{
    union
    {
        struct
        {
            union
            {
                struct
                {
                    uint64 T_MorphismAttributes ;
                    uint64 T_ObjectAttributes ;
                    uint64 T_LispAttributes ;
                    uint64 T_WAllocationType ;
                    uint32 T_WordAttributes ;
                    uint16 T_NumberOfPrefixedArgs ;
                    union
                    {
                        uint16 T_Unused ;
                        uint16 T_UseCount ;
                    } ;
                } ;
                //AttributeBitField abf ;
                //class bitset abt[320] ;
            } ;
            union
            {
                int64 T_NumberOfSlots ;
                int64 T_NumberOfBytes ;
            } ;
            union
            {
                int64 T_Size ;
                int64 T_ChunkSize ; // remember MemChunk is prepended at memory allocation time
            } ;
        } ;
        uint64 AttributeArray [7] ;
    } ;
} AttributeInfo, TypeInfo, TI ;
/*
typedef struct
{
    union
    {
        AttributeInfo o_Attributes ;
        type o_type ; // for future dynamic types and dynamic objects 
    } ;
    union
    {
        slot * o_slots ; // number of slots should be in o_Attributes.T_NumberOfSlots
        object * o_object ; // size should be in o_Attributes.T_Size
    } ;
} Object, Tuple ;
#define Tp_NodeAfter o_slots [0] ;
#define Tp_NodeBefore o_slots [1] ;
#define Tp_SymbolName o_slots [2] ;

typedef object * ( *primop ) ( object * ) ;
typedef Object * ( *Primop ) ( Object * ) ;
 */

typedef struct _node
{
    struct
    {
        union
        {
            struct _node * n_After ;
            struct _node * n_Head ;
        } ;
        union
        {
            struct _node * n_Before ;
            struct _node * n_Tail ;
        } ;
    } ;
} dlnode, node, _dllist ;
typedef struct
{
    _dllist l_List ;
    node * l_CurrentNode ;
} dllist ;
#define Head l_List.n_Head
#define Tail l_List.n_Tail
enum types
{
    BOOL, BYTE, INTEGER, STRING, T_BIGNUM, FLOAT, POINTER, X64CODE, WORD, WORD_LOCATION, ARROW, CARTESIAN_PRODUCT
} ;
typedef struct
{
    union
    {
        struct
        {
            dlnode * do_After ;
            dlnode * do_Before ;
        } ;
        dlnode do_Node ;
    } ;
    struct
    {
        int32 do_Type ;
        int16 do_Size ;
        int8 do_Slots ;
        int8 do_InUseFlag ;
    } ;
    union
    {
        byte * do_unmap ;
        byte * do_bData ;
        int64 * do_iData ;
    } ;
} dobject ;
typedef struct
{
    union
    {
        struct
        {
            dlnode * n_After ;
            dlnode * n_Before ;
        } ;
        dlnode n_Node ;
    } ;
    union
    {
        struct
        {
            int32 n_Type ;
            int16 n_Size ;
            Boolean n_Slots ;
            Boolean n_InUseFlag ;
        } ;
        byte * n_unmap ;
        byte * n_bData ;
        int64 * n_iData ;
        node * n_CurrentNode ;
    } ;
} _DLNode, _Node, _ListNode, _DLList ; // size : 3 x 64 bits
typedef struct
{
    union
    {
        _DLNode n_DLNode ;
        dobject n_dobject ;
    } ;
    AttributeInfo n_Attributes ;
} DLNode, Node, ListNode, DLList, List ;

#define n_Car n_After 
#define n_Cdr n_Before
typedef void ( *MapFunction0 ) ( dlnode * ) ;
typedef int64( *MapFunction1 ) ( dlnode *, int64 ) ;
typedef int64( *MapFunction2 ) ( dlnode *, int64, int64 ) ;
typedef void ( *MapFunction2_64 ) ( dlnode *, uint64, int64 ) ;
typedef int64( *MapFunction3 ) ( dlnode *, int64, int64, int64 ) ;
typedef int64( *MapFunction4wReturn ) ( dlnode *, int64, int64, int64, int64 ) ;
typedef void ( *MapFunction4 ) ( dlnode *, int64, int64, int64, int64 ) ;
typedef void ( *MapFunction5 ) ( dlnode *, int64, int64, int64, int64, int64 ) ;
typedef
Boolean( *BoolMapFunction_1 ) ( dlnode * ) ;
#if 0 // possibly a better use of space 
typedef struct Namespace
{
} ;
typedef struct _Identifier // _Symbol
{
    union
    {
        struct Namespace Namespace ;
        struct Vocabulary Vocabulary ;
        struct Class Class ;
        struct DynamicObject DynamicObject ;
        struct DObject DObject ;
        struct ListObject ListObject ;
        struct Symbol Symbol ;
        struct MemChunk MemChunk
        struct HistoryStringNode HistoryStringNode ;
        struct Buffer Buffer ;
        struct CaseNode CaseNode ;
    } ;
} Identifier ;
#endif
typedef struct _Identifier // _Symbol
{
    DLNode S_Node ;
    int64 CodeSize ;
    byte * S_Name ;
    uint64 State ;
    dllist * S_SymbolList ;
    uint64 S_DObjectValue ; // nb! DynamicObject value can not be a union with S_SymbolList
    uint64 * S_PtrToValue ; // because we copy words with Compiler_PushCheckAndCopyDuplicates and we want the original value
    block Definition ;
    //struct _Identifier * ContaingingStructNamespace ;
    union
    {
        uint64 S_Value ;
        byte * S_BytePtr ;
        byte * S_Object ;
        struct _Identifier * S_ValueWord ;
    } ;
    union // leave this here so we can add a ListObject to a namespace
    {
        struct _Identifier * S_ContainingNamespace ;
        struct _Identifier * S_ContainingList ;
        struct _Identifier * S_Prototype ;
    } ;
    union
    {
        uint64 S_Value2 ;
        dlnode * S_Node2 ;
        byte * S_pb_Data2 ;
        byte * StringMacroValue ;
        struct
        {
            byte N ;
            byte Ttt ;
            union
            {
                int16 NotUsed16 ;
                struct
                {
                    int8 BitFieldSize ; //: 8 ;
                    int8 BitFieldOffset ; //: 8 ;
                } ;
            } ;
            int32 NotUsed32 ;
        } ;
    } ;
    union
    {
        uint64 S_Value3 ;
        dlnode * S_Node3 ;
        byte * S_pb_Data3 ;
        byte * TextMacroValue ;
        byte * JccCode ;
    } ;
    struct _WordData * W_WordData ;
} Identifier, ID, Word, Combinator, Namespace, Vocabulary, Class, DynamicObject, DObject, ListObject, Object, Symbol, Buffer, CaseNode, MemChunk, HistoryStringNode ;
#define S_Car S_Node.n_DLNode.n_After
#define S_Cdr S_Node.n_DLNode.n_Before
#define S_After S_Cdr
#define S_Before S_Car
#define S_CurrentNode n_CurrentNode
#define S_MorphismAttributes S_Node.n_Attributes.T_MorphismAttributes
#define S_ObjectAttributes S_Node.n_Attributes.T_ObjectAttributes
#define S_WordAttributes S_Node.n_Attributes.T_WordAttributes
#define S_WAllocType S_Node.n_Attributes.T_WAllocationType
#define S_LispAttributes S_Node.n_Attributes.T_LispAttributes
#define S_NumberOfPrefixedArgs S_Node.n_Attributes.T_NumberOfPrefixedArgs
#define S_Size S_Node.n_Attributes.T_Size
#define Size S_Size 
#define CompiledDataFieldByteSize Size
#define ObjectByteSize S_Node.n_Attributes.T_NumberOfBytes
#define S_ChunkSize S_Node.n_Attributes.T_ChunkSize
#define S_NumberOfSlots S_Node.n_Attributes.T_NumberOfSlots
#define S_Pointer W_Value
#define S_String W_Value
#define S_unmap S_Node.n_DLNode.n_unmap
#define S_CodeSize CodeSize 
#define S_MacroLength CodeSize 

#define Name S_Name
#define W_Attributes S_Node.n_Attributes
#define W_MorphismAttributes S_MorphismAttributes
#define W_ObjectAttributes S_ObjectAttributes
#define W_LispAttributes S_LispAttributes
#define W_TypeAttributes S_WordAttributes
#define W_NumberOfPrefixedArgs S_NumberOfPrefixedArgs 
#define W_AllocType S_WAllocType
#define W_Filename W_WordData->Filename
#define W_LineNumber W_WordData->LineNumber
#define W_UseCount W_Attributes.T_UseCount
#define CProp S_MorphismAttributes
#define CProp2 S_ObjectAttributes
#define LProp S_LispAttributes
#define WProp S_WordAttributes
#define Data S_pb_Data2
#define InUseFlag S_Node.n_DLNode.n_InUseFlag

#define Lo_CAttribute W_MorphismAttributes
#define Lo_LAttribute W_LispAttributes
#define Lo_CProp W_MorphismAttributes
#define Lo_LProp W_LispAttributes
#define Lo_Name Name
#define Lo_Size CompiledDataFieldByteSize
#define Lo_Head Lo_Car
#define Lo_Tail Lo_Cdr
#define Lo_NumberOfSlots S_NumberOfSlots //Slots
#define Lo_List S_SymbolList 
#define Lo_Value S_Value
#define Lo_PtrToValue S_PtrToValue 
#define Lo_Object Lo_Value
#define Lo_UInteger Lo_Value
#define Lo_Integer Lo_Value
#define Lo_String Lo_Value
#define Lo_LambdaParameters W_WordData->LambdaArgs
#define Lo_LambdaBody W_WordData->LambdaBody

#define W_List S_SymbolList 
#define W_Value S_Value
#define W_Object S_Object
#define W_Value2 S_Value2
#define W_Value3 S_Value3
#define W_BytePtr S_BytePtr
#define W_PtrToValue S_PtrToValue
#define W_DObjectValue S_DObjectValue

// Buffer
#define B_CAttribute S_MorphismAttributes
#define B_Size Size //S_Size
#define B_Data Data //S_pb_Data2

// MemChunk, HistoryStringNode 
#define mc_Node S_Node
//#define mc_Size 
#define mc_Type S_ObjectAttributes
#define mc_unmap  S_Node.n_DLNode.n_unmap
#define  mc_ChunkSize  CodeSize // S_ChunkSize is the total size of the chunk including any prepended accounting structure in that total
#define mc_AllocType  S_WAllocType
#define mc_Name S_Name
#define mc_Data  S_pb_Data2
#define mc_TotalAllocSize 

typedef int64( *cMapFunction_1 ) ( Symbol * ) ;
typedef ListObject* ( *ListFunction0 )( ) ;
typedef ListObject* ( *ListFunction )( ListObject* ) ;
typedef ListObject * ( *ListFunction2 ) ( ListObject*, ListObject* ) ;
typedef ListObject * ( *ListFunction3 ) ( ListObject*, ListObject*, int64 ) ;
typedef ListObject * ( *ListFunction4 ) ( ListObject*, ListObject*, int64, ListObject* ) ;
typedef int64( *MapFunction_Word_PtrInt ) ( ListObject *, Word *, int64 * ) ;
typedef int64( *MapFunction ) ( Symbol * ) ;
typedef int64( *MapFunction_1 ) ( Symbol *, int64 ) ;
typedef int64( *MapFunction_Word ) ( Symbol *, Word * ) ;
typedef
int64( *MapFunction_2 ) ( Symbol *, int64, int64 ) ;
typedef void ( *MapSymbolFunction ) ( Symbol * ) ;
typedef void ( *VMapNodeFunction ) ( dlnode * ) ;
typedef void ( *MapSymbolFunction1 ) ( Symbol *, int64 ) ;
typedef void ( *MapSymbolFunction2 ) ( Symbol *, int64, int64 ) ;
typedef Word* ( *MapNodeFunction ) ( dlnode * node ) ;
typedef void ( *VMapSymbol2 ) ( Symbol * symbol, int64, int64 ) ;
typedef void (*StackMapFunction0 ) ( Word * ) ;
typedef void (*StackMapFunction1 ) ( Word *, byte * ) ;
typedef void (*StackMapFunction2 ) ( Word *, int64, int64 ) ;
typedef void (*StackMapFunction4 ) ( Word *, Stack *, int64, byte * ) ;
typedef struct location
{
    byte * Filename ;
    int32 LineNumber ;
    int32 CursorPosition ;
    Word * LocationWord ;
    byte * LocationAddress ;
} Location ;
typedef union
{
    byte TypeSignatureCodes [8] ;
    Word * TypeNamespace ;
} TypeSignatureInfo ;
typedef struct _ThisWordNode
{
    dlnode WordUseNode ; // used for sorting word use
    Word * ThisWord ;
} ThisWordNode ;
typedef struct _WordData
{
    uint64 WD_RunType ;
    Namespace * TypeNamespace ;
    byte * TypeNamespaceName ;
    byte * CodeStart ;
    union
    {
        byte * WD_Coding ; // nb : !! this field is set by the Interpreter and modified by the Compiler in some cases so we also need (!) CodeStart both are needed !!  
        byte * WD_LogicCodingAfter ;
    } ;
    byte * Filename ; // ?? should be made a part of a accumulated string table ??
    int64 LineNumber ;
    int64 TokenStart_LineIndex ;
    int64 NumberOfNonRegisterArgs ;
    int64 NumberOfNonRegisterLocals ;
    int64 NumberOfVariables ;
    //int64 NumberOfRegisterVariables ;

    byte * ObjectCode ; // used by objects/class words
    byte * StackPushRegisterCode ; // used by the optInfo
    Word * AliasOf, *OriginalWord ;
    int64 Offset ; // used by ClassField
    struct
    {
        uint8 RegToUse ;
        uint8 Opt_Rm ;
        uint8 Opt_Reg ;
        uint8 SrcReg ;
        uint8 DstReg ;
        union
        {
            uint8 RegFlags ; // future uses available here !!
            uint8 NumberOfRegisterVariables ; // future uses available here !!
        } ;
        union
        {
            uint8 OpInsnGroup ;
            uint8 OpNumOfParams ;
        } ;
        uint8 OpInsnCode ;
    } ;
    byte TypeSignature [16] ;
    //Namespace * TypeObjectsNamespaces [16] ; // 16 : increase if need more than 15 objects as args
    union
    {
        dllist * LocalNamespaces ;
        Location * OurLocation ;
        Word * CompiledAsPartOf ;
    } ;
    union
    {
        int64 * WD_ArrayDimensions ;
        byte *WD_OriginalCodeText ; // arrays don't have source code
    } ;
    Stack * WD_NamespaceStack ; // arrays don't have runtime debug code
    union
    {
        ListObject * LambdaBody ;
        int64 AccumulatedOffset ; // used by Do_Object 
    } ;
    union
    {
        ListObject * LambdaArgs ;
        int64 Index ; // used by Variable and LocalWord
        int64 WD_ArrayNumberOfDimensions ;
    } ;
    dllist * SourceCodeWordList ;
    int64 SourceCodeMemSpaceRandMarker ;
    dllist * DebugWordList ;
    ThisWordNode * ThisWordUseNode ; // used for sorting word use
    int64 StartCharRlIndex ;
    int64 SC_WordIndex ;
    Identifier * CSLWord, * BaseObject ;
} WordData ; // try to put all compiler related data here so in the future we can maybe delete WordData at runtime

// to keep using existing code without rewriting ...
#define CodeStart W_WordData->CodeStart // set at Word allocation 
#define Coding W_WordData->WD_Coding // nb : !! this field is set by the Interpreter and modified by the Compiler in some cases so we also need (!) CodeStart both are needed !!  
#define LogicCodingAfter W_WordData->WD_LogicCodingAfter // nb : !! this field is set by the Interpreter and modified by the Compiler in some cases so we also need (!) CodeStart both are needed !!  
#define SourceCoding Coding //W_WordData->SourceCoding // nb : !! this field is set by the Interpreter and modified by the Compiler in some cases so we also need (!) CodeStart both are needed !!  
#define WD_Offset W_WordData->Offset // used by ClassField
#define W_NumberOfNonRegisterArgs W_WordData->NumberOfNonRegisterArgs 
#define W_NumberOfNonRegisterLocals W_WordData->NumberOfNonRegisterLocals 
#define W_NumberOfVariables W_WordData->NumberOfVariables 
#define W_InitialRuntimeDsp W_WordData->InitialRuntimeDsp 
#define TtnReference W_WordData->TtnReference // used by Logic Words
#define RunType W_WordData->WD_RunType // number of slots in Object
#define PtrObject W_WordData->WD_PtrObject 
#define AccumulatedOffset W_WordData->AccumulatedOffset // used by Do_Object
#define Index W_WordData->Index // used by Variable and LocalWord
#define NestedObjects W_WordData->NestedObjects // used by Variable and LocalWord
#define ObjectCode W_WordData->Coding // used by objects/class words
#define W_OurLocation W_WordData->OurLocation
#define W_ThisWordUseNode W_WordData->ThisWordUseNode
#define StackPushRegisterCode W_WordData->StackPushRegisterCode // used by CO
#define W_OriginalCodeText W_WordData->WD_OriginalCodeText 
//#define W_TokenEnd_ReadLineIndex W_WordData->CursorEndPosition 
#define W_TokenStart_LineIndex W_WordData->TokenStart_LineIndex 
#define S_FunctionTypesArray W_WordData->FunctionTypesArray
#define RegToUse W_WordData->RegToUse
#define Opt_Rm W_WordData->Opt_Rm
#define Opt_Reg W_WordData->Opt_Reg
#define RmReg W_WordData->RmReg
#define RegFlags W_WordData->RegFlags
#define ArrayDimensions W_WordData->WD_ArrayDimensions
#define ArrayNumberOfDimensions W_WordData->WD_ArrayNumberOfDimensions
#define W_AliasOf W_WordData->AliasOf
#define TypeNamespace W_WordData->TypeNamespace 
#define TypeNamespaceName W_WordData->TypeNamespaceName 
#define Lo_ListProc W_WordData->ListProc
#define Lo_ListFirst W_WordData->ListFirst
#define ContainingNamespace S_ContainingNamespace
#define ContainingList S_ContainingList
#define Prototype S_Prototype
#define W_SearchNumber W_Value2
#define W_FoundMarker W_Value3
#define WL_OriginalWord W_WordData->OriginalWord
#define W_SC_WordList W_WordData->SourceCodeWordList 
#define W_SC_MemSpaceRandMarker W_WordData->SourceCodeMemSpaceRandMarker
#define W_OpInsnCode W_WordData->OpInsnCode 
#define W_OpInsnGroup W_WordData->OpInsnGroup
#define W_OpNumOfParams W_WordData->OpNumOfParams
#define W_TypeSignatureString W_WordData->TypeSignature
#define W_TypeObjectsNamespaces W_WordData->TypeObjectsNamespaces
#define NamespaceStack W_WordData->WD_NamespaceStack
#define W_MySourceCodeWord W_WordData->CompiledAsPartOf
#define W_BaseObject W_WordData->BaseObject
#define W_RL_Index W_WordData->StartCharRlIndex
#define W_SC_Index W_WordData->SC_WordIndex 
#define W_StartCharRlIndex W_WordData->StartCharRlIndex
#define W_SC_WordIndex W_WordData->SC_WordIndex
#define W_CSLWord W_WordData->CSLWord
#define W_NumberOfRegisterVariables W_WordData->NumberOfRegisterVariables
#define Lo_CSL_Word W_WordData->CSLWord 

struct NamedByteArray ;
typedef struct
{
    MemChunk BA_MemChunk ;
    Symbol BA_Symbol ;
    struct NamedByteArray * OurNBA ;
    int64 BA_DataSize, MemRemaining ;
    byte * StartIndex ;
    byte * EndIndex ;
    byte * bp_Last ;
    byte * BA_Data ;
} ByteArray ;
#define BA_AllocSize BA_MemChunk.mc_ChunkSize
//#define BA_CAttribute BA_MemChunk.S_MorphismAttributes
#define BA_AAttribute BA_MemChunk.mc_AllocType
typedef struct NamedByteArray
{
    MemChunk NBA_MemChunk ;
    Symbol NBA_Symbol ;
    ByteArray *ba_CurrentByteArray ;
    int64 OriginalSize, NBA_DataSize, TotalAllocSize ;
    int64 MemInitial ;
    int64 MemAllocated ;
    int64 MemRemaining, LargestRemaining, SmallestRemaining ;
    int64 NumberOfByteArrays, Allocations, InitFreedRandMarker ;
    dllist NBA_BaList ;
    dlnode NBA_ML_HeadNode ;
    dlnode NBA_ML_TailNode ;
} NamedByteArray, NBA ;
#define NBA_AAttribute NBA_Symbol.S_WAllocType
#define NBA_Chunk_Size NBA_Symbol.S_ChunkSize
#define NBA_Name NBA_Symbol.S_Name
#define CN_CaseBlock S_Value 
#define CN_CaseBytePtrValue S_pb_Data2
#define CN_CaseUint64Value S_Value2
//struct BlockInfo ;
typedef struct
{
    Symbol GI_Symbol ; // the node
    Boolean AddressSet ;
    Word * GI_Word, * Combinator ;
    int64 BlockLevel, CombinatorLevel ;
    byte * GI_BlockInfo ;
    byte * pb_LabelName, * CompiledAtAddress, *OriginalAddress, * LabeledAddress, * pb_JmpOffsetPointer, *PtrToJmpInsn ;
} GotoInfo ;
#define GI_CAttribute GI_Symbol.S_MorphismAttributes
typedef struct
{
    byte *nc, *ncm, *ncp ;
    byte *Ncul, *Ncll ; //next char upper/lower limit
    Boolean andAfter2rparen, orAfter2rparen, andAfterOr, orAfterAnd ;
    int64 i, j, rtrn, oparenlvl, mparenlvl, pparenlvl, andOrCount ;
    //Stack * opStack ;
} Rllafl ;
typedef struct
{
    Symbol BI_Symbol ;
    uint64 State ;
    int64 CopiedSize, JccType, ParenLevel ;
    byte *LocalFrameStart, *AfterLocalFrame, *bp_First, *bp_Last, *PtrToJumpOffset, *PtrToJmpInsn, *CompiledAtAddress ;
    byte *TttnCode, *JccCode, *JccAddedCode, *TestCode, *CmpCode, *AfterCmpCode, *OriginalActualCodeStart ; 
    byte *CopiedToStart, *BI_StackPushRegisterCode, *SetccCode, * MovzxCode ;
    byte *CopiedToEnd, *CopiedToLogicJccCode, *ActualCopiedToJccCode, *JmpToAddress ;
    Boolean SetccTtt, Ttt, SetccNegFlag, N ;
    Word * LogicCodeWord, *OurCombinator, *RegisterVariableControlWord ;
    GotoInfo * BI_Gi ;
    Rllafl * BI_Rllafl ;
    Namespace * BI_LocalsNamespace ;
    //IiFlags
#define PRESERVE_INSN_SIZE      ( (uint64) 1 << 0 )
#define COULD_BE_8              ( (uint64) 1 << 1 )   
#define COULD_BE_16             ( (uint64) 1 << 2 )
#define SHOULD_BE_32            ( (uint64) 1 << 3 )
#define LOGIC_FLAG              ( (uint64) 1 << 4 )   
    int64 IiFlags ;
    byte * InsnAddress ; // compiledAtAddress
    //byte *JmpToAddress ;
    byte InsnType ;
    byte InsnSize ;
    byte OffsetSize ;
    uint16 Insn16 ; // JCC8 JCC32, etc
    byte Insn ;
    int32 Disp ; // Displacement 
} BlockInfo ;
typedef struct
{
    int64 State ;
    union
    {
        struct
        {
            uint64 * Rax ;
            uint64 * Rcx ;
            uint64 * Rdx ;
            uint64 * Rbx ;
            uint64 * Rsp ;
            uint64 * Rbp ;
            uint64 * Rsi ;
            uint64 * Rdi ;
            uint64 * R8d ;
            uint64 * R9d ;
            uint64 * R10d ;
            uint64 * R11d ;
            uint64 * R12d ;
            uint64 * R13d ;
            uint64 * R14d ;
            uint64 * R15d ;
            uint64 * RFlags ;
            uint64 * Rip ;
        } ;
        uint64 * Registers [ 18 ] ;
    } ;
} Cpu ;
typedef struct TCI
{
    uint64 State ;
    int64 TokenFirstChar, TokenLastChar, StringFirstChar, EndDottedPos, DotSeparator, TokenLength, FoundCount, MaxFoundCount ;
    int64 FoundWrapCount, ComparedWordCount, WordWrapCount, LastWordWrapCount, FoundMarker, StartFlag, ShownWrap ;
    byte *SearchToken, * PreviousIdentifier, *Identifier ;
    Word * TrialWord, * OriginalWord, *RunWord, *OriginalRunWord, *NextWord, *ObjectExtWord, * LastFoundWord ;
    Namespace * OriginalContainingNamespace, * MarkNamespace ;
} TabCompletionInfo, TCI ;

struct ReadLiner ;
typedef
byte( *ReadLiner_KeyFunction ) (struct ReadLiner *) ;
typedef struct ReadLiner
{
    uint64 State, svState ;
    int64 InputKeyedCharacter ;
    int64 FileCharacterNumber, LineNumber, OutputLineCharacterNumber ; // set by _CSL_Key
    int64 ReadIndex, svReadIndex, EndPosition ; // index where the next input character is put
    int64 MaxEndPosition ;
    int64 CursorPosition, EscapeModeFlag, InputStringIndex, InputStringLength, LineStartFileIndex ;
    byte *Filename, LastCheckedInputKeyedCharacter, * DebugPrompt, * DebugAltPrompt, * NormalPrompt, * AltPrompt, * Prompt ;
    byte InputLine [BUFFER_SIZE], * InputLineString, * InputStringOriginal, * InputStringCurrent, *svLine ;
    ReadLiner_KeyFunction Key ;
    FILE *InputFile, *OutputFile ;
    HistoryStringNode * HistoryNode ;
    TabCompletionInfo * TabCompletionInfo0 ;
    Stack * TciNamespaceStack ;
} ReadLiner ;
typedef void ( * ReadLineFunction ) ( ReadLiner * ) ;
typedef struct
{
    uint64 State ;
    Word *FoundWord ;
    Namespace * QualifyingNamespace ;
} Finder ;

struct _Interpreter ;
typedef struct SourceCodeInfo
{
    int64 SciIndex, SciQuoteMode, SciFileIndexScStart, SciFileIndexScEnd ;
    Word * SciWord ;
    byte * SciBuffer, *SciSourceCode_AllocatedString ;
} SourceCodeInfo ;
typedef struct _Lexer
{
    uint64 State ;
    uint64 L_MorphismAttributes, L_ObjectAttributes, Token_CompiledDataFieldByteSize ;
    int64 TokenStart_ReadLineIndex, TokenEnd_ReadLineIndex, TokenStart_FileIndex, TokenEnd_FileIndex, Token_Length, SC_Index ; //Tsrli = TokenStart_ReadLineIndex
    int64 CurrentReadIndex, TokenWriteIndex, LineNumber ;
    byte *OriginalToken, *ParsedToken, TokenInputByte, LastLexedChar, CurrentTokenDelimiter ;
    byte * TokenDelimiters, * DelimiterCharSet, * DelimiterOrDotCharSet, *Filename, *LastToken ;
    byte( *NextChar ) (struct _Lexer * lexer ), * TokenBuffer, CurrentChar ;
    union
    {
        uint64 Literal ;
        byte * LiteralString ;
    } ;
    Word * TokenWord ;
    Symbol * NextPeekListItem ;
    ReadLiner * ReadLiner0 ;
    SourceCodeInfo SCI ;
    dllist * TokenList ;
} Lexer ;
typedef struct
{
    DLNode S_Node ;
    union
    {
        uint64 State ;
        struct
        {
            uint8 State_ACC ;
            uint8 State_OREG ;
            uint8 State_OREG2 ;
        } ;
    } ;
    int64 OptimizeFlag ;
    int64 CO_Dest_RegOrMem ;
    int64 CO_Mod ;
    int64 CO_Reg ;
    int64 CO_Rm ;
    int64 CO_Disp ;
    int64 CO_Imm ;
    int64 CO_ImmSize ;
    int64 CO_SrcReg ;
    int64 CO_DstReg ;
    int64 UseReg ;
    union
    {
        struct
        {
            Word *coiw_zero, * coiw_one, *coiw_two, *coiw_three, *coiw_four, *coiw_five, *coiw_six, *coiw_seven ;
        } ;
        Word * COIW [8] ; // CompileOptimizeInfo Word array
    } ;

    int64 rtrn, NumberOfArgs ;
    uint16 ControlFlags ;
    Word *rparenPrevOp, *opWord, *wordn, *wordm, *wordArg1, *wordArg2, *xBetweenArg1AndArg2, *wordArg0_ForOpEqual, *lparen1, *lparen2 ;
    dlnode * node, *nodem, *wordNode, *nextNode, *wordArg2Node, *wordArg1Node ;
    Boolean rvalue, wordArg1_rvalue, wordArg2_rvalue, wordArg1_literal, wordArg2_literal ;
    Boolean wordArg1_Op, wordArg2_Op ;
#define REG_LOCK_BIT              ( 0x10 ) // decimal 16, beyond the 15 regs
#define STACK_ARGS_TO_STANDARD_REGS  ( (uint64) 1 << 16 )
} CompileOptimizeInfo, COI ;
typedef struct TypeDefInfo
{
    int64 State, Tdi_Offset, Tdi_StructureUnion_Size, Tdi_Structure_Size, Tdi_Union_Size, Tdi_Field_Size, T_Type ;
    int64 LineNumber, Token_EndIndex, Token_StartIndex, *Tdi_ArrayDimensions, Tdi_ArrayNumberOfDimensions ;
    int64 Tdi_BitFieldOffset, Tdi_BitFieldSize ; //, T_Type ;
    Namespace *Tdi_InNamespace, * Tdi_StructureUnion_Namespace, * Tdi_Field_Type_Namespace, *FunctionId ;
    Word * Tdi_Field_Object ;
    byte *NextChar, *DataPtr, * TdiToken, *FieldName, *StructureUnionName ;
} TypeDefInfo, TDI ;
//TypeDefStructCompileInfo State flags
#define TDI_CLONE_FLAG                    ( (uint64) 1 << 0 ) 
#define TDI_STRUCT                        ( (uint64) 1 << 1 ) 
#define TDI_UNION                         ( (uint64) 1 << 2 ) 
#define TDI_BITFIELD                        ( (uint64) 1 << 3 ) 
#define TDI_PRINT                         ( (uint64) 1 << 4 ) 
#define TDI_POINTER                       ( (uint64) 1 << 5 ) 
#define TDI_UNION_PRINTED                 ( (uint64) 1 << 6 ) 
#define TDI_POST_STRUCT                   ( (uint64) 1 << 7 ) 
#define TDI_STRUCT_OR_UNION_FLAG          ( (uint64) 1 << 8 ) 
#define TDI_STRUCT_OR_UNION_FIELD         ( (uint64) 1 << 9 ) 
// typedef/parse defines
#define TD_PRE_STRUCTURE_ID            ( (uint8) 1 << 0 )
#define TD_POST_STRUCTURE_ID           ( (uint8) 1 << 1 )
#define TD_TYPE_FIELD                  ( (uint8) 1 << 2 )
#define TD_STRUCT_ID                   ( (uint8) 1 << 3 )
#define TD_ARRAY_FIELD              ( (uint8) 1 << 4 )
#define TD_FUNCTION_TYPE            ( (uint8) 1 << 5 )
#define TD_FUNCTION_PARAMETER_TYPES ( (uint8) 1 << 6 )
#define TD_TYPENAMESPACE_DONE       ( (uint8) 1 << 7 )
#define TD_FUNCTION_ID_FIELD        ( (uint8) 1 << 8 )
#define TD_FORWARD_REF              ( (uint8) 1 << 9 )
#define TD_FIELD_ID                 ( (uint8) 1 << 10 )
#define TD_BIT_FIELD                        ( (uint64) 1 << 11 )
typedef struct
{
    dlnode * JON_Node ;
    byte * JmpToAddress ; // address pointed to by offset
    byte * OffsetPointer ; // pointer to actual code with the offset 
} JccOffsetNode, JON ;
typedef
Boolean( *LocalsRegParameterOrder ) ( Boolean ) ;
typedef struct
{
    uint64 State ;
    byte *IfZElseOffset, *ContinuePoint, * BreakPoint, * StartPoint, *CombinatorStartsAt, *CombinatorEndsAt ;
    int64 NumberOfNonRegisterLocals, NumberOfRegisterLocals, NumberOfLocals ;
    int64 NumberOfNonRegisterVariables, NumberOfRegisterVariables, NumberOfVariables ;
    int64 NumberOfNonRegisterArgs, NumberOfRegisterArgs, NumberOfArgs ;
    int64 LocalsFrameSize ; //, CastSize ;
    int64 SaveCompileMode, SaveOptimizeState ; //, SaveScratchPadIndex ;
    int64 ParenLevel ;
    int64 GlobalParenLevel, BlockLevel, CombinatorLevel, OptimizeForcedReturn ;
    int64 ArrayEnds ;
    byte * InitHere ;
    int64 * AccumulatedOptimizeOffsetPointer ;
    Boolean InLParenBlock, SemicolonEndsThisBlock, TakesLParenAsBlock, BeginBlockFlag ;
    int32 * AccumulatedOffsetPointer ;
    int64 * FrameSizeCellOffset, BlocksBegun ;
    byte * BeginAddress ; //, * RspSaveOffset, * RspRestoreOffset ;
    Word *ReturnWord, * ReturnVariableWord, * ReturnLParenOperandWord, * Current_Word_New, *Current_Word_Create, *PrefixWord ;
    Namespace *C_BackgroundNamespace, *C_FunctionBackgroundNamespace, *Qid_BackgroundNamespace, *LocalsNamespace, *NonCompilingNs ; //, ** FunctionTypesArray ;
    dllist * GotoList, *SetccMovedList ;
    dllist * CurrentMatchList ;
    dllist * RegisterParameterList, *JccAdressOffsetList ;
    CompileOptimizeInfo * OptInfo ;
    dllist *PostfixLists ;
    Stack * LHS_Word ;
    Stack * PointerToJmpInsnStack ;
    Stack * LocalsCompilingNamespacesStack ;
    Stack * CombinatorBlockInfoStack ;
    Stack * BlockStack ; //, *CombinatorStack ;
    Stack * InternalNamespacesStack ;
    Stack * InfixOperatorStack ;
    Stack * BeginAddressStack ;
    dllist * OptimizeInfoList ;
    BlockInfo * CurrentTopBlockInfo ;
    LocalsRegParameterOrder Lrpo ;
} Compiler ;
typedef struct _Interpreter
{
    uint64 State ;
    ReadLiner * ReadLiner0 ;
    Finder * Finder0 ;
    Lexer * Lexer0 ;
    Compiler * Compiler0 ;
    byte * Token ;
    Word *w_Word, *LastWord ;
    Word *CurrentObjectNamespace, *ThisNamespace ;
    int64 WordType ;
    dllist * InterpList ;
} Interpreter ;

struct _Debugger ;
typedef void (* DebuggerFunction ) (struct _Debugger *) ;
typedef struct _Debugger
{
    uint64 State ;
    int64 * SaveDsp, * InitDsp, *RaDsp, *AddressModeSaveDsp, *SaveEdi, *SaveRsp, * WordDsp, * DebugRSP, *DebugRBP ;
    int64 *DebugRSI, *DebugRDI, * LastRsp, LevelBitNamespaceMap, Insn ;
    int64 RL_ReadIndex, SaveTOS, SaveStackDepth, Key, SaveKey, LastScwi, Esi, Edi, InsnSize ;
    Word * w_Word, *w_Alias, *w_AliasOf, *EntryWord, *LastShowInfoWord, *LastShowEffectsWord, *NextEvalWord ;
    Word *LocalsNamespace, *LastPreSetupWord, *SteppedWord, *CurrentlyRunningWord, *LastSourceCodeWord, *SubstitutedWord ; //, *OutWord ;
    byte *Menu, * Token, *DebugAddress, *ReturnAddress, *CopyRSP, *CopyRBP, *LastSourceCodeAddress ;
    byte * PreHere, *SpecialPreHere, *StartHere, *LastDisAddress, *ShowLine, * Filename ;
    block SaveCpuState, RestoreCpuState ;
    Stack *ReturnStack, *BreakReturnStack, *LocalsCompilingNamespacesStack ;
    Cpu * cs_Cpu ;
    ByteArray * StepInstructionBA ;
    byte CharacterTable [ 128 ] ;
    DebuggerFunction CharacterFunctionTable [ 48 ] ;
    ud_t * Udis ;
    dllist * DebugWordList ;
    Word * OutWord ; //for some compiler error (gcc 13/14) reason this has to be here else it breaks the debug <dbg> code!!??
} Debugger ;
typedef struct
{
    uint64 State ;
    int64 NumberBase ;
    long BigNum_Printf_Precision ;
    long BigNum_Printf_Width ;
    int64 ExceptionFlag ;
    struct timespec Timers [ 8 ] ;
} System ;
typedef struct
{
    DLNode C_Node ;
    uint64 State ;
    int64 NsCount, WordCount ; //, ParenLevel ;
    ReadLiner *ReadLiner0 ;
    Lexer *Lexer0 ;
    Finder * Finder0 ;
    Interpreter * Interpreter0 ;
    Compiler *Compiler0 ;
    System * System0 ;
    Stack * ContextDataStack ;
    byte * Location, * CurrentToken ;
    byte * DefaultTokenDelimiters ;
    byte * DefaultDelimiterCharSet ;
    byte * DefaultDelimiterOrDotCharSet ;
    byte * SpecialTokenDelimiters ;
    byte * SpecialDelimiterCharSet ;
    byte * SpecialDelimiterOrDotCharSet ;
    Word * CurrentlyRunningWord, *LastRanWord, *CurrentTokenWord, *TokenDebugSetupWord, *CurrentEvalWord, *LastEvalWord, *NlsWord ;
    Word * CurrentCombinator, *SourceCodeWord, *CurrentDisassemblyWord, * LastCompiledWord, *CurrentWordBeingCompiled ;
    Word * ArrayBaseFieldObject, * SC_CurrentCombinator, * BaseObject ;
    Namespace * QidInNamespace, *TypeCastNamespace ;
    block CurrentlyRunningWordDefinition ;
    dllist * PreprocessorStackList ;
    NBA * ContextNba ;
    Stack * TDI_StructUnionStack ;
    sigjmp_buf JmpBuf0 ;
} Context ;
typedef void (* ContextFunction_3 ) ( Context * cntx, byte* arg1, int64 arg2, int64 arg3 ) ;
typedef void (* ContextFunction_2 ) ( Context * cntx, byte* arg1, int64 arg2 ) ;
typedef void (* ContextFunction_1 ) ( Context * cntx, byte* arg ) ;
typedef void (* ContextFunction ) ( Context * cntx ) ;
typedef void (* LexerFunction ) ( Lexer * ) ;
typedef struct _CombinatorInfo
{
    union
    {
        int64 CI_i32_Info ;
        struct
        {
            unsigned BlockLevel : 16 ;
            unsigned ParenLevel : 16 ;
        } ;
    } ;
} CombinatorInfo ;

struct _CSL ;
typedef struct _LambdaCalculus
{
    uint64 State, DebuggerState, DebuggerSetupFlag ;
    int64 DontCopyFlag, Loop, ParenLevel, EvalListDepth, EvalDepth ;
    Namespace *LispNamespace, *LispDefinesNamespace, *LispTempNamespace, *BackgroundNamespace ;
    ListObject *Lread, *L0, *L1, *Lfirst, *LWord, *Lfunction0, *Lfunction, *Lvalue, *Locals, *Largs0, *Largs, *Largs1, * Nil, *True, *FunctionParameters, *FunctionArgs ;
    ListObject *LastInterpretedWord ;
    ByteArray * SavedCodeSpace ;
    uint64 ItemQuoteState, QuoteState ;
    Stack * QuoteStateStack ;
    Stack * CombinatorInfoStack ;
    uint64 * SaveStackPointer ;
    byte * LC_SourceCode, *LC_Here ;
    Word * Sc_Word, *ArrayBaseObject, *BaseObject ;
    Buffer *OutBuffer, *PrintBuffer ;
    byte * buffer, *outBuffer ;
    block Code ;
    dllist * Lambda_SC_WordList ;
    Boolean ApplyFlag, LetFlag, SavedTypeCheckState, IndentDbgPrint ;
    struct _CSL * OurCSL ;
    sigjmp_buf LC_JmpBuf ;
    //Cpu EvalList_CpuState ;
    block LC_SaveCpuState, LC_RestoreCpuState ;
} LambdaCalculus ;
typedef struct
{
    union
    {
        struct
        {
            unsigned CharFunctionTableIndex : 16 ;
            unsigned CharType : 16 ;
        } ;
        int64 CharInfo ;
    } ;
} CharacterType ;
typedef struct _StringTokenInfo
{
    uint64 State ;
    int64 StartIndex, EndIndex ;
    byte * In, *Out, *Delimiters, *SMNamespace ;
    CharSet * CharSet0 ;
} StringTokenInfo, StrTokInfo ;
// StrTokInfo State constants
#define STI_INITIALIZED     ( 1 << 0 )
typedef struct _CSL
{
    uint64 State, SavedState, * SaveDsp ;
    int64 InitSessionCoreTimes, WordsAdded, FindWordCount, FindWordMaxCount, TerminalLineWidth, IncludeFileStackNumber ;
    int64 WordCreateCount, DObjectCreateCount, DebugLevel ; // SC_Index == SC_Buffer Index ;
    Stack *ReturnStack, * DataStack ;
    Namespace * Namespaces, * InNamespace, *BigNumNamespace, *IntegerNamespace, *StringNamespace, *RawStringNamespace,
    *C_Preprocessor_IncludeDirectory_SearchList, *C_Preprocessor_IncludedList ;
    Context * Context0 ;
    Stack * ContextStack, * TypeWordStack ;
    Debugger * Debugger0 ;
    LambdaCalculus * LC ;
    Cpu * cs_Cpu, * cs_Cpu2 ;
    block CurrentBlock, WordRun, SaveCpuState, SaveCpu2State, RestoreCpuState, RestoreCpu2State, Set_DspReg_FromDataStackPointer, Set_DataStackPointer_FromDspReg ; //, PeekReg, PokeReg ;
    block PopDspToR8AndCall, CallReg_TestRSP, Call_ToAddressThruSREG_TestAlignRSP ; //adjustRSPAndCall, adjustRSP ;
    ByteArray * PeekPokeByteArray ;
    Word * LastFinished_DObject, * LastFinished_Word, *StoreWord, *PokeWord, *RightBracket ;
    Word *DebugWordListWord, *EndBlockWord, *BeginBlockWord, *InfixNamespace ;
    byte ReadLine_CharacterTable [ 256 ], * OriginalInputLine, * TokenBuffer ; // nb : keep this here -- if we add this field to Lexer it just makes the lexer bigger and we want the smallest lexer possible
    ReadLineFunction ReadLine_FunctionTable [ 24 ] ;
    CharacterType LexerCharacterTypeTable [ 256 ] ;
    LexerFunction LexerCharacterFunctionTable [ 28 ] ;
    Buffer *StringB, * TokenB, *OriginalInputLineB, *InputLineB, *svLineB, *SourceCodeBuffer, *StringInsertB, *StringInsertB2,
    *StringInsertB3, *StringInsertB4, *StringInsertB5, *StringInsertB6, *StringInsertB7, *StrCatBuffer, *InputLine ;
    Buffer *TabCompletionBuf, *LC_OutB, * LC_PrintB, * LC_DefineB, *DebugB, *DebugB1, *DebugB2, *DebugB3, *DebugB4, *ScratchB1, *ScratchB2,
    *ScratchB3, *ScratchB4, *ScratchB5, *StringMacroB, *TabCompletion, *DebuggerEscape, *Preprocessor, *FormatRemoval ; // token buffer, tab completion backup, source code scratch pad, 
    byte * LogFilename ;
    FILE * LogFILE ;
    StrTokInfo Sti ;
    dllist * CSL_N_M_Node_WordList ;
    SourceCodeInfo SCI ;
    sigjmp_buf JmpBuf0 ;
} CSL, ContextSensitiveLanguage ;
#define SC_Word SCI.SciWord
#define SC_Buffer SCI.SciBuffer
#define SC_String SCI.SciSourceCode_AllocatedString
#define SC_QuoteMode SCI.SciQuoteMode
#define SC_Index SCI.SciIndex
typedef struct
{
    MemChunk MS_MemChunk ;
    NamedByteArray * SessionObjectsSpace, * TempObjectSpace, * CompilerTempObjectSpace,
    * ContextSpace, * WordRecylingSpace, * LispTempSpace, * LispCopySpace,
    * BufferSpace, * CodeSpace, * ObjectSpace, * LispSpace, * DictionarySpace, * StringSpace, *ForthSpace, * ExceptionSpace ;

    dllist *NBAs ;
    int64 RecycledWordCount, WordsInRecycling, WordsAllocated ;
    Namespace * Namespaces, * InNamespace, *SavedCslNamespaces ;
} MemorySpace ;
typedef struct
{
    int64 Red, Green, Blue ;
} RgbColor ;
typedef struct
{
    RgbColor rgbc_Fg ;
    RgbColor rgbc_Bg ;
} RgbColors ;
typedef struct
{
    int64 Fg ;
    int64 Bg ;
} IntColors ;
typedef struct
{
    union
    {
        RgbColors rgbcs_RgbColors ;
        IntColors ics_IntColors ;
    } ;
} Colors ;

typedef struct
{
    int64 State ; 
    int64 PauseFlag, InfoFlag, Signal, ExceptionCode, RestartCondition, SignalExceptionsHandled ;
    int64 LastRestartCondition, Console, Thrown, SigSegvs, Restarts, StartedTimes, StartIncludeTries ;
    byte * Message ;
    siginfo_t * SignalInfo ;
    Context * Context ;
    byte * Prompt, *ExceptionMessage, *ExceptionSpecialMessage, * ExceptionToken, *Location, *ErrorCommandLine ;
    Word * ExceptionWord ;
    void * SigAddress ;
    //byte Key ;
} Exception ;

typedef struct
{
    MemChunk OVT_MemChunk ;
    uint64 State ; // flags
    CSL * OVT_CSL ;
    Context * OVT_Context ;
    Interpreter * OVT_Interpreter ;
    LambdaCalculus * OVT_LC ;
    ByteArray * CodeByteArray ; // a variable
    int64 LogFlag, DebugOutputFlag ;

    //int64 SignalExceptionsHandled, LastRestartCondition, RestartCondition, Signal, ExceptionCode, Console ;
    int64 Console ;

    byte *InitString, *StartupString, *StartupFilename, *ErrorFilename, *VersionString ; 
    //        *ExceptionMessage, *ExceptionSpecialMessage, * ExceptionToken ;
    //Word * ExceptionWord ;

    int64 Argc ;
    char ** Argv ;
    //void * SigAddress ;
    //byte * SigLocation ;
    Colors *Current, Default, Alert, Debug, Notice, User ;

    //dlnode PML_HeadNode, PML_TailNode ;
    int64 TotalRemainingAccounted, TotalNbaAccountedMemRemaining, TotalNbaAccountedMemAllocated, TotalMemSizeTarget ;
    //int64 Mmap_RemainingMemoryAllocated, OVT_InitialStaticMemory, TotalMemFreed, TotalMemAllocated, NumberOfByteArrays ;

    MemorySpace * MemorySpace0 ;
    //dllist * OvtMemChunkList ;
    dllist * MemorySpaceList ;
    dllist * NBAs ;
    dllist * BufferList ;
    dllist * RecycledWordList ;
    dllist * RecycledOptInfoList ;
    // long term memory
    NamedByteArray * HistorySpace ;
    NamedByteArray * InternalObjectSpace ;
    NamedByteArray * OpenVmTilSpace ;
    NamedByteArray * CSL_InternalSpace ;

    // variables accessible from csl
    //int64 Verbosity, StartIncludeTries, StartedTimes, Restarts, SigSegvs, ReAllocations, Dbi ;
    //int64 StartIncludeTries, RestartCondition ;
    int64 Verbosity, ReAllocations, Dbi ;
    int64 DictionarySize, LispCopySize, LispTempSize, MachineCodeSize, ObjectSpaceSize, InternalObjectsSize, LispSpaceSize, ContextSize ;
    int64 TempObjectsSize, CompilerTempObjectsSize, WordRecylingSize, SessionObjectsSize, DataStackSize, OpenVmTilSize, ForthSize ; 
    int64 CSLSize, BufferSpaceSize, ExceptionSpaceSize, StringSpaceSize, Thrown ;
    Buffer *ExceptionBuffer, *PrintBuffer, *PrintBufferCopy, *PrintBufferConcatCopy ;
    sigjmp_buf JmpBuf0 ;
    struct timespec Timer ;
    byte Pblc, Pbf8 [8] ; // Print Buffer last char/first 8 char 
    Exception * OVT_Exception ;
} OpenVmTil, OVT ;

// note : this puts these namespaces on the search list such that last, in the above list, will be searched first
typedef struct
{
    const char * ccp_Name ;
    union
    {
        const char * pb_TypeSignature ;
        char TypeSignature [8] ;
        uint64 uint64_TypeSignature ;
    } ;
    union
    {
        uint64 OpInsnCodeGroup ;
        uint64 OpNumOfParams ;
    } ;
    uint8 OpInsnCode ;
    block blk_Definition ;
    uint64 ui64_MorphismAttributes ;
    uint64 ui64_ObjectAttributes ;
    uint64 ui64_LispAttributes ;
    const char *NameSpace ;
    const char * SuperNamespace ;
} CPrimitive ;
typedef struct
{
    const char * ccp_Name ;
    uint64 ui64_MorphismAttributes ;
    uint64 ui64_ObjectAttributes ;
    block blk_CallHook ;
    byte * Function ;
    int64 i32_FunctionArg ;
    const char *NameSpace ;
    const char * SuperNamespace ;
} MachineCodePrimitive ;
typedef struct ppibs
{
    union
    {
        int64 int64_Ppibs ; // for ease of initializing and conversion
        struct
        {
            //unsigned State : 1 ; // status of whether we should do an ifBlock or not
            //unsigned Status : 1 ;
            unsigned AccumStatus : 1 ; // status of whether we should do an ifBlock or not
            unsigned Status : 1 ; // remembers when we have done an elif in a block; only one can be done in a block in the C syntax definition whick we emulate
            unsigned IfCond : 1 ; // status of whether we should do an ifBlock or not
            unsigned ElifCond : 1 ; // remembers when we have done an elif in a block; only one can be done in a block in the C syntax definition whick we emulate
            unsigned SvIfCond : 1 ; // remembers when we have done an elif in a block; only one can be done in a block in the C syntax definition whick we emulate
            //unsigned Status : 1 ; // remembers when we have done an elif in a block; only one can be done in a block in the C syntax definition whick we emulate
            //unsigned DoItStatus : 1 ; // controls whether we do nested if block
        } ;
    } ;
    byte * Filename ;
    int64 LineNumber ;
}
PreProcessorIfBlockStatus, Ppibs ;
typedef struct typeStatusInfo
{
#define TSE_ERROR         ( 1 << 0 ) 
#define TSE_SIZE_MISMATCH ( 1 << 1 )   
    Stack * TypeWordStack ;
    Word * OpWord, *WordBeingCompiled, *StackWord0, *StackWord1 ;
    int64 TypeStackDepth, OpWordFunctionTypeSignatureLength ;
    byte *OpWordTypeSignature, ExpandedTypeCodeBuffer [32], ActualTypeStackRecordingBuffer [128] ;
    Boolean TypeErrorStatus, OpWord_ReturnsACodedValue_Flag ;
    byte OpWordReturnSignatureLetterCode ;
} TypeStatusInfo, TSI ;
typedef struct
{
    MemChunk OS_MemChunk ;
    NBA * StaticMemSpace ;
    dllist * OVT_StaticMemList, * HistorySpace_MemChunkStringList ;
    int64 Allocated, Freed, RemainingAllocated ;
    sigjmp_buf JmpBuf0 ;
} OVT_StaticMemSystem, OSMS ;
typedef struct
{
    MemChunk OVT_MemChunk ;
    dllist * OvtMemChunkList ;
    int64 Allocated, Freed, RemainingAllocated ;
} OVT_MemSystem, OMS ;
#if 0// jforth
typedef CELL_BASE_TYPE scell ;
typedef DOUBLE_CELL_BASE_TYPE dscell ;
typedef unsigned CELL_BASE_TYPE cell ;
typedef unsigned DOUBLE_CELL_BASE_TYPE dcell ;
typedef void(*builtin )( ) ;
#endif
#if 0
// retro.c
typedef struct NgaState NgaState ;

typedef void (*Handler )( NgaState * ) ;
struct NgaCore
{
    CELL sp, rp, ip ; /* Stack & instruction pointers */
    CELL active ; /* Is core active?              */
    CELL u ; /* Should next operation be     */
    /* unsigned?                    */
    CELL data[STACK_DEPTH] ; /* The data stack               */
    CELL address[ADDRESSES] ; /* The address stack            */

#ifdef ENABLE_MULTICORE
    CELL registers[24] ; /* Internal Registers           */
#endif
} ;
struct NgaState
{
    /* System Memory */
    CELL memory[IMAGE_SIZE + 1] ;

    /* CPU Cores */
    struct NgaCore cpu[CORES] ;
    int active ;

    /* I/O Devices */
    int devices ;
    Handler IO_deviceHandlers[MAX_DEVICES] ;
    Handler IO_queryHandlers[MAX_DEVICES] ;

    CELL Dictionary, interpret ; /* Interfacing     */
    char string_data[8192] ;

#ifdef ENABLE_FLOATS
    double Floats[256], AFloats[256] ; /* Floating Point */
    CELL fsp, afsp ;
#endif

#ifdef ENABLE_BLOCKS
    char BlockFile[1025] ;
#endif

#ifdef ENABLE_ERROR
    CELL ErrorHandlers[64] ;
#endif

    /* Scripting */
    char **sys_argv ;
    int sys_argc ;
    char scripting_sources[64][8192] ;
    char line[4096] ;
    int current_source ;
    int perform_abort ;
    int interactive ;

    CELL currentLine ;
    CELL ignoreToEOL, ignoreToEOF ;

    /* Configuration of code & test fences for Unu */
    char code_start[256], code_end[256] ;
    char test_start[256], test_end[256] ;
    int codeBlocks ;

    FILE *OpenFileHandles[MAX_OPEN_FILES] ;
} ;
#endif
#if 1 // lbForth - kernel.c
#define REGPARM

typedef long int cell ; 
typedef char char_t ;
typedef unsigned char uchar_t ;
typedef struct word *nt_t ;
typedef struct word *xt_t ;
typedef xt_t * REGPARM
code_t( xt_t *, nt_t ) ;

#define NAME_LENGTH 16
struct word
{
    uchar_t nlen ;
    char_t name[NAME_LENGTH - 1] ;
    nt_t next ;
    cell *does ;
    code_t *code ;
    cell param[] ;
} ;

#define NEXT_XT  (*IP++)
#define EXECUTE(XT)  IP = (XT)->code (IP, XT)

extern struct word SP_word, RP_word ;

#define POP_lbf(TYPE) ((TYPE)(*(*((cell **)SP_word.param))++))
#define PUSH_lbf(X)  (*--(*((cell **)SP_word.param)) = (cell)(X))
#define RPOP(TYPE) ((TYPE)(*(*((cell **)RP_word.param))++))
#define RPUSH(X) (*--(*((cell **)RP_word.param)) = (cell)(X))
#endif