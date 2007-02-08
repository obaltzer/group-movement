#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>

#include "base.h"
#include "list.h"

struct timeslice_config_s
{
    char datfile[256];
    char output_pattern[256];
    int samples_per_slice;
    int step_size;
};
typedef struct timeslice_config_s timeslice_config_t;

void usage()
{
    printf("timeslice [-n SAMPLES_PER_SLICE] [-s STEP_SIZE] [-o OUTPUT_PATTERN\n"\
           "          dataset.dat\n");
}

int configure(timeslice_config_t* config, int argc, char** argv)
{
    int ch;

    static struct option longopts[] = {
        {"samples-per-slice",   required_argument,  NULL,   'n'},
        {"step-size",           required_argument,  NULL,   's'},
        {"output-pattern",      required_argument,  NULL,   'o'},
        {NULL,                  0,                  NULL,   0}
    };

    config->samples_per_slice = 4;
    config->step_size = 1;
    strncpy(config->output_pattern, "output_%04d.dat", sizeof(config->output_pattern));

    while((ch = getopt_long(argc, argv, "n:s:o:", longopts, NULL)) != -1)
    {
        switch(ch)
        {
            case 'n':
                config->samples_per_slice = atoi(optarg);
                break;
            case 's':
                config->step_size = atoi(optarg);
                break;
            case 'o':
                strncpy(config->output_pattern, optarg, sizeof(config->output_pattern));
                config->output_pattern[sizeof(config->output_pattern) - 1] = '\0';
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
        strncpy(config->datfile, argv[0], sizeof(config->datfile));
    
    return TRUE;
}

dataset_t* time_slice_create(dataset_t* data, int* time_samples, int n_time_samples)
{
    int i;
    int j;
    int off;
    int n_samples;
    sample_t* samples;
    dataset_t* slice;
    list_t* l;

    slice = malloc(sizeof(dataset_t));
    if(slice == NULL)
    {
        printf("Cannot allocate memory for time slice.\n");
        return NULL;
    }
    memset(slice, 0, sizeof(dataset_t));
    slice->grid_size = data->grid_size;
    slice->max_t = data->max_t;

    /* allocate memory for the new trajectories */
    slice->n_trajectories = data->n_trajectories;
    slice->trajectories = malloc(sizeof(trajectory_t) * slice->n_trajectories);

    /* for each existing trajectory */
    for(i = 0; i < data->n_trajectories; i++)
    {
        l = list_create();
        off = 0;
        n_samples = data->trajectories[i].n_samples;
        samples = data->trajectories[i].samples;
        /* scan the samples of the trajectory for matching time samples and
         * append the matching samples to the list */
        for(j = 0; j < n_samples; j++)
        {
            for(off = 0; off < n_time_samples; off++)
            {
                if(samples[j].t == time_samples[off])
                    list_append(l, samples + j);
            }
        }
        /* assign extracted samples to the new trajectory */
        slice->trajectories[i].n_samples = list_length(l);
        if(slice->trajectories[i].n_samples)
            slice->trajectories[i].samples = list_to_array(l, sizeof(sample_t));
        else
            slice->trajectories[i].samples = NULL;
        slice->trajectories[i].trajectory_id = data->trajectories[i].trajectory_id;
        list_destroy(l, NULL);
    }

    /* now copy the groups over */
    slice->n_groups = data->n_groups;
    slice->groups = malloc(sizeof(group_t) * slice->n_groups);
    if(slice->groups == NULL)
    {
        printf("Cannot allocate memory for slice groups.\n");
        dataset_destroy(&slice); 
        return NULL;
    }
    for(i = 0; i < slice->n_groups; i++)
    {
        slice->groups[i].group_id = data->groups[i].group_id;
        slice->groups[i].n_trajectories = data->groups[i].n_trajectories;
        slice->groups[i].trajectories = malloc(sizeof(int) * slice->groups[i].n_trajectories);
        memcpy(slice->groups[i].trajectories, data->groups[i].trajectories, sizeof(int) * slice->groups[i].n_trajectories);
    }
    return slice;
}

int sample_time_order(const void* s1, const void* s2)
{
    sample_t* a = (sample_t*)s1;
    sample_t* b = (sample_t*)s2;

    if(a->t > b->t)
        return 1;
    else if(a->t < b->t)
        return -1;
    return 0;
}

void dataset_save(dataset_t* data, char* output)
{
    int fd;
    int i;

    if((fd = open(output, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1)
    {
        printf("Cannot create dataset file %s.\n", output);
        return;
    }
    write(fd, &data->grid_size, sizeof(int));
    write(fd, &data->max_t, sizeof(int));
    
    /* write trajectories */
    write(fd, &data->n_trajectories, sizeof(int));
    for(i = 0; i < data->n_trajectories; i++)
    {
        write(fd, &data->trajectories[i].trajectory_id, sizeof(int));
        write(fd, &data->trajectories[i].n_samples, sizeof(int));
        write(fd, data->trajectories[i].samples, sizeof(sample_t) * data->trajectories[i].n_samples);
    }

    /* write groups */
    write(fd, &data->n_groups, sizeof(int));
    for(i = 0; i < data->n_groups; i++)
    {
        write(fd, &data->groups[i].group_id, sizeof(int));
        write(fd, &data->groups[i].n_trajectories, sizeof(int));
        write(fd, data->groups[i].trajectories, sizeof(int) * data->groups[i].n_trajectories);
    }
    close(fd);
}

void time_slices_create(dataset_t* data, char* output_pattern, int samples_per_slice, int step_size)
{
    int n_samples = 0;
    int n_time_samples = 0;
    int n_slices;
    char output[256];
    int* time_samples;
    int* unique_samples;
    int i = 0;
    int j;
    int c = 0;
    int* cur = NULL;
    list_t* l;
    dataset_t* slice;

    for(; i < data->n_trajectories; i++)
    {
        /* sort each trajectory in increasing time for later */
        qsort(data->trajectories[i].samples, data->trajectories[i].n_samples, sizeof(sample_t), sample_time_order);
        n_samples += data->trajectories[i].n_samples;
    }

    time_samples = malloc(sizeof(int) * n_samples);
    if(time_samples == NULL)
    {   
        printf("Cannot allocate memory for time samples.\n");
        return;
    }
    /* Copy time samples to new array */
    for(i = 0; i < data->n_trajectories; i++)
        for(j = 0; j < data->trajectories[i].n_samples; j++)
            time_samples[c++] = data->trajectories[i].samples[j].t;

    /* sort time samples by increasing order */
    qsort(time_samples, n_samples, sizeof(int), int_cmp);
    
    /* determine the unique time samples */
    l = list_create();
    cur = &time_samples[0];
    for(i = 0; i < n_samples; i++)
    {
        if(time_samples[i] > *cur)
        {
            list_append(l, cur);
            cur = time_samples + i;
        }
    }
    list_append(l, cur);
    /* reassign the unique number of samples to n_samples */
    n_samples = list_length(l);
    /* create array from list */
    unique_samples = list_to_array(l, sizeof(int));
    list_destroy(l, NULL);
    free(time_samples);

    /* compute the number of time slices */
    n_slices = n_samples / step_size + (n_samples % step_size ? 1 : 0);
    for(i = 0; i < n_slices; i++)
    {
        snprintf(output, sizeof(output), output_pattern, i);
        /* compute the remaining samples in the unique_samples array */
        n_time_samples = i * step_size + samples_per_slice < n_samples ? samples_per_slice : n_samples - (i * step_size);
        slice = time_slice_create(data, unique_samples + (i * step_size), n_time_samples);
        dataset_save(slice, output);
        dataset_destroy(&slice);
    }
    free(unique_samples);
    return;
}

int main(int argc, char** argv)
{
    timeslice_config_t config;
    dataset_t* data;

    configure(&config, argc, argv);
    data = dataset_load(config.datfile);
    time_slices_create(data, config.output_pattern, config.samples_per_slice, config.step_size);
    dataset_destroy(&data);
    return 0;
}

    
