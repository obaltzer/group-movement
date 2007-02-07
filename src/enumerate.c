#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "base.h"

struct enumerate_config_s
{
    char infile[256];
    char outfile[256];
    char unique[256];
};
typedef struct enumerate_config_s enumerate_config_t;

void usage()
{
    printf(
        "Usage:\n\n"\
        "enumerate [-o|--output FILENAME] [-u|--unique FILENAME]\n"\
        "          input.map\n\n"
        " -o, --output FILENAME\tfile to store enumerated map\n"\
        " -u, --unique FILENAME\tfile to store mappings for unique samples\n");
    exit(-1);
}

int configure(enumerate_config_t* config, int argc, char** argv)
{
    int ch;
    
    static struct option longopts[] = {
        {"output",      required_argument,  NULL,   'o'},
        {"unique",      required_argument,  NULL,   'u'},
        {NULL,          0,                  NULL,   0}
    };

    /* DEFAULTS */
    strncpy(config->outfile, "output.emp", sizeof(config->outfile) - 1);
    strncpy(config->unique, "output.unq", sizeof(config->unique) - 1);
    while((ch = getopt_long(argc, argv, "o:u:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'o':
                strncpy(config->outfile, optarg, sizeof(config->outfile) - 1);
                break;
            case 'u':
                strncpy(config->unique, optarg, sizeof(config->unique) - 1);
                break;
            default:
                usage();
        }
    }
    argc -= optind;
    argv += optind;
    
    if(argc == 0)
    {
        printf("Input file name missing.\n");
        usage();
        return FALSE;
    }
    strncpy(config->infile, argv[0], sizeof(config->infile) - 1);
    return TRUE;
} 

int main(int argc, char** argv)
{
    map_t* map;
    unique_samples_t* s;
    enumerated_map_t* emap;
    enumerate_config_t config;

    if(configure(&config, argc, argv))
    {
        map = map_load(config.infile);
        s = unique_samples_create(map);
        unique_samples_save(s, config.unique);
        emap = enumerated_map_create(s, map);
        /* map_print(map); */
        /* enumerated_map_print(emap); */
        enumerated_map_save(emap, config.outfile);
        enumerated_map_destroy(emap);
        unique_samples_destroy(s);
        map_destroy(map);
    }
    return 0;
}
