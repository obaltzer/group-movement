#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "base.h"

enum operation_type_e { OT_READ, OT_CREATE };

struct map_config_s
{
    enum operation_type_e ot;
    map_level_t level;
    char infile[256];
    char outfile[256];
    int print;
};
typedef struct map_config_s map_config_t;

int configure(map_config_t* config, int argc, char** argv);
map_level_t extract_level(char* level_str);
void usage();

int main(int argc, char** argv)
{
    map_config_t config;
    dataset_t* dataset;
    map_t* map;

    configure(&config, argc, argv);

    if(config.ot == OT_CREATE)
    {
        dataset = dataset_load(config.infile);
        if(dataset)
        {
            map = map_create(dataset, &config.level);
            if(config.print)
                map_print(map);
            map_save(config.outfile, map);
            map_destroy(map);
        }
        dataset_destroy(&dataset);
    }
    else if(config.ot == OT_READ)
    {
        map = map_load(config.infile);
        if(config.print)
            map_print(map);
        map_destroy(map);
    }
    return 0;
}

int configure(map_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"read",        no_argument,        NULL,   'r'},
        {"create",      no_argument,        NULL,   'c'},
        {"print",       no_argument,        NULL,   'p'},
        {"output",      required_argument,  NULL,   'o'},
        {"level",       required_argument,  NULL,   'l'},
        {NULL,          0,                  NULL,   0}
    };

    /* DEFAULTS */
    config->level.x = 0;
    config->level.y = 0;
    config->level.t = 0;
    config->ot = OT_CREATE;
    config->print = FALSE;
    strcpy(config->outfile, "output.map");

    while((ch = getopt_long(argc, argv, "rcpo:l:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'r':
                config->ot = OT_READ;
                break;
            case 'c':
                config->ot = OT_CREATE;
                break;
            case 'o':
                strncpy(config->outfile, optarg, sizeof(config->outfile) - 1);
                break;
            case 'l':
                config->level = extract_level(optarg);
                break;
            case 'p':
                config->print = TRUE;
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
    }
    strncpy(config->infile, argv[0], sizeof(config->infile) - 1);
    return 0;
}

map_level_t extract_level(char* level_str)
{
    char* s = level_str;
    char* state;
    map_level_t level;

    level.x = atoi(strtok_r(s, ",", &state));
    s = state;
    level.y = atoi(strtok_r(s, ",", &state));
    s = state;
    level.t = atoi(strtok_r(s, ",", &state));
    return level;
}

void usage()
{
    printf(
        "Usage:\n\n"\
        "map [(-r|--read) | (-c|--create)] [-p|--print] [-o|--output FILENAME]\n"\
        "    [-l|--level LEVEL] inputfile.(dat|map)\n\n"\
        "Options:\n"\
        " -r, --read\t\tread the map file specified as input for verification\n"\
        " -c, --create\t\tcreate a map from the specified data file\n"\
        " -p, --print\t\tprint out the either created or read map\n"\
        " -o, --output FILENAME\twrite map to this file (default: output.map)\n"\
        " -l, --level LEVEL\tuse comma separated levels in LEVEL (default: 0,0,0)\n");
    exit(-1);
}
