#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <list.h>
#include <base.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

struct parallel_config_s
{
    char cmd[1024];
    char param_file[256];
    char flag_name[256];
    int n_proc;
};
typedef struct parallel_config_s parallel_config_t;

struct parameter_s
{
    int n_values;
    char** values;
};
typedef struct parameter_s parameter_t;

struct parameter_list_s
{
    int n_parameters;
    parameter_t* parameters;
};
typedef struct parameter_list_s parameter_list_t;

struct parameter_set_s
{	
    int n_params;
    char** params;
};
typedef struct parameter_set_s parameter_set_t;

struct parameter_set_list_s
{
    int n_sets;
    parameter_set_t* sets;
};
typedef struct parameter_set_list_s parameter_set_list_t;

void usage()
{
    printf("Usage:\n");
    exit(-1);
}

int configure(parallel_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"cmd",         required_argument,  NULL,   'c'},
        {"parameters",  required_argument,  NULL,   'p'},
        {"flag-name",   required_argument,  NULL,   'f'},
        {"processes",   required_argument,  NULL,   'n'},
        {NULL,          0,                  NULL,   0}};

    config->cmd[0] = '\0';
    config->param_file[0] = '\0';
    config->flag_name[0] = '\0';
    config->n_proc = 2;


    while((ch = getopt_long(argc, argv, "c:p:f:n:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'c':
                strncpy(config->cmd, optarg, sizeof(config->cmd));
                break;
            case 'p':
                strncpy(config->param_file, optarg, sizeof(config->param_file));
                break;
            case 'f':
                strncpy(config->flag_name, optarg, sizeof(config->flag_name));
                break;
            case 'n':
                config->n_proc = atoi(optarg);
                break;
            default:
                usage();
        }
    }

    if(!strlen(config->cmd) || !strlen(config->param_file) || !strlen(config->flag_name) || config->n_proc < 1)
        usage();
    return 0;
}
        
int substitude_parameters(char* dst, char* src, parameter_set_t params)
{
    char* tmp;
    char* state;
    char* item;
    int pi;

    tmp = malloc(sizeof(char) * strlen(src) + 1);
    if(tmp == NULL)
        return -1;
    strcpy(tmp, src);
    dst[0] = '\0';
    for(item = strtok_r(tmp, "%", &state); item; item = strtok_r(NULL, "%", &state))
    {
        if(item[0] != '\0' && (pi = item[0] - 0x30) < params.n_params && pi >= 0)
        {
            strcat(dst, params.params[pi]);
            item++;
        }
        else if(item != tmp) 
            strcat(dst, "%");
        strcat(dst, item);
    }
    free(tmp);
    return 0;
}

int execute_command(char* command, parameter_set_t* params)
{
    int retval;
    char cmd[1024];

    substitude_parameters(cmd, command, *params);
    retval = system(cmd);
    return WEXITSTATUS(retval);
}

void parameter_set_generate_recurse(parameter_list_t* p_list, int p, int* values, list_t* l)
{
    int i;
    
    if(p == p_list->n_parameters)
    {
        parameter_set_t* ps;
        ps = malloc(sizeof(parameter_set_t));
        ps->n_params = p_list->n_parameters;
        ps->params = malloc(sizeof(char*) * ps->n_params);
        for(i = 0; i < ps->n_params; i++)
            ps->params[i] = p_list->parameters[i].values[values[i]];
        list_append(l, ps);
    }
    else
    {
        for(i = 0; i < p_list->parameters[p].n_values; i++)
        {
            values[p] = i;
            parameter_set_generate_recurse(p_list, p + 1, values, l);
        }
    }
}

parameter_set_list_t* parameter_set_list_create(parameter_list_t* p_list)
{
    int* values;
    list_t* l;
    parameter_set_list_t* set_list;
    
    values = malloc(sizeof(int) * p_list->n_parameters);
    l = list_create();
    parameter_set_generate_recurse(p_list, 0, values, l);
    free(values);
    set_list = malloc(sizeof(parameter_set_list_t));
    set_list->n_sets = list_length(l);
    set_list->sets = list_to_array(l, sizeof(parameter_set_t));
    list_destroy(l, free);
    return set_list;
}

void parameter_set_print(parameter_set_t* ps)
{
    int i;
    for(i = 0; i < ps->n_params; i++)
    {
        printf("%s ", ps->params[i]);
    }
    printf("\n");
}

parameter_list_t* parameter_list_load(char* filename)
{
    FILE* in;
    list_t* l;
    char* line;
    parameter_t* p;
    parameter_list_t* pl;

    if((in = fopen(filename, "r")) == NULL)
    {
        printf("Cannot open parameter list file %s.\n", filename);
        return NULL;
    }
    l = list_create();
    while((line = line_read(in)) != NULL)
    {
        char* status;
        char* item;
        list_t* value_list;

        value_list = list_create();
        for(item = strtok_r(line, ",\n", &status); item; item = strtok_r(NULL, ",\n", &status))
        {
            char** v = malloc(sizeof(char*));
            *v = malloc(strlen(item) + 1);
            strcpy(*v, item);
            list_append(value_list, v);
        }
        p = malloc(sizeof(parameter_t));
        p->n_values = list_length(value_list);
        p->values = list_to_array(value_list, sizeof(char*));
        list_destroy(value_list, free);
        line_destroy(line);
        list_append(l, p);
    }
    fclose(in);
    pl = malloc(sizeof(parameter_list_t));
    pl->n_parameters = list_length(l);
    pl->parameters = list_to_array(l, sizeof(parameter_t));
    list_destroy(l, free);
    return pl;
}

void parameter_set_list_print(parameter_set_list_t* psl)
{
    int i;
    for(i = 0; i < psl->n_sets; i++)
        parameter_set_print(&psl->sets[i]);
}

void parameter_list_destroy(parameter_list_t* pl)
{
    int i;
    int j;
    if(pl)
    {
        for(i = 0; i < pl->n_parameters; i++)
        {
            for(j = 0; j < pl->parameters[i].n_values; j++)
                free(pl->parameters[i].values[j]);
            free(pl->parameters[i].values);
        }
        free(pl->parameters);
        free(pl);
    }
}

void parameter_set_list_destroy(parameter_set_list_t* psl)
{
    int i;
    if(psl)
    {
        for(i = 0; i < psl->n_sets; i++)
            free(psl->sets[i].params);
        free(psl->sets);
        free(psl);
    }
}

int parameter_set_check_flag(char* flag_name, parameter_set_t* params)
{
    char dst[256];
    struct stat stats;

    substitude_parameters(dst, flag_name, *params);
    if(stat(dst, &stats) == 0 && S_ISREG(stats.st_mode))
        return TRUE;
    return FALSE;
}

void parameter_set_flag(char* flag_name, parameter_set_t* params)
{
    char dst[256];
    FILE* flag;
    
    substitude_parameters(dst, flag_name, *params);
    if((flag = fopen(dst, "w")) != NULL)
        fclose(flag);
}

int main(int argc, char** argv)
{
    int i;
    int active;
    int retval;
    MPI_Status status;
    int rank;
    int n_nodes;
    parallel_config_t config;
    parameter_list_t* pl;
    parameter_set_list_t* psl;
    
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_nodes);
    
    /* configure every node, so we only need to send indices across */
    configure(&config, argc, argv);
    pl = parameter_list_load(config.param_file);
    psl = parameter_set_list_create(pl);
    
    if(rank == 0)
    {
        int* active_nodes;
        int n = 0;
        int done = 0;
        
        n_nodes--;
        active_nodes = malloc(sizeof(int) * n_nodes);
        memset(active_nodes, 0, sizeof(int) * n_nodes);
        for(i = 0; i < psl->n_sets; i++)
        {
            n = 0;
            while(n < n_nodes && active_nodes[n] == config.n_proc)
            {
                printf("Master: Checking for free nodes %d\n", n);
                n++;
            }
            if(n == n_nodes)
            {
                printf("Master: waiting for node\n");
	        MPI_Recv(&n, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                printf("Master: received from node %d\n", n);
                active_nodes[n]--;
            }
	    
            MPI_Send(&i, 1, MPI_INT, n + 1, 0, MPI_COMM_WORLD);
            printf("Master: send job %d to node %d\n", i, n);
            active_nodes[n]++;
        }
        /* send all nodes command to terminate */
        n = -1;
        for(i = 0; i < n_nodes; i++)
            MPI_Send(&n, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
        /* wait for all nodes to finish */
        while(!done)
        {
            n = 0;
            while(n < n_nodes && active_nodes[n] == 0)
                n++;
            if(n != n_nodes)
            {
	        MPI_Recv(&n, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                active_nodes[n]--;
            }
            else
                done = 1;
        }
        free(active_nodes);
    }
    else
    {
        int done = 0;
        int i;
        active = 0;
        while(!done)
        {
            if(active == config.n_proc)
            {
                wait(&retval);
                i = rank - 1;
                MPI_Send(&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                active--;
            }
	    MPI_Recv(&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            printf("Node %d: received %d\n", rank, i);
            if(i != -1)
            {
                printf("Node %d: start %d child\n", rank, active + 1);
                if(fork() == 0)
                {
                    if(!parameter_set_check_flag(config.flag_name, &psl->sets[i]))
                    {
                        if(execute_command(config.cmd, &psl->sets[i]) != 0)
                            parameter_set_flag(config.flag_name, &psl->sets[i]);
                    }   
                    else
                    {
                        printf("Do not execute. Parameter set is flagged: ");
                        parameter_set_print(&psl->sets[i]);
                    }
                    parameter_set_list_destroy(psl);
                    parameter_list_destroy(pl);
                    exit(0);
                }
                else
                    active++;
            }
            else
                done = 1;
        }
        while(active)
        {
            wait(&retval);
            i = rank - 1;
            MPI_Send(&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            active--;
        }
    }
    parameter_set_list_destroy(psl);
    parameter_list_destroy(pl);
    MPI_Finalize();
    return 0;
}
