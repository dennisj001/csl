#include "../include/csl.h"

mpz_t *
_BigInt_New ( int64 initializer )
{
    //cell allocationType ;
    //if ( GetState( _Context_->Compiler0, BLOCK_MODE ) ) allocationType = OBJECT_MEMORY ; 
    //else allocationType = SESSION ;
    mpz_t *bn = ( mpz_t* ) Mem_Allocate ( sizeof ( mpz_t ), OBJECT_MEMORY ) ;
    mpz_init_set_si ( *bn, initializer ) ;
    return bn ;
}

void
BigInt_Init ( )
{
    // assuming TOS is a uint64 
    _DataStack_SetTop ( (int64) _BigInt_New ( _DataStack_GetTop ( ) ) ) ;
}

void
BigInt_Add ( )
{
    mpz_t *sum = _BigInt_New ( 0 ) ;
    mpz_t * op1 = ( mpz_t* ) DataStack_Pop ( ), *op2 = ( mpz_t* ) DataStack_Pop ( ) ;
    mpz_add ( *sum, *op1, *op2 ) ;
    DataStack_Push ( ( int64 ) sum ) ;
}

void
BigInt_PlusEqual ( )
{
    mpz_t *sum = _BigInt_New ( 0 ) ;
    mpz_t * op1 = ( mpz_t* ) DataStack_Pop ( ), **p_op2 = ( mpz_t** ) DataStack_Pop ( ) ;
    mpz_add ( *sum, *op1, **p_op2 ) ;
    *p_op2 = sum ;
    //DataStack_Push ( ( cell_t ) sum ) ;
}

// --

void
BigInt_MinusMinus ( )
{
    mpz_t *sum = _BigInt_New ( 0 ) ;
    mpz_t * op1 = ( mpz_t* ) _DataStack_GetTop ( ), *op2 = ( mpz_t* ) _BigInt_New ( 1 ) ;
    mpz_sub ( *sum, *op1, *op2 ) ;
    _DataStack_SetTop ( ( int64 ) sum ) ;
}

// ++

void
BigInt_PlusPlus ( )
{
    mpz_t *sum = _BigInt_New ( 0 ) ;
    mpz_t * op1 = ( mpz_t* ) _DataStack_GetTop ( ), *op2 = ( mpz_t* ) _BigInt_New ( 1 ) ;
    mpz_add ( *sum, *op1, *op2 ) ;
    _DataStack_SetTop ( ( int64 ) sum ) ;
}

void
BigInt_Multiply ( )
{
    mpz_t *prod = _BigInt_New ( 0 ) ;
    mpz_t * op1 = ( mpz_t* ) DataStack_Pop ( ), *op2 = ( mpz_t* ) DataStack_Pop ( ) ;
    mpz_mul ( *prod, *op1, *op2 ) ;
    //mpz_clear ( *op1 ) ; mpz_clear ( *op2 ) ;
    DataStack_Push ( ( int64 ) prod ) ;
}

void
BigInt_Power ( )
{
    //void mpz_pow_ui (mpz_t rop, mpz_t base, unsigned long int exp)
    mpz_t *rop = _BigInt_New ( 0 ) ;
    // unsigned long int mpz_get_ui (mpz_t exp) 
    mpz_t * expz = (mpz_t*) DataStack_Pop ( ) ;
    int64 exp = mpz_get_ui ( *expz ) ;
    mpz_t * base = ( mpz_t* ) DataStack_Pop ( );
    mpz_pow_ui ( *rop, *base, exp ) ;
    DataStack_Push ( ( int64 ) rop ) ;
}

void
BigInt_MultiplyEqual ( )
{
    mpz_t *prod = _BigInt_New ( 0 ) ;
    mpz_t * op1 = ( mpz_t* ) DataStack_Pop ( ), **p_op2 = ( mpz_t** ) DataStack_Pop ( ) ;
    mpz_mul ( *prod, *op1, **p_op2 ) ;
    *p_op2 = prod ;
}

void
BigInt_DivideWithRemainder ( )
{
    mpz_t *quotient = _BigInt_New ( 0 ) ;
    mpz_t *remainder = _BigInt_New ( 0 ) ;
    mpz_t * denominator = ( mpz_t* ) DataStack_Pop ( ), *numerator = ( mpz_t* ) DataStack_Pop ( ) ;
    mpz_cdiv_qr ( *quotient, *remainder, *numerator, *denominator ) ;
    DataStack_Push ( ( int64 ) remainder ) ;
    DataStack_Push ( ( int64 ) quotient ) ;
}

void
BigInt_DivideEqual_RemainderDiscarded ( ) // remainder discarded
{
    mpz_t *quotient = _BigInt_New ( 0 ) ;
    mpz_t *remainder = _BigInt_New ( 0 ) ;
    mpz_t * denominator = ( mpz_t* ) DataStack_Pop ( ), **numerator = ( mpz_t** ) DataStack_Pop ( ) ;
    mpz_cdiv_qr ( *quotient, *remainder, **numerator, *denominator ) ;
    *numerator = quotient ;
    //DataStack_Push ( ( cell_t ) remainder ) ;
    //DataStack_Push ( ( cell_t ) quotient ) ;
}

void
BigInt_Subtract ( )
{
    mpz_t *diff = _BigInt_New ( 0 ) ;
    mpz_t * op2 = ( mpz_t* ) DataStack_Pop ( ), *op1 = ( mpz_t* ) DataStack_Pop ( ) ;
    mpz_sub ( *diff, *op1, *op2 ) ; // diff = op1 - op2
    DataStack_Push ( ( int64 ) diff ) ;
}

void
BigInt_MinusEqual ( )
{
    mpz_t *diff = _BigInt_New ( 0 ) ;
    mpz_t * op2 = ( mpz_t* ) DataStack_Pop ( ), **p_op1 = ( mpz_t** ) DataStack_Pop ( ) ;
    mpz_sub ( *diff, **p_op1, *op2 ) ; // diff = op1 - op2
    *p_op1 = diff ;
    //DataStack_Push ( ( cell_t ) diff ) ;
}

int64
BigInt_Cmp ( )
{
    mpz_t * op2 = ( mpz_t* ) DataStack_Pop ( ), *op1 = ( mpz_t* ) DataStack_Pop ( ) ;
    return mpz_cmp ( *op1, *op2 ) ;
}

void
BigInt_LessThanOrEqual ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigInt_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigInt_Cmp ( ) > 0 ? 0 : 1 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigInt_Cmp ( ) > 0 ? 0 : 1 ) ;
#endif
}

void
BigInt_LogicalEquals ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigInt_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigInt_Cmp ( ) == 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigInt_Cmp ( ) == 0 ? 1 : 0 ) ;
#endif
}

void
BigInt_LogicalDoesNotEqual ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigInt_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigInt_Cmp ( ) == 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigInt_Cmp ( ) == 0 ? 0 : 1 ) ;
#endif
}

void
BigInt_GreaterThan ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigInt_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigInt_Cmp ( ) > 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigInt_Cmp ( ) > 0 ? 1 : 0 ) ;
#endif
}

void
BigInt_GreaterThanOrEqual ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigInt_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigInt_Cmp ( ) < 0 ? 0 : 1 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigInt_Cmp ( ) < 0 ? 0 : 1 ) ;
#endif
}

void
BigInt_LessThan ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigInt_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigInt_Cmp ( ) < 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigInt_Cmp ( ) < 0 ? 1 : 0 ) ;
#endif
}

void
BigInt_Print ( )
{
    mpz_t * value = ( mpz_t* ) DataStack_Pop ( ) ;
    if ( Verbosity () ) gmp_printf ( " %Zd", * value ) ;
}

mpf_t *
_BigFloat_New ( int64 initializer )
{
    mpf_t *bn = ( mpf_t* ) Mem_Allocate ( sizeof ( mpf_t ), OBJECT_MEMORY ) ;
    mpf_init_set_si ( *bn, ( int64 ) initializer ) ;
    return bn ;
}

void
BigFloat_Init ( )
{
    // assuming TOS is a uint64 
    _DataStack_SetTop ( (int64) _BigFloat_New ( _DataStack_GetTop ( ) ) ) ;
}

void
BigFloat_Precision ( )
{
    mpf_t * prec = ( mpf_t* ) DataStack_Pop ( ) ;
    int64 precision = mpf_get_si ( *prec ) ;
    mpf_set_default_prec ( ( mp_bitcnt_t ) precision ) ;
    _Context_->System0->BigNum_Printf_Precision = precision ;
}

void
BigFloat_Width ( )
{
    mpf_t * _width = ( mpf_t* ) DataStack_Pop ( ) ;
    int64 width = mpf_get_si ( *_width ) ;
    //mpf_set_default_prec ( (mp_bitcnt_t) precision ) ;
    _Context_->System0->BigNum_Printf_Width = width ;
}

void
BigFloat_FPrint ( )
{
    mpf_t * value = ( mpf_t* ) DataStack_Pop ( ) ;
    if ( Verbosity () ) gmp_printf ( " %*.*Ff\n", _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
}

void
BigFloat_EPrint ( )
{
    mpf_t * value = ( mpf_t* ) DataStack_Pop ( ) ;
    if ( Verbosity () ) gmp_printf ( " %*.*Fe\n", _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
}
void
BigFloat_Add ( )
{
    mpf_t *sum = _BigFloat_New ( 0 ) ;
    mpf_t * op1 = ( mpf_t* ) DataStack_Pop ( ), *op2 = ( mpf_t* ) DataStack_Pop ( ) ;
    mpf_add ( *sum, *op1, *op2 ) ;
    DataStack_Push ( ( int64 ) sum ) ;
}

void
BigFloat_Multiply ( )
{
    mpf_t *prod = _BigFloat_New ( 0 ) ;
    mpf_t * op1 = ( mpf_t* ) DataStack_Pop ( ), *op2 = ( mpf_t* ) DataStack_Pop ( ) ;
    mpf_mul ( *prod, *op1, *op2 ) ;
    DataStack_Push ( ( int64 ) prod ) ;
}

void
BigFloat_Divide ( )
{
    mpf_t *quotient = _BigFloat_New ( 0 ) ;
    mpf_t * denominator = ( mpf_t* ) DataStack_Pop ( ), *numerator = ( mpf_t* ) DataStack_Pop ( ) ;
    mpf_div ( *quotient, *numerator, *denominator ) ;
    DataStack_Push ( ( int64 ) quotient ) ;
}

void
BigFloat_Power ( )
{
    // void mpf_pow_ui (mpf_t rop, mpf_t op1, unsigned long int op2)
    // unsigned long mpf_get_ui (mpf_t op)
    mpf_t *rop = _BigFloat_New ( 0 ) ;
    mpf_t * expf = ( mpf_t* ) DataStack_Pop ( ) ;  
    int64 exp = mpf_get_ui ( *expf ) ;
    mpf_t * op1 =  ( mpf_t* ) DataStack_Pop ( ) ;  
    mpf_pow_ui ( *rop, *op1, exp ) ;
    DataStack_Push ( ( int64 ) rop ) ;
}

void
BigFloat_Subtract ( )
{
    mpf_t *diff = _BigFloat_New ( 0 ) ;
    mpf_t * op2 = ( mpf_t* ) DataStack_Pop ( ), *op1 = ( mpf_t* ) DataStack_Pop ( ) ;
    mpf_sub ( *diff, *op1, *op2 ) ; // diff = op1 - op2
    DataStack_Push ( ( int64 ) diff ) ;
}

int64
BigFloat_Cmp ( )
{
    mpf_t * op2 = ( mpf_t* ) DataStack_Pop ( ), *op1 = ( mpf_t* ) DataStack_Pop ( ) ;
    return mpf_cmp ( *op1, *op2 ) ;
    //_Compiler_Setup_tttn ( _Context->Compiler0, LE, N ) ;
    //return value ;
}

void
BigFloat_Equal ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigFloat_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigFloat_Cmp ( ) == 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigFloat_Cmp ( ) == 0 ? 1 : 0 ) ;
#endif
}

void
BigFloat_LessThanOrEqual ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigFloat_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigFloat_Cmp ( ) > 0 ? 0 : 1 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigFloat_Cmp ( ) > 0 ? 0 : 1 ) ;
#endif
}

void
BigFloat_GreaterThan ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigFloat_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigFloat_Cmp ( ) > 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigFloat_Cmp ( ) > 0 ? 1 : 0 ) ;
#endif
}

void
BigFloat_GreaterThanOrEqual ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigFloat_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigFloat_Cmp ( ) < 0 ? 0 : 1 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigFloat_Cmp ( ) < 0 ? 0 : 1 ) ;
#endif
}

void
BigFloat_LessThan ( )
{
#if USE_ZF_TO_PASS_LOGIC
    BigFloat_Cmp ( ) ;
#elif USE_AL_TO_PASS_LOGIC
    return BigFloat_Cmp ( ) < 0 ? 1 : 0 ;
#elif USE_TOS_TO_PASS_BIGNUM_LOGIC
    DataStack_Push ( BigFloat_Cmp ( ) < 0 ? 1 : 0 ) ;
#endif
}

