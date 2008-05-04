#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#ifdef WITH_LIMITS
#include <sys/resource.h>
#endif

#include "base.h"
#include "list.h"

struct both_config_s
{
    char datfile[256];
    char clfile[256];
    char fifile[256];
    char empfile[256];
    char grpfile[256];
    double ost;
    double ast;
};
typedef struct both_config_s both_config_t;

struct both_data_s
{
    enumerated_map_t* emap;
    frequent_itemset_list_t* fi;
    double* group_strength_list;
    double ost;
    double ast;
};
typedef struct both_data_s both_data_t;

int configure(both_config_t* config, int argc, char** argv);
void usage();

int configure(both_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"frequent-itemsets",   required_argument,  NULL,   'f'},
        {"enumerated-map",      required_argument,  NULL,   'e'},
        {"ost",                 required_argument,  NULL,   'o'},
        {"ast",                 required_argument,  NULL,   'a'},
        {NULL,                  0,                  NULL,   0}
    };

    config->ost = 0.0;
    config->ast = 0.0;
    config->empfile[0] = '\0';
    config->fifile[0] = '\0';
    while((ch = getopt_long(argc, argv, "o:a:f:e:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'o':
                config->ost = atof(optarg);
                break;
            case 'a':
                config->ast = atof(optarg);
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
        "Usage:\n\n"
        "both [-o OST] [-a AST] -e INPUT.emp -f INPUT.fi\n"
        "     INPUT.dat INPUT.cl OUTPUT.grp\n\n");
}

void alignment_amount(clique_t* c, int t1, int t2, void* weight, void* user_data)
{
    both_data_t* data = (both_data_t*)user_data;
    int len_fi = data->fi->frequent_itemsets[c->frequent_itemset_id].n_items;
    int len_t1 = data->emap->trajectories[c->trajectories[t1]].n_sample_ids;
    int len_t2 = data->emap->trajectories[c->trajectories[t2]].n_sample_ids;
    double w = (double)(len_fi * 2) / (double)(len_t1 + len_t2);

    if(w > data->ost && (*(double*)weight) < w)
        (*(double*)weight) = w;
    /*
    clique_print(c);
    frequent_itemset_print(&data->fi->frequent_itemsets[c->frequent_itemset_id]);
    printf("%d(%d), %d(%d) : %d -> %f\n", c->trajectories[t1], len_t1, c->trajectories[t2], len_t2, len_fi, (*(double*)weight));
    */
}

#if 0
void threshold_connection(int t1, int t2, void* weight, void* user_data)
{
    wc_data_t* data = (wc_data_t*)user_data;
    double* w = (double *)weight;
    if(*w < data->threshold)
        *w = 0.0;
}
#endif

#if 0
double thresholded_boolean_strength(group_list_t* groups, int g1, int g2, void* user_data)
{
    both_data_t* data = (both_data_t*)user_data;
    int i;
    int j;
    double* w;

    for(i = 0; i < groups->groups[g1].n_trajectories; i++)
        for(j = 0; j < groups->groups[g2].n_trajectories; j++)
        {
            w = data->matrix->matrix + (data->matrix->weight_size * ((data->matrix->n_trajectories * groups->groups[g2].trajectories[j]) + groups->groups[g1].trajectories[i]));
            if(*w > data->ost)
                return 1.0;
        }

    return 0.0;
}
#endif

group_list_t* group_list_from_matrix(matrix_t* matrix)
{
    int i, j;
    int g1;
    int g2;
    int t1;
    int t2;
    int tr1;
    int tr2;
    group_list_t* groups;
    group_t* new_list;
    double w;
    int done = FALSE;
        
    groups = malloc(sizeof(group_list_t));
    if(groups == NULL)
    {
        printf("Cannot allocate group list.\n");
        return NULL;
    }
    memset(groups, 0, sizeof(group_list_t));
    
    groups->n_groups = matrix->n_trajectories;
    groups->groups = malloc(sizeof(group_t) * groups->n_groups);
    if(groups->groups == NULL)
    {
        printf("Cannot allocate memory for groups.\n");
        group_list_destroy(groups);
        return NULL;
    }
    memset(groups->groups, 0, sizeof(group_t) * groups->n_groups);
    for(i = 0; i < groups->n_groups; i++)
    {
        groups->groups[i].group_id = i;
        groups->groups[i].n_trajectories = 1;
        groups->groups[i].trajectories = malloc(sizeof(int));
        if(groups->groups[i].trajectories == NULL)
        {
            printf("Cannot allocate memory for group trajectories.\n");
            group_list_destroy(groups);
            return NULL;
        }
        groups->groups[i].trajectories[0] = i;
    }

    do
    {
        done = FALSE;
        for(i = 0; !done && i < groups->n_groups; i++)
        {
            for(t1 = 0; !done && t1 < groups->groups[i].n_trajectories; t1++)
            {
                tr1 = groups->groups[i].trajectories[t1];              
                for(j = i + 1; !done && j < groups->n_groups; j++)
                {
                    for(t2 = 0; !done && t2 < groups->groups[j].n_trajectories; t2++)
                    {
                        assert(i < groups->n_groups && j < groups->n_groups);
                        tr2 = groups->groups[j].trajectories[t2];
                        if(i != j && tr1 != tr2)
                        {
                            /*printf("Checking groups %d and %d:\n", g1, g2);
                            group_print(&groups->groups[g1]);
                            group_print(&groups->groups[g2]);*/
                            w = *(double*)(matrix->matrix + (matrix->weight_size * (matrix->n_trajectories * tr1 + tr2)));
                            if(w > 0.0)
                            {
                                g1 = i;
                                g2 = j;
                                done = TRUE;
                            }
                        }
                    }
                }
            }
        }
        if(done && g1 != g2)
        {
            assert(g1 < groups->n_groups && g2 < groups->n_groups);
            done = FALSE;
            new_list = malloc(sizeof(group_t) * (groups->n_groups - 1));
            j = 0;
            for(i = 0; i < groups->n_groups; i++)
            {
                if(i != g1 && i != g2)
                    new_list[j++] = groups->groups[i];
            }
            new_list[j] = group_merge(groups->groups + g1, groups->groups + g2);
            free(groups->groups[g1].trajectories);
            free(groups->groups[g2].trajectories);
            free(groups->groups);
            groups->groups = new_list;
            groups->n_groups--;
        }
        else
            done = TRUE;
    }
    while(!done);
    
    /* Remove singletons from list. */
    new_list = NULL;
    j = 0;
    for(i = 0; i < groups->n_groups; i++)
    {
        if(groups->groups[i].n_trajectories > 1)
        {
            if((new_list = realloc(new_list, sizeof(group_t) * ++j)) != NULL)
            {
                new_list[j - 1].n_trajectories = groups->groups[i].n_trajectories;
                new_list[j - 1].group_id = groups->groups[i].group_id;
                new_list[j - 1].trajectories = groups->groups[i].trajectories;
            }
        }
        else
        {
            free(groups->groups[i].trajectories);
        }
    }
    free(groups->groups);
    groups->groups = new_list;
    groups->n_groups = j;
    return groups;
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

void matrix_print_double(matrix_t* matrix)
{
    int t1;
    int t2;
    double* w;
    
    printf("        ");
    for(t1 = 0; t1 < matrix->n_trajectories; t1++)
        printf("%8d", t1);
    printf("\n");
    for(t1 = 0; t1 < matrix->n_trajectories; t1++)
        printf("--------");
    printf("\n");

    for(t1 = 0; t1 < matrix->n_trajectories; t1++)
    {
        printf("%6d |", t1);
        for(t2 = 0; t2 < matrix->n_trajectories; t2++)
        {
            w = matrix->matrix + (matrix->weight_size * ((matrix->n_trajectories * t2) + t1));
            printf("%8.3f", *w);
        }
        printf("\n");
    }
}

double ast_strength(group_list_t* groups, int g1, int g2, void* user_data)
{
    both_data_t* d = (both_data_t*)user_data;
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

    if(r1 >= d->ast && r2 >= d->ast)
        return (d->group_strength_list[g1] + d->group_strength_list[g2]) / 2.0;
    else
        return 0.0;
}

void group_strength_list_update(int target, int g1, int g2, double new_strength, void* user_data)
{
    both_data_t* d = (both_data_t*)user_data;
    d->group_strength_list[target] = new_strength;
}

void matrix_insert_groups(matrix_t* m, group_list_t* groups, double w)
{
    int i;
    int t1, t2;
    
    for(i = 0; i < groups->n_groups; i++)
    {
        for(t1 = 0; t1 < groups->groups[i].n_trajectories; t1++)
        {
            for(t2 = t1 + 1; t2 < groups->groups[i].n_trajectories; t2++)
            {
                *(double*)(m->matrix + (m->weight_size * (m->n_trajectories * groups->groups[i].trajectories[t1] + groups->groups[i].trajectories[t2]))) = w;
            }
        }
    }
}

int main(int argc, char** argv)
{
    both_config_t config;
    dataset_t* data;
    clique_list_t* cl;
    matrix_t* matrix;
    group_list_t* attachment_groups;
    group_list_t* overlap_groups;
    both_data_t both_data;
#ifdef WITH_LIMITS
    struct rlimit cpu_limit = { 120, 120 };
    struct rlimit mem_limit = { 419430400, 419430400};

    setrlimit(RLIMIT_CPU, &cpu_limit  );
    setrlimit(RLIMIT_AS, &mem_limit  );
#endif
    configure(&config, argc, argv);
    data = dataset_load(config.datfile);
    cl = clique_list_load(config.clfile);
    printf("Generate matrix for Group by Attachment\n");
    /* both_data.attachement_matrix = matrix_create(data, cl, sizeof(int), NULL, boolean_connection); */
    attachment_groups = group_list_from_clique_list(cl);
    both_data.fi = frequent_itemset_list_load(config.fifile, 0);
    both_data.group_strength_list = group_strength_list_create(cl, both_data.fi);
    printf("Compute Group by Attachment\n");
    both_data.ast = config.ast;
    group_list_merge(attachment_groups, ast_strength, group_strength_list_update, &both_data);
    /* compute overlap groups */
    both_data.emap = enumerated_map_load(config.empfile);
    both_data.ost = config.ost;
    matrix = matrix_create(data, cl, sizeof(double), &both_data, alignment_amount);
    /* insert attachment groups into overlap matrix */
    matrix_insert_groups(matrix, attachment_groups, 1.0);
    printf("Compute Group by Overlap\n");
    overlap_groups = group_list_from_matrix(matrix);
    group_list_save(overlap_groups, config.grpfile);
    group_list_destroy(overlap_groups);
    group_list_destroy(attachment_groups);
    matrix_destroy(matrix);
    enumerated_map_destroy(both_data.emap);
    frequent_itemset_list_destroy(both_data.fi);
    clique_list_destroy(cl);
    dataset_destroy(&data);
    return 0;
} 
