#include "forth.h" 
extern struct word docolcomma_word ;

int
main ( void )
{
    extern struct word dp0_word, sp0_word, rp0_word, SP_word, RP_word,
        limit_word, latest0_word, turnkey_word ;
    static cell data_stack[110] ;
    static cell return_stack[256] ;
    static cell dictionary[19000] ;
    xt_t *IP = 0, xt = & turnkey_word ;

    sp0_word.param[0] = ( cell ) ( &data_stack[100] ) ;
    rp0_word.param[0] = ( cell ) ( &return_stack[256] ) ;
    dp0_word.param[0] = ( cell ) dictionary ;
    limit_word.param[0] = ( cell ) dictionary + sizeof dictionary ;
    latest0_word.param[0] = ( cell ) ( &turnkey_word ) ;

    SP_word.param[0] = sp0_word.param[0] ;
    RP_word.param[0] = rp0_word.param[0] ;

    for ( ; ; )
    {
        EXECUTE ( xt ) ;
        xt = NEXT_XT ;
    }

    return 0 ;

}

xt_t * REGPARM
exit_code ( xt_t *IP, xt_t word )
{
    IP = RPOP ( xt_t * ) ;
    return IP ;
}

xt_t * REGPARM
dodoes_code ( xt_t *IP, xt_t word )
{
    PUSH ( word->param ) ;
    RPUSH ( IP ) ;
    IP = ( xt_t * ) ( word->does ) ;
    return IP ;
}

xt_t * REGPARM
zerobranch_code ( xt_t *IP, xt_t word )
{
    xt_t *addr = * ( xt_t ** ) IP ;
    cell x = POP ( cell ) ;
    if ( ! x )
        IP = addr ;
    else
        IP ++ ;
    return IP ;
}

xt_t * REGPARM
_literal__code ( xt_t *IP, xt_t word )
{
    PUSH ( *( cell * ) IP ) ;
    IP ++ ;
    return IP ;
}

xt_t * REGPARM
store_code ( xt_t *IP, xt_t word )
{
    cell *addr = POP ( cell * ) ;
    cell x = POP ( cell ) ;
    *addr = x ;
    return IP ;
}

xt_t * REGPARM
fetch_code ( xt_t *IP, xt_t word )
{
    cell *addr = POP ( cell * ) ;
    PUSH ( *addr ) ;
    return IP ;
}

xt_t * REGPARM
plus_code ( xt_t *IP, xt_t word )
{
    cell y = POP ( cell ) ;
    cell x = POP ( cell ) ;
    PUSH ( x + y ) ;
    return IP ;
}

xt_t * REGPARM
greaterr_code ( xt_t *IP, xt_t word )
{
    cell x = POP ( cell ) ;
    RPUSH ( x ) ;
    return IP ;
}

xt_t * REGPARM
rto_code ( xt_t *IP, xt_t word )
{
    cell x = RPOP ( cell ) ;
    PUSH ( x ) ;
    return IP ;
}

xt_t * REGPARM
nand_code ( xt_t *IP, xt_t word )
{
    cell y = POP ( cell ) ;
    cell x = POP ( cell ) ;
    PUSH ( ~ ( x & y ) ) ;
    return IP ;
}

xt_t * REGPARM
cstore_code ( xt_t *IP, xt_t word )
{
    char_t *addr = POP ( char_t * ) ;
    cell c = POP ( cell ) ;
    *addr = c ;
    return IP ;
}

xt_t * REGPARM
cfetch_code ( xt_t *IP, xt_t word )
{
    uchar_t *addr = POP ( uchar_t * ) ;
    PUSH ( *addr ) ;
    return IP ;
}

xt_t * REGPARM
emit_code ( xt_t *IP, xt_t word )
{
    cell c = POP ( cell ) ;
    putchar ( c ) ;
    return IP ;
}

xt_t * REGPARM
bye_code ( xt_t *IP, xt_t word )
{
    exit ( 0 ) ;
    return IP ;
}

xt_t * REGPARM
close_file_code ( xt_t *IP, xt_t word )
{
    FILE *fileid = POP ( FILE * ) ;
    PUSH ( fclose ( fileid ) == 0 ? 0 : errno ) ;
    return IP ;
}

xt_t * REGPARM
open_file_code ( xt_t *IP, xt_t word )
{
    char *mode = POP ( char * ) ;
    int i, u = POP ( cell ) ;
    char_t *addr = POP ( char_t * ) ;
    char *name = malloc ( u + 1 ) ;
    FILE *fileid ;

    for ( i = 0 ; i < u ; i ++ )
        name[i] = * addr ++ ;
    name[i] = 0 ;
    fileid = fopen ( name, mode ) ;
    free ( name ) ;
    PUSH ( fileid ) ;
    PUSH ( fileid == 0 ? errno : 0 ) ;
    return IP ;
}

xt_t * REGPARM
read_file_code ( xt_t *IP, xt_t word )
{
    static char buffer[1024] ;
    FILE *fileid = POP ( FILE * ) ;
    cell u1 = POP ( cell ) ;
    char_t *addr = POP ( char_t * ) ;
    size_t u2 ;
    int i ;

    if ( fileid == 0 )
        fileid = stdin ;
    if ( u1 > sizeof buffer )
        u1 = sizeof buffer ;
    u2 = fread ( buffer, 1, u1, fileid ) ;
    for ( i = 0 ; i < u2 ; i ++ )
        addr[i] = buffer[i] ;
    PUSH ( u2 ) ;
    PUSH ( ferror ( fileid ) ? errno : 0 ) ;
    return IP ;
}
extern struct word less_word ;
struct word cold_word = { 4, "cold", 0, 0, ( code_t * ) main,
    {
    } } ;
struct word exit_word = { 4, "exit", &cold_word, 0, exit_code, { } } ;
struct word dodoes_word = { 6, "dodoes", &exit_word, 0, dodoes_code, { } } ;
struct word zerobranch_word = { 7, "0branch", &dodoes_word, 0, zerobranch_code, { } } ;
struct word _literal__word = { 9, "(literal)", &zerobranch_word, 0, _literal__code, { } } ;
struct word store_word = { 1, "!", &_literal__word, 0, store_code, { } } ;
struct word fetch_word = { 1, "@", &store_word, 0, fetch_code, { } } ;
struct word plus_word = { 1, "+", &fetch_word, 0, plus_code, { } } ;
struct word greaterr_word = { 2, ">r", &plus_word, 0, greaterr_code, { } } ;
struct word rto_word = { 2, "r>", &greaterr_word, 0, rto_code, { } } ;
struct word nand_word = { 4, "nand", &rto_word, 0, nand_code, { } } ;
struct word cstore_word = { 2, "c!", &nand_word, 0, cstore_code, { } } ;
struct word cfetch_word = { 2, "c@", &cstore_word, 0, cfetch_code, { } } ;
struct word emit_word = { 4, "emit", &cfetch_word, 0, emit_code, { } } ;
struct word bye_word = { 3, "bye", &emit_word, 0, bye_code, { } } ;
struct word close_file_word = { 10, "close-file", &bye_word, 0, close_file_code, { } } ;
struct word open_file_word = { 9, "open-file", &close_file_word, 0, open_file_code, { } } ;
struct word read_file_word = { 9, "read-file", &open_file_word, 0, read_file_code, { } } ;
struct word noop_word = { 4, "noop", &read_file_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & exit_word,
    } } ;
struct word SP_word = { 2, "SP", &noop_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word RP_word = { 2, "RP", &SP_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word cell_word = { 4, "cell", &RP_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & exit_word,
    } } ;
struct word cellplus_word = { 5, "cell+", &cell_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word spfetch_word = { 3, "sp@", &cellplus_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & SP_word,
        ( cell ) & fetch_word,
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word spstore_word = { 3, "sp!", &spfetch_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & SP_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word rpfetch_word = { 3, "rp@", &spstore_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & RP_word,
        ( cell ) & fetch_word,
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word temp_word = { 4, "temp", &rpfetch_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word drop_word = { 4, "drop", &temp_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & temp_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word twodrop_word = { 5, "2drop", &drop_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & drop_word,
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word threedrop_word = { 5, "3drop", &twodrop_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twodrop_word,
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word rfetch_word = { 2, "r@", &threedrop_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rpfetch_word,
        ( cell ) & cellplus_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word swap_word = { 4, "swap", &rfetch_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & temp_word,
        ( cell ) & store_word,
        ( cell ) & rto_word,
        ( cell ) & temp_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word over_word = { 4, "over", &swap_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & greaterr_word,
        ( cell ) & rfetch_word,
        ( cell ) & rto_word,
        ( cell ) & temp_word,
        ( cell ) & store_word,
        ( cell ) & rto_word,
        ( cell ) & temp_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word rot_word = { 3, "rot", &over_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & swap_word,
        ( cell ) & rto_word,
        ( cell ) & swap_word,
        ( cell ) & exit_word,
    } } ;
struct word twotor_word = { 3, "2>r", &rot_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & swap_word,
        ( cell ) & rot_word,
        ( cell ) & greaterr_word,
        ( cell ) & greaterr_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word tworto_word = { 3, "2r>", &twotor_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & rto_word,
        ( cell ) & rto_word,
        ( cell ) & rot_word,
        ( cell ) & greaterr_word,
        ( cell ) & swap_word,
        ( cell ) & exit_word,
    } } ;
struct word dup_word = { 3, "dup", &tworto_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & spfetch_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word twodup_word = { 4, "2dup", &dup_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & over_word,
        ( cell ) & over_word,
        ( cell ) & exit_word,
    } } ;
struct word threedup_word = { 4, "3dup", &twodup_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & greaterr_word,
        ( cell ) & rfetch_word,
        ( cell ) & over_word,
        ( cell ) & tworto_word,
        ( cell ) & over_word,
        ( cell ) & greaterr_word,
        ( cell ) & rot_word,
        ( cell ) & swap_word,
        ( cell ) & rto_word,
        ( cell ) & exit_word,
    } } ;
struct word questiondup_word = { 4, "?dup", &threedup_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & questiondup_word.param[4],
        ( cell ) & dup_word,
        ( cell ) & exit_word,
    } } ;
struct word nip_word = { 3, "nip", &questiondup_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & swap_word,
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word invert_word = { 6, "invert", &nip_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & nand_word,
        ( cell ) & exit_word,
    } } ;
struct word negate_word = { 6, "negate", &invert_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & invert_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word minus_word = { 1, "-", &negate_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & negate_word,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word branch_word = { 6, "branch", &minus_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & fetch_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word _plusloop__word = { 7, "(+loop)", &branch_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & swap_word,
        ( cell ) & rto_word,
        ( cell ) & plus_word,
        ( cell ) & rfetch_word,
        ( cell ) & over_word,
        ( cell ) & greaterr_word,
        ( cell ) & less_word,
        ( cell ) & invert_word,
        ( cell ) & swap_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word unloop_word = { 6, "unloop", &_plusloop__word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & tworto_word,
        ( cell ) & twodrop_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word oneplus_word = { 2, "1+", &unloop_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word plusstore_word = { 2, "+!", &oneplus_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & swap_word,
        ( cell ) & over_word,
        ( cell ) & fetch_word,
        ( cell ) & plus_word,
        ( cell ) & swap_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word zeroequal_word = { 2, "0=", &plusstore_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & zerobranch_word,
        ( cell ) & zeroequal_word.param[6],
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & branch_word,
        ( cell ) & zeroequal_word.param[8],
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & exit_word,
    } } ;
struct word equal_word = { 1, "=", &zeroequal_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & minus_word,
        ( cell ) & zeroequal_word,
        ( cell ) & exit_word,
    } } ;
struct word lessto_word = { 2, "<>", &equal_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & equal_word,
        ( cell ) & zeroequal_word,
        ( cell ) & exit_word,
    } } ;
struct word min_word = { 3, "min", &lessto_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twodup_word,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & min_word.param[7],
        ( cell ) & drop_word,
        ( cell ) & branch_word,
        ( cell ) & min_word.param[8],
        ( cell ) & nip_word,
        ( cell ) & exit_word,
    } } ;
struct word bounds_word = { 6, "bounds", &min_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & over_word,
        ( cell ) & plus_word,
        ( cell ) & swap_word,
        ( cell ) & exit_word,
    } } ;
struct word count_word = { 5, "count", &bounds_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & oneplus_word,
        ( cell ) & swap_word,
        ( cell ) & cfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word aligned_word = { 7, "aligned", &count_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & plus_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & minus_word,
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & negate_word,
        ( cell ) & nand_word,
        ( cell ) & invert_word,
        ( cell ) & exit_word,
    } } ;
struct word _sliteral__word = { 10, "(sliteral)", &aligned_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & dup_word,
        ( cell ) & fetch_word,
        ( cell ) & swap_word,
        ( cell ) & cellplus_word,
        ( cell ) & twodup_word,
        ( cell ) & plus_word,
        ( cell ) & aligned_word,
        ( cell ) & greaterr_word,
        ( cell ) & swap_word,
        ( cell ) & exit_word,
    } } ;
struct word i_word = { 1, "i", &_sliteral__word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & rfetch_word,
        ( cell ) & swap_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word cr_word = { 2, "cr", &i_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 10U,
        ( cell ) & emit_word,
        ( cell ) & exit_word,
    } } ;
struct word type_word = { 4, "type", &cr_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & type_word.param[16],
        ( cell ) & bounds_word,
        ( cell ) & twotor_word,
        ( cell ) & i_word,
        ( cell ) & cfetch_word,
        ( cell ) & emit_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & _plusloop__word,
        ( cell ) & zerobranch_word,
        ( cell ) & type_word.param[5],
        ( cell ) & unloop_word,
        ( cell ) & branch_word,
        ( cell ) & type_word.param[17],
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word execute_word = { 7, "execute", &type_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & execute_word.param[3],
        ( cell ) & store_word,
        ( cell ) & noop_word,
        ( cell ) & exit_word,
    } } ;
struct word perform_word = { 7, "perform", &execute_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & fetch_word,
        ( cell ) & execute_word,
        ( cell ) & exit_word,
    } } ;
struct word state_word = { 5, "state", &perform_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word zerofrom_word = { 2, "0<", &state_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 9223372036854775808U,
        ( cell ) & nand_word,
        ( cell ) & invert_word,
        ( cell ) & zerobranch_word,
        ( cell ) & zerofrom_word.param[10],
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & branch_word,
        ( cell ) & zerofrom_word.param[12],
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
    } } ;
struct word or_word = { 2, "or", &zerofrom_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & invert_word,
        ( cell ) & swap_word,
        ( cell ) & invert_word,
        ( cell ) & nand_word,
        ( cell ) & exit_word,
    } } ;
struct word xor_word = { 3, "xor", &or_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twodup_word,
        ( cell ) & nand_word,
        ( cell ) & oneplus_word,
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & plus_word,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word less_word = { 1, "<", &xor_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twodup_word,
        ( cell ) & xor_word,
        ( cell ) & zerofrom_word,
        ( cell ) & zerobranch_word,
        ( cell ) & less_word.param[9],
        ( cell ) & drop_word,
        ( cell ) & zerofrom_word,
        ( cell ) & branch_word,
        ( cell ) & less_word.param[11],
        ( cell ) & minus_word,
        ( cell ) & zerofrom_word,
        ( cell ) & exit_word,
    } } ;
struct word cmove_word = { 5, "cmove", &less_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & cmove_word.param[17],
        ( cell ) & bounds_word,
        ( cell ) & twotor_word,
        ( cell ) & count_word,
        ( cell ) & i_word,
        ( cell ) & cstore_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & _plusloop__word,
        ( cell ) & zerobranch_word,
        ( cell ) & cmove_word.param[5],
        ( cell ) & unloop_word,
        ( cell ) & drop_word,
        ( cell ) & branch_word,
        ( cell ) & cmove_word.param[18],
        ( cell ) & twodrop_word,
        ( cell ) & exit_word,
    } } ;
struct word cabs_word = { 4, "cabs", &cmove_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 127U,
        ( cell ) & over_word,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & cabs_word.param[10],
        ( cell ) & _literal__word,
        ( cell ) 256U,
        ( cell ) & swap_word,
        ( cell ) & minus_word,
        ( cell ) & exit_word,
    } } ;
struct word latest_word = { 6, "latest", &cabs_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word latestxt_word = { 8, "latestxt", &latest_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word dp_word = { 2, "dp", &latestxt_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word here_word = { 4, "here", &dp_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dp_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word allot_word = { 5, "allot", &here_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dp_word,
        ( cell ) & plusstore_word,
        ( cell ) & exit_word,
    } } ;
struct word align_word = { 5, "align", &allot_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dp_word,
        ( cell ) & fetch_word,
        ( cell ) & aligned_word,
        ( cell ) & dp_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word comma_word = { 1, ",", &align_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & here_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & allot_word,
        ( cell ) & exit_word,
    } } ;
struct word ccomma_word = { 2, "c,", &comma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & here_word,
        ( cell ) & cstore_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & allot_word,
        ( cell ) & exit_word,
    } } ;
struct word movecomma_word = { 5, "move,", &ccomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & here_word,
        ( cell ) & swap_word,
        ( cell ) & dup_word,
        ( cell ) & allot_word,
        ( cell ) & cmove_word,
        ( cell ) & exit_word,
    } } ;
struct word quotecomma_word = { 2, "\",", &movecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & movecomma_word,
        ( cell ) & align_word,
        ( cell ) & exit_word,
    } } ;
struct word greaterlfa_word = { 4, ">lfa", &quotecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 16U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word greatercode_word = { 5, ">code", &greaterlfa_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 32U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word greaterbody_word = { 5, ">body", &greatercode_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 40U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word greaternextxt_word = { 7, ">nextxt", &greaterbody_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterlfa_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word greaterdoes_word = { 5, ">does", &greaternextxt_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 24U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word codestore_word = { 5, "code!", &greaterdoes_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & latestxt_word,
        ( cell ) & greatercode_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word doesstore_word = { 5, "does!", &codestore_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) dodoes_code,
        ( cell ) & codestore_word,
        ( cell ) & latestxt_word,
        ( cell ) & greaterdoes_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word doescomma_word = { 5, "does,", &doesstore_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word questioncodecomma_word = { 6, "?code,", &doescomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & here_word,
        ( cell ) & cellplus_word,
        ( cell ) & comma_word,
        ( cell ) & exit_word,
    } } ;
struct word compilecomma_word = { 8, "compile,", &questioncodecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & comma_word,
        ( cell ) & exit_word,
    } } ;
struct word current_word = { 7, "current", &compilecomma_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word chaincomma_word = { 6, "chain,", &current_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & fetch_word,
        ( cell ) & comma_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word lateststore_word = { 7, "latest!", &chaincomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & latest_word,
        ( cell ) & greaterbody_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & latestxt_word,
        ( cell ) & greaterbody_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word linkcomma_word = { 5, "link,", &lateststore_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & lateststore_word,
        ( cell ) & current_word,
        ( cell ) & fetch_word,
        ( cell ) & greaterbody_word,
        ( cell ) & fetch_word,
        ( cell ) & comma_word,
        ( cell ) & exit_word,
    } } ;
struct word reveal_word = { 6, "reveal", &linkcomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & latest_word,
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & reveal_word.param[8],
        ( cell ) & current_word,
        ( cell ) & fetch_word,
        ( cell ) & greaterbody_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word cells_word = { 5, "cells", &reveal_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word codecomma_word = { 5, "code,", &cells_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & codestore_word,
        ( cell ) & _literal__word,
        ( cell ) 8U,
        ( cell ) & allot_word,
        ( cell ) & exit_word,
    } } ;
struct word _doesto__word = { 7, "(does>)", &codecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & doesstore_word,
        ( cell ) & exit_word,
    } } ;
struct word stdin_word = { 5, "stdin", &_doesto__word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word io_init_word = { 7, "io-init", &stdin_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & exit_word,
    } } ;
struct word rslasho_word = { 3, "r/o", &io_init_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _sliteral__word,
        ( cell ) 1U,
        ( cell ) 114U,
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word docolcomma_word = { 6, "docol,", &rslasho_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) dodoes_code,
        ( cell ) & comma_word,
        ( cell ) & _doesto__word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word dovarcomma_word = { 6, "dovar,", &docolcomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) dodoes_code,
        ( cell ) & comma_word,
        ( cell ) & _doesto__word,
        ( cell ) & exit_word,
    } } ;
struct word doconcomma_word = { 6, "docon,", &dovarcomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) dodoes_code,
        ( cell ) & comma_word,
        ( cell ) & _doesto__word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word dodefcomma_word = { 6, "dodef,", &doconcomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) dodoes_code,
        ( cell ) & comma_word,
        ( cell ) & _doesto__word,
        ( cell ) & perform_word,
        ( cell ) & exit_word,
    } } ;
struct word numbername_word = { 5, "#name", &dodefcomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 16U,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & minus_word,
        ( cell ) & exit_word,
    } } ;
struct word namecomma_word = { 5, "name,", &numbername_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & numbername_word,
        ( cell ) & min_word,
        ( cell ) & ccomma_word,
        ( cell ) & numbername_word,
        ( cell ) & quotecomma_word,
        ( cell ) & exit_word,
    } } ;
struct word headercomma_word = { 7, "header,", &namecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & align_word,
        ( cell ) & here_word,
        ( cell ) & greaterr_word,
        ( cell ) & namecomma_word,
        ( cell ) & rto_word,
        ( cell ) & linkcomma_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & comma_word,
        ( cell ) & exit_word,
    } } ;
struct word greaternfa_word = { 4, ">nfa", &headercomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & exit_word,
    } } ;
struct word greatername_word = { 5, ">name", &greaternfa_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaternfa_word,
        ( cell ) & count_word,
        ( cell ) & cabs_word,
        ( cell ) & exit_word,
    } } ;
struct word noheadercomma_word = { 9, "noheader,", &greatername_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _sliteral__word,
        ( cell ) 0U,
        ( cell ) & headercomma_word,
        ( cell ) & exit_word,
    } } ;
struct word sysdir_word = { 6, "sysdir", &noheadercomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _sliteral__word,
        ( cell ) 4U,
        ( cell ) 795046515U,
        ( cell ) & exit_word,
    } } ;
struct word lowercasequestion_word = { 10, "lowercase?", &sysdir_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & _literal__word,
        ( cell ) 97U,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & lowercasequestion_word.param[10],
        ( cell ) & drop_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
        ( cell ) & _literal__word,
        ( cell ) 123U,
        ( cell ) & less_word,
        ( cell ) & exit_word,
    } } ;
struct word upcase_word = { 6, "upcase", &lowercasequestion_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & dup_word,
        ( cell ) & lowercasequestion_word,
        ( cell ) & zerobranch_word,
        ( cell ) & upcase_word.param[7],
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551584U,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word cfromto_word = { 3, "c<>", &upcase_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & upcase_word,
        ( cell ) & swap_word,
        ( cell ) & upcase_word,
        ( cell ) & lessto_word,
        ( cell ) & exit_word,
    } } ;
struct word nameequal_word = { 5, "name=", &cfromto_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twotor_word,
        ( cell ) & rfetch_word,
        ( cell ) & lessto_word,
        ( cell ) & tworto_word,
        ( cell ) & rot_word,
        ( cell ) & zerobranch_word,
        ( cell ) & nameequal_word.param[11],
        ( cell ) & threedrop_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
        ( cell ) & bounds_word,
        ( cell ) & twotor_word,
        ( cell ) & dup_word,
        ( cell ) & cfetch_word,
        ( cell ) & i_word,
        ( cell ) & cfetch_word,
        ( cell ) & cfromto_word,
        ( cell ) & zerobranch_word,
        ( cell ) & nameequal_word.param[25],
        ( cell ) & drop_word,
        ( cell ) & unloop_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
        ( cell ) & oneplus_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & _plusloop__word,
        ( cell ) & zerobranch_word,
        ( cell ) & nameequal_word.param[13],
        ( cell ) & unloop_word,
        ( cell ) & drop_word,
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & exit_word,
    } } ;
struct word ntequal_word = { 3, "nt=", &nameequal_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greatername_word,
        ( cell ) & nameequal_word,
        ( cell ) & exit_word,
    } } ;
struct word immediatequestion_word = { 10, "immediate?", &ntequal_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaternfa_word,
        ( cell ) & cfetch_word,
        ( cell ) & _literal__word,
        ( cell ) 127U,
        ( cell ) & swap_word,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & immediatequestion_word.param[12],
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & branch_word,
        ( cell ) & immediatequestion_word.param[14],
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & exit_word,
    } } ;
struct word traverse_wordli_word = { 15, "traverse-wordli", &immediatequestion_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & greaterbody_word,
        ( cell ) & fetch_word,
        ( cell ) & dup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & traverse_wordli_word.param[17],
        ( cell ) & rfetch_word,
        ( cell ) & over_word,
        ( cell ) & greaterr_word,
        ( cell ) & execute_word,
        ( cell ) & rto_word,
        ( cell ) & swap_word,
        ( cell ) & zerobranch_word,
        ( cell ) & traverse_wordli_word.param[17],
        ( cell ) & greaternextxt_word,
        ( cell ) & branch_word,
        ( cell ) & traverse_wordli_word.param[3],
        ( cell ) & rto_word,
        ( cell ) & twodrop_word,
        ( cell ) & exit_word,
    } } ;
struct word questionnttoxt_word = { 6, "?nt>xt", &traverse_wordli_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & threedup_word,
        ( cell ) & ntequal_word,
        ( cell ) & zerobranch_word,
        ( cell ) & questionnttoxt_word.param[15],
        ( cell ) & greaterr_word,
        ( cell ) & threedrop_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & rto_word,
        ( cell ) & dup_word,
        ( cell ) & immediatequestion_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & branch_word,
        ( cell ) & questionnttoxt_word.param[18],
        ( cell ) & drop_word,
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & exit_word,
    } } ;
struct word _find__word = { 6, "(find)", &questionnttoxt_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twotor_word,
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & swap_word,
        ( cell ) & tworto_word,
        ( cell ) & _literal__word,
        ( cell ) & questionnttoxt_word,
        ( cell ) & traverse_wordli_word,
        ( cell ) & rot_word,
        ( cell ) & zerobranch_word,
        ( cell ) & _find__word.param[13],
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
    } } ;
struct word abort_word = { 5, "abort", &_find__word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word _abortquote__word = { 8, "(abort\")", &abort_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word undef_word = { 5, "undef", &_abortquote__word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _sliteral__word,
        ( cell ) 11U,
        ( cell ) 7308895133777555029U,
        ( cell ) 2112100U,
        ( cell ) & type_word,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & abort_word,
        ( cell ) & exit_word,
    } } ;
struct word questionundef_word = { 6, "?undef", &undef_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & zerobranch_word,
        ( cell ) & questionundef_word.param[3],
        ( cell ) & undef_word,
        ( cell ) & exit_word,
    } } ;
struct word literal_word = { 249, "literal", &questionundef_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & _literal__word,
        ( cell ) & compilecomma_word,
        ( cell ) & comma_word,
        ( cell ) & exit_word,
    } } ;
struct word questionliteral_word = { 8, "?literal", &literal_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & state_word,
        ( cell ) & fetch_word,
        ( cell ) & zerobranch_word,
        ( cell ) & questionliteral_word.param[5],
        ( cell ) & literal_word,
        ( cell ) & exit_word,
    } } ;
struct word number_word = { 6, "number", &questionliteral_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word _number__word = { 8, "(number)", &number_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & over_word,
        ( cell ) & cfetch_word,
        ( cell ) & _literal__word,
        ( cell ) 45U,
        ( cell ) & equal_word,
        ( cell ) & dup_word,
        ( cell ) & greaterr_word,
        ( cell ) & zerobranch_word,
        ( cell ) & _number__word.param[15],
        ( cell ) & swap_word,
        ( cell ) & oneplus_word,
        ( cell ) & swap_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & minus_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & rot_word,
        ( cell ) & rot_word,
        ( cell ) & dup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & _number__word.param[60],
        ( cell ) & over_word,
        ( cell ) & cfetch_word,
        ( cell ) & _literal__word,
        ( cell ) 48U,
        ( cell ) & minus_word,
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & over_word,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & _number__word.param[59],
        ( cell ) & dup_word,
        ( cell ) & _literal__word,
        ( cell ) 10U,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & _number__word.param[59],
        ( cell ) & twotor_word,
        ( cell ) & oneplus_word,
        ( cell ) & swap_word,
        ( cell ) & dup_word,
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & plus_word,
        ( cell ) & dup_word,
        ( cell ) & plus_word,
        ( cell ) & rto_word,
        ( cell ) & plus_word,
        ( cell ) & swap_word,
        ( cell ) & rto_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & minus_word,
        ( cell ) & branch_word,
        ( cell ) & _number__word.param[19],
        ( cell ) & drop_word,
        ( cell ) & questiondup_word,
        ( cell ) & questionundef_word,
        ( cell ) & drop_word,
        ( cell ) & rto_word,
        ( cell ) & zerobranch_word,
        ( cell ) & _number__word.param[67],
        ( cell ) & negate_word,
        ( cell ) & questionliteral_word,
        ( cell ) & exit_word,
    } } ;
struct word greaterin_word = { 3, ">in", &_number__word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word input_word = { 5, "input", &greaterin_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word inputfetch_word = { 6, "input@", &input_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & cells_word,
        ( cell ) & input_word,
        ( cell ) & fetch_word,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word ticksource_word = { 7, "'source", &inputfetch_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & inputfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word numbersource_word = { 7, "#source", &ticksource_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & inputfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word sourcenumber_word = { 7, "source#", &numbersource_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 2U,
        ( cell ) & inputfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word tickrefill_word = { 7, "'refill", &sourcenumber_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 3U,
        ( cell ) & inputfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word tickprompt_word = { 7, "'prompt", &tickrefill_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 4U,
        ( cell ) & inputfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word sourceto_word = { 7, "source>", &tickprompt_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 5U,
        ( cell ) & inputfetch_word,
        ( cell ) & exit_word,
    } } ;
struct word slashinput_source_word = { 13, "/input-source", &sourceto_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 48U,
    } } ;
struct word forth_word = { 5, "forth", &slashinput_source_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
        ( cell ) 0U,
    } } ;
struct word compiler_words_word = { 14, "compiler-words", &forth_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
        ( cell ) 0U,
    } } ;
struct word search_paths_word = { 12, "search-paths", &compiler_words_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
        ( cell ) 0U,
    } } ;
struct word included_files_word = { 14, "included-files", &search_paths_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
        ( cell ) 0U,
    } } ;
struct word context_word = { 7, "context", &included_files_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
        ( cell ) 0U,
    } } ;
struct word rfetchplus_word = { 3, "r@+", &context_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & rto_word,
        ( cell ) & dup_word,
        ( cell ) & cellplus_word,
        ( cell ) & greaterr_word,
        ( cell ) & fetch_word,
        ( cell ) & swap_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word search_context_word = { 14, "search-context", &rfetchplus_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & rfetchplus_word,
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & search_context_word.param[11],
        ( cell ) & _find__word,
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & search_context_word.param[1],
        ( cell ) & branch_word,
        ( cell ) & search_context_word.param[14],
        ( cell ) & drop_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & rto_word,
        ( cell ) & drop_word,
        ( cell ) & exit_word,
    } } ;
struct word find_name_word = { 9, "find-name", &search_context_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & swap_word,
        ( cell ) & over_word,
        ( cell ) & numbername_word,
        ( cell ) & min_word,
        ( cell ) & context_word,
        ( cell ) & search_context_word,
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & find_name_word.param[13],
        ( cell ) & rot_word,
        ( cell ) & drop_word,
        ( cell ) & branch_word,
        ( cell ) & find_name_word.param[16],
        ( cell ) & swap_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
    } } ;
struct word source_word = { 6, "source", &find_name_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & ticksource_word,
        ( cell ) & fetch_word,
        ( cell ) & numbersource_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word sourcequestion_word = { 7, "source?", &source_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterin_word,
        ( cell ) & fetch_word,
        ( cell ) & source_word,
        ( cell ) & nip_word,
        ( cell ) & less_word,
        ( cell ) & exit_word,
    } } ;
struct word lesssource_word = { 7, "<source", &sourcequestion_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & source_word,
        ( cell ) & greaterin_word,
        ( cell ) & fetch_word,
        ( cell ) & dup_word,
        ( cell ) & rot_word,
        ( cell ) & equal_word,
        ( cell ) & zerobranch_word,
        ( cell ) & lesssource_word.param[13],
        ( cell ) & twodrop_word,
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & branch_word,
        ( cell ) & lesssource_word.param[19],
        ( cell ) & plus_word,
        ( cell ) & cfetch_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & greaterin_word,
        ( cell ) & plusstore_word,
        ( cell ) & exit_word,
    } } ;
struct word blankquestion_word = { 6, "blank?", &lesssource_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 33U,
        ( cell ) & less_word,
        ( cell ) & exit_word,
    } } ;
struct word skip_word = { 4, "skip", &blankquestion_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & sourcequestion_word,
        ( cell ) & zerobranch_word,
        ( cell ) & skip_word.param[12],
        ( cell ) & lesssource_word,
        ( cell ) & blankquestion_word,
        ( cell ) & zeroequal_word,
        ( cell ) & zerobranch_word,
        ( cell ) & skip_word.param[0],
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & greaterin_word,
        ( cell ) & plusstore_word,
        ( cell ) & exit_word,
    } } ;
struct word parse_name_word = { 10, "parse-name", &skip_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & skip_word,
        ( cell ) & source_word,
        ( cell ) & drop_word,
        ( cell ) & greaterin_word,
        ( cell ) & fetch_word,
        ( cell ) & plus_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & sourcequestion_word,
        ( cell ) & zerobranch_word,
        ( cell ) & parse_name_word.param[19],
        ( cell ) & oneplus_word,
        ( cell ) & lesssource_word,
        ( cell ) & blankquestion_word,
        ( cell ) & zerobranch_word,
        ( cell ) & parse_name_word.param[8],
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & minus_word,
        ( cell ) & exit_word,
    } } ;
struct word _previous__word = { 10, "(previous)", &parse_name_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & forth_word,
        ( cell ) & context_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word also_word = { 4, "also", &_previous__word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word previous_word = { 8, "previous", &also_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word catch_word = { 5, "catch", &previous_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word interpreters_word = { 12, "interpreters", &catch_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & execute_word,
        ( cell ) & number_word,
        ( cell ) & execute_word,
    } } ;
struct word questionexception_word = { 10, "?exception", &interpreters_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & zerobranch_word,
        ( cell ) & questionexception_word.param[9],
        ( cell ) & cr_word,
        ( cell ) & _sliteral__word,
        ( cell ) 10U,
        ( cell ) 8028075836850796613U,
        ( cell ) 8558U,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & exit_word,
    } } ;
struct word interpret_xt_word = { 12, "interpret-xt", &questionexception_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & oneplus_word,
        ( cell ) & cells_word,
        ( cell ) & interpreters_word,
        ( cell ) & plus_word,
        ( cell ) & fetch_word,
        ( cell ) & catch_word,
        ( cell ) & questionexception_word,
        ( cell ) & exit_word,
    } } ;
struct word lbracket_word = { 255, "[", &interpret_xt_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & state_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & execute_word,
        ( cell ) & interpreters_word,
        ( cell ) & store_word,
        ( cell ) & previous_word,
        ( cell ) & exit_word,
    } } ;
struct word rbracket_word = { 1, "]", &lbracket_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & state_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & compilecomma_word,
        ( cell ) & interpreters_word,
        ( cell ) & store_word,
        ( cell ) & also_word,
        ( cell ) & _literal__word,
        ( cell ) & compiler_words_word,
        ( cell ) & context_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word csp_word = { 3, "csp", &rbracket_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word dotlatest_word = { 7, ".latest", &csp_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & latestxt_word,
        ( cell ) & greatername_word,
        ( cell ) & type_word,
        ( cell ) & exit_word,
    } } ;
struct word questionbad_word = { 4, "?bad", &dotlatest_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rot_word,
        ( cell ) & zerobranch_word,
        ( cell ) & questionbad_word.param[12],
        ( cell ) & type_word,
        ( cell ) & _sliteral__word,
        ( cell ) 13U,
        ( cell ) 8388357179922801696U,
        ( cell ) 138419269481U,
        ( cell ) & type_word,
        ( cell ) & dotlatest_word,
        ( cell ) & cr_word,
        ( cell ) & abort_word,
        ( cell ) & twodrop_word,
        ( cell ) & exit_word,
    } } ;
struct word storecsp_word = { 4, "!csp", &questionbad_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & csp_word,
        ( cell ) & fetch_word,
        ( cell ) & _sliteral__word,
        ( cell ) 6U,
        ( cell ) 110386908194126U,
        ( cell ) & questionbad_word,
        ( cell ) & spfetch_word,
        ( cell ) & csp_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word questioncsp_word = { 4, "?csp", &storecsp_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & spfetch_word,
        ( cell ) & csp_word,
        ( cell ) & fetch_word,
        ( cell ) & lessto_word,
        ( cell ) & _sliteral__word,
        ( cell ) 10U,
        ( cell ) 7164771175311240789U,
        ( cell ) 25701U,
        ( cell ) & questionbad_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & csp_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word semicolon_word = { 255, ";", &questioncsp_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & reveal_word,
        ( cell ) & _literal__word,
        ( cell ) & exit_word,
        ( cell ) & compilecomma_word,
        ( cell ) & lbracket_word,
        ( cell ) & questioncsp_word,
        ( cell ) & exit_word,
    } } ;
struct word refill_word = { 6, "refill", &semicolon_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & greaterin_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & numbersource_word,
        ( cell ) & store_word,
        ( cell ) & tickrefill_word,
        ( cell ) & perform_word,
        ( cell ) & exit_word,
    } } ;
struct word questionprompt_word = { 7, "?prompt", &refill_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & tickprompt_word,
        ( cell ) & perform_word,
        ( cell ) & exit_word,
    } } ;
struct word source_id_word = { 9, "source-id", &questionprompt_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & sourcenumber_word,
        ( cell ) & fetch_word,
        ( cell ) & exit_word,
    } } ;
struct word slashfile_word = { 5, "/file", &source_id_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 256U,
    } } ;
struct word file_refill_word = { 11, "file-refill", &slashfile_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & ticksource_word,
        ( cell ) & fetch_word,
        ( cell ) & slashfile_word,
        ( cell ) & bounds_word,
        ( cell ) & twotor_word,
        ( cell ) & i_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & source_id_word,
        ( cell ) & read_file_word,
        ( cell ) & zerobranch_word,
        ( cell ) & file_refill_word.param[16],
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & unloop_word,
        ( cell ) & exit_word,
        ( cell ) & zeroequal_word,
        ( cell ) & zerobranch_word,
        ( cell ) & file_refill_word.param[23],
        ( cell ) & source_word,
        ( cell ) & nip_word,
        ( cell ) & unloop_word,
        ( cell ) & exit_word,
        ( cell ) & i_word,
        ( cell ) & cfetch_word,
        ( cell ) & _literal__word,
        ( cell ) 10U,
        ( cell ) & equal_word,
        ( cell ) & zerobranch_word,
        ( cell ) & file_refill_word.param[32],
        ( cell ) & branch_word,
        ( cell ) & file_refill_word.param[41],
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & numbersource_word,
        ( cell ) & plusstore_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & _plusloop__word,
        ( cell ) & zerobranch_word,
        ( cell ) & file_refill_word.param[5],
        ( cell ) & unloop_word,
        ( cell ) & _literal__word,
        ( cell ) 18446744073709551615U,
        ( cell ) & exit_word,
    } } ;
struct word file_source_word = { 11, "file-source", &file_refill_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word save_input_word = { 10, "save-input", &file_source_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterin_word,
        ( cell ) & fetch_word,
        ( cell ) & input_word,
        ( cell ) & fetch_word,
        ( cell ) & _literal__word,
        ( cell ) 2U,
        ( cell ) & exit_word,
    } } ;
struct word restore_input_word = { 13, "restore-input", &save_input_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & drop_word,
        ( cell ) & input_word,
        ( cell ) & store_word,
        ( cell ) & greaterin_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
    } } ;
struct word backtrace_word = { 9, "backtrace", &restore_input_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word sigint_word = { 6, "sigint", &backtrace_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & cr_word,
        ( cell ) & backtrace_word,
        ( cell ) & abort_word,
        ( cell ) & exit_word,
    } } ;
struct word sp0_word = { 3, "sp0", &sigint_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word rp0_word = { 3, "rp0", &sp0_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word dp0_word = { 3, "dp0", &rp0_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word limit_word = { 5, "limit", &dp0_word, &noop_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word image0_word = { 6, "image0", &limit_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word latest0_word = { 7, "latest0", &image0_word, &dup_word.param[1], ( code_t * ) dodoes_code,
    {
        ( cell ) 0U,
    } } ;
struct word parsed_word = { 6, "parsed", &latest0_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word _parsed__word = { 8, "(parsed)", &parsed_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & find_name_word,
        ( cell ) & interpret_xt_word,
        ( cell ) & exit_word,
    } } ;
struct word questionstack_word = { 6, "?stack", &_parsed__word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & sp0_word,
        ( cell ) & spfetch_word,
        ( cell ) & cellplus_word,
        ( cell ) & less_word,
        ( cell ) & zerobranch_word,
        ( cell ) & questionstack_word.param[14],
        ( cell ) & _sliteral__word,
        ( cell ) 15U,
        ( cell ) 7959303562048140371U,
        ( cell ) 33618033594492260U,
        ( cell ) & cr_word,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & abort_word,
        ( cell ) & exit_word,
    } } ;
struct word interpret_word = { 9, "interpret", &questionstack_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & parse_name_word,
        ( cell ) & dup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & interpret_word.param[8],
        ( cell ) & parsed_word,
        ( cell ) & questionstack_word,
        ( cell ) & branch_word,
        ( cell ) & interpret_word.param[0],
        ( cell ) & twodrop_word,
        ( cell ) & exit_word,
    } } ;
struct word interpreting_word = { 12, "interpreting", &interpret_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & refill_word,
        ( cell ) & zerobranch_word,
        ( cell ) & interpreting_word.param[7],
        ( cell ) & interpret_word,
        ( cell ) & questionprompt_word,
        ( cell ) & branch_word,
        ( cell ) & interpreting_word.param[0],
        ( cell ) & exit_word,
    } } ;
struct word zerosource_word = { 7, "0source", &interpreting_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & tickprompt_word,
        ( cell ) & store_word,
        ( cell ) & tickrefill_word,
        ( cell ) & store_word,
        ( cell ) & sourcenumber_word,
        ( cell ) & store_word,
        ( cell ) & ticksource_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & sourceto_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word sourcecomma_word = { 7, "source,", &zerosource_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & input_word,
        ( cell ) & fetch_word,
        ( cell ) & greaterr_word,
        ( cell ) & here_word,
        ( cell ) & input_word,
        ( cell ) & store_word,
        ( cell ) & slashinput_source_word,
        ( cell ) & allot_word,
        ( cell ) & zerosource_word,
        ( cell ) & rto_word,
        ( cell ) & input_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word filecomma_word = { 5, "file,", &sourcecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & _literal__word,
        ( cell ) & file_refill_word,
        ( cell ) & _literal__word,
        ( cell ) & noop_word,
        ( cell ) & sourcecomma_word,
        ( cell ) & slashfile_word,
        ( cell ) & allot_word,
        ( cell ) & exit_word,
    } } ;
struct word plusfile_word = { 5, "+file", &filecomma_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & here_word,
        ( cell ) & sourceto_word,
        ( cell ) & store_word,
        ( cell ) & filecomma_word,
        ( cell ) & exit_word,
    } } ;
struct word fileto_word = { 5, "file>", &plusfile_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & sourceto_word,
        ( cell ) & fetch_word,
        ( cell ) & questiondup_word,
        ( cell ) & zerobranch_word,
        ( cell ) & fileto_word.param[9],
        ( cell ) & input_word,
        ( cell ) & store_word,
        ( cell ) & branch_word,
        ( cell ) & fileto_word.param[10],
        ( cell ) & plusfile_word,
        ( cell ) & exit_word,
    } } ;
struct word alloc_file_word = { 10, "alloc-file", &fileto_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & file_source_word,
        ( cell ) & input_word,
        ( cell ) & store_word,
        ( cell ) & ticksource_word,
        ( cell ) & fetch_word,
        ( cell ) & zerobranch_word,
        ( cell ) & alloc_file_word.param[10],
        ( cell ) & fileto_word,
        ( cell ) & branch_word,
        ( cell ) & alloc_file_word.param[3],
        ( cell ) & exit_word,
    } } ;
struct word file_input_word = { 10, "file-input", &alloc_file_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & alloc_file_word,
        ( cell ) & sourcenumber_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 6U,
        ( cell ) & inputfetch_word,
        ( cell ) & ticksource_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word include_file_word = { 12, "include-file", &file_input_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & save_input_word,
        ( cell ) & drop_word,
        ( cell ) & twotor_word,
        ( cell ) & file_input_word,
        ( cell ) & interpreting_word,
        ( cell ) & source_id_word,
        ( cell ) & close_file_word,
        ( cell ) & drop_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & ticksource_word,
        ( cell ) & store_word,
        ( cell ) & tworto_word,
        ( cell ) & _literal__word,
        ( cell ) 2U,
        ( cell ) & restore_input_word,
        ( cell ) & zerobranch_word,
        ( cell ) & include_file_word.param[27],
        ( cell ) & _sliteral__word,
        ( cell ) 17U,
        ( cell ) 8391162071565492546U,
        ( cell ) 8462385097841406575U,
        ( cell ) 116U,
        ( cell ) & cr_word,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & abort_word,
        ( cell ) & exit_word,
    } } ;
struct word plusstring_word = { 7, "+string", &include_file_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twodup_word,
        ( cell ) & twotor_word,
        ( cell ) & plus_word,
        ( cell ) & over_word,
        ( cell ) & greaterr_word,
        ( cell ) & swap_word,
        ( cell ) & cmove_word,
        ( cell ) & rto_word,
        ( cell ) & tworto_word,
        ( cell ) & rot_word,
        ( cell ) & plus_word,
        ( cell ) & exit_word,
    } } ;
struct word pathname_word = { 8, "pathname", &plusstring_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greaterr_word,
        ( cell ) & twodup_word,
        ( cell ) & rto_word,
        ( cell ) & greatername_word,
        ( cell ) & here_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & plusstring_word,
        ( cell ) & plusstring_word,
        ( cell ) & exit_word,
    } } ;
struct word questioninclude_word = { 8, "?include", &pathname_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & zerobranch_word,
        ( cell ) & questioninclude_word.param[7],
        ( cell ) & drop_word,
        ( cell ) & _literal__word,
        ( cell ) 1U,
        ( cell ) & branch_word,
        ( cell ) & questioninclude_word.param[15],
        ( cell ) & greaterr_word,
        ( cell ) & twodrop_word,
        ( cell ) & rto_word,
        ( cell ) & include_file_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
    } } ;
struct word questionopen_word = { 5, "?open", &questioninclude_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & pathname_word,
        ( cell ) & rslasho_word,
        ( cell ) & open_file_word,
        ( cell ) & questioninclude_word,
        ( cell ) & exit_word,
    } } ;
struct word questionerror_word = { 6, "?error", &questionopen_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & zerobranch_word,
        ( cell ) & questionerror_word.param[10],
        ( cell ) & _sliteral__word,
        ( cell ) 14U,
        ( cell ) 8390045716234135878U,
        ( cell ) 110425579415072U,
        ( cell ) & cr_word,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & abort_word,
        ( cell ) & exit_word,
    } } ;
struct word search_file_word = { 11, "search-file", &questionerror_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & search_paths_word,
        ( cell ) & _literal__word,
        ( cell ) & questionopen_word,
        ( cell ) & traverse_wordli_word,
        ( cell ) & questionerror_word,
        ( cell ) & exit_word,
    } } ;
struct word greatercurrent_word = { 8, ">current", &search_file_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & current_word,
        ( cell ) & fetch_word,
        ( cell ) & rto_word,
        ( cell ) & twotor_word,
        ( cell ) & current_word,
        ( cell ) & store_word,
        ( cell ) & exit_word,
    } } ;
struct word currentto_word = { 8, "current>", &greatercurrent_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & rto_word,
        ( cell ) & rto_word,
        ( cell ) & current_word,
        ( cell ) & store_word,
        ( cell ) & greaterr_word,
        ( cell ) & exit_word,
    } } ;
struct word plusname_word = { 5, "+name", &currentto_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & greatercurrent_word,
        ( cell ) & headercomma_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & comma_word,
        ( cell ) & reveal_word,
        ( cell ) & currentto_word,
        ( cell ) & exit_word,
    } } ;
struct word remember_file_word = { 13, "remember-file", &plusname_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & included_files_word,
        ( cell ) & plusname_word,
        ( cell ) & exit_word,
    } } ;
struct word included_word = { 8, "included", &remember_file_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & twodup_word,
        ( cell ) & remember_file_word,
        ( cell ) & search_file_word,
        ( cell ) & exit_word,
    } } ;
struct word searched_word = { 8, "searched", &included_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & _literal__word,
        ( cell ) & search_paths_word,
        ( cell ) & plusname_word,
        ( cell ) & exit_word,
    } } ;
struct word dummy_catch_word = { 11, "dummy-catch", &searched_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & execute_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & exit_word,
    } } ;
struct word quit_word = { 4, "quit", &dummy_catch_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & abort_word,
    } } ;
struct word warm_word = { 4, "warm", &quit_word, &docolcomma_word.param[4], ( code_t * ) dodoes_code,
    {
        ( cell ) & io_init_word,
        ( cell ) & _sliteral__word,
        ( cell ) 7U,
        ( cell ) 29401432419885676U,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & dp0_word,
        ( cell ) & dp_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & noop_word,
        ( cell ) & dup_word,
        ( cell ) & _literal__word,
        ( cell ) & backtrace_word.param[0],
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & also_word.param[0],
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & dummy_catch_word,
        ( cell ) & _literal__word,
        ( cell ) & catch_word.param[0],
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & _number__word,
        ( cell ) & _literal__word,
        ( cell ) & number_word.param[0],
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & _parsed__word,
        ( cell ) & _literal__word,
        ( cell ) & parsed_word.param[0],
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & _previous__word,
        ( cell ) & _literal__word,
        ( cell ) & previous_word.param[0],
        ( cell ) & store_word,
        ( cell ) & latest0_word,
        ( cell ) & dup_word,
        ( cell ) & _literal__word,
        ( cell ) & latestxt_word,
        ( cell ) & greaterbody_word,
        ( cell ) & store_word,
        ( cell ) & forth_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & forth_word,
        ( cell ) & current_word,
        ( cell ) & store_word,
        ( cell ) & here_word,
        ( cell ) & _literal__word,
        ( cell ) & file_source_word,
        ( cell ) & greaterbody_word,
        ( cell ) & store_word,
        ( cell ) & filecomma_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & forth_word,
        ( cell ) & cellplus_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & compiler_words_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & forth_word,
        ( cell ) & compiler_words_word,
        ( cell ) & cellplus_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & search_paths_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & compiler_words_word,
        ( cell ) & included_files_word,
        ( cell ) & cellplus_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & included_files_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & search_paths_word,
        ( cell ) & included_files_word,
        ( cell ) & cellplus_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) & forth_word,
        ( cell ) & dup_word,
        ( cell ) & context_word,
        ( cell ) & store_word,
        ( cell ) & context_word,
        ( cell ) & cellplus_word,
        ( cell ) & store_word,
        ( cell ) & _literal__word,
        ( cell ) 0U,
        ( cell ) & context_word,
        ( cell ) & _literal__word,
        ( cell ) 2U,
        ( cell ) & cells_word,
        ( cell ) & plus_word,
        ( cell ) & store_word,
        ( cell ) & _sliteral__word,
        ( cell ) 4U,
        ( cell ) 795046515U,
        ( cell ) & searched_word,
        ( cell ) & sysdir_word,
        ( cell ) & searched_word,
        ( cell ) & _sliteral__word,
        ( cell ) 0U,
        ( cell ) & searched_word,
        ( cell ) & lbracket_word,
        ( cell ) & _sliteral__word,
        ( cell ) 33U,
        ( cell ) 7165064483209180463U,
        ( cell ) 7310012246412717153U,
        ( cell ) 7526766699490733103U,
        ( cell ) 8387442364690426927U,
        ( cell ) 104U,
        ( cell ) & included_word,
        ( cell ) & _sliteral__word,
        ( cell ) 2U,
        ( cell ) 27503U,
        ( cell ) & type_word,
        ( cell ) & cr_word,
        ( cell ) & quit_word,
        ( cell ) & exit_word,
    } } ;
struct word turnkey_word = { 7, "turnkey", &warm_word, &perform_word.param[0], ( code_t * ) dodoes_code,
    {
        ( cell ) & warm_word,
    } } ;
/* Meta-OK */
