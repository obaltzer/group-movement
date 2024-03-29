#include <cairo.h>
#include <cairo-pdf.h>
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

double gray[] = { 0.8, 0.8, 0.8 /* dark grey */ };

struct visualize_config_s
{
    /* Variable: width
     * Width of the output image in pixel. */
    int width;
    
    /* Variable: height
     * Height of the output image in pixel. */
    int height;
    
    /* Variable: output
     * Buffer for the output image's filename or a filename pattern in case we write multiple images. */
    char output[256];
    
    /* Variable: input
     * Buffer for the dataset file's name. */
    char input[256];

    /* Variable: grpfile
     * Buffer for the group file's name. */
    char grpfile[256];

    /* Variable: have_groups
     * Flag whether a groups file was specified. */
    int have_groups;

    /* Variable: highlight
     * Flag whether to highlight groups with different colors. */
    int highlight;

    /* Variable: split
     * Flag whether to write each group into a separate file. */
    int split;

    /* Variable: line_width
     * Width used to draw a trajectory's line. */
    double line_width;

    /* Variable: print
     * Flag whether to print the dataset to stdout for debugging. */
    int print;

    /* Variable: overlay_groups
     * Flag whether to draw the reference dataset underneath the groups. */
    int overlay_groups;
};
typedef struct visualize_config_s visualize_config_t;

/* Forward declarations. */
void usage();
int configure(visualize_config_t* config, int argc, char** argv);
void dataset_draw(visualize_config_t* config, dataset_t* dataset, group_list_t* groups);
void dataset_draw_help(visualize_config_t* config, dataset_t* dataset, group_list_t* groups, cairo_t* cr);

int main(int argc, char** argv)
{
    /* configuration for the visualization program */
    visualize_config_t config;
    /* reference to the loaded dataset */
    dataset_t* dataset;
    group_list_t* groups = NULL;

    configure(&config, argc, argv);   
    
    /* load dataset from input file */
    dataset = dataset_load(config.input);
    if(dataset)
    {
        /* if a group list is defined, load that one too */
        if(config.have_groups)
            groups = group_list_load(config.grpfile);
        
        /* if the dataset is supposed to be printed to stdout in text format,
         * then do so */
        if(config.print)
            dataset_print(dataset);
        
        /* now draw the dataset */
        dataset_draw(&config, dataset, groups);
        
        /* release group list from memory if there is one */
        if(groups)
            group_list_destroy(groups);

        /* release dataset from memory */
        dataset_destroy(&dataset);

    }

    return 0;
}

int configure(visualize_config_t* config, int argc, char** argv)
{
    /* current option */
    int ch;

    static struct option longopts[] = {
        {"width",           required_argument,  NULL,   'w'},
        {"height",          required_argument,  NULL,   'h'},
        {"output",          required_argument,  NULL,   'o'},
        {"groups",          required_argument,  NULL,   'g'},
        {"split-groups",    no_argument,        NULL,   'G'},
        {"highlight",       no_argument,        NULL,   'l'},
        {"print",           no_argument,        NULL,   'p'},
        {"line-width",         required_argument,    NULL,   't'},
        {"overlay",            no_argument,    NULL,    'O'},
        {NULL,      0,                  NULL,   0 }
    };
    
    /* DEFAULTS */
    config->width = 400;
    config->height = 400;
    config->highlight = FALSE;
    config->print = FALSE;
    config->have_groups = FALSE;
    config->line_width = 0.5;
    config->split = FALSE;
    config->output[0] = '\0';
    config->overlay_groups = FALSE;
    
    while((ch = getopt_long(argc, argv, "Gg:w:h:o:g:lpt:O", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'w':
                config->width = atoi(optarg);
                break;
            case 'h':
                config->height = atoi(optarg);
                break;
            case 't':
                config->line_width = atof(optarg);
                break;
            case 'o':
                strncpy(config->output, optarg, sizeof(config->output));
                break;
            case 'g':
                strncpy(config->grpfile, optarg, sizeof(config->grpfile));
                config->have_groups = TRUE;
                config->highlight = TRUE;
                break;
            case 'G':
                config->split = TRUE;
                break;
            case 'l':
                config->highlight = TRUE;
                break;
            case 'p':
                config->print = TRUE;
                break;
            case 'O':
                config->overlay_groups = TRUE;
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
    
    if(!strlen(config->output))
    {
        switch(config->split)
        {
            case TRUE:
                strcpy(config->output, "output_%04d.pdf");
                break;
            default:
                strcpy(config->output, "output.pdf");
        }
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
        " -o, --output FILE\tfilename of the output PNG file (default: output.png)\n"\
        " -g, --groups FILE\tfilename of the groups definition file (default: none)\n"\
        " -G, --split-groups\tCreate an individual image for each group (default: off)\n"\
        " -t, --line-width\twidth of the lines to draw (default: 0.5)\n"
        " -O, --overlay\t\toverlay output PNG on reference image (default: off)\n");
    exit(-1);
}

/**
 * Function: dataset_draw
 *
 * Draws the dataset's trajectories into a PNG image highlighting different
 * groups with different colors.
 *
 * Parameters:
 * 
 *   config  - pointer to a <visualize_config_t> structure containing
 *             information about the size of the image to generate and to what
 *             file it should be saved to
 *
 *   dataset - pointer to a dataset
 *
 *   groups  - pointer to a groups list
 */
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
    char filename_buf[256];

    /* allocate memory for image buffer -- the area Cairo will draw in */
    if((imgbuf = malloc(sizeof(unsigned char) * config->width * config->height * 4)) == NULL)
    {
        printf("Cannot allocate memory for image buffer.\n");
        return;
    }

    /* create a drawing surface */
    #if 0
    surface = cairo_image_surface_create_for_data(
        imgbuf,
        CAIRO_FORMAT_ARGB32,
        config->width,
        config->height,
        config->width * 4
    );
    #endif
    surface = cairo_pdf_surface_create(
        config->output,
        config->width,
        config->height
    );
    
    /* create a cairo context on the surface */
    cr = cairo_create(surface);
        
    /* compute the scaling factor based on the size of the dataset grid and the
     * size of the image */
    scale_x = (double)config->width / (double)dataset->grid_size;
    scale_y = (double)config->height / (double)dataset->grid_size;
    
    /* set the line width */
    cairo_set_line_width(cr, config->line_width);

    /* a dataset can define its own groups, use those as default if there are
     * any */
    n_groups = dataset->n_groups;
    gl = dataset->groups;
    
    /* if there is an explicitly defined set of groups, use those instead */ 
    if(groups && (groups->n_groups > 0))
    {
        n_groups = groups->n_groups;
        gl = groups->groups;
    }
    
    /* if there are no groups defined or we only draw one image, fill the
     * surface with white background */
    if(!config->split || n_groups == 0)
    {
        cairo_rectangle(cr, 0, 0, (double)config->width, (double)config->height);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_fill(cr);
    }
    
    /* FIXME: don't know what happens here, I think this draws the entire
     * dataset */
    dataset_draw_help( config, dataset, groups, cr );
    
    /* If there are groups defined draw each group individually. */
    if(n_groups > 0)
    {
        /* for each group */
        for(g = 0; g < n_groups; g++)
        {
            /* If we split up the group into individual images, we need to
             * clear the current surface with a white background. */
            if(config->split)
            {
                cairo_rectangle(cr, 0, 0, (double)config->width, (double)config->height);
                cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
                cairo_fill(cr);
                dataset_draw_help( config, dataset, groups, cr );    // inserted ~~ wih fray
            }
                    
            /* for each trajectory in the group */
            for(t = 0; t < gl[g].n_trajectories; t++)
            {    
                trajectory_t* tr =
                    &dataset->trajectories[gl[g].trajectories[t]];
                /* move to the beginning of the trajectory */
                cairo_move_to(
                    cr, 
                    (double)tr->samples[0].x * scale_x, 
                    config->height - ((double)tr->samples[0].y * scale_y)
                );
                
                /* now draw a polyline by iterating through each of the
                 * trajectory's samples */
                for(s = 1; s < tr->n_samples; s++)
                    cairo_line_to(
                        cr, 
                        (double)tr->samples[s].x * scale_x,
                        config->height - ((double)tr->samples[s].y * scale_y)
                    );
            }

            /* If groups should have a different color than the other
             * (non-grouped) trajectories (config->highlight) and the current
             * group is in fact a group of more than one trajectories, then pick a
             * different color. */
            if(config->highlight && gl[g].n_trajectories != 1)
            {   
                fprintf(stderr, "Size of colors: %d\n", (g % (sizeof(colors) / 24)) * 3);
                c = &colors[(g % (sizeof(colors) / 24)) * 3];   
                cairo_set_source_rgb(cr, c[0], c[1], c[2]);
            }
            else
                /* otherwise draw everything greenish */
                cairo_set_source_rgb(cr, 0.0, 0.7, 0.0);

            /* finally draw all of the group */
            cairo_stroke(cr);
        }
    }

    /* free up all that memory */
    cairo_save (cr);
    cairo_show_page (cr);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(imgbuf);
}

void dataset_draw_help( visualize_config_t* config, dataset_t* dataset, group_list_t* groups, cairo_t* cr )
{
    int n_groups = dataset->n_groups;
    group_t* gl = dataset->groups;
    int t = 0;
    int s = 0;
    double scale_x = (double)config->width / (double)dataset->grid_size;
    double scale_y = (double)config->height / (double)dataset->grid_size;
    
    n_groups = dataset->n_groups;
    gl = dataset->groups;
    
    if(groups && (groups->n_groups > 0))
    {
        n_groups = groups->n_groups;
        gl = groups->groups;
    }
    
    
    if(!n_groups || config->overlay_groups)
    {
        if(!n_groups) {
            cairo_set_source_rgb(cr, 0.0, 0.7, 0.0);
        }
        else {         
            cairo_set_source_rgb(cr, gray[0], gray[1], gray[2]);
        }
        
        for(t = 0; t < dataset->n_trajectories; t++)
        {
            trajectory_t* tr = &dataset->trajectories[t];
            cairo_move_to(cr, (double)tr->samples[0].x * scale_x, 
                                        config->height - ((double)tr->samples[0].y * scale_y));
            for(s = 1; s < tr->n_samples; s++)
                cairo_line_to(
                                cr, 
                                (double)tr->samples[s].x * scale_x,
                                config->height - ((double)tr->samples[s].y * scale_y)
                                );
        }
        cairo_stroke(cr);
    }
}


