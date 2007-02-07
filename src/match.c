#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "base.h"
#include "list.h"

struct match_config_s
{
    int min_length;
    /* text file containing the frequent itemsets */
    char fifile[256];
    /* text file containing the enumerated trajectories */
    char empfile[256];
    /* file listing the cliques of trajectories that have been identified */
    char clfile[256];
};
typedef struct match_config_s match_config_t;

int configure(match_config_t* config, int argc, char** argv);
void usage();

void usage()
{
    printf(
        "match\n"\
        "-----\n\n"\
        "Determines the trajectories that are matched by each frequent\n"\
        "itemset that was found. Each set of those trajectories is called\n"\
        "a clique.\n\n"\
        "Usage:\n\n"\
        "match [-l NUM] INPUT.fi INPUT.emp [OUTPUT.cl]\n\n"\
        "Options:\n"\
        " -l, --min-length NUM\tminimum number of items in each itemset\n\n"\
        "The output file OUTPUT.cl is optional and by default output.cl.\n");
}

int configure(match_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"min-length",  required_argument,  NULL,   'l'},
        {NULL,          0,                  NULL,   0}
    };

    config->min_length = 0;

    while((ch = getopt_long(argc, argv, "l:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'l':
                config->min_length = atoi(optarg);
                break;
            default:
                usage();
                exit(-1);
        }
    }
    argc -= optind;
    argv += optind;

    if(argc < 2)
    {
        usage();
        exit(-1);
        return FALSE;
    }   
    else
    {
        strncpy(config->fifile, argv[0], sizeof(config->fifile));
        strncpy(config->empfile, argv[1], sizeof(config->empfile));
        if(argc == 3)
            strncpy(config->clfile, argv[2], sizeof(config->clfile));
        else
            strncpy(config->clfile, "output.cl", sizeof(config->clfile));
    }
    return TRUE;
}

int main(int argc, char** argv)
{
    match_config_t config;
    enumerated_map_t* emap;
    frequent_itemset_list_t* fil;
    clique_list_t* cl;

    configure(&config, argc, argv);
    emap = enumerated_map_load(config.empfile);
    fil = frequent_itemset_list_load(config.fifile, config.min_length);
    cl = clique_list_create(fil, emap);
    clique_list_save(cl, config.clfile);
    clique_list_destroy(cl);
    frequent_itemset_list_destroy(fil);
    enumerated_map_destroy(emap);
    
    return 0;
}
