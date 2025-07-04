
// Intel notes - cf. InstructionSet-A-M-253666.pdf - section 2.1; InstructionSet-N-Z-253667.pdf section B.1/B.2 :
// b prefix = binary code
// -----------------------------------------------------------------------
// instuction format ( in bytes )
// prefixes  opcode  modRm     sib     disp   immediate
//  0 - 4    1 - 3   0 - 1    0 - 1    0,1,4    0,1,4      -- number of bytes
// optional          ------------optional--------------
// -----------------------------------------------------------------------
//   modRm byte ( bits )  mod 0 : no disp ;; mod 1 : 1 byte disp : mod 2 : 4 byte disp ;; mod 3 : just reg value : sections 2.1.3/2.1.5, Table 2-2
//    mod     reg      rm
//   7 - 6   5 - 3   2 - 0 
//    0-3              4 - b100 => sib, instead of reg ESP   : mod bit determines size of displacement 
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

// note : intex syntax  : instruction dst, src
//        att   syntax  : instruction src, dst
#define x86_emit_byte( x ) _Compile_Int8 ( x )
#define x86_emit_word( x ) _Compile_Int16 ( x )
#define x86_emit_long( x ) _Compile_Int32 ( x )


#define NEW_JMP_CALL 0
#if NEW_JMP_CALL
#define OFFSET_BACKTRACK ( CELL_SIZE + 2 )
#else
#define OFFSET_BACKTRACK CELL_SIZE
#endif

// x86 insn opcodes
//#define CALL8 not in 64 bit mode
#define CALL32 (0xe8 ) // x86 - call opcode
#define _RET (0xc3) // _RET : don't conflict with RET insn : x86 -
#define NOOP ( 0x90 )
#define JMP32 (0xe9 ) // x86 - jmp opcode
#define JMP8 (0xeb ) // x86 - jmp opcode
#define JCC8 (0x70)
#define JCC16 (0x80) //??
#define JCC32 (0x0f) // this is somehow useful now ? //0x0f8x
#define JCC32_2 (0x800f)
#define NOOP4 (0x000401f0f) //(0x0f1f4000)
#define TEST_ISN ()
// insn types
#define T_JCC 1
#define T_JMP 2
#define T_CALL 4
#define IS_INSN_JCC8( insn ) ( ( (byte)insn != 0x0f ) && (( (byte)insn >> 4 ) == 0x7 ) )
#define IsSetCcInsn( addr ) (( (* addr) == 0x0f ) && ( ((*(addr+1)) >> 4) == 0x9 ))  //( ((*addr+1) >> 4) == 0x9 )) 
#define CALL_REG 0xff
#define MOD_RM_R8 0x10
#define MOVI 0xc7

#define JE8 (0x74 ) // x86 - jcc opcode
#define JNE8 (0x75 ) // x86 - jcc opcode
#define JE32 (0x84 ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix
#define JE32_W (0x840f ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix - little endian
#define JNE32 (0x85 ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix
#define JGE32 (0x8d ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix
#define JLE32 (0x8e ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix
#define JG32 (0x8f ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix
#define JL32 (0x8c ) // x86 - jcc opcode : remember 32 bit offset has 0x0f opcode prefix

//#define PUSHAD (0x60 )// intel x86 pushad instruction
#define PUSHFD (0x9c )// intel x86 pushfd instruction
#define _POPAD (0x61 )// intel x86 pushad instruction
#define POPFD (0x9d )// intel x86 pushfd instruction
#define PUSHI32 (0x68 )
#define POPToR8 (0x58 )
#define _LAHF (0x9f )
#define _SAHF (0x9e )

#define INT3 (0xcc) // cell_t 3 interrupt code

// mod reg r/m
// mod
#define MOD_DISP_0 0
#define MOD_DISP_8 1
#define MOD_DISP_32 2
#define MOD_REG 3
#define TO_REG 3
#define TO_MEM MOD_DISP_32
// mod field values
#define REG MOD_REG
#define MEM 0 // 0 - 2 based on displacement, calculated at compile time
#define RM_IS_ADDR MEM // based on displacement, calculated at compile time
#define ADDR MEM // based on displacement, calculated at compile time
#define RM_IS_VALUE REG
#define RM_IS_REG_VALUE REG
#define RM_SIB 0x4
#define VALUE REG
// SIB byte conversions
#define SCALE_1 0
#define SCALE_2 1
#define SCALE_4 2
#define SCALE_8 3
// control flag bits for _Compile_InstructionXxx
#define REX_W       ( 1 << 0 ) 
#define MODRM_B     ( 1 << 1 ) // backwards compatibility
#define SIB_B       ( 1 << 2 ) 
#define DISP_B      ( 1 << 3 ) 
#define IMM_B       ( 1 << 4 ) 
#define REX_B       ( 1 << 5 )
#define DIR_TO_REG ( 1 << 6 )
#define DIR_TO_MEM ( 1 << 7 )
#define REX 0x40
//#define REX_W 0x8 
#define REX_R 0x4
#define REX_X 0x2
//#define REX_B 0x1 // same symbol with two different contextual meanings but has same internal value
#if X64
#define SCALE_CELL SCALE_8
#else
#define SCALE_CELL SCALE_4
#endif

#define BYTE_SIZE (sizeof (byte))
#define INT16_SIZE (sizeof (int16))
#define INT32_SIZE (sizeof (int32))
#define INT64_SIZE (sizeof (int64))
// size_w field
#if X64
#define CELL_T INT64_T
#else
#define CELL_T INT32_T
#endif
//#define INT INT32_SIZE
#define DISPLACEMENT_32_SIZE ( sizeof ( int32 ) )
#define DISP_SIZE DISPLACEMENT_32_SIZE


// xx xxx xxx
// intel r/m byte reg field values

#define RAX ( 0x0 )
#define RCX ( 0x1 )
#define RDX ( 0x2 )
#define RBX ( 0x3 )
#define RSP ( 0x4 )
#define RBP ( 0x5 )
#define RSI ( 0x6 )
#define RDI ( 0x7 )
#define R8  ( 0x8 )
#define R9  ( 0x9 )
#define R10 ( 0xa )
#define R11 ( 0xb )
#define R12 ( 0xc )
#define R13 ( 0xd )
#define R14 ( 0xe )
#define R15 ( 0xf )
#if X64
#define R8D R8
#define R9D R9 
#define R10D R10
#define R11D R11
#define R12D R12 // RSP
#define R13D R13 // RBP
#define R14D R14 //Stack Pointer
#define R15D R15 //Frame Pointer
#endif
#define NO_INDEX ( 0x4 ) // for sib byte with no index

#if 0
	index : kernel/git/torvalds/linux.git
            root/arch/x86/entry/calling.h
 x86 function call convention, 64-bit:
 -------------------------------------
  arguments           |  callee-saved      | extra caller-saved | return
 [callee-clobbered]   |                    | [callee-clobbered] |
 ---------------------------------------------------------------------------
 rdi rsi rdx rcx r8-9 | rbx rbp [*] r12-15 | r10-11             | rax, rdx [**]

 ( rsp is obviously invariant across normal function calls. (gcc can 'merge'
   functions when it sees tail-call optimization possibilities) rflags is
   clobbered. Leftover arguments are passed over the stack frame.)

 [*]  In the frame-pointers case rbp is fixed to the stack frame.

 [**] for struct return values wider than 64 bits the return convention is a
      bit more complex: up to 128 bits width we return small structures
      straight in rax, rdx. For structures larger than that (3 words or
      larger) the caller puts a pointer to an on-stack return struct
      [allocated in the caller_s stack frame] into the first argument - i.e.
      into rdi. All other arguments shift up by one in this case.
      Fortunately this case is rare in the kernel.
#endif

#define REG_ORDER { RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } //System V AMD64 ABI
#define CSL_REG_ORDER { RDI, RSI, R8D, R9D } 
#define NUM_CSL_REGS 4      
#define THRU_REG                R11D //(RegOrder(7)) 
#define CALL_THRU_REG           THRU_REG                // R11D
#define SCRATCH_REG             R10D //(RegOrder(6)) 
#define SREG                    SCRATCH_REG             // R10D
#define ACCUMULATOR_REG         RAX                     // rax
#define ACC                     ACCUMULATOR_REG
#define CPU_ACCUM               RAX
#define ACCUM                   ACCUMULATOR_REG
#define RETURN_REG              ACC
#define RETURN_REG_2            RDX                     // rdx
#define OPERAND_REG             RCX                     // rcx
#define OP_REG                  OPERAND_REG
#define OREG                    OPERAND_REG
#define CPU_OREG                RCX
#define OPERAND_2_REG           RBX                     // rbx
#define OREG2                   OPERAND_2_REG 
#define TEMP_REG                OPERAND_2_REG 
#define CPU_OREG2               RBX
#define DIV_MUL_REG_2           RDX                     // rdx
#if DSP_IS_GLOBAL_REGISTER 
register int64 *_DspReg_        asm ( "r14" ) ;
register uint64 *_Fp_           asm ( "r15" ) ;
#endif
#define STACK_POINTER           R14D                    // r14
#define DSP                     STACK_POINTER 
#define CPU_DSP                 R14d
#define FRAME_POINTER           R15D                    // r15
#define CPU_FP                  R15d
#define FP                      FRAME_POINTER

//register uint64 *_RspReg_ asm ( "r10" ) ;
#define CSL_RETURN_STACK_POINTER    RBX         // rbx
#define CSL_RSP                     CSL_RETURN_STACK_POINTER
#define CPU_CSL_RSP                 RBX

// EFLAGS
#define CARRY_FLAG ( 1 << 0 )
#define PARITY_FLAG ( 1 << 2 )
#define AUX_FLAG ( 1 << 4 )
#define ZERO_FLAG ( 1 << 6 )
#define SIGN_FLAG ( 1 << 7 )
#define OVERFLOW_FLAG ( 1 << 11 )


#define BYTE ((byte) 1)
#define LABEL ((byte) 2)
#define INT32 sizeof (int32) 
//#define XSIZE ((byte) 8)

// sign extend field
//#define SIGN_EXT 1
//#define NO_SIGN_EXT 0
// group 1 code field
#define ADD 0
#define OR 1
#define ADC 2
#define SBB 3
#define AND 4
//#define AND_OPCODE 4
#define SUB 5
#define XOR 6
#define CMP 7
#define MUL_OP 8  // internal code - not an intel code
#define DIV_OP 9  // internal code - not an intel code
#define EQUALS 10  // internal code - not an intel code
#define LESS 11  // internal code - not an intel code
#define GREATER 12  // internal code - not an intel code
//BlockInfo BI
#define BI_OR   ( ( uint64 ) 1 << OR ) // OR == 1 
#define BI_AND  ( ( uint64 ) 1 << AND ) // AND == 4
#define BI_FINISHED  ( ( uint64 ) 1 << 13 )
#define BI_2_LPAREN  ( ( uint64 ) 1 << 14 )
#define BI_2_RPAREN  ( ( uint64 ) 1 << 15 )

// group 5 codes : inc/dec call/jmp push
// 0xfe-0xff
#define INC 0
#define DEC 1
#define CALL 2
#define CALLF 3
#define JMP 4
#define JMPF 5
#define PUSH 6
#define POP 7

// group 2
// shift instructions immediate count reg or mem
// 0xc0-0xc1 0xd0-0xd4
// reg field of modRm byte : ( mod reg rm )
#define ROL 0
#define ROR 1
#define RCL 2
#define RCR 3
#define SHL 4
#define SHR 5
#define SAL 6
#define SAR 7

// group 3
// arithmetic instructions
// 0x84-0x85 0xf6-0xf7
#define TEST 0
#define TEST1 1
#define NOT 2
#define NEG 3
#define MUL 4
#define IMUL 5
#define DIV 6
#define IDIV 7
#define IMUL_f ( 0x8 | IMUL ) // distinguish from other overloaded group codes
#define IDIV_f ( 0x8 | IDIV )

// below are arbitrarily chosen
#define MODULO 9
#define MOV 10
#define DIVIDE 1
#define MOD 2

// conditional jump
// jcc32 0x0f ( 0x8 << 8 | ttt << 1 | n ) : 2 bytes : little endian -> { 0x8 << 8 | ttt << 1 | n, 0x0f }
// setcc32 0x0f ( 0x9 << 8 | ttt << 1 | n ) : 2 bytes : little endian -> { 0x9 << 8 | ttt << 1 | n, 0x0f }
// jcc8 ( 0x7 << 4 | ttt << 1 | n ) : 1 byte
// n : n bit on/off 
#define ZERO 0
#define NOT_ZERO 1 
//#define Z ZERO
//#define NZ NOT_ZERO
// n bit N in tttn
#define N_0 ZERO        // n bit off
#define N_1 NOT_ZERO    // n bit on
#define N_Z N_0         // n bit off
#define N_NZ N_1        // n bit on
// zero flag ZF
// ttt : condition codes
// remember these are left shifted one
#define TTT_OVERFLOW 0
#define TTT_BELOW 1
#define TTT_EQUAL 2    
#define TTT_ZERO 2
#define TTT_BE 3   // BE below or equal    
#define TTT_A 3 // A above
#define TTT_SIGN 4
#define TTT_PARITY 5
#define TTT_LESS 6
#define TTT_LE 7   

#define _RM( insnAddr )  (*( (byte*) insnAddr + 1) & 7 )   // binary : 00000111
#define _REG( insnAddr ) (*( (byte*) insnAddr + 1) & 56 )  // binary : 00111000 
#define _MOD( insnAddr ) (*( (byte*) insnAddr + 1) & 192 ) // binary : 11000000 

#define WORD 2
#define DOUBLE 4
#define QUAD 8
#define OCTAL 8
