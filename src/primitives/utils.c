#include "../include/csl.h"

void
handle_sigint ( int sig )
{
}

void
handle_sigtstp ( int sig )
{
}

void
handle_sigchld ( int sig )
{
    pid_t pid ;
    int status ;
    pid = waitpid ( - 1, &status, ( WNOHANG | WUNTRACED | WCONTINUED ) ) ;
}

int
get_type_process ( const char* cmd )
{
    if ( strncmp ( cmd, "cd", 2 ) == 0 )
    {
        return 2 ;
    }
    return 0 ;
}
#if 0
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
#endif

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
