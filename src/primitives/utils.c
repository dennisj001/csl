#include "../include/csl.h"

int
get_type_process ( const char* cmd )
{
    if ( strncmp ( cmd, "export", 6 ) == 0 )
    {
        return 1 ;
    }
    else if ( strncmp ( cmd, "cd", 2 ) == 0 )
    {
        return 2 ;
    }
    else if ( strncmp ( cmd, "history", 7 ) == 0 )
    {
        return 3 ;
    }
    else if ( strncmp ( cmd, "kill", 4 ) == 0 )
    {
        return 4 ;
    }
    else if ( strncmp ( cmd, "jobs", 4 ) == 0 )
    {
        return 5 ;
    }
    else if ( strncmp ( cmd, "fg", 2 ) == 0 )
    {
        return 6 ;
    }
    else if ( strncmp ( cmd, "bg", 2 ) == 0 )
    {
        return 7 ;
    }
    else if ( strncmp ( cmd, "echo", 4 ) == 0 )
    {
        return 8 ;
    }
    else if ( strncmp ( cmd, "set", 3 ) == 0 )
    {
        return 9 ;
    }
    return 0 ;
}

//Add commands to .history

void
_add_history ( const char cmd[100], const char hpath[500] )
{
    char hist[52][100] ;
    int cnt_lines = 0 ;
    char* line = NULL ;
    size_t len = 0, sz_line ;

    FILE* file = fopen ( hpath, "r" ) ;
    if ( file )
    {
        while ( getline ( &line, &len, file ) != - 1 )
        {
            strcpy ( hist[cnt_lines], line ) ;
            sz_line = strlen ( line ) ;
            hist[cnt_lines][sz_line - 1] = '\0' ;
            cnt_lines ++ ;
        }
        free ( line ) ;
        fclose ( file ) ;

        if ( cnt_lines == 50 )
        {
            for ( int i = 0 ; i < cnt_lines - 1 ; i ++ )
                strcpy ( hist[i], hist[i + 1] ) ;
            strcpy ( hist[49], cmd ) ;
        }
        else
        {
            strcpy ( hist[cnt_lines ++], cmd ) ;
        }

        file = fopen ( hpath, "w" ) ;
        for ( int i = 0 ; i < cnt_lines ; i ++ )
        {
            fprintf ( file, "%s\n", hist[i] ) ;
        }
        fclose ( file ) ;
    }
    else
    {
        perror ( "history" ) ;
    }
}

void
whereami ( char path[100] )
{
    strcpy ( path, getenv ( "PWD" ) ) ;

    int i, j = 0, cnt = 0, len = strlen ( path ) ;

    for ( i = 0 ; i < len && cnt < 3 ; i ++ )
    {
        cnt += ( path[i] == '/' ) ;
    }

    if ( cnt < 3 ) return ;

    i -- ;

    path[j ++] = '~' ;
    for ( ; i < len ; i ++, j ++ )
    {
        path[j] = path[i] ;
    }
    path[j] = '\0' ;
}

int
parse_args ( char cmd[200], char *args[100], int t_process )
{
    char parser[50][100] ;
    memset ( parser, 0, sizeof (parser ) ) ;

    char *pch, *path ;
    char *sptr1, *sptr2 ;

    char aux[200], path_aux[200], cmd_aux[200], pg[100] ;
    FILE *file ;

    //Parse cmd
    strcpy ( cmd_aux, cmd ) ;
    pch = strtok_r ( cmd_aux, " ", &sptr2 ) ;

    int argc = 0, cod = 0 ;
    while ( pch != NULL )
    {
        if ( strncmp ( pch, ">", 1 ) == 0 )
        {
            cod = 1 ;
        }
        else if ( strncmp ( pch, "<", 1 ) == 0 )
        {
            cod = 2 ;
        }
        else if ( strncmp ( pch, "2>", 2 ) == 0 )
        {
            cod = 3 ;
        }
        else
        {
            strcpy ( parser[argc + cod], pch ) ;
            if ( cod == 0 )
            {
                argc ++ ;
            }
        }
        pch = strtok_r ( NULL, " ", &sptr2 ) ;
    }

    if ( parser[argc + 1][0] != '\0' )
    {
        file = fopen ( parser[argc + 1], "w" ) ;
        if ( file != NULL )
        {
            dup2 ( fileno ( file ), STDOUT_FILENO ) ;
            fclose ( file ) ;
        }
        else
        {
            fprintf ( stderr, "Error opening file\n" ) ;
            exit ( 1 ) ;
        }
    }

    if ( parser[argc + 2][0] != '\0' )
    {
        file = fopen ( parser[argc + 2], "r" ) ;
        if ( file != NULL )
        {
            dup2 ( fileno ( file ), STDIN_FILENO ) ;
            fclose ( file ) ;
        }
        else
        {
            fprintf ( stderr, "Error opening file\n" ) ;
            exit ( 1 ) ;
        }
    }

    if ( parser[argc + 3][0] != '\0' )
    {
        file = fopen ( parser[argc + 3], "w" ) ;
        if ( file != NULL )
        {
            dup2 ( fileno ( file ), STDERR_FILENO ) ;
            fclose ( file ) ;
        }
        else
        {
            fprintf ( stderr, "Error opening file\n" ) ;
            exit ( 1 ) ;
        }
    }

    for ( int i = 0 ; i < argc ; i ++ )
    {
        args[i] = parser[i] ;
    }
    args[argc] = ( char * ) NULL ;

    strcpy ( pg, args[0] ) ;

    if ( t_process )
    {
        return 0 ;
    }
    else if ( cmd[0] == '.' || cmd[0] == '/' )
    {

        if ( access ( args[0], F_OK ) == 0 )
        {
            return 0 ;
        }

    }
    else
    {
        strcpy ( path_aux, getenv ( "MYPATH" ) ) ;
        path = strtok_r ( path_aux, ":", &sptr1 ) ;

        while ( path != NULL )
        {

            strcpy ( aux, path ) ;
            strcat ( aux, "/" ) ;
            strcat ( aux, pg ) ;

            strcpy ( args[0], aux ) ;

            if ( access ( args[0], F_OK ) == 0 )
            {
                return 0 ;
            }

            path = strtok_r ( NULL, ":", &sptr1 ) ;
        }
    }

    strcpy ( args[0], pg ) ;
    return - 1 ;
}

void
_export ( char **args, void *shmem )
{

    char out[200], aux[200] ;
    memset ( out, 0, sizeof (out ) ) ;

    int len, j = - 1, p = 0 ;

    Boolean ok = false, qmark = false ;

    for ( int arg = 1 ; args[arg] != NULL ; arg ++ )
    {
        len = strlen ( args[arg] ) ;
        for ( int i = 0 ; i < len ; i ++ )
        {
            if ( args[arg][i] == '\"' )
            {
                qmark = ! qmark ;
                if ( ! ok ) exit ( 0 ) ;
            }
            else if ( args[arg][i] == '$' && ! qmark )
            {
                j ++ ;
                if ( ! ok ) exit ( 0 ) ;
            }
            else if ( args[arg][i] == ':' && ! qmark )
            {
                aux[j] = '\0' ;

                char *s = getenv ( aux ) ;
                if ( s != NULL )
                {
                    strcat ( out, s ) ;
                    p = strlen ( out ) ;
                    out[p ++] = ':' ;
                }
                else
                {
                    exit ( 0 ) ;
                }
                j = - 1 ;

            }
            else if ( j >= 0 && ! qmark )
            {
                aux[j ++] = args[arg][i] ;
            }
            else
            {
                out[p ++] = args[arg][i] ;
            }

            if ( args[arg][i] == '=' ) ok = true ;
        }
        if ( qmark ) out[p ++] = ' ' ;
    }


    memcpy ( shmem, out, sizeof (out ) ) ;
    exit ( 0 ) ;
}

void
_cd ( char** args, void* shmem )
{
    int len ;
    char path[500] ;

    if ( strncmp ( args[1], "..", 2 ) == 0 )
    {
        strcpy ( path, getenv ( "PWD" ) ) ;
        len = strlen ( path ) ;

        while ( path[len - 1] != '/' )
        {
            path[-- len] = '\0' ;
        }
        if ( len > 1 )
            path[-- len] = '\0' ;
    }
    else if ( args[1][0] == '/' )
    {
        strcpy ( path, args[1] ) ;
    }
    else
    {
        strcpy ( path, getenv ( "PWD" ) ) ;
        if ( strcmp ( path, "/" ) != 0 )
            strcat ( path, "/" ) ;
        strcat ( path, args[1] ) ;
    }

    memcpy ( shmem, path, sizeof (path ) ) ;
    exit ( 0 ) ;
}

void
_jobs ( snode* jobs )
{
    snode *ptr = jobs ;
    while ( ptr != NULL )
    {
        fprintf ( stdout, "PID = [%d] - ", ptr->pid ) ;
        if ( ptr->status )
        {
            fprintf ( stdout, "Running\n" ) ;
        }
        else
        {
            fprintf ( stdout, "Stopped\n" ) ;
        }

        ptr = ptr->prox ;
    }
    exit ( 0 ) ;
}

//Printa historico de comandos - Max 50

void
_history ( char *hpath )
{
    FILE *file = fopen ( hpath, "r" ) ;

    char* line = NULL ;
    size_t len = 0 ;
    int idx = 1 ;
    if ( file )
    {
        while ( getline ( &line, &len, file ) != - 1 )
        {
            fprintf ( stdout, "%4d  %s", idx ++, line ) ;
        }
        fclose ( file ) ;
    }
    else
    {
        perror ( "history" ) ;
    }
    exit ( 0 ) ;
}

void
_kill ( char **args, void* shmem )
{
    if ( args[1] == NULL || args[2] == NULL )
    {
        fprintf ( stderr, "Missing arguments.\n" ) ;
        exit ( 1 ) ;
    }

    pid_t pid = 0 ;
    int signal = 0, len ;

    //Catching pid number
    len = strlen ( args[1] ) ;
    for ( int i = 0 ; i < len ; i ++ )
    {
        if ( isdigit ( args[1][i] ) )
        {
            pid = pid * 10 + ( args[1][i] - '0' ) ;
        }
    }

    //Catching signal number
    len = strlen ( args[2] ) ;
    for ( int i = 0 ; i < len ; i ++ )
    {
        if ( isdigit ( args[2][i] ) )
        {
            signal = signal * 10 + ( args[2][i] - '0' ) ;
        }
    }

    if ( pid == 0 || signal == 0 )
    {
        fprintf ( stderr, "Missing values\n" ) ;
    }
    else
    {
        sprintf ( shmem, "%d %d", pid, signal ) ;
    }

    exit ( 0 ) ;
}

void
_echo ( char **args )
{

    char **s ;
    char aux[100], *env ;

    s = args + 1 ;
    while ( *s != NULL )
    {
        if ( **s == '$' )
        {
            int i = 0, len = strlen ( *s ) ;
            for ( ; i < len - 1 ; i ++ )
            {
                aux[i] = ( *s )[i + 1] ;
            }
            aux[i] = '\0' ;

            env = getenv ( aux ) ;
            if ( env )
            {
                fprintf ( stdout, "%s", env ) ;
            }
            else
            {
                fprintf ( stdout, "%s", *s ) ;
            }
        }
        else
        {
            fprintf ( stdout, "%s", *s ) ;
        }
        s ++ ;
        if ( s != NULL ) fprintf ( stdout, " " ) ;
    }

    fprintf ( stdout, "\n" ) ;
    exit ( 0 ) ;
}

void
_set ( )
{
    for ( char ** env = environ ; *env != NULL ; env ++ )
    {
        fprintf ( stdout, "%s\n", *env ) ;
    }
    exit ( 0 ) ;
}

void
_bg ( char *args[100], void* shmem )
{
    pid_t pid = 0 ;
    int len = strlen ( args[1] ) ;
    for ( int i = 0 ; i < len ; i ++ )
    {
        if ( isdigit ( args[1][i] ) )
        {
            pid = pid * 10 + ( args[1][i] - '0' ) ;
        }
    }

    sprintf ( shmem, "%d", pid ) ;
    exit ( 0 ) ;
}

void
_fg ( char *args[100], void* shmem )
{
    pid_t pid = 0 ;
    int len = strlen ( args[1] ) ;
    for ( int i = 0 ; i < len ; i ++ )
    {
        if ( isdigit ( args[1][i] ) )
        {
            pid = pid * 10 + ( args[1][i] - '0' ) ;
        }
    }

    sprintf ( shmem, "%d", pid ) ;
    exit ( 0 ) ;
}

void
insert ( snode** list, int pid, int status )
{
    snode* new_node = ( snode* ) malloc ( sizeof (snode ) ) ;
    new_node->pid = pid ;
    new_node->status = status ;
    new_node->prox = NULL ;

    if ( *list == NULL )
    {
        *list = new_node ;
    }
    else
    {
        snode* ptr = * list ;
        while ( ptr->prox != NULL )
        {
            ptr = ptr->prox ;
        }
        ptr->prox = new_node ;
    }
}

void
update_status ( snode** list, int pid, int status )
{
    snode* ptr = * list ;

    while ( ptr != NULL )
    {
        if ( ptr->pid == pid )
        {
            ptr->status = status ;
            return ;
        }
        ptr = ptr->prox ;
    }
}

void
del ( snode** list, int pid )
{
    if ( *list == NULL ) return ;

    snode* ptr = * list ;

    if ( ptr->pid == pid )
    {
        ( *list ) = ( *list )->prox ;
        free ( ptr ) ;
    }
    else
    {
        while ( ptr->prox != NULL && ( ptr->prox )->pid != pid )
        {
            ptr = ptr->prox ;
        }

        if ( ptr->prox != NULL )
        {
            snode *aux = ptr->prox ;
            ptr->prox = aux->prox ;
            free ( aux ) ;
        }
    }
}

void
kill_jobs_and_free_memory ( snode* lista )
{
    if ( lista == NULL )
        return ;
    kill_jobs_and_free_memory ( lista->prox ) ;
    kill ( lista->pid, 9 ) ;
    free ( lista ) ;
}

// Linked list of commands

void
add_cmd ( cnode** list, char * val )
{
    cnode* new_node = ( cnode* ) malloc ( sizeof (cnode ) ) ;
    strcpy ( new_node->cmd, val ) ;
    new_node->prox = NULL ;

    if ( *list == NULL )
    {
        *list = new_node ;
    }
    else
    {
        cnode* ptr = * list ;
        while ( ptr->prox != NULL )
        {
            ptr = ptr->prox ;
        }
        ptr->prox = new_node ;
    }
}

void
clear_cmd ( cnode* list )
{
    if ( list == NULL )
        return ;
    clear_cmd ( list->prox ) ;
    free ( list ) ;
}
