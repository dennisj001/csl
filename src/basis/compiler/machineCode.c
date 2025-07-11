#include "../../include/csl.h"

// Intel notes - cf. InstructionSet-A-M-253666.pdf - section 2.1; InstructionSet-N-N_0-253667.pdf section B.1/B.2 :
// Intel instructions (insn) can only operate on one memory operand per instruction, i.e. one cannot move mem to mem in one insn, you must move mem to reg and reg to mem
// the direction is either mem to reg or reg to mem determined in some instructions by a direction bit in the opCode
// b prefix = binary code
// -----------------------------------------------------------------------
// instuction format ( in bytes )
// prefixes  opcode  modRm     sib     disp   immediate
//  0 - 4    1 - 3   0 - 1    0 - 1    0,1,4    0,1,2,4      -- number of bytes
// optional          ------------optional--------------
//   REX Prefix 
//   0x40 = 0100 + WRSB  W = 64 bit operand flag, R = reg ext. flag bit, S = sib ext, B = r/m reg ext flag bit
// -----------------------------------------------------------------------
//   modRm byte ( bits ) :: mod 0 : no disp ;; mod 1 : 1 byte disp : mod 2 : 4 byte disp ;; mod 3 : reg value :: sections 2.1.3/2.1.5, Table 2-2
//   the mod field is a semantic function on the r/m field determining its meaning either as the reg value itself or the value at the [reg] as an addr + offset
//   Intel InstructionSet-N-N_0-253667.pdf section 2.1.5
//    mod     reg     r/m  
//   7 - 6   5 - 3   2 - 0 
//    0-3              4 - b100 => sib, instead of reg ESP   : mod bit determines size of displacement 
// bit fields :
//  mod reg r/m 
//   00 000 000
// *if insn has a mod/rm byte then ::
// #define RM( insnAddr )  (*( (byte*) insnAddr + 1) & 7 )   // binary : 00000111
// #define REG( insnAddr ) (*( (byte*) insnAddr + 1) & 56 )  // binary : 00111000 
// #define MOD( insnAddr ) (*( (byte*) insnAddr + 1) & 192 ) // binary : 11000000 
// -----------------------------------------------------------------------
//  reg/rm codes :
//  EAX 0, ECX 1, EDX 2, EBX 3, ESP 4, EBP 5, ESI 6, EDI 7
// -----------------------------------------------------------------------
//  bit positions encoding :  ...|7|6|5|4|3|2|1|0|  but nb. intel is little endian
// -----------------------------------------------------------------------
//  opCode direction bit 'd' is bit position 1 : 1 => rm/sib to reg ; 0 => reg to rm/sib -- for some instructions
//  sign extend bit 's' is bit position 1 for some instructions
//  operand size bit 'w' is bit position 0 for some instructions
// -----------------------------------------------------------------------
//       sib byte ( bits ) with rm 4 - b100 - ESP
//    scale  index   base
//    7 - 6  5 - 3  2 - 0
//    scale 0 : [index * 1 + base]
//    scale 1 : [index * 2 + base]
//    scale 2 : [index * 4 + base]
//    scale 1 : [index * 4 + base]
// -----------------------------------------------------------------------
// intel syntax : opcode dst, src
// att syntax   : opcode src, dst

// note : x86-32 instruction format : || prefixes : 0-4 bytes | opCode : 1-3 bytes | mod : 0 - 1 byte | sib : 0 - 1 byte | disp : 0-4 bytes | immediate : 0-4 bytes ||
// note : intex syntax  : instruction dst, src - csl uses this order convention
//        att   syntax  : instruction src, dst
// note : rm : reg memory - the register which contains the memory address in mod instructions

// csl uses intel syntax convention

// ----------------------------------
// | intel addressing ideas summary |
// ----------------------------------
// remember : the intel cpus can not reference two memory operands in one instruction so
// the modr/m byte selects with the mod and rm field an operand to use with the reg field value (generally)
// the mod field ( 2 bits ) contols whether the r/m field reg refers to a direct reg or indirect + disp values (disp values are in the displacement field)
// mod 0 is for register indirect -- no displacement the register is interpreted as an address; it refers to a value in a memory address with no disp
// mod 1 is for register indirect -- 8 bit disp the register is interpreted as an address; it refers to a value in a memory address with 8 bit disp
// mod 2 is for register indirect -- 32 bit disp the register is interpreted as an address; it refers to a value in a memory address with 32 bit disp
// mod 3 is for register direct   -- using the direct register value -- not as an address 
// the reg field of the modr/m byte generally refers to to register to use with the mod modified r/m field -- intel can't address two memory fields in any instruction
// --------------------------------------

// controlFlags : bits ::  4      3       2     1      0         
//                       rex    imm    disp   sib    modRm      
//                      rex=16 imm=8, disp=4 sib=2  mod/Rm=1
//                      REX_B  IMM_B  DISP_B SIB_B   MODRM_B

void
_Compile_Write_Instruction_X64 ( Boolean rex, uint8 opCode0, uint8 opCode1, Boolean modRm, int16 controlFlags, Boolean sib,
    int64 disp, Boolean dispSize, int64 imm, Boolean immSize )
{
    d1 ( byte * here = Here ) ;
    if ( rex ) _Compile_Int8 ( rex ) ;
    if ( opCode0 ) _Compile_Int8 ( ( byte ) opCode0 ) ;
    if ( opCode1 ) _Compile_Int8 ( ( byte ) opCode1 ) ;
    if ( ( controlFlags & MODRM_B ) ) _Compile_Int8 ( modRm ) ;
    if ( sib && ( controlFlags & SIB_B ) ) _Compile_Int8 ( sib ) ;
    if ( disp || ( controlFlags & DISP_B ) ) _Compile_ImmDispData ( disp, dispSize, 0 ) ;
    if ( imm || ( controlFlags & IMM_B ) ) _Compile_ImmDispData ( imm, immSize, ( controlFlags & IMM_B ) ) ;
    if ( _DBI || ( _O_->Dbi > 1 ) )
    {
        //d1 ( Debugger_UdisOneInstruction ( _Debugger_, 0, here, ( byte* ) "", ( byte* ) "" ) ; ) ;
        d1 ( _Debugger_Disassemble ( _Debugger_, 0, ( byte* ) here, Here - here, 1 ) ) ;
    }
}

int64
OperandSize ( int64 operand )
{
    int64 size ;
    if ( operand <= 0xff ) size = 1 ;
    else if ( operand <= 0xffff ) size = 2 ;
    else if ( operand <= 0xffffffff ) size = 4 ;
    else size = 8 ;
    return size ;
}
Boolean
LispRegParameterOrder ( Boolean n )
{
    Boolean regOrder [] = REG_ORDER ; //REG_ORDER ; //{ RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } ;
    //Boolean regOrder [] = { RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } ;
    //Boolean regOrder [] = { RCX, RDX, RDI, RSI, R8D, R9D, R10D, R11D } ;
    return regOrder [n] ;
}

// no difference at this point 
Boolean
LocalsRegParameterOrder_Init ( Boolean n )
{
    Boolean regOrder [] = CSL_REG_ORDER ; //REG_ORDER ; //{ RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } ;
    //Boolean regOrder [] = { RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } ;
    //Boolean regOrder [] = { RCX, RDX, RDI, RSI, R8D, R9D, R10D, R11D } ;
    return regOrder [n] ;
}

Boolean
LocalsRegParameterOrder_Optimized ( Boolean n )
{
    Boolean regOrder [] = CSL_REG_ORDER ; //{ RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } ;
    //Boolean regOrder [] = { RCX, RDX, RDI, RSI, R8D, R9D, R10D, R11D } ;
    return regOrder [n] ;
}

// some checks of the internal consistency of the instruction bits

int64
CalculateSib ( int64 scale, int64 indexReg, int64 baseReg )
{
    //  scale index base bits  : scale 1 = *2, 2 = *4, 3 = *8 ; index and base refer to registers
    //  00    000   000
    //if (( indexReg > 0x7 ) || ( baseReg > 0x7 ) ) Exception ( MACHINE_CODE_ERROR, ABORT ) ;
    Boolean sib = ( scale << 6 ) | ( ( indexReg & 0x7 ) << 3 ) | ( baseReg & 0x7 ) ;
    return sib ;
}

// instruction letter codes : I - immediate data ; 32 : 32 bit , 8 : 8 bit ; EAX, DSP : registers
// we could have a mod of 0 so the modRmImmDispFlag is necessary
// operandSize : specific size of immediate data - BYTE or WORD
// SIB : scale, index, base addressing byte

void
_Compile_ImmDispData ( int64 immDisp, Boolean size, Boolean forceFlag )
{
    // the opcode probably is all that needs to be adjusted for this to not be necessary    
    // to not compile an imm when imm is a parameter, set isize == 0 and imm == 0
    if ( size > 0 )
    {
        if ( size == BYTE )
            _Compile_Int8 ( ( byte ) immDisp ) ;
        else if ( size == 2 )
            _Compile_Int16 ( immDisp ) ;
        else if ( size == 4 )
            _Compile_Int32 ( immDisp ) ;
        else if ( size == CELL_SIZE )
            _Compile_Cell ( immDisp ) ;
    }
    else // with operandSize == 0 let the compiler use the minimal size ; nb. can't be imm == 0
    {
        if ( labs ( immDisp ) >= 0x7fffffff )
            _Compile_Int64 ( immDisp ) ;
        else if ( labs ( immDisp ) >= 0x7f )
            _Compile_Int32 ( immDisp ) ;
        else if ( immDisp || forceFlag )
            _Compile_Int8 ( ( byte ) immDisp ) ;
    }
}

// Intel - InstructionSet-A-M-253666.pdf - section 2.1 :
//-----------------------------------------------------------------------
// instuction format ( number of bytes )
// prefixes  opcode  modRm   sib       disp    immediate
//  0 - 4    1 - 3   0 - 1  0 - 1    0,1,2,4    0,1,2,4      -- number of bytes
//-----------------------------------------------------------------------
//   modRm byte ( bits )  mod 0 : no disp ; mod 1 : 1 byte disp : mod 2 : 4 byte disp ; mod 3 : just reg value
//    mod     reg      rm
//   7 - 6   5 - 3   2 - 0 
//-----------------------------------------------------------------------
//  reg/rm values :
//  EAX 0, ECX 1, EDX 2, ECX 3, ESP 4, EBP 5, ESI 6, EDI 7
//-----------------------------------------------------------------------
//       sib byte ( bits )
//    scale  index   base
//    7 - 6  5 - 3  2 - 0
//-----------------------------------------------------------------------

uint8
Calculate_Rex ( Boolean reg, Boolean rm, Boolean rex_w_flag, Boolean rex_b_flag )
{
    Boolean rex = ( ( rex_w_flag ? 8 : 0 ) | ( ( reg > 7 ) ? 4 : 0 ) | ( ( rm > 7 ) ? 1 : 0 ) ) ;
    if ( rex || rex_b_flag || rex_w_flag ) rex |= 0x40 ;
    return rex ;
}

Boolean
CalculateModRegardingDisplacement ( Boolean mod, int64 disp )
{
    // bits :
    //  mod reg r/m 
    //   00 000 000
    if ( mod != REG )
    {
        if ( disp == 0 ) mod = 0 ;
        else if ( disp <= 0x7f ) mod = 1 ; // displacement can be either plus or minus 0x7f ( 127 )
        else mod = 2 ;
    }
    return mod ;
}

//-----------------------------------------------------------------------
//   modRm byte ( bits )  mod 0 : no disp ; mod 1 : 1 byte disp : mod 2 : 4 byte disp ; mod 3 : just reg value
//    mod     reg      rm
//   7 - 6   5 - 3   2 - 0
//-----------------------------------------------------------------------
//  reg/rm values :
//  EAX 0, ECX 1, EDX 2, ECX 3, ESP 4, EBP 5, ESI 6, EDI 7
//-----------------------------------------------------------------------

uint8
CalculateModRmByte ( Boolean mod, Boolean reg, Boolean rm, Boolean sib, int64 disp )
{
    uint8 modRm ;
    mod = CalculateModRegardingDisplacement ( mod, disp ) ;
    if ( ( mod < 3 ) && ( rm == 4 ) ) //|| ( ( rm == 5 ) && ( disp == 0 ) ) ) )
        //if ( ( mod < 3 ) && ( ( ( rm == 4 ) && ( sib == 0 ) ) || ( ( rm == 5 ) && ( disp == 0 ) ) ) )
    {
        // cf. InstructionSet-A-M-253666.pdf Table 2-2
        //CSL_Exception ( MACHINE_CODE_ERROR, 0, 1 ) ;
        Error ( "\nCalculateModRmByte : MACHINE_CODE_ERROR", QUIT ) ;
    }
    if ( sib )
    {
        rm = 4 ; // from intel mod tables
        reg = 0 ;
    }
    modRm = ( mod << 6 ) + ( ( reg & 0x7 ) << 3 ) + ( rm & 0x7 ) ; // only use 3 bits of reg/rm
    //modRm = ( mod << 6 ) + ( ( reg ) << 3 ) + ( rm ) ; // only use 3 bits of reg/rm
    return modRm ;
}

byte
Calculate_Mod ( int disp )
{
    byte mod ;
    if ( disp == 0 ) mod = 0 ;
    else if ( abs ( disp ) <= 0x7f ) mod = 1 ;
    else if ( abs ( disp ) >= 0x100 ) mod = 2 ;
    return mod ;
}

void
Compile_CalculateWrite_Instruction_X64 ( uint8 opCode0, uint8 opCode1, Boolean mod, uint8 reg, Boolean rm,
    uint16 controlFlags, Boolean sib, int64 disp, uint8 dispSize, int64 imm, uint8 immSize )
{
    Boolean rex = Calculate_Rex ( reg, rm, ( immSize == 8 ) || ( controlFlags & REX_W ), ( controlFlags & REX_B ) ) ;
    //Boolean rex = Calculate_Rex (reg, rm, ( immSize == 8 ), ( controlFlags & REX_B ) ) ;
    uint8 modRm = CalculateModRmByte ( mod, reg, rm, sib, disp ) ;
    _Compile_Write_Instruction_X64 ( rex, opCode0, opCode1, modRm, controlFlags, sib, disp, dispSize, imm, immSize ) ;
}

// intel syntax : opcode dst, src
// mov reg to mem or mem to reg
// note the order of the operands match intel syntax with dst always before src

// the basic move instruction
// mov reg to mem or mem to reg 
// note this function uses the bit order of operands in the mod byte : (mod) reg r/m
// the 'rmReg' parameter must always refer to a memory location; 'reg' refers to the register, either to or from which we move mem

// controlFlags : bits ::  3       2     1      0         D D - - I D S M
//                       imm    disp   sib    modRm      mod               (mod) only in move insn
//                     imm=8, disp=4 sib=2  mod/Rm=1

// size refers to size of the operand or immediate
// mod :: TO_REG == 3 : TO_MEM == 0,1,2 depending on disp size ; 0 == no disp ; 1 == 8 bit disp ; 2 == 32 bit disp

void
Compile_Move ( uint8 direction, uint8 mod, uint8 reg, uint8 rm, uint8 operandSize,
    uint8 sib, int64 disp, uint8 dispSize, int64 imm, uint8 immSize )
{
    uint8 opCode0 = 0, opCode ; //, svOpSize = operandSize ;
    uint16 controlFlags = ( disp ? DISP_B : 0 ) | ( sib ? SIB_B : 0 ) ;
    if ( ! operandSize ) operandSize = 8 ;
    if ( ! mod ) mod = Calculate_Mod ( disp ) ;
    if ( imm || immSize )
    {
        //DBI_ON ;
        controlFlags |= ( IMM_B ) ;
        reg = 0 ; // the rm is the destination for all move immediate
        if ( immSize >= 8 )
        {
            if ( imm && ( imm < ( int64 ) 0xffffffff ) && ( rm < 8 ) )
            {
                byte modRm ;
                immSize = 4 ;
                if ( direction == TO_REG )
                {
                    opCode = 0xb8 | rm ; //0xc7 ;
                    modRm = 0 ; //0xc0 | rm ;//, controlFlags = ( MODRM_B ) ;
                }
                else if ( direction == TO_MEM )
                {
                    opCode = 0xc7 ; //0xc7 ;
                    controlFlags |= ( MODRM_B ) ;
                    modRm = ( mod << 6 ) | rm ; //, controlFlags = ( MODRM_B ) ;
                }
                _Compile_Write_Instruction_X64 ( 0, 0, opCode, modRm, controlFlags, 0, 0, 0, imm, immSize ) ;
                //DBI_OFF ;
                return ;
            }
            controlFlags |= ( IMM_B | REX_W ) ;
            if ( imm < ( int64 ) 0xffffffff ) // sign extend 32 bit to 64 bit -> smaller faster insn
            {
                //DBI_ON ;
                opCode = 0xc7 ;
                immSize = 4 ;
                controlFlags |= ( MODRM_B ) ;
            }
            else
            {
                if ( direction == TO_MEM ) Error ( "\nCompile_Move : MACHINE_CODE_ERROR", QUIT ) ; //CSL_Exception ( MACHINE_CODE_ERROR, 0, 1 ) ;
                opCode = 0xb8 ;
                opCode += ( rm & 7 ) ;
            }
        }
        else
        {
            //DBI_ON ;
            if ( rm > 7 ) immSize = 4 ;
            if ( immSize == 1 ) opCode = 0xb0 + rm ;
            else if ( immSize == 2 ) opCode0 = 0x66, opCode = 0xb8 | rm ;
            else if ( immSize == 4 ) opCode = 0xb8 | rm ;
        }
    }
    else // non immediate
    {
        if ( operandSize < 4 )
        {
            if ( operandSize == 1 ) _Compile_MOVZX_BYTE_REG ( reg, reg ) ;
            else if ( operandSize == 2 ) _Compile_MOVZX_WORD_REG ( reg, reg ) ;
            operandSize = 8 ; // for the next insn
        }
        controlFlags |= ( MODRM_B ) ;
        opCode = 0x88 ;
        if ( operandSize > 1 ) opCode |= 1 ; //nb. if using rex then we cannot use AH, BH, CH, DH per Intel instruction manual
        if ( direction == TO_REG ) opCode |= 2 ; // 0x8b ; // 0x89 |= 2 ; // d : direction bit = 'bit 1' : 0 == dest is mem ; 1 == dest is reg
        if ( operandSize == 2 ) opCode0 = 0x66 ; // choose 16 bit operand
        if ( ( operandSize >= 8 ) || ( immSize >= 8 ) ) controlFlags |= ( REX_W ) ;
    }
    Compile_CalculateWrite_Instruction_X64 ( opCode0, opCode, mod, reg, rm, controlFlags, sib, disp, dispSize, imm, immSize ) ;
    //DBI_OFF ;
}

void
Compile_Move_WithSib ( uint8 direction, Boolean mod, Boolean reg, Boolean rm, Boolean scale, Boolean index, Boolean base )
{

    Compile_Move ( direction, mod, reg, rm, 8, CalculateSib ( scale, index, base ), 0, 0, 0, 0 ) ;
}

void
Compile_Move_Reg_To_Rm ( Boolean rm, Boolean reg, int64 disp, byte size )
{

    Compile_Move ( MEM, 0, reg, rm, size, 0, disp, 0, 0, 0 ) ;
}

// intel syntax : opcode dst, src
// mov reg to mem or mem to reg
// note the order of the operands match intel syntax with dst always before src

void
Compile_Move_Rm_To_Reg ( Boolean rm, Boolean reg, int64 disp, byte size )
{

    Compile_Move ( REG, 0, rm, reg, size, 0, disp, 0, 0, 0 ) ;
}
// intel syntax : opcode dst, src
// mov reg to mem or mem to reg
// note the order of the operands match intel syntax with dst always before src

void
Compile_Move_Reg_To_Reg ( Boolean dstReg, int64 srcReg, byte size )
{

    if ( dstReg != srcReg ) Compile_Move ( REG, REG, dstReg, srcReg, size, 0, 0, 0, 0, 0 ) ; //size ) ; // nb! mod == REG in move reg to reg
}

void
Compile_MoveImm ( Boolean mod, Boolean rm, int64 disp, int64 imm, byte immSize )
{
    if ( ! immSize ) immSize = 8 ;
    if ( mod == MEM )
    {
        // this is a special case whereas it should be integrated into Move    
        //if ( disp && ( imm <= 0xffffffff ) )
        if ( ( immSize < 8 ) || ( imm <= 0xffffffff ) )
        {
            //DBI_ON ;
            Compile_Move ( TO_MEM, mod, 0, rm, 0, 0, disp, 0, imm, immSize ) ;
            //DBI_OFF ;
        }
        else if ( ( immSize >= 8 ) || ( imm > 0xffffffff ) )
        {
            // there is no x64 instruction to move imm64 to mem directly
            Compile_Move ( TO_REG, REG, 0, THRU_REG, 0, 0, 0, 0, imm, immSize ) ;
            Compile_Move_Reg_To_Rm ( rm, THRU_REG, disp, immSize ) ;
        }
    }

    else Compile_Move ( TO_REG, mod, 0, rm, 0, 0, disp, 0, imm, immSize ) ; //Operand_Size ( imm ) ) ; // nb. reg == 0 in a move immediate
}

void
Compile_MoveImm_To_Reg ( Boolean rm, int64 imm, byte immSize )
{

    Compile_MoveImm ( REG, rm, 0, imm, immSize ) ;
}

void
Compile_MoveImm_To_Mem ( Boolean rm, int64 imm, byte immSize )
{

    Compile_MoveImm ( MEM, rm, 0, imm, immSize ) ;
}

void
Compile_MoveMemValue_To_Reg ( Boolean reg, byte * address, byte iSize )
{

    Compile_MoveImm_To_Reg ( reg, ( int64 ) address, iSize ) ;
    Compile_Move_Rm_To_Reg ( reg, reg, 0, iSize ) ;
}

void
Compile_MoveMemValue_ToReg_ThruReg ( Boolean reg, byte * address, Boolean iSize, Boolean thruReg )
{

    Compile_MoveImm_To_Reg ( thruReg, ( int64 ) address, iSize ) ;
    Compile_Move_Rm_To_Reg ( reg, thruReg, 0, iSize ) ;
}

void
Compile_MoveReg_ToAddress_ThruReg ( Boolean reg, byte * address, Boolean thruReg )
{

    Compile_MoveImm_To_Reg ( thruReg, ( int64 ) address, CELL_SIZE ) ;
    Compile_Move_Reg_To_Rm ( thruReg, reg, 0, 0 ) ;
}

// set the value at address to reg - value in reg

void
_Compile_SetAtAddress_WithReg ( int64 * address, int64 reg, int64 thruReg )
{

    _Compile_Move_Literal_Immediate_To_Reg ( thruReg, ( int64 ) address, 0 ) ;
    Compile_Move_Reg_To_Rm ( thruReg, reg, 0, 0 ) ;
}

void
_Compile_Move_Literal_Immediate_To_Reg ( int64 reg, int64 value, int size )
{

    Compile_MoveImm_To_Reg ( reg, value, size ) ;
}

// opCode group 1 - 0x80-0x83 : ADD OR ADC SBB AND SUB XOR CMP : but not with immediate data
// s and w bits of the x86 opCode : w seems to refer to word and is still used probably for historic and traditional reasons
// note : the opReg - operand register parameter is always used for the rm field of the resulting machine code
// These are all operating on a memory operand
// for use of immediate data with this group use _Compile_Group1_Immediate

void
_Compile_X_Group1 ( Boolean code, Boolean toRegOrMem, Boolean mod, Boolean reg, Boolean rm, Boolean sib, int64 disp, Boolean operandSize )
{
    //if ( code == CMP ) SetCompilerField ( CurrentTopBlockInfo, CmpCode, Here ) ;
    int64 opCode = code << 3, controlFlags = DISP_B | MODRM_B ;
    if ( operandSize > BYTE ) opCode |= 1 ;
    if ( toRegOrMem == TO_REG ) opCode |= 2 ;
    // we need to be able to set the size so we can know how big the instruction will be in eg. CompileVariable
    // otherwise it could be optimally deduced but let caller control by keeping operandSize parameter
    // some times we need cell_t where bytes would work
    Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
    if ( operandSize < 8 )
    {
        int64 opCode0 ;
        if ( operandSize == 1 ) opCode |= rm ;
        else if ( operandSize == 2 )
        {
            opCode0 = 0x66 ;
            opCode |= rm ;
        }
        else if ( operandSize == 4 ) opCode |= rm ;
        Compile_CalculateWrite_Instruction_X64 ( opCode0, opCode, mod, reg, rm, controlFlags, sib, disp, 0, 0, operandSize ) ;
    }
    else //if ( osize >= 8 ) 
    {
        Compile_CalculateWrite_Instruction_X64 ( 0, opCode, mod, reg, rm, controlFlags, sib, disp, 0, 0, operandSize ) ;
    }
}

// opCode group 1 - 0x80-0x83 : ADD OR ADC SBB AND_OPCODE SUB XOR CMP : with immediate data
// this is for immediate operands operating on REG direction
// mod := REG | MEM
// rm is operand register
// ?!? shouldn't we just combine this with _Compile_Group1 (above) ?!?

void
_Compile_X_Group1_Reg_To_Reg ( Boolean code, Boolean dstReg, int64 srcReg )
{
    _Compile_X_Group1 ( code, TO_REG, REG, srcReg, dstReg, 0, 0, CELL_SIZE ) ;
}

// opCode group 1 - 0x80-0x83 : CMP ADD OR ADC SBB AND_OPCODE SUB XOR
// to reg ( toRm == 0 ) is default
// toRm flag is like a mod field but is coded as part of opCode
// toRm flag = 0 => ( dst is reg, src is rm ) is default - reg  - like mod 3
// toRm flag = 1 => ( dst is rm, src is reg )             [reg] - like mod 0 // check this ??


// X variable op compile for group 1 opCodes : +/-/cmp/and/or/xor - ia32 

// subtract second operand from first and store result in first

void
_Compile_X_Group1_Immediate ( Boolean code, Boolean mod, Boolean rm, int64 disp, uint64 imm, Boolean iSize )
{
    // 0x80 is the base opCode for this group of instructions but 0x80 is an alias for 0x82
    // we always sign extend so opCodes 0x80 and 0x82 are not being used
    // 1000 00sw 
    //DBI_ON ;
    //if ( ( code == CMP ) || ( code == XOR ) ) SetCompilerField ( CurrentTopBlockInfo, CmpCode, Here ) ;
    byte opCode = 0x80 ;
    int64 controlFlags = ( IMM_B | MODRM_B | ( disp ? DISP_B : 0 ) ) ;
    //if ( ( mod == MEM ) && ( ( iSize > 4 ) || ( imm >= 0x100000000 ) ) )
    if ( ( mod == MEM ) && ( ( iSize > 4 ) || ( imm > 0xffffffff ) ) )
    {
        // x64 has no insn to do a group1_op with imm int64 to mem directly so ...
        // first move to a reg then from that reg group1Op to mem location
        //DBI_ON ;
        //iPrintf ( "\n%s", Context_Location () ) ;
        Compile_MoveImm_To_Reg ( THRU_REG, imm, iSize ) ;
        _Compile_X_Group1 ( code, REG, MEM, THRU_REG, rm, 0, disp, CELL_SIZE ) ;
    }
    else
    {
        rm &= 0xff, controlFlags |= REX_W ; //DBI_ON ;
        if ( ( iSize > BYTE ) || ( abs ( ( int ) imm ) >= 0x100 ) ) opCode |= 1 ;
        else if ( ( iSize <= BYTE ) || ( abs ( ( int ) imm ) < 0x100 ) ) opCode |= 3, iSize = 1 ; //, rm &= 0xff, controlFlags |= REX_W ;
        // we need to be able to set the size so we can know how big the instruction will be in eg. CompileVariable
        // otherwise it could be optimally deduced but let caller control by keeping operandSize parameter
        // some times we need cell_t where bytes would work
        //_Compile_InstructionX86 ( int8 opCode, int8 mod, int8 reg, int8 rm, int8 controlFlags, int8 sib, int64 disp, int8 dispSize, int64 imm, int8 immSize )
        //DBI_ON ;
        //Compile_CalculateWrite_Instruction_X64 ( uint8 opCode0, uint8 opCode1, Boolean mod, uint8 reg, Boolean rm,
        //uint16 controlFlags, Boolean sib, int64 disp, uint8 dispSize, int64 imm, uint8 immSize )
        //SetCompilerField ( CurrentTopBlockInfo, CmpCode, Here ) ;
        Compile_CalculateWrite_Instruction_X64 ( 0, opCode, mod, code, rm, controlFlags, 0, disp, 0, imm, iSize ) ;
    }
    //DBI_OFF ;
}
// X variable op compile for group 1 opCodes : +/-/and/or/xor - ia32 

void
_Compile_optInfo_X_Group1 ( Compiler * compiler, int64 op )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
    if ( ( optInfo->OptimizeFlag & CO_IMM ) && optInfo->CO_Imm )
    {
        //int64 imm = optInfo->CO_Imm ;
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
        _Compile_X_Group1_Immediate ( op, optInfo->CO_Mod,
            optInfo->CO_Rm, optInfo->CO_Disp,
            optInfo->CO_Imm, OperandSize ( optInfo->CO_Imm ) ) ; //( imm >= 0x100000000 ) ? CELL : ( ( imm >= 0x100 ) ? 4 : 1 ) ) ;
    }
    else
    {
        //Compiler_Word_SCHCPUSCA (optInfo->opWord, 0) ;

        _Compile_X_Group1 ( op, optInfo->CO_Dest_RegOrMem, optInfo->CO_Mod,
            optInfo->CO_Reg, optInfo->CO_Rm, 0,
            optInfo->CO_Disp, CELL_SIZE ) ;
    }
}

void
Compile_X_Group1 ( Compiler * compiler, int64 op )
{
    int64 optSetupFlag = CO_CheckOptimize ( compiler, 0 ) ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ; //Compiler_CheckOptimize may change the optInfo
    if ( optSetupFlag == CO_DONE ) return ;
    else if ( optSetupFlag )
    {
        //DBI ;
        _Compile_optInfo_X_Group1 ( compiler, op ) ;
#if 1        
        Compiler_SetBiTttN ( _Context_->Compiler0, TTT_ZERO, N_0, GI_JCC_TO_FALSE ) ;
        if ( optInfo->CO_Rm == DSP ) // if the result is to a reg and not tos
        {
            if ( optInfo->CO_Dest_RegOrMem == MEM ) return ; //_Compile_Move_Reg_To_StackN ( DSP, 0, optInfo->CO_Reg ) ; //return ;
            else if ( ( optInfo->CO_Dest_RegOrMem == REG ) && ( ! optInfo->xBetweenArg1AndArg2 ) ) _Compile_Move_Reg_To_StackN ( DSP, 0, optInfo->CO_Reg, 0 ) ;
        }
            //else if (( optInfo->wordArg1 ) && ( ! ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) _Word_CompileAndRecord_PushReg ( optInfo->COIW[0], optInfo->CO_Reg, true ) ; // 0 : ?!? should be the exact variable 
        else _Word_CompileAndRecord_PushReg ( optInfo->COIW[0], optInfo->CO_Reg, true, 0 ) ; // 0 : ?!? should be the exact variable 
        //DBI_OFF ;
#endif
    }
    else // this works on unoptimized code and should be example for other functions !?
    {
        //DBI_ON ;

        _Compile_Stack_PopToReg ( DSP, RAX ) ;
        _Compile_X_Group1 ( op, MEM, MEM, RAX, DSP, 0, 0, CELL_SIZE ) ; // result is on TOS
        //DBI_OFF ;
    }
}

// Group2 : 0pcodes C0-C3/D0-D3
// ROL RLR RCL RCR SHL SHR SAL SAR
// mod := REG | MEM

void
_Compile_Group2 ( Boolean mod, Boolean regOpCode, Boolean rm, int64 controlFlags, Boolean sib, int64 disp, int64 imm )
{
    //cell opCode = 0xc1 ; // rm32 imm8
    // _Compile_InstructionX86 ( opCode, mod, reg, rm, modRmImmDispFlag, sib, disp, imm, immSize )

    Compile_CalculateWrite_Instruction_X64 ( 0, 0xc1, mod, regOpCode, rm, ( controlFlags | REX_W | MODRM_B | ( disp ? DISP_B : 0 ) ), sib, disp, 0, imm, ( imm ? BYTE : 0 ) ) ;
}

void
_Compile_Group2_CL ( Boolean mod, Boolean regOpCode, Boolean rm, Boolean sib, int64 disp )
{
    // _Compile_InstructionX86 ( opCode, mod, reg, rm, modRmImmDispFlag, sib, disp, imm, immSize )

    Compile_CalculateWrite_Instruction_X64 ( 0, 0xd3, mod, regOpCode, rm, REX_W | MODRM_B | ( disp ? DISP_B : 0 ), sib, disp, 0, 0, 0 ) ;
}

// some Group 3 code is UNTESTED
// opCodes TEST NOT NEG MUL DIV IMUL IDIV // group 3 - F6-F7
// s and w bits of the x86 opCode : w seems to refer to word and is still used probably for historic reasons
// note : the opReg - operand register parameter is always used for the rm field of the resulting machine code
// operating with either a direct register, or indirect memory operand on a immediate operand
// mod := RegOrMem - direction is REG or MEM
// 'size' is operand size

void
_Compile_Group3 ( Boolean code, Boolean mod, Boolean rm, Boolean controlFlags, Boolean sib, int64 disp, int64 imm, Boolean size )
{
    // _Compile_InstructionX86 ( opCode, mod, reg, rm, modRmImmDispFlag, sib, disp, imm, immSize )

    Compile_CalculateWrite_Instruction_X64 ( 0, 0xf7, mod, code, rm, REX_W | MODRM_B | controlFlags, sib, disp, 0, imm, size ) ;
}

// inc/dec only ( not call or push which are also group 5 - cf : sandpile.org )
// inc/dec/push/pop

void
_Compile_Group5 ( Boolean code, Boolean mod, Boolean rm, Boolean sib, int64 disp, Boolean size )
{
    //Compile_CalcWrite_Instruction_X64 (  opCode, mod, code, rm, REX_B | MODRM_B | IMM_B | DISP_B, 0, disp, 0, imm, iSize ) ;

    Compile_CalculateWrite_Instruction_X64 ( 0, 0xff, mod, code, rm, ( REX_W | MODRM_B | DISP_B ), sib, disp, size, 0, 0 ) ;
}

void
Compile_X_Group5 ( Compiler * compiler, int64 op )
{
    int64 optSetupFlag = CO_CheckOptimize ( compiler, 0 ) ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ; //Compiler_CheckOptimize may change the optInfo
    Word *one = CSL_WordList ( 1 ) ; // assumes two values ( n m ) on the DSP stack 
    if ( optSetupFlag & CO_DONE ) return ;
    else if ( optSetupFlag )
    {
        if ( optInfo->OptimizeFlag & CO_IMM )
        {
            Compile_MoveImm_To_Reg ( ACC, optInfo->CO_Imm, CELL_SIZE ) ;
            optInfo->CO_Mod = REG ;
            optInfo->CO_Rm = ACC ;
        }
        Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
        _Compile_Group5 ( op, optInfo->CO_Mod, optInfo->CO_Rm, 0, optInfo->CO_Disp, 0 ) ;
    }
    else if ( one && one->W_ObjectAttributes & ( PARAMETER_VARIABLE | LOCAL_VARIABLE | NAMESPACE_VARIABLE ) ) // *( ( cell* ) ( TOS ) ) += 1 ;
    {
        SetHere ( one->Coding ) ;
        if ( one->W_ObjectAttributes & REGISTER_VARIABLE ) _Compile_Group5 ( op, REG, one->RegToUse, 0, 0, 0 ) ;
        else
        {
            //DBI_ON ;
            //_Compile_Group5 ( Boolean code, Boolean mod, Boolean rm, Boolean sib, int64 disp, Boolean size )
            Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
            _Compile_Group5 ( op, MEM, FP, 0, LocalOrParameterVar_Disp ( one ), 0 ) ;
            //DBI_OFF ;
            return ;
        }
    }
    else
    {
        // assume rvalue on stack
        Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
        _Compile_Group5 ( op, MEM, DSP, 0, 0, 0 ) ;
    }
    Compiler_SetBiTttN ( _Context_->Compiler0, TTT_ZERO, N_0, 0 ) ;
    _Word_CompileAndRecord_PushReg ( CSL_WordList ( 0 ), optInfo->CO_Reg, true, 0 ) ; // 0 : ?!? should be the exact variable 
}

// load reg with effective address of [ mod rm sib disp ]

void
_Compile_LEA ( Boolean reg, Boolean rm, Boolean sib, int64 disp )
{
    // _Compile_InstructionX86 ( opCode, mod, reg, rm, modRmImmDispFlag, sib, disp, imm, immSize )

    Compile_CalculateWrite_Instruction_X64 ( 0, 0x8d, TO_MEM, reg, rm, REX_W | MODRM_B | DISP_B, sib, disp, 0, 0, 0 ) ;
}

void
_Compile_TEST_Reg_To_Reg ( Boolean dstReg, int64 srcReg, Boolean size )
{
    //SetCompilerField ( CurrentTopBlockInfo, TestCode, Here ) ;
    Boolean opCode1 ;
    if ( size == BYTE ) opCode1 = 0x84 ;

    else opCode1 = 0x85 ;
    Compile_CalculateWrite_Instruction_X64 ( 0, opCode1, TO_REG, srcReg, dstReg, REX_W | MODRM_B, 0, 0, 0, 0, 0 ) ;
}

void
Compile_TEST_Reg_To_Reg ( Boolean dstReg, int64 srcReg )
{
    //Compile_CalcWrite_Instruction_X64 ( 0, 0x85, TO_REG, srcReg, dstReg, REX_B | MODRM_B, 0, 0, 0, 0, 0 ) ;

    _Compile_TEST_Reg_To_Reg ( dstReg, srcReg, CELL_SIZE ) ;
}

void
_Compile_SETcc ( Boolean setTtn, Boolean setNegFlag, Boolean reg )
{
    //DBI_ON ;

    //SetCompilerField ( CurrentTopBlockInfo, SetccCode, Here ) ;
    uint8 opCode0, opCode1, modRm, rex ;
    rex = Calculate_Rex ( 0, reg, 0, 0 ) ; //REX_B ) ; //( immSize == 8 ) || ( controlFlag & REX_B ) ) ;
    opCode0 = ( byte ) 0x0f ;
    opCode1 = ( ( 0x9 << 4 ) | ( setTtn << 1 ) | setNegFlag ) ;
    modRm = CalculateModRmByte ( REG, reg, 0, 0, 0 ) ;
    //_Compile_Write_Instruction_X64 ( int8 rex, uint8 opCode0, uint8 opCode1, int8 modRm, int16 controlFlags, int8 sib, int64 disp, int8 dispSize, int64 imm, int8 immSize )
    _Compile_Write_Instruction_X64 ( rex, opCode0, opCode1, modRm, MODRM_B, 0, 0, 0, 0, 0 ) ; //controlFlags, sib, disp, dispSize, imm, immSize ) ;
    //DBI_OFF ;
}

void
_Compile_SETccRm ( Boolean setTtn, Boolean setNegFlag, Boolean rm )
{
    DBI_ON ;

    //SetCompilerField ( CurrentTopBlockInfo, SetccCode, Here ) ;
    uint8 opCode0, opCode1, modRm, rex ;
    rex = 0x41 ; //Calculate_Rex ( 0, reg, 0, 0 ) ; //REX_B ) ; //( immSize == 8 ) || ( controlFlag & REX_B ) ) ;
    opCode0 = ( byte ) 0x0f ;
    opCode1 = ( ( 0x9 << 4 ) | ( setTtn << 1 ) | setNegFlag ) ;
    //CalculateModRmByte ( Boolean mod, Boolean reg, Boolean rm, Boolean sib, int64 disp )
    modRm = rm ; //CalculateModRmByte ( MEM, 0, rm, 0, 0 ) ;
    //_Compile_Write_Instruction_X64 ( int8 rex, uint8 opCode0, uint8 opCode1, int8 modRm, int16 controlFlags, int8 sib, int64 disp, int8 dispSize, int64 imm, int8 immSize )
    _Compile_Write_Instruction_X64 ( rex, opCode0, opCode1, modRm, MODRM_B, 0, 0, 0, 0, 0 ) ; //controlFlags, sib, disp, dispSize, imm, immSize ) ;
    DBI_OFF ;
}

void
Compile_CmpCode_Reg ( Boolean reg, int64 value )
{
    Compile_CMPI ( REG, reg, 0, value, 0 ) ;
}

void
Compile_Xor_Reg ( Boolean reg, int64 value )
{

    Compile_XORI ( REG, reg, 0, value, 0 ) ;
}

void
Compile_Logical_X_Group1 ( Compiler * compiler, int64 op, Boolean ttt, Boolean negFlag )
{
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( compiler->CombinatorBlockInfoStack ) ;
    if ( bi ) bi->State |= ( ( uint64 ) 1 << op ) ;
    int64 optSetupFlag = CO_CheckOptimize ( compiler, 0 ) ;
    if ( optSetupFlag == CO_DONE ) return ;
    else if ( optSetupFlag ) _Compile_X_Group1 ( op, REG, REG, ACC, OREG, 0, 0, CELL_SIZE ) ;
    else
    {
        // operands are still on the stack
        _Compile_Move_StackN_To_Reg ( ACC, DSP, 0, 0 ) ;
        //_Compile_Group1 ( int64 code, int64 toRegOrMem, int64 mod, int64 reg, int64 rm, int64 sib, int64 disp, int64 osize )
        _Compile_X_Group1 ( op, REG, MEM, ACC, DSP, 0, CELL_SIZE, CELL_SIZE ) ;
#if OLD_Setcc            
        _Compile_Stack_DropN ( DSP, 2 ) ;
#else            
        _Compile_Stack_DropN ( DSP, 1 ) ;
        Compile_MoveImm_To_TOS ( R14, 0, 8 ) ;
#endif            
    }

    if ( ! Check_Logic ( bi, ttt, negFlag ) ) BI_SetccAndStackPushAcc ( bi, ttt, ! negFlag ) ; // looks ??
}

// mul reg with imm to rm

void
_Compile_IMUL ( Boolean mod, Boolean reg, Boolean rm, Boolean sib, int64 disp, uint64 imm )
{
    int64 opCode = 0x69 ;

    if ( imm && ( imm < 256 ) ) opCode |= 2 ;
    //Compile_CalcWrite_Instruction_X64 ( uint8 opCode0, uint8 opCode1, int8 mod, int8 reg, int8 rm, int16 controlFlags, int8 sib, int64 disp, int8 dispSize, uint64 imm, int8 immSize )
    Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xaf, mod, reg, rm, REX_W | MODRM_B | ( imm ? IMM_B : 0 ), sib, disp, 0, imm, 0 ) ; //size ) ;
}

void
Compile_IMULI ( Boolean mod, Boolean reg, Boolean rm, Boolean sib, int64 disp, uint64 imm )
{
    int64 opCode = 0x69, immSize ;
    if ( imm < 128 )
    {
        opCode |= 2 ;
        immSize = 1 ;
    }

    else immSize = 4 ;
    Compile_CalculateWrite_Instruction_X64 ( 0, opCode, mod, reg, rm, REX_W | MODRM_B | IMM_B, sib, disp, 0, imm, immSize ) ; //size ) ;
}

void
Compile_IMUL ( Boolean mod, Boolean reg, Boolean rm, Boolean controlFlags, Boolean sib, int64 disp )
{

    Boolean rex = Calculate_Rex ( reg, rm, 0, 0 ) ;
    Boolean modRm = CalculateModRmByte ( mod, reg, rm, sib, disp ) ;
    //_Compile_Write_Instruction_X64 ( int8 rex, uint8 opCode0, uint8 opCode1, int8 modRm, int16 controlFlags, int8 sib, int64 disp, int8 dispSize, int64 imm, int8 immSize )
    _Compile_Write_Instruction_X64 ( rex, 0x0f, 0xaf, modRm, controlFlags, sib, disp, 0, 0, 0 ) ;
}

void
_Compile_Test ( Boolean mod, Boolean reg, Boolean rm, Boolean controlFlags, int64 disp, int64 imm )
{
    Compile_CalculateWrite_Instruction_X64 ( 0, 0xf7, mod, reg, rm, REX_W | MODRM_B | controlFlags, 0, disp, 0, imm, 0 ) ; //??
}

byte *
Compile_UninitializedJccEqualZero ( )
{
    byte * compiledAtAddress = Here ;
    _Compile_Jcc ( JCC32, TTT_ZERO, N_0, 0 ) ;
    return compiledAtAddress ;
}

byte *
Compile_UninitializedJccNotEqualZero ( )
{
    byte * compiledAtAddress = Here ;
    _Compile_Jcc ( JCC32, TTT_ZERO, N_1, 0 ) ;
    return compiledAtAddress ;
}

void
_Compile_JumpToDisp ( int32 disp, byte insn )
{
    if ( ( insn != JMP32 ) && ( ( disp > - 127 ) && ( disp <= 128 ) ) )
        Compile_CalculateWrite_Instruction_X64 ( 0, JMP8, 0, 0, 0, DISP_B, 0, disp, BYTE, 0, 0 ) ;
    else Compile_CalculateWrite_Instruction_X64 ( 0, JMP32, 0, 0, 0, DISP_B, 0, disp, INT32_SIZE, 0, 0 ) ;
}

void
Compile_JumpToAddress ( byte * jmpToAddr, byte insn ) // runtime
{
    int32 disp = CalculateOffsetForCallOrJump (Here, jmpToAddr, T_JMP , 0) ;
    _Compile_JumpToDisp ( disp, insn ) ;
}

void
Compile_JumpToRegAddress ( Boolean reg )
{
    _Compile_Group5 ( JMP, 0, reg, 0, 0, 0 ) ;
}

void
_Compile_UninitializedJmpOrCall ( Boolean jmpOrCall ) // jmpOrCall : CALL32/JMP32
{
    Compile_CalculateWrite_Instruction_X64 ( 0, jmpOrCall, 0, 0, 0, DISP_B, 0, 0, INT32_SIZE, 0, 0 ) ;
}

void
_Compile_JumpWithOffset ( int64 disp )
{
    _Compile_JumpToDisp ( disp, 0 ) ;
}

byte *
_Compile_UninitializedCall ( )
{
    byte * compiledAtAddress = Here ;
    _Compile_UninitializedJmpOrCall ( CALL32 ) ;
    return compiledAtAddress ;
}

byte *
Compile_UninitializedJump ( )
{
    byte * compiledAtAddress = Here ;
    _Compile_UninitializedJmpOrCall ( JMP32 ) ;
    return compiledAtAddress ;
}

void
_Compile_CallReg ( Boolean reg, Boolean regOrMem )
{
    _Compile_Group5 ( CALL, regOrMem, reg, 0, 0, 0 ) ;
}

void
Compile_CallThru_AdjustRSP ( Boolean reg, Boolean regOrMem )
{

    Compile_SUBI ( REG, RSP, 0, 8, 0 ) ;
    _Compile_CallReg ( reg, regOrMem ) ;
    Compile_ADDI ( REG, RSP, 0, 8, 0 ) ;
}

void
Compile_Call ( byte * address )
{
    Compile_MoveImm_To_Reg ( CALL_THRU_REG, ( int64 ) address, CELL_SIZE ) ;
    _Compile_CallReg ( CALL_THRU_REG, REG ) ;
}

void
_Compile_Call_ThruReg_TestAlignRSP ( Boolean thruReg )
{
    //DBI_ON ;

    Compile_Move_Reg_To_Reg ( RAX, RSP, 0 ) ;
    Compile_TEST_AL_ImmByte ( 0x8 ) ;
    Compile_Jcc ( TTT_ZERO, N_0, Here + 15, JCC8 ) ;
    Compile_CallThru_AdjustRSP ( thruReg, REG ) ;
    _Compile_JumpToDisp ( 3, JMP8 ) ;
    _Compile_CallReg ( thruReg, REG ) ;
    //DBI_OFF ;
    //Pause () ;
}

void
Compile_Call_ToAddressThruReg_TestAlignRSP ( byte * address, Boolean thruReg )
{
    Compile_MoveImm_To_Reg ( thruReg, ( int64 ) address, CELL_SIZE ) ;
    _Compile_Call_ThruReg_TestAlignRSP ( thruReg ) ;
}

void
Compile_Call_ToAddressThruSREG_TestAlignRSP ( )
{
    _Compile_Call_ThruReg_TestAlignRSP ( SREG ) ;
}

void
Compile_Call_TestRSP ( byte * address )
{

    Compile_MoveImm_To_Reg ( SREG, ( int64 ) address, CELL_SIZE ) ;
    Compile_Call ( ( byte* ) _CSL_->Call_ToAddressThruSREG_TestAlignRSP ) ;
}

void
Compile_Call_X84_ABI_RSP_ADJUST ( byte * address )
{
    Compile_Call_TestRSP ( address ) ;
}

void
Compile_CallWord_Check_X84_ABI_RSP_ADJUST ( Word * word )
{
    if ( word->W_MorphismAttributes & CPRIMITIVE )
    {
        d0 ( iPrintf ( "\n_Word_Compile : %s : name = %s", Context_Location ( ), word->Name ) ) ;
        // there is an slight overhead for CPRIMITIVE functions to align RSP for ABI-X64
        Compile_Call_TestRSP ( ( byte* ) word->Definition ) ;
    }
    else Compile_Call ( ( byte* ) word->Definition ) ;
}

void
_Compile_PushReg ( Boolean reg )
{
    // only EAX ECX EDX EBX : 0 - 4

    Compile_CalculateWrite_Instruction_X64 ( 0, 0x50 + reg, REG, 0, 0, REX_W, 0, 0, 0, 0, 0 ) ;
}

// pop from the C esp based stack with the 'pop' instruction

void
_Compile_PopToReg ( Boolean reg )
{
    // only EAX ECX EDX EBX : 0 - 4

    Compile_CalculateWrite_Instruction_X64 ( 0, 0x58 + reg, REG, 0, 0, REX_W, 0, 0, 0, 0, 0 ) ;
}

void
_Compile_PopFD ( )
{

    Compile_CalculateWrite_Instruction_X64 ( 0, 0x9d, 0, 0, 0, REX_W, 0, 0, 0, 0, 0 ) ;
}

void
_Compile_PushFD ( )
{

    Compile_CalculateWrite_Instruction_X64 ( 0, 0x9c, 0, 0, 0, REX_W, 0, 0, 0, 0, 0 ) ;
}

void
Compile_TEST_AL_ImmByte ( byte imm )
{

    Compile_CalculateWrite_Instruction_X64 ( 0, 0xa8, 0, 0, 0, IMM_B, 0, 0, 0, imm, BYTE ) ;
}

void
_Compile_INT80 ( )
{

    Compile_CalculateWrite_Instruction_X64 ( 0xcd, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) ;
}

void
_Compile_Noop ( )
{

    Compile_CalculateWrite_Instruction_X64 ( 0, 0x90, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) ;
}

// Zero eXtend from byte to cell
// byte : 8 bit

void
_Compile_MOVZX_BYTE_REG ( Boolean reg, Boolean rm )
{

    //SetCompilerField ( CurrentTopBlockInfo, MovzxCode, Here ) ;
    //Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xb6, REG, reg, rm, ( REX_W | MODRM_B ), 0, 0, 0, 0, 0 ) ;
    Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xb6, REG, reg, rm, ( REX_W | MODRM_B ), 0, 0, 0, 0, 0 ) ;
}

// word : 16 bit

void
_Compile_MOVZX_WORD_REG ( Boolean reg, Boolean rm )
{

    //SetCompilerField ( CurrentTopBlockInfo, MovzxCode, Here ) ;
    //Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xb6, REG, reg, rm, ( REX_W | MODRM_B ), 0, 0, 0, 0, 0 ) ;
    Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xb7, REG, reg, rm, ( REX_W | MODRM_B ), 0, 0, 0, 0, 0 ) ;
}

#if 0 // not needed auto zero-extend with 32 to 64 bit 
// long : 32 bit

void
_Compile_MOVZX_LONG_REG ( Boolean reg, Boolean rm )
{
    //SetCompilerField ( CurrentTopBlockInfo, MovzxCode, Here ) ;
    //Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xb6, REG, reg, rm, ( REX_W | MODRM_B ), 0, 0, 0, 0, 0 ) ;
    //Compile_CalculateWrite_Instruction_X64 ( 0x0f, 0xb7, REG, reg, rm, ( REX_W | MODRM_B ), 0, 0, 0, 0, 0 ) ;
    //_Compile_Int8 ( 0x48 ) ; _Compile_Int8 ( 0x0f ) ; _Compile_Int8 ( 0xb7 ) ; _Compile_Int8 ( 0xc0 ) ;

    Compile_Move ( REG, REG, reg, reg, 4, 0, 0, 0, 0, 0 ) ;
    //_Compile_Write_Instruction_X64 ( 0x41, 0x0f, 0xb6, modRm, controlFlags, sib, disp, dispSize, imm, immSize ) ;
}
#endif

void
_Compile_Return ( )
{
    //Compiler_WordStack_SCHCPUSCA( 0, 0 )  ;

    Compile_CalculateWrite_Instruction_X64 ( 0, 0xc3, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) ;
}

void
_Compile_Nop ( )
{
    //Compiler_WordStack_SCHCPUSCA( 0, 0 )  ;
    Compile_CalculateWrite_Instruction_X64 ( 0, 0x90, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) ;
}

