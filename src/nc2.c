#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#ifdef WITH_LIMITS
#include <sys/resource.h>
#endif

#include "base.h"
#include "list.h"

struct nc2_config_s
{
    char datfile[256];
    char clfile[256];
    char fifile[256];
    char grpfile[256];
    double threshold;
};
typedef struct nc2_config_s nc2_config_t;

struct nc2_data_s
{
    matrix_t* matrix;
    double* group_strength_list;
    double threshold;
};
typedef struct nc2_data_s nc2_data_t;

int configure(nc2_config_t* config, int argc, char** argv);
void usage();

int configure(nc2_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"frequent-itemsets",   required_argument,  NULL,   'f'},
        {"threshold",   required_argument,  NULL,   't'},
        {NULL,          0,                  NULL,   0}
    };

    config->threshold = 0.0;
    config->fifile[0] = '\0';

    while((ch = getopt_long(argc, argv, "f:t:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 't':
                config->threshold = atof(optarg);
                break;
            case 'f':
                strncpy(config->fifile, optarg, sizeof(config->fifile));
                config->fifile[sizeof(config->fifile) - 1] = '\0';
                break;
            default:
                usage();
                exit(-1);
        }
    }
    argc -= optind;
    argv += optind;

    if(argc < 3 || !strlen(config->fifile))
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

void boolean_connection(clique_t* c, int t1, int t2, void* weight, void* user_data)
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

double nc2_strength(group_list_t* groups, int g1, int g2, void* user_data)
{
    nc2_data_t* d = (nc2_data_t*)user_data;
    int i;
    int j;
    int g1ids[groups->groups[g1].n_trajectories];
    int g2ids[groups->groups[g2].n_trajectories];
    int total = 0;
    double r1;
    double r2;

    /* XXX use memcpy instead */
    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
        g1ids[i] = groups->groups[g1].trajectories[i];
    for(i = 0; i < groups->groups[g2].n_trajectories; i++)
        g2ids[i] = groups->groups[g2].trajectories[i];
    qsort(g1ids, groups->groups[g1].n_trajectories, sizeof(int), int_cmp);
    qsort(g2ids, groups->groups[g2].n_trajectories, sizeof(int), int_cmp);

    for(i = 0, j = 0; i < groups->groups[g1].n_trajectories && j < groups->groups[g2].n_trajectories; )
    {
        if(g1ids[i] == g2ids[j])
        {
            total++;
            i++;
            j++;
        }
        else if(g1ids[i] < g2ids[j])
            i++;
        else
            j++;
    }
    
    r1 = (double)total / (double)groups->groups[g1].n_trajectories;
    r2 = (double)total / (double)groups->groups[g2].n_trajectories;

    if(r1 >= d->threshold && r2 >= d->threshold)
        return (d->group_strength_list[g1] + d->group_strength_list[g2]) / 2.0;
    else
        return 0.0;
}

void group_strength_list_update(int target, int g1, int g2, double new_strength, void* user_data)
{
    nc2_data_t* d = (nc2_data_t*)user_data;
    d->group_strength_list[target] = new_strength;
}

double* group_strength_list_create(clique_list_t* cliques, frequent_itemset_list_t* fi)
{
    double* strength_list;
    int i = 0;
    strength_list = malloc(sizeof(double) * cliques->n_cliques);

    for(; i < cliques->n_cliques; i++)
        strength_list[i] = (double)fi->frequent_itemsets[cliques->cliques[i].frequent_itemset_id].n_items;
    return strength_list;
}

void group_strength_list_destroy(double* strength_list)
{
    if(strength_list)
        free(strength_list);
}

int main(int argc, char** argv)
{
    nc2_config_t config;
    dataset_t* data;
    clique_list_t* cl;
    matrix_t* matrix;
    group_list_t* groups;
    nc2_data_t nc2_data;
    frequent_itemset_list_t* fi;

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
    matrix = matrix_create(data, cl, sizeof(int), NULL, boolean_connection);
    /* matrix_print_int(matrix); */
    printf("Generate group list...\n");
    groups = group_list_from_clique_list(cl);
    fi = frequent_itemset_list_load(config.fifile, 0);
    nc2_data.group_strength_list = group_strength_list_create(cl, fi);
    /* group_list_print(groups); */
    printf("Merging groups...\n");
    nc2_data.matrix = matrix;
    nc2_data.threshold = config.threshold;
    group_list_merge(groups, nc2_strength, group_strength_list_update, &nc2_data);
    group_list_save(groups, config.grpfile);
    group_list_destroy(groups);
    matrix_destroy(matrix);
    clique_list_destroy(cl);
    dataset_destroy(&data);
    return 0;
}
