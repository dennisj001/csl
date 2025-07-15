
#include "../../include/csl.h"

void
Debug_ShowCurrentLine_Vars ( int64 dbgOnlyFlag, Word * word, byte * functionName, byte * varName, int64 varValue, byte * var2Name, int64 var2Value )
{
    if ( ( dbgOnlyFlag && Is_DebugOn ) || ( ! dbgOnlyFlag ) )
    {
        if ( ! word ) word = _Lexer_->TokenWord ; //WordStack ( 0 ) ;
        if ( word ) CSL_Show_SourceCode_TokenLine ( word, functionName, 0, word->Name, "" ) ;
        if ( functionName ) iPrintf ( "\n%s :", functionName ) ;
        CSL_PrintDataStack ( ) ;
        ReadLiner * rl = _ReadLiner_ ;
        iPrintf ( "%s\n[0]:%s ==> [%d]:%s", Context_Location ( ), & rl->InputLine [0], rl->ReadIndex, & rl->InputLine [rl->ReadIndex] ) ;
        if ( varName ) iPrintf ( IsString ( ( byte * ) varValue ) ? " %s = %s" : " %s = %d", varName, varValue ) ;
        if ( var2Name ) iPrintf ( IsString ( ( byte * ) var2Value ) ? " %s = %s" : " %s = %d", var2Name, var2Value ) ;
    }
}

void
Debugger_Menu ( Debugger * debugger )
{
    iPrintf ( ( byte* ) debugger->Menu, c_gd ( Context_Location ( ) ) ) ;
    SetState ( debugger, DBG_MENU, false ) ;
}

void
_Debugger_Locals_ShowALocal ( Cpu * cpu, Word * localsWord, Word * scWord ) // use a debugger buffer instead ??
{
    if ( Compiling ) scWord = _CSL_->SC_Word ;
    if ( cpu && localsWord && scWord )
    {
        Word * word2 = 0 ;
        byte * buffer = Buffer_DataCleared ( _CSL_->DebugB ) ;
        uint64 * fp = cpu->CPU_FP ; //? ( uint64* ) ((* cpu->CPU_FP)? ( uint64* ) (* cpu->CPU_FP) : (cpu->CPU_FP)) : 0 ;
        int64 localVarFlag = ( localsWord->W_ObjectAttributes & LOCAL_VARIABLE ) ; // nb! not a Boolean with '='
        //int64 varOffset = LocalOrParameterVar_Offset ( localsWord ) ;
        int64 varOffset = _LocalOrParameterVar_Offset ( localsWord, scWord->W_NumberOfNonRegisterArgs, Word_IsFrameNecessary ( scWord ) ) ;
        byte * address = ( byte* ) ( uint64 ) ( fp ? fp [ varOffset ] : 0 ) ;

        byte * stringValue = String_CheckForAtAdddress ( address, &_O_->Default, &_O_->User ) ;
        if ( address && ( ! stringValue ) ) word2 = Word_GetFromCodeAddress ( ( byte* ) ( address ) ) ;
        if ( word2 ) sprintf ( ( char* ) buffer, "< %s.%s %s", word2->ContainingNamespace->Name, c_u ( word2->Name ), c_g ( ">" ) ) ;

        if ( localsWord->W_ObjectAttributes & REGISTER_VARIABLE )
        {
            iPrintf ( "\n%-018s : index = [%3s:%d   ]  : <register  location> = 0x%016lx : %16s.%-16s : %s",
                "Register Variable", Get_Register_Name ( localsWord->RegToUse ), localsWord->RegToUse, cpu->Registers [ localsWord->RegToUse ],
                localsWord->S_ContainingNamespace->Name, c_u ( localsWord->Name ), word2 ? buffer : stringValue ? stringValue : ( byte* ) "" ) ;
        }
        else iPrintf ( "\n%-018s : index = [r15%s0x%02x]  : <0x%016lx> = 0x%016lx : %16s.%-16s : %s",
            localVarFlag ? "LocalVariable" : "Parameter Variable", localVarFlag ? "+" : "-", Abs ( varOffset * CELL_SIZE ),
            fp + varOffset, ( uint64 ) ( fp ? fp [ varOffset ] : 0 ),
            localsWord->S_ContainingNamespace ? localsWord->S_ContainingNamespace->Name : ( byte* ) "",
            c_u ( localsWord->Name ), word2 ? buffer : stringValue ? stringValue : ( byte* ) "" ) ;
    }
}

void
_Debugger_Locals_Show_Loop ( Cpu * cpu, Word * scWord )
{
    _Word_ShowSourceCode ( scWord ) ;
    if ( ! Compiling )
    {
        int64 * fp = ( int64* ) cpu->CPU_FP, * dsp = ( int64* ) cpu->CPU_DSP ;
        if ( ( ( uint64 ) fp < 0x7f0000000 ) ) fp = dsp ;
        iPrintf ( "\n%s.%s.%s : \nFrame Pointer = R15 = <0x%016lx> = 0x%016lx : Stack Pointer = R14 <0x%016lx> = 0x%016lx",
            scWord->ContainingNamespace ? c_gd ( scWord->ContainingNamespace->Name ) : ( byte* ) "", c_gd ( scWord->Name ), c_gd ( "locals" ),
            ( uint64 ) fp, fp ? *fp : 0, ( uint64 ) dsp, dsp ? *dsp : 0 ) ;
    }
    _Word_Show_NamespaceStackWords ( scWord, 1 ) ;
}

void
Debugger_Locals_Show ( Debugger * debugger )
{
    Word * scWord = Compiling ? _Context_->CurrentWordBeingCompiled : //Debugger_GetWordFromAddress ( debugger ) ; //debugger->w_Word :
        debugger->DebugAddress ? Word_UnAlias ( Word_GetFromCodeAddress ( debugger->DebugAddress ) ) :
        _Context_->CurrentlyRunningWord ? _Context_->CurrentlyRunningWord : debugger->w_Word ;
    if ( scWord && ( scWord->W_NumberOfVariables || _Context_->Compiler0->NumberOfVariables ) )
        _Debugger_Locals_Show_Loop ( debugger->cs_Cpu, scWord ) ;
}

Boolean
_Debugger_ShouldWeShow ( Debugger * debugger, Word * word, Boolean stepFlag, int64 debugLevel, Boolean force )
{
    //uint64* dsp = ( GetState_TrueFalse ( debugger, DBG_STEPPING, ( DBG_COMMAND_LINE | DBG_ESCAPED ) ) ) ? ( _DspReg_ = debugger->cs_Cpu->R14d ) : _DspReg_ ;
    uint64* dsp = _DspReg_ ;
    if ( ! dsp ) CSL_Exception ( STACK_ERROR, 0, QUIT ) ;
    //if ( Is_DebugOn && ( !_LC_ ) && ( _CSL_->DebugLevel >= debugLevel ) && ( force || stepFlag || ( word && ( word != debugger->LastShowEffectsWord ) ) ||
    if ( Is_DebugOn && ( _CSL_->DebugLevel >= debugLevel ) && ( force || stepFlag || ( word && ( word != debugger->LastShowEffectsWord ) ) ||
        ( debugger->PreHere && ( Here > debugger->PreHere ) ) ) )
    {
        return true ;
    }
    return false ;
}

void
_Debugger_ShowEffects ( Debugger * debugger, Word * word, Boolean stepFlag, int64 debugLevel, Boolean force )
{
    if ( _Debugger_ShouldWeShow ( debugger, word, stepFlag, debugLevel, force ) )
    {
        DebugColors ;
        if ( word && ( word->W_ObjectAttributes & OBJECT_FIELD ) ) Word_PrintOffset ( word, TOS, 0 ) ;
        _Debugger_DisassembleWrittenCode ( debugger ) ;
        Debugger_ShowChange ( debugger, word, stepFlag ) ;
        //DefaultColors ;
        debugger->LastShowEffectsWord = word ;
    }
    if ( ! GetState ( debugger, DBG_STEPPING ) ) Debugger_Setup_ResetState ( debugger ) ;
    debugger->ShowLine = 0 ;
}

void
_Debugger_ShowInfo ( Debugger * debugger, byte * prompt, int64 signal, int64 force )
{
    Exception * e = _O_->OVT_Exception ;
    if ( force || ( debugger->w_Word != debugger->LastShowInfoWord ) )
    {
        Context * cntx = _Context_ ;
        ReadLiner * rl = cntx->ReadLiner0 ;
        byte *location = Context_Location ( ) ; //( rl->Filename ) ? rl->Filename : Context_Location ( ) ;//( byte* ) "<command line>" ;
        byte signalAscii [ 128 ] ;
        int64 iw = false ;
        Word * word = debugger->w_Word ? debugger->w_Word : ( ( ! debugger->Token ) ? ( _Context_->CurrentlyRunningWord ? _Context_->CurrentlyRunningWord : _Context_->CurrentTokenWord ) : 0 ) ;
        byte wordName [300], aliasName [256], * token0 ;
        if ( word && ( iw = String_Equal ( "init", word->Name ) ) )
        {
            debugger->SubstitutedWord = word ;
            word = cntx->Interpreter0->w_Word ;
        } // 'new'
        debugger->w_AliasOf = debugger->SubstitutedWord ? debugger->SubstitutedWord : debugger->w_AliasOf ;
        if ( debugger->w_AliasOf ) //= Word_UnAlias ( word ) )
        {
            if ( iw ) snprintf ( aliasName, 255, "%s.%s",
                ( debugger->w_AliasOf->S_ContainingNamespace ? debugger->w_AliasOf->S_ContainingNamespace->Name : ( byte* ) "" ), debugger->w_AliasOf->Name ) ;
            snprintf ( wordName, 299, "%s%s%s%s", word ? ( char* ) word->Name : "", ( ( char* ) debugger->w_AliasOf ? " -> " : "" ),
                iw ? aliasName : ( debugger->w_AliasOf ? debugger->w_AliasOf->Name : ( byte* ) "" ), debugger->w_AliasOf ? ( byte* ) " " : ( byte* ) "" ) ;
            debugger->SubstitutedWord = 0 ;
            SetState ( debugger, DBG_OUTPUT_SUBSTITUTION, true ) ;
            token0 = wordName ;
        }
        else token0 = word ? word->Name : debugger->Token ;
        //if ( debugger->w_Word == cntx->LastEvalWord ) word = 0, debugger->w_Word = 0, token0 = cntx->CurrentToken ;

        if ( ! ( cntx && cntx->Lexer0 ) ) Throw ( ( byte* ) "\n_CSL_ShowInfo:", ( byte* ) "\nNo token at _CSL_ShowInfo\n", QUIT ) ;
        if ( e && (( signal == 11 ) || e->SigAddress ))
        {
            snprintf ( ( char* ) signalAscii, 127, ( char * ) "Error : signal " INT_FRMT ":: attempting address : \n" UINT_FRMT, signal, ( uint64 ) e->SigAddress ) ;
            debugger->DebugAddress = ( byte* ) e->SigAddress ;
        }
        else if ( signal ) snprintf ( ( char* ) signalAscii, 127, ( char * ) "Error : signal " INT_FRMT " ", signal ) ;
        else signalAscii[0] = 0 ;

        DebugColors ;

        if ( ( location == debugger->Filename ) && ( GetState ( debugger, DBG_FILENAME_LOCATION_SHOWN ) ) )
            location = ( byte * ) "..." ;
        SetState ( debugger, DBG_FILENAME_LOCATION_SHOWN, true ) ;

        if ( token0 ) CSL_Show_SourceCode_TokenLine ( word, prompt, signal, token0, signalAscii ) ;
        else
        {
            byte *cc_line = ( char* ) Buffer_DataCleared ( _CSL_->DebugB ) ;
            strcpy ( cc_line, ( char* ) rl->InputLine ) ;
            String_RemoveEndWhitespace ( ( byte* ) cc_line ) ;
            iPrintf ( "\n%s %s:: %s : %03d.%03d :> %s", // <:: " INT_FRMT "." INT_FRMT,
                prompt, signal ? signalAscii : ( byte* ) "", location, rl->LineNumber, rl->ReadIndex,
                cc_line ) ;
        }
        DefaultColors ;
        //        if ( ! String_Equal ( "...", location ) ) debugger->Filename = location ;
        debugger->LastShowInfoWord = word ;
    }
    else SetState_TrueFalse ( _Debugger_, DBG_AUTO_MODE_ONCE, true ) ;
    SetState ( _Debugger_, ( DBG_OUTPUT_SUBSTITUTION ), false ) ;
    debugger->w_AliasOf = 0 ;
}

void
Debugger_ShowInfo ( Debugger * debugger, byte * prompt, int64 signal )
{
    if ( ! _O_->OVT_Exception ) _O_->OVT_Exception = Exception_New ( ) ; 
    Exception * e = _O_->OVT_Exception ;
    Context * cntx = _Context_ ;
    int64 sif = 0 ;
    if ( ( GetState ( debugger, DBG_INFO ) ) || GetState ( debugger, DBG_STEPPING ) )
    {
        _Debugger_ShowInfo ( debugger, prompt, signal, 1 ) ;
        sif = 1 ;
    }
    if ( ! ( cntx && cntx->Lexer0 ) )
    {
        iPrintf ( "\nSignal Error : signal = %d\n", signal ) ;
        return ;
    }
    if ( ! GetState ( _Debugger_, DBG_ACTIVE ) )
    {
        debugger->Token = cntx->Lexer0->OriginalToken ;
        Debugger_FindUsing ( debugger ) ;
    }
    else if ( debugger->w_Word ) debugger->Token = debugger->w_Word->Name ;
    if ( ( e->SigSegvs < 2 ) && GetState ( debugger, DBG_STEPPING ) )
    {
        iPrintf ( "\nDebug Stepping Address : 0x%016lx", ( uint64 ) debugger->DebugAddress ) ;
        Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "", ( byte* ) "" ) ; // the next instruction
    }
    if ( ( ! sif ) && ( ! GetState ( debugger, DBG_STEPPING ) ) && ( GetState ( debugger, DBG_INFO ) ) ) _Debugger_ShowInfo ( debugger, prompt, signal, 1 ) ;
    //if ( prompt == e->ExceptionMessage ) e->ExceptionMessage = 0 ;
    if ( prompt ) e->ExceptionMessage = String_New ( prompt, TEMPORARY ) ;
}

void
CSL_ShowInfo ( Debugger * debugger, Word * word, byte * prompt, int64 signal )
{
    if ( word && ( word->W_AliasOf ) )
    {
        debugger->w_Alias = word ;
        debugger->w_AliasOf = Word_UnAlias ( word ) ;
    }
    _Debugger_->w_Word = Word_UnAlias ( word ) ;
    _Debugger_ShowInfo ( debugger, prompt, signal, 1 ) ;
}

void
Debugger_ShowState ( Debugger * debugger, byte * prompt )
{
    ReadLiner * rl = _Context_->ReadLiner0 ;
    Word * word = debugger->w_Word ;
    int64 cflag = 0 ;
    if ( word && ( word->W_ObjectAttributes & CONSTANT ) ) cflag = 1 ;
    DebugColors ;
    byte * token = debugger->Token ;
    token = String_ConvertToBackSlash ( token, 0 ) ;
    if ( word )
    {
        iPrintf ( ( cflag ? "\n%s :: %03d.%03d : %s : <constant> : %s%s%s " : word->ContainingNamespace ? "\n%s :: %03d.%03d : %s : <word> : %s%s%s " : "\n%s :: %03d.%03d : %s : <word?> : %s%s%s " ),
            prompt, rl->LineNumber, rl->ReadIndex, Debugger_GetStateString ( debugger ),
            // _O_->CSL->Namespaces doesn't have a ContainingNamespace
            word->ContainingNamespace ? word->ContainingNamespace->Name : ( byte* ) "",
            word->ContainingNamespace ? ( byte* ) "." : ( byte* ) "", // the dot between
            c_gd ( word->Name ) ) ;
    }
    else if ( token )
    {
        iPrintf ( ( cflag ? "\n%s :: %03d.%03d : %s : <constant> :> %s " : "\n%s :: %03d.%03d : %s : <literal> :> %s " ),
            prompt, rl->LineNumber, rl->ReadIndex, Debugger_GetStateString ( debugger ), c_gd ( token ) ) ;
    }
    else iPrintf ( "\n%s :: %03d.%03d : %s : ", prompt, rl->LineNumber, rl->ReadIndex, Debugger_GetStateString ( debugger ) ) ;
    if ( ! debugger->Key )
    {
        if ( word ) _CSL_Source ( word, 0 ) ;

        if ( GetState ( debugger, DBG_STEPPING ) )
            Debugger_UdisOneInstructionWithSourceCode ( debugger, 0, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ; // current insn
    }
}

void
_Debugger_DoNewlinePrompt ( Debugger * debugger )
{
    iPrintf ( "\n%s=> ", GetState ( debugger, DBG_RUNTIME ) ? ( byte* ) "<dbg>" : ( byte* ) "dbg" ) ; //, (char*) ReadLine_GetPrompt ( _Context_->ReadLiner0 ) ) ;
    Debugger_SetNewLine ( debugger, false ) ;
}

void
Debugger_DoState ( Debugger * debugger )
{
    if ( GetState ( debugger, DBG_RETURN ) )
    {
        iPrintf ( "\r" ) ;
        SetState ( debugger, DBG_RETURN, false ) ;
    }
    if ( GetState ( debugger, DBG_MENU ) )
    {
        Debugger_Menu ( debugger ) ;
        SetState ( debugger, DBG_FILENAME_LOCATION_SHOWN, false ) ;
        if ( GetState ( debugger, DBG_NEWLINE ) ) CSL_NewLine ( ), SetState ( debugger, DBG_NEWLINE, false ) ;
        //if ( GetState ( debugger, DBG_PROMPT ) ) 
    }
    //if ( GetState ( debugger, DBG_INFO ) ) Debugger_ShowInfo ( debugger, GetState ( debugger, DBG_RUNTIME ) ? ( byte* ) "<dbg>" : ( byte* ) "dbg", 0 ) ;
    byte prompt [16] ;
    if ( GetState ( debugger, DBG_RUNTIME ) )
        snprintf ( prompt, 16, _CSL_->DebugLevel ? "<dbg%ld>" : "<dbg>", _CSL_->DebugLevel ) ;
    else snprintf ( prompt, 16, _CSL_->DebugLevel ? "dbg%ld" : "dbg", _CSL_->DebugLevel ) ;
    if ( GetState ( debugger, DBG_INFO ) ) Debugger_ShowInfo ( debugger, prompt, 0 ) ;
    else if ( GetState ( debugger, DBG_SHOW ) && GetState ( debugger, DBG_PROMPT ) ) Debugger_ShowState ( debugger, prompt ) ;
        //else if ( ( GetState ( debugger, DBG_STEPPING ) ) && ( ! GetState ( debugger, DBG_CONTINUE_MODE ) ) )
    else if ( GetState ( debugger, DBG_SHOW ) && ( GetState ( debugger, DBG_STEPPING | DBG_START_STEPPING ) ) && ( ! GetState ( debugger, DBG_CONTINUE_MODE ) ) )
    {
        if ( GetState ( debugger, DBG_START_STEPPING ) && GetState ( _CSL_, DBG_UDIS ) ) iPrintf ( "\n ... Next stepping instruction ..." ) ;
        SetState ( debugger, DBG_START_STEPPING, false ) ;
        debugger->cs_Cpu->Rip = ( uint64 * ) debugger->DebugAddress ;
        if ( debugger->DebugAddress != debugger->LastDisAddress )
            Debugger_UdisOneInstructionWithSourceCode ( debugger, debugger->w_Word, debugger->DebugAddress, ( byte* ) "\r", ( byte* ) "" ) ;
        debugger->LastDisAddress = 0 ;
    }
    if ( _LC_ && _LC_->DebuggerState && GetState ( debugger, DBG_LC_DEBUG ) ) LC_Debug_Output ( _LC_ ) ;
}

void
Debugger_ShowStackChange ( Debugger * debugger, Word * word, byte * insert, byte * achange, Boolean stepFlag )
{
    int64 sl, i = 0 ;
    char *name, *location, *b, *b2 = ( char* ) Buffer_DataCleared ( _CSL_->DebugB2 ) ;
    b = ( char* ) Buffer_DataCleared ( _CSL_->DebugB1 ) ;
    if ( stepFlag ) sprintf ( ( char* ) b2, "0x%016lx", ( uint64 ) debugger->DebugAddress ) ;
    location = stepFlag ? ( char* ) c_gd ( b2 ) : ( char* ) Context_Location ( ) ;
    name = word ? ( char* ) c_gd ( String_ConvertToBackSlash ( word->Name, 0 ) ) : ( char* ) "" ;
    while ( 1 )
    {
        if ( GetState ( debugger, DBG_STEPPING ) )
            snprintf ( ( char* ) b, BUFFER_IX_SIZE, "\nStack : %s at %s :> %s <: %s", insert, location, ( char* ) c_gd ( name ), ( char* ) c_gd ( achange ) ) ;
        else snprintf ( ( char* ) b, BUFFER_IX_SIZE, "\nStack : %s at %s :> %s <: %s", insert, ( char* ) location, name, ( char* ) c_gd ( achange ) ) ;
        if ( ( sl = strlen ( ( char* ) b ) ) > GetTerminalWidth ( ) ) //220 ) //183 ) //GetTerminalWidth ( ) ) //_Debugger_->TerminalLineWidth ) //220 ) 
        {
            location = ( char* ) "..." ;
            if ( ++ i > 1 ) name = ( char* ) "" ;
            if ( i > 2 ) insert = ( byte* ) "" ;
            if ( i > 3 ) achange = ( byte* ) "" ;
            if ( i > 4 ) break ;
        }
        else break ;
    }
    iPrintf ( "%s", b ) ;
    if ( Verbosity ( ) > 3 ) _Debugger_PrintDataStack ( 2 ) ;
}

void
Debugger_ShowChange ( Debugger * debugger, Word * word, Boolean stepFlag )
{
    const char * insert ;
    uint64 change ;
    int64 depthChange ;
    if ( Debugger_IsStepping ( debugger ) )
    {
        change = _DspReg_ - debugger->SaveDsp ;
        debugger->SaveDsp = _DspReg_ ;
    }
    else
    {
        change = debugger->WordDsp ? ( _DspReg_ - debugger->WordDsp ) : 0 ;
        debugger->WordDsp = _DspReg_ ;
    }
    depthChange = DataStack_Depth ( ) - debugger->SaveStackDepth ;
    if ( word && ( debugger->WordDsp && ( GetState ( debugger, DBG_SHOW_STACK_CHANGE ) ) || ( change ) || ( debugger->SaveTOS != TOS ) || ( depthChange ) ) )
    {
        byte * name, pb_change [ 300 ] ;
        char * b = ( char* ) Buffer_DataCleared ( _CSL_->DebugB ), *op ;
        char * c = ( char* ) Buffer_DataCleared ( _CSL_->DebugB2 ) ;
        pb_change [ 0 ] = 0 ;

        if ( GetState ( debugger, DBG_SHOW_STACK_CHANGE ) ) SetState ( debugger, DBG_SHOW_STACK_CHANGE, false ) ;
        if ( depthChange > 0 ) snprintf ( ( char* ) pb_change, 256, "%ld %s%s", depthChange, ( depthChange > 1 ) ? "cells" : "cell", " pushed. " ) ;
        else if ( depthChange ) snprintf ( ( char* ) pb_change, 256, "%ld %s%s", - depthChange, ( depthChange < - 1 ) ? "cells" : "cell", " popped. " ) ;
        if ( _DspReg_ && ( debugger->SaveTOS != TOS ) ) op = ( char* ) "changed" ;
        else op = ( char* ) "set" ;
        snprintf ( ( char* ) c, BUFFER_IX_SIZE, ( char* ) "0x%016lx", ( uint64 ) TOS ) ;
        snprintf ( ( char* ) b, BUFFER_IX_SIZE, ( char* ) "%s %s to %s.", DataStack_Depth ( ) ? "[TOS]" : "TOS", op, c_gd ( c ) ) ;
        strncat ( ( char* ) pb_change, ( char* ) b, 299 ) ; // strcat ( (char*) _change, cc ( ( char* ) c, &_O_->Default ) ) ;
        name = word->Name ;
        if ( name ) name = String_ConvertToBackSlash ( name, 0 ) ;
        char * achange = ( char* ) pb_change ;
        if ( stepFlag )
        {
            Word * word = Word_GetFromCodeAddress ( debugger->DebugAddress ) ;
            if ( ( word ) && ( ( byte* ) word->Definition == debugger->DebugAddress ) )
            {
                insert = "function call" ;
                if ( achange [0] ) Debugger_ShowStackChange ( debugger, word, ( byte* ) insert, ( byte* ) achange, stepFlag ) ;
            }
        }
        else
        {
            if ( word ) insert = "word" ;
            else insert = "token" ;
            if ( achange [0] ) Debugger_ShowStackChange ( debugger, word, ( byte* ) insert, ( byte* ) achange, stepFlag ) ;
        }
        if ( GetState ( _Context_->Lexer0, KNOWN_OBJECT ) )
        {
            if ( _DspReg_ > debugger->SaveDsp ) iPrintf ( "\nLiteral :> 0x%016lx <: was pushed onto the stack ...", TOS ) ;
            else if ( _DspReg_ < debugger->SaveDsp ) iPrintf ( "\n%s popped %d value off the stack.", insert, ( debugger->SaveDsp - _DspReg_ ) ) ;
            DefaultColors ;
        }
        //if ( ( ! ( achange [0] ) ) && ( ( change > 1 ) || ( change < - 1 ) || ( Verbosity () > 1 ) ) ) _Debugger_PrintDataStack ( change + 1 ) ;
        if ( ( ! achange [0] ) && ( change || ( Verbosity ( ) > 1 ) ) ) _Debugger_PrintDataStack ( change + 1 ) ;
    }
}

byte *
String_HighlightTokenInputLine ( byte * nvw, int64 lef, int64 leftBorder, int64 ts, byte * token, int64 rightBorder, int64 ref )
{
    int64 svState = GetState ( _Debugger_, DEBUG_SHTL_OFF ) ;
    SetState ( _Debugger_, DEBUG_SHTL_OFF, false ) ;
    // |ilw...------ inputLine  -----|lef|--- leftBorder ---|---token---|---  rightBorder  ---|ref|------ inputLine -----...ilw| -- ilw : inputLine window
    // |ilw...------ inputLine  -----|lef|pad?|-------------|tp|---token---|---  rightBorder  ---|ref|------ inputLine -----...ilw| -- ilw : inputLine window
    //_String_HighlightTokenInputLine ( byte * nvw, int8 lef, int64 leftBorder, int64 tokenStart, byte *token, int64 rightBorder, int8 ref )
    //cc_line = _String_HighlightTokenInputLine ( nvw, lef, leftBorder, ts, token, rightBorder, ref ) ; // nts : new token start is a index into b - the nwv buffer
    byte * cc_line = _String_HighlightTokenInputLine ( nvw, lef, leftBorder, ts, token, rightBorder, ref ) ; // nts : new token start is a index into b - the nwv buffer
    SetState ( _Debugger_, DEBUG_SHTL_OFF, svState ) ;
    return cc_line ;
}

byte *
PSCS_Using_WordSC ( byte* scs, byte * token, int64 index, int64 tvw ) // scs : source code string
{
    byte *nvw ;
    // ts : tokenStart ; tp : text point - where we want to start source code text to align with disassembly ; ref : right ellipsis flag
    int64 i, tw, slt, tp, lef, leftBorder, ts, rightBorder, ref, slsc, pad ;
    slsc = strlen ( ( char* ) scs ) ;
    nvw = Buffer_DataCleared ( _CSL_->DebugB3 ) ; //Buffer_New_pbyte ( ( slsc > BUFFER_SIZE ) ? slsc : BUFFER_SIZE ) ;
    slt = Strlen ( token ) ;
    tw = tvw ? tvw : TerminalLineWidth ( ) ;
    tp = GetState ( _CSL_, UDIS_ON ) ? 42 : 0 ; // 42 : aligned with disassembler code
    //strncpy ( nvw, il, BUFFER_IX_SIZE) ;
    if ( tvw && ( index > tvw ) )
    {
        //leftBorder = tvw / 2 ; //0 ; //ts = tp ;
        leftBorder = ts = tp ;
        lef = ref = rightBorder = 0 ;
        strncat ( nvw, & scs [index - tp], tvw ) ; // must Strncat because we might have done a strcat above based on the 'pad' variable
    }
    else if ( ( slsc > tp ) && ( index > tp ) )
    {
        lef = 4 ;
        leftBorder = ts = tp ;
        rightBorder = tw - ( ts + slt ) ;
        ref = ( slsc - 4 ) > tw ? 4 : 0 ;
        strncat ( nvw, & scs [index - tp], tw - ( lef + ref ) ) ;
    }
    else
    {
        pad = tp - index ;
        if ( pad >= 4 ) lef = 4 ;
        else lef = 0 ;
        for ( i = 0 ; i < pad ; i ++ ) strncat ( ( char* ) nvw, " ", BUFFER_IX_SIZE ) ;
        leftBorder = ts = tp ;
        ref = ( slsc - 4 ) > tw ? 4 : 0 ;
        if ( ( ! ref ) && ( tw > slsc - 4 ) ) ref = 4 ;
        rightBorder = tw - ( tp + slt ) - ref ;
        strncat ( nvw, scs, tw - ( lef + pad + ref ) ) ; // must Strncat because we might have done a strcat above based on the 'pad' variable
    }
    return String_HighlightTokenInputLine ( nvw, lef, leftBorder, ts, token, rightBorder, ref ) ;
}

byte *
PSCS_Using_ReadlinerInputString ( byte* il, byte * token1, int64 scswci, int64 tvw ) // scs : source code string ; tvw : text view sliding window 
{
    if ( il ) // there is at least one case where we need to test for this!
    {
        byte *nvw ;
        int64 tw = TerminalLineWidth ( ) ;
        tvw = tvw < tw ? tvw : tw ; 
        // ts : tokenStart ; tp : text point - where we want to start source code text to align with disassembly ; ref : right ellipsis flag
        int64 slt1, lef, leftBorder, ts, rightBorder, ref, slil ;
        int64 totalBorder, idealBorder, nws, nts, slNvw, lbm ;
        slil = strlen ( ( char* ) il ) ;
        nvw = Buffer_DataCleared ( _CSL_->DebugB4 ) ; //Buffer_New_pbyte ( ( slil > BUFFER_SIZE ) ? slil : BUFFER_SIZE ) ;
        slt1 = Strlen ( token1 ) ;
        totalBorder = ( tvw - slt1 ) ; // the borders allow us to slide token within the window of tvw
        idealBorder = ( totalBorder / 2 ) ;
        leftBorder = idealBorder ; // tentatively set leftBorder/rightBorder as ideally equal
        rightBorder = idealBorder ; // tentatively set leftBorder/rightBorder as ideally equal
        nws = scswci - idealBorder ;
        nts = idealBorder ;
        if ( nws < 0 )
        {
            nws = 0 ;
            nts = leftBorder = scswci ;
            rightBorder = totalBorder - leftBorder ;
        }
#if 0        
            //else if ( ( lbm = slil - ( scswci + slt1 + idealBorder ) ) > 0 )
        else if ( ( lbm = slil - ( scswci + slt1 + totalBorder ) ) > 0 )
        {
            nws = slil - tvw ;
            rightBorder = slil - ( scswci + slt1 ) ; // nb! : try to keep all on right beyond token - the cutoff should be on the left side
            if ( nws < 0 )
            {
                nws = 0 ;
                rightBorder += ( tvw - slil ) ;
            }
            leftBorder = totalBorder - rightBorder ;
            nts = leftBorder ; //+ (ins ? (slt/2 + 1) : 0 ) ;
        }
#endif        
        //else { use the defaults above }
        if ( GetState ( _Debugger_, ( DBG_OUTPUT_SUBSTITUTION ) ) )
        {
            Strncpy ( nvw, &il[nws], leftBorder ) ; //scswci ) ; // tvw ) ; // copy the the new view window to buffer nvw
            Strncat ( nvw, token1, slt1 ) ; // tvw ) ; // copy the the new view window to buffer nvw
            //Strncat ( nvw, &il[nws + slt0], tvw ) ; // tvw ) ; // copy the the new view window to buffer nvw
            Strncat ( nvw, &il[nws + leftBorder + slt1], rightBorder ) ; // - slt1 ) ; // tvw ) ; // copy the the new view window to buffer nvw
        }
        else Strncpy ( nvw, &il[nws], tvw ) ; // copy the the new view window to buffer nvw
        slNvw = Strlen ( nvw ) ;
        if ( slNvw > ( tvw + 8 ) ) // is there a need for ellipsis
        {
            if ( ( scswci - leftBorder ) < 4 ) lef = 0, ref = 1 ;
            else lef = ref = 1 ;
        }
        else if ( slNvw > ( tvw + 4 ) ) // is there a need for one ellipsis
        {
            if ( ( scswci - leftBorder ) < 4 ) lef = 0, ref = 1 ;
            else lef = 1, ref = 0 ; // choose lef as preferable
        }
        else lef = ref = 0 ;
        ts = nts ;
        return String_HighlightTokenInputLine ( nvw, lef, leftBorder, ts, token1, rightBorder, ref ) ;
    }
    else return "" ;
}
// ...source code source code TP source code source code ... EOL

byte *
DBG_PrepareShowInfoString ( Word * scWord, Word * word, byte* token0, byte* il, int tvw, int rlIndex, Boolean useScFlag ) // otoken : original token; il : instruction line ; tvw text view window
{
    // usingSC == 1 denotes il string is from word->W_OriginalCodeText else il is copied from rl->InputLineString
    Debugger * debugger = _Debugger_ ;
    byte * cc_line = ( byte* ) "", *scs = 0 ;
    if ( ! scWord )
    {
        if ( ( _LC_ && _LC_->Sc_Word ) && word && ( word->W_MySourceCodeWord == _LC_->Sc_Word ) ) scWord = _LC_->Sc_Word ;
            //if ( ! scWord ) 
        else scWord = Get_SourceCodeWord ( word ) ; //_CSL_->SC_Word ;
    }
    scs = scWord ? scWord->W_OriginalCodeText : 0 ;
    if ( ( ! scs ) && useScFlag )
    {
        scWord = Get_SourceCodeWord ( word ) ;
        scs = scWord ? scWord->W_OriginalCodeText : 0 ;
    }
    // if no scs we use il : by design : it works!
    if ( word || token0 )
    {
        byte *token = ( word ? word->Name : token0 ), * token1, *token2 ;
        int64 slt, index, index0 ;
        if ( GetState ( debugger, DBG_OUTPUT_INSERTION | DBG_OUTPUT_SUBSTITUTION ) )
        {
            token1 = debugger->SubstitutedWord ? debugger->SubstitutedWord->Name : token0 ? token0 : word->Name ;
        }
        else token1 = token0 ;
        token2 = String_ConvertToBackSlash ( token1, 0 ) ;
        slt = Strlen ( token2 ) ;
        index0 = scs && word ? word->W_SC_Index : rlIndex ;
        if ( debugger->w_AliasOf )
        {
            token2 = word->Name ;
            slt = Strlen ( word->Name ) ;
            index = String_FindStrnCmpIndex ( scs ? scs : il, word->Name, index0, slt, slt ) ;
        }
        else index = String_FindStrnCmpIndex ( scs ? scs : il, token2, index0, slt, slt ) ;
        if ( index != - 1 ) // did we find token in scs
        {
            // these two functions maybe should be integrated ; the whole series of 'ShowInfo' functions could be reworked
            if ( scs ) cc_line = PSCS_Using_WordSC ( scs, token2, index, tvw ) ; // scs : source code string
                //else cc_line = PSCS_Using_ReadlinerInputString ( il, token2, token, scswci, tvw ) ; 
            else cc_line = PSCS_Using_ReadlinerInputString ( il, token2, index, tvw ) ;
        }
        else cc_line = ( byte* ) "" ;
    }
    return cc_line ;
}


// NB!! : remember the highlighting formatting characters don't add any additional *visible length* to the output line
// ots : original token start (index into the source code), nws : new window start ; tvw: targetViewWidth ; nts : new token start
// lef : left ellipsis flag, ref : right ellipsis flag

byte *
CSL_PrepareDbgShowInfoString ( Word * word, byte* token, int64 twAlreayUsed )
{
    byte * cc_line = ( byte* ) "" ;
    if ( word || token )
    {
        ReadLiner * rl = _Context_->ReadLiner0 ;
        byte * il = Buffer_DataCleared ( _CSL_->StringInsertB6 ) ; //nb! dont tamper with the input line. eg. removing its newline will affect other code which depends on newline
        strncpy ( il, String_ConvertToBackSlash ( rl->InputLineString, 1 ), BUFFER_IX_SIZE ) ;
        String_RemoveEndWhitespace ( ( byte* ) il ) ;
        int64 tw0 = TerminalLineWidth ( ) ;
        int64 fel, tvw, tw = ( tw0 > 0 ) ? tw0 : 140 ; // GetTerminalWidth ( )  is not very accurate, sometimes even returning 0
        fel = 31 ; //fe : formating estimated length : 2 formats with 8/12 chars on each sude - 32/48 ::  -1 : a litte leave way
        //tw = Debugger_TerminalLineWidth ( debugger ) ; // 139 ; //139 : nice width :: Debugger_TerminalLineWidth ( debugger ) ; 
        tvw = tw - ( ( twAlreayUsed > fel ) ? ( twAlreayUsed - fel ) : fel ) ; //subtract the formatting chars which don't add to visible length
        cc_line = DBG_PrepareShowInfoString ( 0, word, token, il, tvw, word ? word->W_RL_Index : _Lexer_->TokenStart_ReadLineIndex, 0 ) ; //tvw, tvw/2 ) ;// sc : source code ; scwi : source code word index
        //cc_line = DBG_PrepareShowInfoString ( word, word, token, il, tvw, word ? word->W_RL_Index : _Lexer_->TokenStart_ReadLineIndex, 0 ) ; //tvw, tvw/2 ) ;// sc : source code ; scwi : source code word index
    }
    return cc_line ;
}

void
CSL_Show_ErrorCommandLine ( )
{
    Exception *e = _O_->OVT_Exception ;
    if ( ! GetState ( e, EXCEPTION_ERROR_COMMAND_LINE ) )
    {
        SetState ( e, EXCEPTION_ERROR_COMMAND_LINE, true ) ;
        ReadLiner * rl = _ReadLiner_ ;
        if ( AtCommandLine ( rl ) )
        {
            byte * buffer = Buffer_DataCleared ( _CSL_->ScratchB5 ), *b ;
            strncpy ( buffer, _ReadLiner_->InputLineString, BUFFER_SIZE ) ;
            b = String_RemoveFinalNewline ( buffer ) ;
            e->ErrorCommandLine = String_New ( b, EXCEPTION_SPACE) ;
            iPrintf ( "\nError Command Line : <? \'%s\' ?>", e->ErrorCommandLine ) ;
        }
    }
}

void
CSL_Show_SourceCode_TokenLine ( Word * word, byte * prompt, int64 signal, byte * token0, byte * signalAscii )
{
    char * mode ;
    if ( GetState ( _Context_, TDI_PARSING ) ) mode = "[td]" ;
    else mode = ( char* ) ( ( signal || ( int64 ) signalAscii[0] ) ?
        ( CompileMode ? "\n[c] " : "\n[i] " ) : ( CompileMode ? "[c] " : "[i] " ) ) ;
    byte * buffer = Buffer_DataCleared ( _CSL_->ScratchB2 ) ;
    byte * obuffer = Buffer_DataCleared ( _CSL_->DebugB1 ) ;
    byte * token1 = String_ConvertToBackSlash ( token0, 0 ) ;
    char * cc_Token = ( char* ) cc ( token1, &_O_->Notice ) ;
    char * cc_location = ( char* ) cc ( Context_Location ( ), &_O_->Debug ) ;

    prompt = prompt ? prompt : ( byte* ) "" ;
    strncpy ( buffer, prompt, BUFFER_IX_SIZE ) ;
    strncat ( buffer, mode, 32 ) ;
    prompt = ( byte* ) buffer ;
    if ( word )
    {
        if ( word->W_MorphismAttributes & CPRIMITIVE )
        {
            snprintf ( ( char* ) obuffer, BUFFER_IX_SIZE, "\n%s%s:: %s : %s :> %s <: cprimitive :> ", // <:: " INT_FRMT "." INT_FRMT " ",
                prompt, signal ? ( char* ) signalAscii : " ", cc_location,
                word->ContainingNamespace ? ( char* ) word->ContainingNamespace->Name : "<literal>",
                cc_Token ) ;
        }
        else
        {
            snprintf ( ( char* ) obuffer, BUFFER_IX_SIZE, "\n%s%s:: %s : %s :> %s <: 0x%016lx :> ", // <:: " INT_FRMT "." INT_FRMT " ",
                prompt, signal ? ( char* ) signalAscii : " ", cc_location,
                word->ContainingNamespace ? ( char* ) word->ContainingNamespace->Name : ( char* ) "<literal>",
                ( char* ) cc_Token, ( uint64 ) word ) ;
        }
    }
    else
    {
        snprintf ( ( char* ) obuffer, BUFFER_IX_SIZE, "\n%s%s:: %s : %s :> %s <::> ", // <:: " INT_FRMT "." INT_FRMT " ",
            prompt, signal ? signalAscii : ( byte* ) " ", cc_location,
            "<literal>", cc_Token ) ; //, _O_->StartedTimes, _O_->SignalExceptionsHandled ) ;
    }
    //int columns = atoi(getenv("COLUMNS"));
    CSL_Show_ErrorCommandLine ( ) ;
    byte *cc_line = ( char* ) CSL_PrepareDbgShowInfoString ( word, token1, ( int64 ) Strlen ( obuffer ) ) ;
    if ( cc_line ) strncat ( obuffer, cc_line, BUFFER_IX_SIZE ) ;
    _Printf ( "%s", obuffer ) ;
}

void
LO_Debug_ExtraShow ( int64 showStackFlag, int64 verbosity, int64 wordList, byte *format, ... )
{
    if ( GetState ( _CSL_, DEBUG_MODE ) )
    {
        if ( Verbosity ( ) > verbosity )
        {
            va_list args ;
            va_start ( args, format ) ;
            char * out = ( char* ) Buffer_DataCleared ( _CSL_->DebugB ) ;
            vsprintf ( ( char* ) out, format, args ) ;
            va_end ( args ) ;
            DebugColors ;
            if ( wordList ) _CSL_SC_WordList_Show ( ( byte* ) out, 0, 0 ) ;
            else _Printf ( "%s", out ) ;
            if ( showStackFlag && Verbosity ( ) > verbosity ) Stack ( ) ;
            DefaultColors ;
        }
    }
}

