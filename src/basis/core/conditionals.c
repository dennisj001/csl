#include "../../include/csl.h"

Boolean
Match_MapFunction ( dlnode * node, uint64 switchValue )
{
    CaseNode * cnode = ( CaseNode* ) node ;
    if ( IsString ( ( byte* ) switchValue ) && IsString ( ( byte* ) cnode->CN_CaseBytePtrValue )
        && String_Equal ( cnode->CN_CaseBytePtrValue, ( byte* ) switchValue ) )
    {
        ( ( block ) ( cnode->CN_CaseBlock ) ) ( ) ;
        return true ;
    }
    else if ( switchValue == cnode->CN_CaseUint64Value )
    {
        ( ( block ) ( cnode->CN_CaseBlock ) ) ( ) ;
        return true ;
    }
    else return false ;
}

void
MatchAccessFunction ( )
{
    dllist_Map1_Break ( ( dllist* ) TOS, ( MapFunction1 ) Match_MapFunction, _DspReg_ [ - 1 ] ) ;
    DataStack_DropN ( 2 ) ;
}

CaseNode *
_CaseNode_New ( uint64 type, block block, int64 value )
{
    CaseNode * cnode = ( CaseNode* ) Mem_Allocate ( sizeof ( CaseNode ), type ) ;
    cnode->CN_CaseBlock = ( uint64 ) block ;
    cnode->CN_CaseUint64Value = value ; //nb. CN_CaseUint64Value is member of a union 
    return cnode ;
}

void
_CSL_Switch ( uint64 allocType )
{
    if ( ! _Compiler_->CurrentMatchList ) _Compiler_->CurrentMatchList = _dllist_New ( allocType ) ;
    _Do_LiteralValue ( ( int64 ) _Compiler_->CurrentMatchList ) ;
    Compile_Call_TestRSP ( ( byte* ) MatchAccessFunction ) ;
    _Compiler_->CurrentMatchList = 0 ; // this allows no further "case"s to be added to this "switch" list a new list will be started with the next "case"
}

void
_CS_Case ( uint64 allocType )
{
    Interpreter * interp = _Interpreter_ ;
    int64 caseValue = 0 ;
    byte * token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ) ;
    Word * word = _Interpreter_TokenToWord ( interp, token, - 1, - 1 ) ;
    word = CSL_Parse_Interpret_KeywordOperand ( word, 1 ) ;
    SetState ( _Compiler_, DOING_CASE, true ) ;
    caseValue = word->S_Value ;
    CSL_Interpret_C_Blocks (1, 0, 0 , 0) ; // mabye we need another function - CSL_Interpret_Blocks
    block caseBlock = ( block ) DataStack_Pop ( ) ;
    CaseNode * cnode = _CaseNode_New ( allocType, caseBlock, caseValue ) ;
    if ( ! _Compiler_->CurrentMatchList ) _Compiler_->CurrentMatchList = _dllist_New ( allocType ) ;
    dllist_AddNodeToTail ( _Compiler_->CurrentMatchList, ( dlnode* ) cnode ) ;
    SetState ( _Compiler_, DOING_CASE, false ) ;
}

void
_CS_Match ( uint64 allocType )
{
    Word * word ;
    byte * token ;
    token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ) ;
    word = _Interpreter_TokenToWord ( _Interpreter_, token, - 1, - 1 ) ;
    CSL_Parse_Interpret_KeywordOperand ( word, 1 ) ;
    if ( GetState ( _Context_, C_SYNTAX ) ) CSL_Interpret_C_Blocks (1, 0, 0 , 0) ;
    if ( ! _Compiler_->CurrentMatchList ) _Compiler_->CurrentMatchList = _dllist_New ( allocType ) ;
    _Do_LiteralValue ( ( int64 ) _Compiler_->CurrentMatchList ) ;
    Compile_Call_TestRSP ( ( byte* ) MatchAccessFunction ) ;
    if ( GetState ( _Context_, C_SYNTAX ) ) DataStack_DropN ( 1 ) ; // drop the block
    _Compiler_->CurrentMatchList = 0 ; // this allows no further "case"s to be added to this "switch" list a new list will be started with the next "case"
}

void
CSL_Switch ( )
{
    _CSL_Switch ( DICTIONARY ) ;
}

void
CS_Case ( )
{
    _CS_Case ( DICTIONARY ) ;
}

void
CS_Match ( )
{
    _CS_Match ( DICTIONARY ) ;
}

#if 0

void
_CSL_Case ( uint64 allocType )
{
    block caseBlock = ( block ) DataStack_Pop ( ) ;
    int64 caseValue = ( uint64 ) String_New ( ( byte* ) DataStack_Pop ( ), STRING_MEM ) ;
    if ( ! _Compiler_->CurrentMatchList ) _Compiler_->CurrentMatchList = _dllist_New ( allocType ) ;
    CaseNode * cnode = _CaseNode_New ( allocType, caseBlock, caseValue ) ;
    dllist_AddNodeToTail ( _Compiler_->CurrentMatchList, ( dlnode* ) cnode ) ;
}

void
CSL_Case ( )
{
    _CSL_Case ( DICTIONARY ) ;
}
#endif




