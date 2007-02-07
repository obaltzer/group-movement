#include <cairo.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "base.h"

double colors[] = { 
    0.7, 0.0, 0.0,   /* red           */
    0.0, 0.7, 0.0,   /* green         */
    0.0, 0.0, 0.7,   /* blue          */
    0.7, 0.0, 0.7,   /* pink          */
    0.0, 0.7, 0.7,   /* cyan          */
    0.7, 0.7, 0.0,   /* dirty yellow  */
    1.0, 0.5, 0.0,   /* orange        */
    0.0, 0.4, 0.0,   /* dark green    */
    0.3, 0.0, 0.6,   /* dark violet   */
    0.6, 0.6, 0.6,   /* gray          */
    1.0, 0.6, 0.6,   /* pastel red    */
    1.0, 1.0, 0.0,   /* yellow        */
    0.6, 0.6, 1.0,   /* pastel violet */
    0.4, 0.0, 0.0,   /* dark red      */
    0.0, 0.0, 0.4,   /* dark blue     */
    0.0, 0.25, 0.25  /* dark cyan   */
};

struct visualize_config_s
{
    int width;
    int height;
    char output[256];
    char input[256];
    char grpfile[256];
    int have_groups;
    int highlight;
    int print;
};
typedef struct visualize_config_s visualize_config_t;

void usage();
int configure(visualize_config_t* config, int argc, char** argv);
void dataset_draw(visualize_config_t* config, dataset_t* dataset, group_list_t* groups);

int main(int argc, char** argv)
{
    /* configuration for the visualization program */
    visualize_config_t config;
    /* reference to the loaded dataset */
    dataset_t* dataset;
    group_list_t* groups = NULL;

    configure(&config, argc, argv);   
    
    dataset = dataset_load(config.input);
    if(dataset)
    {
        if(config.have_groups)
            groups = group_list_load(config.grpfile); 
        
        if(config.print)
            dataset_print(dataset);
        dataset_draw(&config, dataset, groups);
        
        if(groups)
            group_list_destroy(groups);
        dataset_destroy(&dataset);

    }

    return 0;
}

int configure(visualize_config_t* config, int argc, char** argv)
{
    /* current option */
    int ch;

    static struct option longopts[] = {
        {"width",       required_argument,  NULL,   'w'},
        {"height",      required_argument,  NULL,   'h'},
        {"output",      required_argument,  NULL,   'o'},
        {"groups",      required_argument,  NULL,   'g'},
        {"highlight",   no_argument,        NULL,   'l'},
        {"print",       no_argument,        NULL,   'p'},
        {NULL,      0,                  NULL,   0 }
    };
    
    /* DEFAULTS */
    config->width = 400;
    config->height = 400;
    config->highlight = 0;
    config->print = FALSE;
    config->have_groups = FALSE;
    strcpy(config->output, "output.png");
    
    while((ch = getopt_long(argc, argv, "w:h:o:g:lp", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'w':
                config->width = atoi(optarg);
                break;
            case 'h':
                config->height = atoi(optarg);
                break;
            case 'o':
                strncpy(config->output, optarg, sizeof(config->output));
                break;
            case 'g':
                strncpy(config->grpfile, optarg, sizeof(config->grpfile));
                config->have_groups = TRUE;
                config->highlight = 1;
                break;
            case 'l':
                config->highlight = 1;
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
    
    if(config->width < 1)
    {
        printf("Image width must be at least 1.\n");
        exit(-2);
    }
    if(config->height < 1)
    {
        printf("Image height must be at least 1.\n");
        exit(-2);
    }
    if(argc == 0)
    {
        printf("Input file name missing.\n");
        usage();
    }
    strncpy(config->input, argv[0], sizeof(config->input) - 1);
    return 0;
}

/**
 * Print program usage and exit with code -1.
 */
void usage()
{
    printf(
        "Usage:\n\n"\
        "visualize [-w|--width n] [-h|--height n] [-l|--highlight]\n"\
        "          [-o|--output FILE] inputfile.dat\n\n"\
        "Options:\n"
        " -w, --width n\t\twidth n of the output image in pixels (default 400)\n"\
        " -h, --height n\t\theight n of the output image in pixels (default 400)\n"\
        " -l, --highlight\tif set known individual groups will be highlighted\n"\
        " -o, --output FILE\tfilename of the output PNG file (default: output.png)\n");
    exit(-1);
}

void dataset_draw(visualize_config_t* config, dataset_t* dataset, group_list_t* groups)
{
    unsigned char* imgbuf;
    cairo_surface_t* surface;
    cairo_t* cr;
    double scale_x;
    double scale_y;
    int g;
    int t;
    int s;
    int n_groups;
    group_t* gl;
    double* c;

    if((imgbuf = malloc(sizeof(unsigned char) * config->width * config->height * 4)) == NULL)
    {
        printf("Cannot allocate memory for image buffer.\n");
        return;
    }

    surface = cairo_image_surface_create_for_data(
        imgbuf,
        CAIRO_FORMAT_ARGB32,
        config->width,
        config->height,
        config->width * 4
    );

    cr = cairo_create(surface);

    cairo_rectangle(cr, 0, 0, (double)config->width, (double)config->height);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill(cr);
    
    scale_x = (double)config->width / (double)dataset->grid_size;
    scale_y = (double)config->height / (double)dataset->grid_size;

    cairo_set_line_width(cr, 1.0);
    n_groups = dataset->n_groups;
    gl = dataset->groups;
    if(groups && (groups->n_groups > 0))
    {
        n_groups = groups->n_groups;
        gl = groups->groups;
    }
    if(n_groups > 0)
    {
        for(g = 0; g < n_groups; g++)
        {
            for(t = 0; t < gl[g].n_trajectories; t++)
            {
                trajectory_t* tr =
                    &dataset->trajectories[gl[g].trajectories[t]];
                cairo_move_to(cr, (double)tr->samples[0].x * scale_x, 
                              (double)tr->samples[0].y * scale_y);
                for(s = 1; s < tr->n_samples; s++)
                    cairo_line_to(
                        cr, 
                        (double)tr->samples[s].x * scale_x,
                        (double)tr->samples[s].y * scale_y
                    );
            }
            if(config->highlight && gl[g].n_trajectories != 1)
            {   
                c = &colors[(g % (sizeof(colors) / 24)) * 3];   
                cairo_set_source_rgb(cr, c[0], c[1], c[2]);
            }
            else
                cairo_set_source_rgb(cr, 0.0, 0.7, 0.0);
            cairo_stroke(cr);
        }
    }
    else
    {
        cairo_set_source_rgb(cr, 0.0, 0.7, 0.0);
        for(t = 0; t < dataset->n_trajectories; t++)
        {
            trajectory_t* tr = &dataset->trajectories[t];
            cairo_move_to(cr, (double)tr->samples[0].x * scale_x, 
                          (double)tr->samples[0].y * scale_y);
            for(s = 1; s < tr->n_samples; s++)
                cairo_line_to(
                    cr, 
                    (double)tr->samples[s].x * scale_x,
                    (double)tr->samples[s].y * scale_y
                );
        }
        cairo_stroke(cr);
    }

    cairo_surface_write_to_png(surface, config->output);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(imgbuf);
}
