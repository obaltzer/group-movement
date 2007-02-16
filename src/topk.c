#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "base.h"

struct topk_config_s
{
    char infile[256];
    char outfile[256];
    int k;
};
typedef struct topk_config_s topk_config_t;

void usage()
{
    printf(
        "Reduces a group file to only the largest k groups.\n\n"
        "Usage:\n"
        "\ttopk [-k N] [-o output.grp] input.grp\n\n"
        "Options:\n"
        " -k N\t\tnumber of groups to keep (default: 5)\n"
        " -o FILE\toutput file (default: output.grp)\n"
    );
    exit(-1);
}

int configure(topk_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"output",  required_argument,  NULL,   'o'},
        {"top-k",   required_argument,  NULL,   'k'},
        {NULL,      0,                  NULL,   0}
    };

    config->k = 5;
    strncpy(config->outfile, "output.grp", sizeof(config->outfile));

    while((ch = getopt_long(argc, argv, "o:k:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'k':
                config->k = atoi(optarg);
                break;
            case 'o':
                strncpy(config->outfile, optarg, sizeof(config->outfile));
                config->outfile[sizeof(config->outfile) - 1] = '\0';
                break;
            default:
                usage();
                exit(-1);
        }
    }
    argc -= optind;
    argv += optind;

    if(argc < 1)
    {
        usage();
        exit(-1);
        return FALSE;
    }
    else
    {
        strncpy(config->infile, argv[0], sizeof(config->infile));
        config->infile[sizeof(config->infile) - 1] = '\0';
    }
    
    return TRUE;
}

int cmp_group_size(const void* a, const void* b)
{
    return int_cmp(&((group_t*)a)->n_trajectories, &((group_t*)b)->n_trajectories) * -1;
}

group_list_t* group_list_topk(group_list_t* in, int k)
{
    group_list_t* out;
    int i;

    out = malloc(sizeof(group_list_t));
    if(out == NULL)
    {
        printf("Cannot allocate memory for new group list.\n");
        return NULL;
    }
    out->n_groups = k > 0 && k < in->n_groups ? k : in->n_groups;
    out->groups = malloc(sizeof(group_t) * out->n_groups);
    if(out->groups == NULL)
    {
        printf("Cannot allocate memory for new groups.\n");
        free(out);
        return NULL;
    }
    memset(out->groups, 0, sizeof(group_t) * out->n_groups);
    qsort(in->groups, in->n_groups, sizeof(group_t), cmp_group_size);
    for(i = 0; i < out->n_groups; i++)
    {
        out->groups[i].group_id = in->groups[i].group_id;   
        out->groups[i].n_trajectories = in->groups[i].n_trajectories;
        out->groups[i].trajectories = malloc(sizeof(int) * out->groups[i].n_trajectories);
        if(out->groups[i].trajectories == NULL)
        {
            printf("Cannot allocate memory for trajectories.\n");
            group_list_destroy(out);
            return NULL;
        }
        memcpy(out->groups[i].trajectories, in->groups[i].trajectories, sizeof(int) * out->groups[i].n_trajectories);
    }
    return out;
}

int main(int argc, char** argv)
{
    group_list_t* in;
    group_list_t* out;
    topk_config_t config;

    configure(&config, argc, argv);
    in = group_list_load(config.infile);
    out = group_list_topk(in, config.k);
    group_list_save(out, config.outfile);
    group_list_print(out);
    group_list_destroy(out);
    group_list_destroy(in);
    return 0;
}
