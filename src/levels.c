#include <stdio.h>
#include <limits.h>

#include "base.h"

int l2(int x)
{
    int c = 0;
    while(x >> c)
        c++;
    return c;
}

void usage()
{
    printf("level datafile.dat\n");
}

int main(int argc, char** argv)
{
    dataset_t* data;
    int t;
    int s;
    sample_t max;
    sample_t* sample;
    trajectory_t* trajectory;

    if(argc < 2)
    {
        usage();
        return -1;
    }

    data = dataset_load(argv[1]);
    max.x = INT_MIN;
    max.y = INT_MIN;
    max.t = INT_MIN;

    for(t = 0; t < data->n_trajectories; t++)
    {
        trajectory = data->trajectories + t;
        for(s = 0; s < trajectory->n_samples; s++)
        {
            sample = trajectory->samples + s;
            if(sample->x > max.x)
                max.x = sample->x;
            if(sample->y > max.y)
                max.y = sample->y;
            if(sample->t > max.t)
                max.t = sample->t;
        }
    }
    printf("%d %d %d\n", l2(max.x), l2(max.y), l2(max.t));
    dataset_destroy(&data);
    return 0;
}
