#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "base.h"
#include "list.h"

struct cc_config_s
{
    char datfile[256];
    char clfile[256];
    char grpfile[256];
};
typedef struct cc_config_s cc_config_t;

int configure(cc_config_t* config, int argc, char** argv);
void usage();

int configure(cc_config_t* config, int argc, char** argv)
{
    if(argc < 4)
    {
        usage();
        exit(-1);
        return FALSE;
    }
    else
    {
        strncpy(config->datfile, argv[1], sizeof(config->datfile));
        strncpy(config->clfile, argv[2], sizeof(config->clfile));
        strncpy(config->grpfile, argv[3], sizeof(config->grpfile));
    }
    return TRUE;
}

void usage()
{
    printf(
        "cc - connected components\n"\
        "-------------------------\n\n"\
        "Performs a connected components search among the cliques to form groups.\n\n"\
        "Usage:\n\n"
        "cc INPUT.dat INPUT.cl OUTPUT.grp\n\n");
}

void relative_length(dataset_t* data, clique_t* c, int t1, int t2, void* weight)
{
    double* w = (double*)weight;
    double nw;

    nw = 1.0 / ((double)data->trajectories[c->trajectories[t1]].n_samples +
            (double)data->trajectories[c->trajectories[t2]].n_samples);

    *w = nw;
}

void boolean_connection(dataset_t* data, clique_t* c, int t1, int t2, void* weight)
{
    (*(int*)weight) = TRUE;
}

void matrix_print_double(matrix_t* matrix)
{
    int t1;
    int t2;
    double* w;

    for(t1 = 0; t1 < matrix->n_trajectories; t1++)
    {
        for(t2 = 0; t2 < matrix->n_trajectories; t2++)
        {
            w = matrix->matrix + (matrix->weight_size * ((matrix->n_trajectories * t2) + t1));
            printf("  %.4f", *w);
        }
        printf("\n");
    }
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

double boolean_strength(group_list_t* groups, matrix_t* matrix, int g1, int g2)
{
    int i;
    int j;
    int* w;

    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
        for(j = 0; j < groups->groups[g2].n_trajectories; j++)
        {
            w = matrix->matrix + (matrix->weight_size * ((matrix->n_trajectories * groups->groups[g2].trajectories[j]) + groups->groups[g1].trajectories[i]));
            /* printf("[%d, %d] = %d\n", groups->groups[g1].trajectories[i], groups->groups[g2].trajectories[j], *w); */
            if(*w == TRUE)
                return 1.0;
        }

    return 0.0;
}

int main(int argc, char** argv)
{
    cc_config_t config;
    dataset_t* data;
    clique_list_t* cl;
    matrix_t* matrix;
    group_list_t* groups;

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
    group_list_merge(groups, matrix, boolean_strength);
    group_list_save(groups, config.grpfile);
    group_list_destroy(groups);
    matrix_destroy(matrix);
    clique_list_destroy(cl);
    dataset_destroy(&data);
    return 0;
}
