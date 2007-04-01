#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#ifdef WITH_LIMITS
#include <sys/resource.h>
#endif

#include "base.h"
#include "list.h"

struct wc_config_s
{
    char datfile[256];
    char clfile[256];
    char fifile[256];
    char empfile[256];
    char grpfile[256];
    double threshold;
};
typedef struct wc_config_s wc_config_t;

struct wc_data_s
{
    matrix_t* matrix;
    enumerated_map_t* emap;
    frequent_itemset_list_t* fi;
    double threshold;
};
typedef struct wc_data_s wc_data_t;

int configure(wc_config_t* config, int argc, char** argv);
void usage();

int configure(wc_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"frequent-itemsets",   required_argument,  NULL,   'f'},
        {"enumerated-map",      required_argument,  NULL,   'e'},
        {"threshold",           required_argument,  NULL,   't'},
        {NULL,                  0,                  NULL,   0}
    };

    config->threshold = 0.0;
    config->empfile[0] = '\0';
    config->fifile[0] = '\0';
    while((ch = getopt_long(argc, argv, "t:f:e:", longopts, NULL)) != -1)
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
            case 'e':
                strncpy(config->empfile, optarg, sizeof(config->empfile));
                config->empfile[sizeof(config->empfile) - 1] = '\0';
                break;
            default:
                usage();
                exit(-1);
        }
    }
    argc -= optind;
    argv += optind;

    if(argc < 3 || !strlen(config->empfile) || !strlen(config->fifile))
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
        "wc - total strength of connections\n"\
        "----------------------------------\n\n"\
        "Groups those components of the graph together that have the largest\n"\
        "strength among the association of their nodes. Such strength is\n"\
        "computed from the amount of shared distinct locations of two nodes.\n\n"\
        "Usage:\n\n"
        "wc [-t THRESHOLD] -e INPUT.emp -f INPUT.fi\n"
        "   INPUT.dat INPUT.cl OUTPUT.grp\n\n");
}

void alignment_amount(clique_t* c, int t1, int t2, void* weight, void* user_data)
{
    wc_data_t* data = (wc_data_t*)user_data;
    int len_fi = data->fi->frequent_itemsets[c->frequent_itemset_id].n_items;
    int len_t1 = data->emap->trajectories[c->trajectories[t1]].n_sample_ids;
    int len_t2 = data->emap->trajectories[c->trajectories[t2]].n_sample_ids;

    (*(double*)weight) = (double)(len_fi * 2) / (double)(len_t1 + len_t2);
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
            printf("%2f", *w);
        }
        printf("\n");
    }
}

double wc_strength(group_list_t* groups, int g1, int g2, void* user_data)
{
    wc_data_t* d = (wc_data_t*)user_data;
    int i;
    int j;
    double w;
    double g1c[groups->groups[g1].n_trajectories];
    double g2c[groups->groups[g2].n_trajectories];
    double total = 0.0;

    memset(g1c, 0, sizeof(double) * groups->groups[g1].n_trajectories);
    memset(g2c, 0, sizeof(double) * groups->groups[g2].n_trajectories);

    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
    {
        for(j = 0; j < groups->groups[g2].n_trajectories; j++)
        {
            w = *(double*)(d->matrix->matrix + (d->matrix->weight_size * ((d->matrix->n_trajectories * groups->groups[g2].trajectories[j]) + groups->groups[g1].trajectories[i])));
            g1c[i] += w;
            g2c[j] += w;
        }
    }
    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
        if(g1c[i] / (double)groups->groups[g2].n_trajectories < d->threshold)
            return 0;
        else
            total += g1c[i];

    for(i = 0; i < groups->groups[g2].n_trajectories; i++)
        if(g2c[i] / (double)groups->groups[g1].n_trajectories < d->threshold)
            return 0;
        else
            total += g2c[i];
    
    return total;
}

int main(int argc, char** argv)
{
    wc_config_t config;
    dataset_t* data;
    clique_list_t* cl;
    matrix_t* matrix;
    group_list_t* groups;
    wc_data_t wc_data;
#ifdef WITH_LIMITS
    struct rlimit cpu_limit = { 600, 600 };
    struct rlimit mem_limit = { 268435456, 268435456};

    setrlimit(RLIMIT_CPU, &cpu_limit  );
    setrlimit(RLIMIT_AS, &mem_limit  );
#endif
    configure(&config, argc, argv);
    data = dataset_load(config.datfile);
    cl = clique_list_load(config.clfile);
    wc_data.emap = enumerated_map_load(config.empfile);
    wc_data.fi = frequent_itemset_list_load(config.fifile, 0);
    printf("Generate matrix...\n");
    wc_data.threshold = config.threshold;
    matrix = matrix_create(data, cl, sizeof(double), &wc_data, alignment_amount);
    /* matrix_print_int(matrix); */
    printf("Generate group list...\n");
    groups = group_list_from_clique_list(cl);
    /* group_list_print(groups); */
    printf("Merging groups...\n");
    wc_data.matrix = matrix;
    group_list_merge(groups, wc_strength, NULL, &wc_data);
    group_list_save(groups, config.grpfile);
    group_list_destroy(groups);
    matrix_destroy(matrix);
    enumerated_map_destroy(wc_data.emap);
    frequent_itemset_list_destroy(wc_data.fi);
    clique_list_destroy(cl);
    dataset_destroy(&data);
    return 0;
} 
