#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#ifdef WITH_LIMITS
#include <sys/resource.h>
#endif

#include "base.h"
#include "list.h"

struct nc_config_s
{
    char datfile[256];
    char clfile[256];
    char grpfile[256];
    double threshold;
};
typedef struct nc_config_s nc_config_t;

struct nc_data_s
{
    matrix_t* matrix;
    double threshold;
};
typedef struct nc_data_s nc_data_t;

int configure(nc_config_t* config, int argc, char** argv);
void usage();

int configure(nc_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"threshold",   required_argument,  NULL,   't'},
        {NULL,          0,                  NULL,   0}
    };

    config->threshold = 0.0;

    while((ch = getopt_long(argc, argv, "t:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 't':
                config->threshold = atof(optarg);
                break;
            default:
                usage();
                exit(-1);
        }
    }
    argc -= optind;
    argv += optind;

    if(argc < 3)
    {
        usage();
        exit(-1);
        return FALSE;
    }
    else
    {
        strncpy(config->datfile, argv[0], sizeof(config->datfile));
        strncpy(config->clfile, argv[1], sizeof(config->clfile));
        strncpy(config->grpfile, argv[2], sizeof(config->grpfile));
    }
    return TRUE;
}

void usage()
{
    printf(
        "nc - number of connections\n"\
        "--------------------------\n\n"\
        "Groups those components of the graph together that have the most\n"\
        "connections across them and this number is greater than a given\n"\
        "threshold.\n\n"\
        "Usage:\n\n"
        "cc [-t THRESHOLD] INPUT.dat INPUT.cl OUTPUT.grp\n\n");
}

void boolean_connection(dataset_t* data, clique_t* c, int t1, int t2, void* weight)
{
    (*(int*)weight) = TRUE;
}

void matrix_print_int(matrix_t* matrix)
{
    int t1;
    int t2;
    int* w;

    for(t1 = 0; t1 < matrix->n_trajectories; t1++)
    {
        for(t2 = 0; t2 < matrix->n_trajectories; t2++)
        {
            w = matrix->matrix + (matrix->weight_size * ((matrix->n_trajectories * t2) + t1));
            printf("%2d", *w);
        }
        printf("\n");
    }
}

double nc_strength(group_list_t* groups, int g1, int g2, void* user_data)
{
    nc_data_t* d = (nc_data_t*)user_data;
    int i;
    int j;
    int* w;
    int g1c[groups->groups[g1].n_trajectories];
    int g2c[groups->groups[g2].n_trajectories];
    int total = 0;

    memset(g1c, 0, sizeof(int) * groups->groups[g1].n_trajectories);
    memset(g2c, 0, sizeof(int) * groups->groups[g2].n_trajectories);

    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
    {
        for(j = 0; j < groups->groups[g2].n_trajectories; j++)
        {
            w = d->matrix->matrix + (d->matrix->weight_size * ((d->matrix->n_trajectories * groups->groups[g2].trajectories[j]) + groups->groups[g1].trajectories[i]));
            if(*w == TRUE)
            {
                g1c[i]++;
                g2c[j]++;
            }
        }
    }
    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
        if((double)g1c[i] / (double)groups->groups[g2].n_trajectories < d->threshold)
            return 0;
        else
            total += g1c[i];

    for(i = 0; i < groups->groups[g2].n_trajectories; i++)
        if((double)g2c[i] / (double)groups->groups[g1].n_trajectories < d->threshold)
            return 0;
        else
            total += g2c[i];
    
    return (double)total;
}

int main(int argc, char** argv)
{
    nc_config_t config;
    dataset_t* data;
    clique_list_t* cl;
    matrix_t* matrix;
    group_list_t* groups;
    nc_data_t nc_data;
#ifdef WITH_LIMITS
    struct rlimit cpu_limit = { 600, 600 };
    struct rlimit mem_limit = { 268435456, 268435456};

    setrlimit(RLIMIT_CPU, &cpu_limit  );
    setrlimit(RLIMIT_AS, &mem_limit  );
#endif
    configure(&config, argc, argv);
    data = dataset_load(config.datfile);
    cl = clique_list_load(config.clfile);
    printf("Generate matrix...\n");
    matrix = matrix_create(data, cl, sizeof(int), boolean_connection);
    /* matrix_print_int(matrix); */
    printf("Generate group list...\n");
    groups = group_list_from_clique_list(cl);
    /* group_list_print(groups); */
    printf("Merging groups...\n");
    nc_data.matrix = matrix;
    nc_data.threshold = config.threshold;
    group_list_merge(groups, nc_strength, &nc_data);
    group_list_save(groups, config.grpfile);
    group_list_destroy(groups);
    matrix_destroy(matrix);
    clique_list_destroy(cl);
    dataset_destroy(&data);
    return 0;
}
