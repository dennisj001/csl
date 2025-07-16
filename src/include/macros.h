
//#define myprintf(a, b, rest...) printf (a, b, ## rest)
#define Exception( type, response ) CSL_Exception (type, 0, response )

#define _Context_ _O_->OVT_Context
#define _CSL_ _O_->OVT_CSL
#define _Compiler_ _Context_->Compiler0
#define _Interpreter_ _Context_->Interpreter0
#define _ReadLiner_ _Context_->ReadLiner0
#define _Lexer_ _Context_->Lexer0
#define _Finder_ _Context_->Finder0
#define _DataStack_ _CSL_->DataStack
#define _DataStackPointer_ _DataStack_->StackPointer
#define _DSP_C_PTR_ _DataStackPointer_ 
#define _ReturnStack_ _CSL_->ReturnStack
#define _ReturnStack_ _CSL_->ReturnStack
#define _ReturnStackPointer_ _ReturnStack_->StackPointer
#define _RSP_ _ReturnStackPointer_ 
#define _O_CodeByteArray _O_->CodeByteArray
#define _O_CodeSpace _O_->MemorySpace0->CodeSpace
#define _LC_ _O_->OVT_LC 

#define _Compile_Int8( value ) ByteArray_AppendCopyInteger ( _O_CodeByteArray, 1, value )
#define _Compile_Int16( value ) ByteArray_AppendCopyInteger ( _O_CodeByteArray, 2, value )
#define _Compile_Int32( value ) ByteArray_AppendCopyInteger ( _O_CodeByteArray, 4, value )
#define _Compile_Int64( value ) ByteArray_AppendCopyInteger ( _O_CodeByteArray, 8, value )
#define _Compile_Cell( value ) ByteArray_AppendCopyInteger ( _O_CodeByteArray, sizeof(int64), value )
#define Here ( _ByteArray_Here ( _O_CodeByteArray ) )
#define _SetHere( address )  _ByteArray_SetHere ( _O_CodeByteArray, address ) 
#define SetDebuggerPreHere( address ) _Debugger_->PreHere = (address) 
#define Set_CompilerSpace( byteArray ) (_O_CodeByteArray = (byteArray))
#define Get_CompilerSpace( ) _O_CodeByteArray

//#define abs( x ) ((int64) (((x) >= 0) ? (x) : (-x))) 
#define TOS (_DspReg_[0]) // top of stack
#define NOS (_DspReg_[-1]) // next on stack
#define _TOS_ ( _DspReg_ ? _DspReg_ [ 0 ] : CSL_Exception (STACK_ERROR, 0, QUIT ), (uint64)-1 )
#define DSP_Top( ) TOS 
#define _DataStack_Top( ) TOS 
#define _DataStack_GetTop( ) TOS
#define _DataStack_SetTop( v ) _DspReg_ [ 0 ] = v ;
#define DataStack_SetTop( v ) { if ( _DspReg_ ) { _DataStack_SetTop( v ) } else { CSL_Exception (STACK_ERROR, 0, QUIT ) ; } }
#define _GetTop( ) TOS
#define _SetTop( v ) (TOS = v)
#define _DataStack_Drop (_DspReg_ -- )
#define Stack() CSL_PrintDataStack ( )
#define DataStack( n ) _DspReg_ [ - (n) ] 
#define Dsp( n ) DataStack( n ) 
#define Stack_Clear( stk ) Stack_Init ( stk )
#define TWS( n ) ( (Word*) (_CSL_->TypeWordStack->StackPointer [(n)]) )
//#define ResetOutputPrintBuffer() { strncpy ( _O_->PrintBufferCopy, _O_->PrintBuffer, BUFFER_SIZE ) ; Buffer_Data (_O_->PrintBuffer)[0]  = 0 ; } 

#define Calculate_FrameSize( numberOfLocals )  ( ( numberOfLocals + 1 ) * CELL_SIZE ) // 1 : space for fp

#define ClearState( obj ) ( (obj)->State = 0 ) 
#define _GetState( aState, state ) ( (aState) & (state) ) 
#define GetState( obj, state ) ((obj) && _GetState( (obj)->State, (state) )) 
#define GetState_TrueFalse( obj, _true, _false )  (obj) ? ( ( _GetState( (obj)->State, (_true) ) ) && ( ! _GetState( (obj)->State, (_false) ))  ) : 0
#define _SetStateVar( state, newState, flag ) ( ( (flag) > 0 ) ? ( (state) |= (newState) ) : ( (state) &= ~ (newState) ) )
#define SetState_TrueFalse( obj, _true, _false )  (obj) ? ( ( (obj)->State |= (_true) ), ( (obj)->State &= ~ (_false) ) ) : 0
#define SetState( obj, state, flag ) (obj) ? _SetStateVar ( ((obj)->State), (state), flag ) : 0
#define SaveAndSetState( obj, state, flag ) (obj)->SavedState = ((obj)->State & (state)), _SetStateVar ( ((obj)->State), (state), flag )
#define RestoreSavedState( obj, state ) SetState ( obj, state, (obj)->SavedState )
#define Debugger_IsStepping( debugger ) GetState ( debugger, DBG_STEPPING )
#define Debugger_SetStepping( debugger, flag ) SetState ( debugger, DBG_STEPPING, flag )  
#define Debugger_SetMenu( debugger, flag ) SetState ( debugger, DBG_MENU, flag )
#define Debugger_IsNewLine( debugger ) GetState ( debugger, DBG_NEWLINE )
#define Debugger_SetNewLine( debugger, flag ) SetState ( debugger, DBG_NEWLINE, flag ) 

#define Set_CompileMode( tf ) SetState ( _Compiler_, (COMPILE_MODE), tf ) //; _LC_ ? SetState ( _LC_, LC_COMPILE_MODE, tf ) : 0 ; 
#define Get_CompileMode() GetState ( _Compiler_, (COMPILE_MODE) )  //|| ( _LC_ ? GetState ( _LC_, LC_COMPILE_MODE ) : 0 ) ) 
#define _LC_CompileMode( lc ) GetState( lc, LC_COMPILE_MODE ) 
#define LC_CompileMode _LC_CompileMode( _LC_ )
#define CompileMode GetState ( _Compiler_, (COMPILE_MODE) || ( _LC_ ? _LC_CompileMode( _LC_ ) : 0 ) ) 
#define CompileModeOff Set_CompileMode ( false ) 
#define CompileModeOn Set_CompileMode ( true ) 
#define Compiling CompileMode
#define RunMode GetState ( _Context_, RUN_MODE )
#define ImmediateWord( word) (word->W_MorphismAttributes & IMMEDIATE)
#define CPrimitiveWord( word) (word->W_MorphismAttributes & CPRIMITIVE)

#define Stack_N( stack, offset ) ((stack)->StackPointer [ (offset) ] )
#define Stack_OffsetValue( stack, offset ) ((stack)->StackPointer [ (offset) ] )
#define WordsBack( n ) CSL_WordList( (n) )
#define WordStack( n ) CSL_WordList( (n) )
#define N_FREE  1
#define N_UNLOCKED 2
#define N_LOCKED  4
#define N_IN_USE N_LOCKED
#define N_PERMANENT 8
#define _Buffer_Data( b ) b->B_Data
#define Buffer_Data( b ) _Buffer_Data ( b ) //Buffer_DataCleared ( b ) 
#define Buffer_Size( b ) b->B_Size
#define SetBuffersUnused( force ) Buffers_SetAsUnused ( force ) 
#define Buffer_MakePermanent( b ) b->InUseFlag = N_PERMANENT
#define Buffer_Lock( b ) b->InUseFlag = N_LOCKED
#define Buffer_Unlock( b ) b->InUseFlag = N_UNLOCKED
#define _Buffer_SetAsFree( b )  b->InUseFlag = N_FREE 

#define Attribute_FromWord( word ) (( Attribute * ) (word)->This )

// formatting
// ansi/vt102 escape code
#define ClearLine iPrintf ( "\r%c[J", ESC ) 
#define Cursor_Up( n ) iPrintf ( (byte*) "%c[%dA", ESC, n )
#define Color_Black 0
#define Color_Red 1
#define Color_Green 2
#define Color_Yellow 3
#define Color_Blue 4
#define Color_Magenta 5
#define Color_Cyan 6
#define Color_White 7
#define Color_Default 9

#define Colors_Setup6( c, fr, fg, fb, br, bg, bb )\
    int64 fr, fg, fb, br, bg, bb ;\
    fr = c->rgbcs_RgbColors.rgbc_Fg.Red, fg = c->rgbcs_RgbColors.rgbc_Fg.Green, fb = c->rgbcs_RgbColors.rgbc_Fg.Blue ;\
    br = c->rgbcs_RgbColors.rgbc_Bg.Red, bg = c->rgbcs_RgbColors.rgbc_Bg.Green, bb = c->rgbcs_RgbColors.rgbc_Bg.Blue ;

#define Colors_Get6( c, fr, fg, fb, br, bg, bb )\
    c->rgbcs_RgbColors.rgbc_Fg.Red = fr, c->rgbcs_RgbColors.rgbc_Fg.Green = fg, c->rgbcs_RgbColors.rgbc_Fg.Blue = fb ;\
    c->rgbcs_RgbColors.rgbc_Bg.Red = br, c->rgbcs_RgbColors.rgbc_Bg.Green = bg, c->rgbcs_RgbColors.rgbc_Bg.Blue = bb ;

#define _Show2Colors( fg, bg ) printf ( "%c[%ld;%ldm", ESC, fg, bg )
#define _ShowColors( fg, bg ) _Show2Colors( fg + 30, bg + 40 )
#define _String_Show2( buf, fg, bg ) sprintf ( (char*) buf, "%c[%ld;%ldm", ESC, fg, bg )
#define _String_ShowColors( buf, fg, bg ) _String_Show2 ( buf, fg + 30, bg + 40 )

#define DefaultColors Ovt_DefaultColors () 
#define AlertColors Ovt_AlertColors () 
#define DebugColors Ovt_DebugColors () 
#define NoticeColors Ovt_NoticeColors () 

// Change Colors
// code :: cc change colors : u user : d  default : a alert : g debug
#define cc( s, c ) (byte*) _String_InsertColors ( (byte*) ( (byte*) s ? (byte*) s : (byte*) "" ), (c) ) 
#define c_ud( s ) cc ( (byte*) s, (_O_->Current == &_O_->UserColor) ? &_O_->Default : &_O_->UserColor ) 
#define c_ad( s ) cc ( (byte*) s, (_O_->Current == &_O_->Alert) ? &_O_->Default : &_O_->Alert ) 
#define c_da( s ) cc ( (byte*) s, (_O_->Current == &_O_->Default) ? &_O_->Alert : &_O_->Default ) 
#define c_gd( s ) cc ( (byte*) s, (_O_->Current == &_O_->DebugColor) ? &_O_->Default : &_O_->DebugColor ) 
#define c_dg( s ) cc ( (byte*) s, (_O_->Current == &_O_->Default) ? &_O_->DebugColor : &_O_->Default ) 
#define c_gu( s ) cc ( (byte*) s, (_O_->Current == &_O_->DebugColor) ? &_O_->UserColor : &_O_->DebugColor ) 
#define c_ug( s ) cc ( (byte*) s, (_O_->Current == &_O_->UserColor) ? &_O_->DebugColor : &_O_->UserColor ) 
#define c_gn( s ) cc ( (byte*) s, (_O_->Current == &_O_->DebugColor) ? &_O_->Notice : &_O_->DebugColor ) 
#define c_g( s ) cc ( (byte*) s, &_O_->DebugColor ) 
#define c_a( s ) cc ( (byte*) s, &_O_->Alert ) 
#define c_d( s ) cc ( (byte*) s, &_O_->Default ) 
#define c_u( s ) cc ( (byte*) s, &_O_->UserColor ) 
#define c_n( s ) cc ( (byte*) s, &_O_->Notice ) 

//#define TemporaryString_New( string ) String_New ( string, TEMPORARY ) 
#define IsWordRecursive CSL_CheckForGotoPoints ( GI_RECURSE )
#define AppendCharToSourceCode( c ) //_Lexer_AppendCharToSourceCode ( lexer, c ) 
#define ReadLine_Nl (ReadLine_PeekNextChar ( _Context_->ReadLiner0 ) == '\n')
#define ReadLine_Eof (ReadLine_PeekNextChar ( _Context_->ReadLiner0 ) == eof)
#define ReadLine_ClearLineQuick _Context_->ReadLiner0->InputLine [ 0 ] = 0 
#define _ReadLine_CursorPosition( rl ) (rl->CursorPosition)
#define ReadLine_GetCursorChar( rl ) (rl->InputLine [ _ReadLine_CursorPosition (rl) ])
#define ReadLine_SetCursorChar( rl, c ) (rl->InputLine [ _ReadLine_CursorPosition (rl) ] = c )

// exception handling
#define _try( object ) if ( _OpenVmTil_Try ( &object->JmpBuf0 ) ) 
//#define _catch( e ) if ( _OpenVmTil_Catch () ) // nb. : if no _throw in _catch block don't use 'return'
#define _finally _OpenVmTil_Finally () // nb. : ! use only once and after the first _try block !
//OpenVmTil_Throw (byte * excptMessage, byte * specialMessage, int64 restartCondition, int64 infoFlag , int64 pauseFlag)
#define _Throw( e ) OpenVmTil_Throw ((e == QUIT) ? (byte*) "\nQuit?\n" : (e == ABORT) ? (byte*) "\nAbort?\n" : (byte*) "", "", e, 1)
#define _throw( e ) _Throw (e) _longjmp( *(jmp_buf*) _Stack_PopOrTop ( _O_->ExceptionStack ), e ) 
#define _ThrowIt OpenVmTil_Throw ((byte*) "", (byte*) "", 0,  _O_->Thrown)
#define Throw( emsg, smsg, e ) OpenVmTil_Throw ((byte*) emsg, smsg, e, 1)
#define ThrowIt( msg ) OpenVmTil_Throw ((byte*) msg, 0,  _O_->Thrown, 1)
#define catchAll if ( _OpenVmTil_Catch () ) 
#define _SyntaxError( message, abortFlag ) CSL_Exception (SYNTAX_ERROR, message, abortFlag )
#define SyntaxError( abortFlag ) _SyntaxError( 0, abortFlag ) 
#define stopThisTry _OVT_PopExceptionStack ( )
#define stopTrying _OVT_ClearExceptionStack ( )

#define Pause() OpenVmTil_Pause ()
#define _Pause( msg ) _OpenVmTil_Pause ()
#define Pause_1( msg ) AlertColors; iPrintf ( (byte*)"\n%s", msg ) ; OpenVmTil_Pause () ;
#define Pause_2( msg, arg ) AlertColors; iPrintf ( (byte*)msg, arg ) ; OpenVmTil_Pause () ;

#define Error_Abort( emsg, smsg ) Throw ( emsg, smsg, ABORT )
#define Error_1( msg, arg, state ) AlertColors; if (state & PAUSE ) Pause () ; if (state >= QUIT ) Throw ( (byte*) msg, state ) ; 
#define Warning2( msg, str ) iPrintf ( (byte*)"\n%s : %s", (byte*) msg, str ) ; 
#define Warning( msg, str, pauseFlag ) iPrintf ( (byte*)"\n%s : %s", (byte*) msg, str ) ; if ( pauseFlag ) Pause () ;
#define ErrorWithContinuation( msg, continuation ) Throw ( (byte*) msg, continuation )
#define Error_Quit( msg ) ErrorWithContinuation( msg, QUIT )
#define ErrorN( n ) Throw ( (byte*) "", n )

// !! Get - Set - Exec !!
#define Get( obj, field ) obj->field
#define Set( obj, field, value ) (obj)->(field) = (value) 

#define ReadLiner_GetLastChar() _ReadLiner_->InputKeyedCharacter
#define ReadLiner_SetLastChar( chr ) if (_ReadLiner_) _ReadLiner_->InputKeyedCharacter = chr
#define _Lexer_IsCharDelimiter( lexer, c ) lexer->DelimiterCharSet [ c ]
#define _Lexer_IsCharDelimiterOrDot( lexer, c ) lexer->DelimiterOrDotCharSet [ c ]
#define TokenBuffer_AppendPoint( lexer ) &lexer->TokenBuffer [ lexer->TokenWriteIndex ]
#define _AppendCharacterToTokenBuffer( lex, character ) lexer->TokenBuffer [ lex->TokenWriteIndex ] = character
 
#if 0
#define NAMESPACE_TYPE ( NAMESPACE | DOBJECT | CLASS | C_TYPE | C_CLASS | CLASS_CLONE | STRUCT )
#else
#define NAMESPACE_TYPE ( NAMESPACE | DOBJECT | CLASS | C_TYPE | C_CLASS | CLASS_CLONE | STRUCT | OBJECT )
#endif
#define CLASS_TYPE ( DOBJECT | CLASS | C_TYPE | C_CLASS | CLASS_CLONE | STRUCT | OBJECT )
#define STRUCTURE_TYPE ( OBJECT | STRUCT )
#define NAMESPACE_RELATED_TYPE ( NAMESPACE_TYPE | DOBJECT_FIELD )
#define OBJECT_TYPE ( LITERAL | CONSTANT | NAMESPACE_VARIABLE | LOCAL_VARIABLE | STRUCTURE_TYPE | OBJECT_FIELD | DOBJECT | PARAMETER_VARIABLE | T_LISP_SYMBOL | THIS ) // | T_LISP_SYMBOL
#define VARIABLE_TYPE ( NAMESPACE_VARIABLE | LOCAL_VARIABLE | OBJECT | OBJECT_FIELD | DOBJECT | PARAMETER_VARIABLE | T_LISP_SYMBOL )
#define NON_MORPHISM_TYPE ( OBJECT_TYPE | VARIABLE_TYPE | NAMESPACE_RELATED_TYPE )
#define IS_NON_MORPHISM_TYPE(word) (word->W_MorphismAttributes & NON_MORPHISM_TYPE)
#define IS_VARIABLE_TYPE(word) (word->W_MorphismAttributes & VARIABLE_TYPE)
#define IS_OBJECT_TYPE(word) ((word->W_MorphismAttributes & OBJECT_TYPE)||(word->W_ObjectAttributes & OBJECT_TYPE))
#define IS_NAMESPACE_RELATED_TYPE(word) (word->W_MorphismAttributes & NAMESPACE_RELATED_TYPE)
#define IS_MORPHISM_TYPE( word ) ( ( ( ! ( word->W_ObjectAttributes & ( NON_MORPHISM_TYPE ) ) ) \
        && ( ! ( word->W_MorphismAttributes & ( DEBUG_WORD | OBJECT_OPERATOR ) ) ) \
        && ( ! ( word->W_LispAttributes & (T_LISP_SYMBOL ) ) ) ) || ( word->W_MorphismAttributes & ( CATEGORY_OP|KEYWORD|ADDRESS_OF_OP|BLOCK ) ) \
        || ( word->W_LispAttributes & ( T_LAMBDA | LISP_C_RTL_ARGS | T_LISP_SPECIAL | LISP_VOID_RETURN ) ) )

//#define Is_NamespaceType( w ) ( w ? (( ( Namespace* ) w )->W_ObjectAttributes & (NAMESPACE_TYPE)) && (! ( (w )->W_ObjectAttributes & O_POINTER ) ) : 0 )
#define Is_NamespaceType( w ) ( w ? (( ( Namespace* ) w )->W_ObjectAttributes & (NAMESPACE_TYPE)) : 0 )
#define Is_ValueType( w ) ( w ? NON_MORPHISM_TYPE (w) : 0 )
#define IsWordAttribute( word, type, attribute ) word->type & (attribute)

// memory allocation
#define _Allocate( size, nba ) _ByteArray_AppendSpace ( nba->ba_CurrentByteArray, size ) 
#define object_Allocate( type, slots, allocType ) (type *) _object_Allocate ( sizeof ( type ) * slots, allocType ) 
#define _listObject_Allocate( nodeType, slotType, slots, allocType ) (type *) object_Allocate ( sizeof ( nodeType ) + (sizeof ( slotType ) * slots), allocType ) 

#define Get_NbaSymbolNode_To_NBA( s )  ( NamedByteArray* ) ( ( ( Symbol* ) s )->S_pb_Data2 ) 
#define Set_NbaSymbolNode_To_NBA( nba )  nba->NBA_Symbol.S_pb_Data2 = ( byte* ) nba
//#define Get_NbaLinkNode_To_NBA( node )  ( NamedByteArray* ) ( ( ( Symbol* ) node )->S_pb_Data2 ) 
#define Get_BA_Symbol_To_BA( s )  ( ByteArray* ) ( ( ( Symbol* ) s )->S_pb_Data2 ) 
#define Set_BA_Symbol_To_BA( ba )  ba->BA_Symbol.S_pb_Data2 = ( byte* ) ba
#define MemCheck( block ) { OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( 1 ) ; block ; OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( 1 ) ; }
#define MemCpy(dst, src, size) _MemCpy ((byte*)dst, (byte*)src, (int64) size)

#define _Debugger_ _CSL_->Debugger0
#define DebugOff SetState ( _CSL_, DEBUG_MODE|_DEBUG_SHOW_, false )
#define DebugOn SetState ( _CSL_, DEBUG_MODE|_DEBUG_SHOW_, true ) 
#define DebugModeOff SetState ( _CSL_, DEBUG_MODE, false )
#define DebugShow_Off SetState ( _CSL_, _DEBUG_SHOW_, false ) 
#define DebugShow_On SetState ( _CSL_, _DEBUG_SHOW_, true ) 
#define Is_DebugModeOn ( GetState ( _CSL_, DEBUG_MODE ) ) 
#define Is_DebugShowOn ( GetState ( _CSL_, _DEBUG_SHOW_ ) ) 
#define _Is_DebugOn Is_DebugShowOn
#define Is_DebugOn (_Is_DebugOn && Is_DebugModeOn)
#define DEBUG_PRINTSTACK if ( GetState ( _CSL_, DEBUG_MODE )  ) CSL_PrintDataStack () ;
#define DEBUG_SETUP_TOKEN( token, debugLevel ) _Debug_Setup (0, token, 0, 0, debugLevel) ;
#define DEBUG_SETUP_ADDRESS( address, force ) if ( (address) && Is_DebugModeOn ) Debugger_PreSetup (_Debugger_, 0, 0, address, force, 0) ;
#define DEBUG_SETUP( word, debugLevel ) _Debug_Setup (word, 0, 0, 0, debugLevel)
#define DEBUG_SHOW( word, debugLevel, force ) _Debugger_PostShow (_Debugger_, word, debugLevel, force) ; //, token, word ) ;
//#define DEBUG_SHOW Debugger_PostShow ( _Debugger_ ) 
#define DEBUG_ASM_SHOW_ON SetState ( _Debugger_, DBG_ASM_SHOW_ON, true ) 
#define DEBUG_ASM_SHOW_OFF SetState ( _Debugger_, DBG_ASM_SHOW_ON, false ) 
#define _DBI GetState ( _Debugger_, DBG_ASM_SHOW_ON ) 
#define DBI_ON DEBUG_ASM_SHOW_ON
#define DBI_OFF DEBUG_ASM_SHOW_OFF
#define DBI ( Is_DebugOn & _DBI )
#define Is_DebugOn_DBI ( Is_DebugOn ? DBI_ON : 0 )
#define _VERBOSITY _O_->Verbosity
#define DBI_N( n ) (GetState ( _Debugger_, DBG_ASM_SHOW_ON ) && ( Verbosity () > n ) )
#define IS_INCLUDING_FILES ( _CSL_ && _CSL_->IncludeFileStackNumber )
#define Is_DbiOn _O_->Dbi
#define RegOrder(n) LocalsRegParameterOrder_Optimized ( (n) )

#define C_SyntaxOn ( GetState ( _Context_, (C_SYNTAX|INFIX_MODE)) )
#define InfixOn ( GetState ( _Context_, INFIX_MODE) )

#define List_Init( list ) _dllist_Init ( list )
#define List_DropN( list, n ) _dllist_DropN ( list, n )
#define List_Pick_N_M( list, n ) List_Get_N_Node_M_Slot( list, n, 0 )  
#define List_New( allocType ) _dllist_New ( allocType ) 

#define _WordList_Pop( list ) _dllist_PopNode ( list )
#define DebugWordList_Push( dobj ) _dllist_AddNodeToHead ( _CSL_->DebugWordList, ( dlnode* ) dobj )
#define DbgWL_Push( node ) DebugWordList_Push( node )  
#define IsGlobalsSourceCodeOn ( GetState ( _CSL_, GLOBAL_SOURCE_CODE_MODE ))
#define _IsSourceCodeOn ( GetState ( _CSL_, DEBUG_SOURCE_CODE_MODE ) )
#define IsSourceCodeOn ( _IsSourceCodeOn || IsGlobalsSourceCodeOn )
#define IsSourceCodeOff (!IsSourceCodeOn) //( GetState ( CSL, DEBUG_SOURCE_CODE_MODE ) || IsGlobalsSourceCodeOn ))
#define Compiler_Word_SCHCPUSCA( word, clearFlag ) Compiler_SCA_Word_SetCodingHere_And_ClearPreviousUse ( word, clearFlag) 
#define Compiler_WordStack_SCHCPUSCA( index, clearFlag ) Compiler_Word_SCHCPUSCA ( CSL_WordList ( index ), clearFlag ) 
#define _SC_Global_On SetState ( _CSL_, GLOBAL_SOURCE_CODE_MODE, true )
#define SC_Global_On if ( GetState ( _CSL_, DEBUG_SOURCE_CODE_MODE ) ) { _SC_Global_On ; }
#define SC_Global_Off SetState ( _CSL_, GLOBAL_SOURCE_CODE_MODE, false )
#define COWordList_Reset( compiler ) List_Init ( _CSL_->CompilerWordList ) 
#if 0
#define Word_SetTsrliScwi( word, tsrli, scwi ) \
    if ( word )\
    {\
        word->W_RL_Index = word->W_RL_Index ? word->W_RL_Index : (( tsrli == - 1 ) ? _Lexer_->TokenStart_ReadLineIndex : tsrli) ;\
        word->W_SC_Index = word->W_SC_Index ? word->W_SC_Index : (( scwi == - 1 ) ? _Lexer_->SC_Index : scwi );\
    }
#endif
#define Stringn_Equal( string1, string2, n ) (Strncmp ( (byte *) string1, (byte *) string2, n ) == 0 )
#define Stringni_Equal( string1, string2, n ) (Strnicmp ( (byte *) string1, (byte *) string2, n ) == 0 )
#define Stringi_Equal( string1, string2 ) (Stricmp ( (byte *) string1, (byte *) string2 ) == 0 )
#define String_Equal( string1, string2 ) (Strcmp ( (byte *) string1, (byte *) string2 ) == 0 )
#define String_IndexedCharEqual( string, c, index ) ( string [(index)] == c )
#define _String_EqualSingleCharString( str, c ) (String_IndexedCharEqual ( str, c, 0 ) )
#define String_EqualSingleCharString( str, c ) (_String_EqualSingleCharString ( str, c ) && (str[1] == 0))
#define Strcpy( dst, src ) _Strcpy ( (char*) dst, (char*) src )
#define Strncpy( dst, src, len ) _Strncpy ( (char*) dst, (char*) src, (strlen ( (char*) src ) + 1) < len ? (strlen ( (char*) src ) + 1) : len )
#define Strcat( dst, src ) _Strncat ( (char*) dst, (char*) src, (strlen ( (char*) src ) + 1) )
#define Strncat( dst, src, len ) _Strncat ( (char*) dst, (char*) src, (strlen ( (char*) src ) + 1) < len ? (strlen ( (char*) src ) + 1) : len ) 
//#define Strncat( dst, src, n ) strncat ( (char *__restrict) dst, (const char *__restrict) src, (size_t) n )
#define sconvbs( d, s ) (byte*) _String_ConvertStringToBackSlash (d, s, -1 , 0)
#define String_CB( string0 ) String_ConvertToBackSlash (string0, 0)
#define Strlen( s ) ( s ? strlen ( (const char *) s ) : 0 )
#define StringLength( s ) Strlen ( s )
#define String_Init( s ) s[0]=0 ; 
#define Map0( dllist, mf ) dllist_Map ( dllist, (MapFunction0) mf )

#define _CONTEXT_TDI_STACK (_Context_->TDI_StructUnionStack)
#define CONTEXT_TDI_STACK( cntx ) (((Context*) cntx) ? cntx->TDI_StructUnionStack : _CONTEXT_TDI_STACK)

#define NUMBER_BASE_GET() _Context_->System0->NumberBase
#define NUMBER_BASE_SET( value ) _Context_->System0->NumberBase = ( value )
#define DUMP_MOD 16

// OVT_RecyclingAccounting flags
#define OVT_RA_ADDED 1
#define OVT_RA_RECYCLED 2

#define Error( msg, state ) _Error ( (byte * ) msg, (uint64) state )
#define MEM_FREE            0 
#define MEM_ALLOC           1 

//#define SetCompilerField( field, subfield, address ) if ( _Compiler_->field ) { _Compiler_->field->subfield = address ; }
//#define _CurrentTopBlockInfo( code ) SetCompilerField( CurrentTopBlockInfo #code )
#define M_CheckOffset( offset, bits ) ( ( offset >= (- (pow (2, (bits-1)) - 1)) ) && (offset <= ( (pow (2, (bits-1)) ) ) ) )
