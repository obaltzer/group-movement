#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base.h"
#include "list.h"

int enumerated_map_save(enumerated_map_t* map, const char* filename)
{
    FILE* file;
    int i = 0;
    int j;

    if((file = fopen(filename, "w")) == NULL)
    {
        printf("Cannot open file %s.\n", filename);
        return FALSE;
    }
    for(; i < map->n_trajectories; i++)
    {
        for(j = 0; j < map->trajectories[i].n_sample_ids - 1; j++)
            fprintf(file, "%d ", map->trajectories[i].sample_ids[j]);
        fprintf(file, "%d\n", map->trajectories[i].sample_ids[j]);
    }
    fclose(file);
    return TRUE;
}

void enumerated_trajectory_print(enumerated_trajectory_t* t)
{
    int i = 0;
    printf("%d: ", t->trajectory_id);
    for(; i < t->n_sample_ids - 1; i++)
        printf("%d ", t->sample_ids[i]);
    printf("%d\n", t->sample_ids[i]);
}

void enumerated_map_print(enumerated_map_t* map)
{
    int i = 0;
    for(; i < map->n_trajectories; i ++)
        enumerated_trajectory_print(map->trajectories + i);
}

enumerated_map_t* enumerated_map_create(unique_samples_t* samples, map_t* map)
{
    int i = 0;
    enumerated_map_t* res;

    res = malloc(sizeof(enumerated_map_t));
    if(res == NULL)
    {
        printf("Cannot allocate memory for enumerated map.\n");
        return NULL;
    }
    memset(res, 0, sizeof(enumerated_map_t));
    res->n_trajectories = map->n_trajectories;
    res->trajectories = malloc(sizeof(enumerated_trajectory_t) * res->n_trajectories);
    if(res->trajectories == NULL)
    {
        printf("Cannot allocate memory for enumerated trajectories.\n");
        free(res);
        return NULL;
    }
    memset(res->trajectories, 0, sizeof(enumerated_trajectory_t) * res->n_trajectories);
    for(; i < res->n_trajectories; i++)
    {
        if(!enumerate_trajectory(map->trajectories + i, res->trajectories + i, samples))
        {
            enumerated_map_destroy(res);            
            return NULL;
        }
    }
    return res;
}

void enumerated_map_destroy(enumerated_map_t* map)
{
    if(map)
    {
        if(map->trajectories)
        {
            int i = 0;
            for(; i < map->n_trajectories; i++)
                free((map->trajectories + i)->sample_ids);
            free(map->trajectories);
        }
        free(map);
    }
}

int cmp_int(const void* a, const void* b)
{
    if(*(const int*)a == *(const int*)b)
    {
        return 0;
    }
    else if(*(const int*)a < *(const int*)b)
    {
        return -1;
    }
    else 
        return 1;
}

int enumerate_trajectory(trajectory_t* t, enumerated_trajectory_t* e, unique_samples_t* u)
{
    int i = 0;
    int* with_doubles;
    int prev;
    int n_unique = 0;
   
    e->trajectory_id = t->trajectory_id;
    
    /* enumerate samples first */
    with_doubles = malloc(sizeof(int) * t->n_samples);
    if(with_doubles == NULL)
    {
        printf("Cannot allocate memory for enumerated trajectory samples.\n");
        return FALSE;
    }
    for(; i < t->n_samples; i++)
        with_doubles[i] = unique_samples_index(u, t->samples + i);
    
    /* now count the number of unique ID without the duplicates */
    qsort(with_doubles, t->n_samples, sizeof(int), cmp_int);
    prev = 0;
    for(i = 1; i < t->n_samples; i++)
    {
        if(with_doubles[prev] != with_doubles[i])
        {
            n_unique++;
            prev = i;
        }
    }
    n_unique++;

    /* now allocate memory for the unique IDs and store them */
    e->n_sample_ids = n_unique;
    e->sample_ids = malloc(sizeof(int) * e->n_sample_ids);
    prev = 0;
    e->sample_ids[prev] = with_doubles[prev];
    n_unique = 0;
    for(i = 1; i < t->n_samples; i++)
    {
        if(e->sample_ids[prev] != with_doubles[i])
        {
            prev++;
            e->sample_ids[prev] = with_doubles[i];
        }
    }
    free(with_doubles);
    return TRUE;
}

int sample_compare(const void* a, const void* b)
{
    const sample_t* s1 = (const sample_t*)a;
    const sample_t* s2 = (const sample_t*)b;
    if(s1->x == s2->x)
        if(s1->y == s2->y)
            if(s1->t == s2->t)
                return 0;
            else if(s1->t > s2->t)
                return 1;
            else
                return -1;
        else if(s1->y > s2->y)
            return 1;
        else
            return -1;
    else if(s1->x > s2->x)
        return 1;
    else
        return -1;
}

unique_samples_t* unique_samples_create(map_t* map)
{
    int i = 0;
    int total = 0;
    int off = 0;
    sample_t* as;
    sample_t* prev;
    unique_samples_t* res;
    sample_t* next;

    for(; i < map->n_trajectories; i++)
        total += map->trajectories[i].n_samples;

    printf("Total number of samples: %d\n", total);

    /* allocate memory for all samples */
    as = malloc(sizeof(sample_t) * total);
    if(as == NULL)
    {
        printf("Cannot allocate memory to store all samples.\n");
        return NULL;
    }
    /* copy all samples into one big array */
    for(i = 0; i < map->n_trajectories; i++)
    {
        memcpy(as + off, map->trajectories[i].samples, sizeof(sample_t) * map->trajectories[i].n_samples);
        off += map->trajectories[i].n_samples;
    }
    /* sort the array */
    qsort((void*)as, total, sizeof(sample_t), sample_compare);
    /* determine number of unique samples */
    res = malloc(sizeof(unique_samples_t));
    if(res == NULL)
    {
        printf("Cannot allocate memory for unique samples data structure.\n");
        free(as);
        return NULL;
    }
    memset(res, 0, sizeof(unique_samples_t));
    res->n_samples = 1;
    prev = as;
    for(i = 0; i < total; i++)
    {
        if(sample_compare(prev, as + i) == -1)
        {
            res->n_samples++;
            prev = as + i;
        }
    }
    printf("Number of unique samples: %d\n", res->n_samples);
    res->samples = malloc(sizeof(sample_t) * res->n_samples);
    if(res->samples == NULL)
    {
        printf("Cannot allocate memory for unique samples.\n");
        free(as);
        free(res);
        return NULL;
    }
    prev = as;
    next = res->samples;
    for(i = 0; i < total; i++)
    {
        if(sample_compare(prev, as + i) == -1)
        {
            memcpy(next, prev, sizeof(sample_t));
            prev = as + i;
            next++;
        } 
    }
    memcpy(next, prev, sizeof(sample_t));
    free(as);
    return res;
}

void unique_samples_print(unique_samples_t* samples)
{
    int i = 0;
    for(; i < samples->n_samples; i++)
    {
        sample_print(samples->samples + i);
        putchar('\n');
    }
}

void unique_samples_destroy(unique_samples_t* samples)
{
    if(samples != NULL)
    {
        if(samples->samples != NULL)
            free(samples->samples);
        free(samples);
    }
}

void unique_samples_save(unique_samples_t* samples, const char* filename)
{
    FILE* out;
    int i = 0;

    if((out = fopen(filename, "w")) == NULL)
    {
        printf("Unable to create file unique samples file %s.\n", filename);
        return;
    }
    for(; i < samples->n_samples; i++)
    {
        sample_fprint(out, samples->samples + i);
        fputc('\n', out);
    }
    fclose(out);
}

int unique_samples_index(unique_samples_t* samples, sample_t* sample)
{
    int done = FALSE;
    int a = 0;
    int b = samples->n_samples - 1;
    int i;

    while(!done)
    {
        i = a + (b - a) / 2;
        switch(sample_compare(sample, samples->samples + i))
        {
            case 1:
                a = i + 1;
                break;
            case -1:
                b = i;
                break;
            case 0:
                done = TRUE;
                break;
        }
        if(b < 0 || a >= samples->n_samples)
            *((int*)0x0) = 10;
    }
    return i;
}

void dataset_print(dataset_t* dataset)
{
    int i = 0;
    for(; i < dataset->n_trajectories; i++)
        trajectory_print(&dataset->trajectories[i]);

    for(i = 0; i < dataset->n_groups; i++)
        group_print(&dataset->groups[i]);
}

void trajectory_print(trajectory_t* trajectory)
{
    int i = 0;
    printf("%d: ", trajectory->trajectory_id);
    if(trajectory->n_samples > 0)
    {
        for(; i < trajectory->n_samples - 1; i++)
        {
            sample_print(&trajectory->samples[i]);
            printf(", ");
        }
        sample_print(&trajectory->samples[i]);
    }
    printf("\n");
}

void sample_fprint(FILE* file, sample_t* sample)
{
    fprintf(file, "(%d, %d, %d)", sample->x, sample->y, sample->t);
}

void sample_print(sample_t* sample)
{
    sample_fprint(stdout, sample);    
}

void group_print(group_t* group)
{
    int i = 0;
    printf("Group %d: ", group->group_id);
    fflush(stdout);
    for(; i < group->n_trajectories - 1; i++)
        printf("%d, ", group->trajectories[i]);
    printf("%d\n", group->trajectories[i]);
}

void dataset_destroy(dataset_t** dataset_ref)
{
    dataset_t* d = *dataset_ref;

    if(d != NULL)
    {
        if(d->trajectories != NULL)
        {
            int i = 0;
            for(; i < d->n_trajectories; i++)
                trajectory_destroy(&d->trajectories[i]);
            free(d->trajectories);
            d->trajectories = NULL;
        }

        if(d->groups != NULL)
        {
            int i = 0;
            for(; i < d->n_groups; i++)
                group_destroy(&d->groups[i]);
            free(d->groups);
            d->groups = NULL;
        }
        free(d);
        *dataset_ref = NULL;
    }
}

void group_destroy(group_t* g)
{
    if(g != NULL)
    {
        if(g->trajectories != NULL)
        {
            free(g->trajectories);
            g->trajectories = NULL;
        }
    }
}

void sample_destroy(sample_t* sample)
{
    /* */
}

void trajectory_destroy(trajectory_t* t)
{
    if(t != NULL) 
    {
        if(t->samples != NULL) 
        {
            int i = 0;
            for(; i < t->n_samples; i++)
                sample_destroy(&t->samples[i]);
            free(t->samples);
            t->samples = NULL;
        }
    }
}

dataset_t* dataset_load(const char* filename)
{
    dataset_t* dataset;
    int fd;
    int i;

    if((fd = open(filename, O_RDONLY)) == -1)
    {
        printf("Cannot open input file '%s'\n", filename);
        return NULL;
    }
    
    /* allocate memory for dataset structure */
    dataset = malloc(sizeof(dataset_t));
    if(dataset == NULL)
    {
        printf("Cannot allocate memory for dataset structure.\n");
        return NULL;
    }
    memset(dataset, 0, sizeof(dataset_t));

    read(fd, &dataset->grid_size, sizeof(int));
    read(fd, &dataset->max_t, sizeof(int));
    read(fd, &dataset->n_trajectories, sizeof(int));
    dataset->trajectories = 
        malloc(dataset->n_trajectories * sizeof(trajectory_t));
    if(dataset->trajectories == NULL)
    {
        printf("Cannot allocate memory for trajectories.\n");
        dataset_destroy(&dataset);
        return NULL;
    }
    memset(dataset->trajectories, 0, dataset->n_trajectories * sizeof(trajectory_t));
    for(i = 0; i < dataset->n_trajectories; i++)
    {
        read(fd, &dataset->trajectories[i].trajectory_id, sizeof(trajectory_id_t));
        read(fd, &dataset->trajectories[i].n_samples, sizeof(int));
        dataset->trajectories[i].samples = 
            malloc(dataset->trajectories[i].n_samples * sizeof(sample_t));
        if(dataset->trajectories[i].samples == NULL) 
        {
            printf("Cannot allocate memory for samples of trajectory: %d\n",
                   dataset->trajectories[i].trajectory_id);
            dataset_destroy(&dataset);
            return NULL;
        }
        read(fd, dataset->trajectories[i].samples, 
             sizeof(sample_t) * dataset->trajectories[i].n_samples);
    }
    
    /* load all known groups */
    read(fd, &dataset->n_groups, sizeof(int));
    dataset->groups = malloc(sizeof(group_t) * dataset->n_groups);
    if(dataset->groups == NULL)
    {
        printf("Cannot allocate memory for group structures.\n");
        dataset_destroy(&dataset);
        return NULL;
    }
    memset(dataset->groups, 0, sizeof(group_t) * dataset->n_groups);
    for(i = 0; i < dataset->n_groups; i++)
    {
        read(fd, &dataset->groups[i].group_id, sizeof(group_id_t));
        read(fd, &dataset->groups[i].n_trajectories, sizeof(int));
        dataset->groups[i].trajectories = 
            malloc(sizeof(trajectory_id_t) * dataset->groups[i].n_trajectories);
        if(dataset->groups[i].trajectories == NULL)
        {
            printf("Cannot allocate memory for trajectory ID list for "\
                   "group: %d\n", dataset->groups[i].group_id);
            dataset_destroy(&dataset);
            return NULL;
        }
        read(fd, dataset->groups[i].trajectories, 
             sizeof(trajectory_id_t) * dataset->groups[i].n_trajectories);
    }

    return dataset;
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

int trajectory_write(trajectory_t* t, int fd)
{
    /* int i = 0; */

    write(fd, &t->trajectory_id, sizeof(trajectory_id_t));
    write(fd, &t->n_samples, sizeof(int));
    write(fd, t->samples, sizeof(sample_t) * t->n_samples);
    /*
    for(; i < t->n_samples; i++)
        if(!sample_write(t->samples + i, fd))
            return FALSE;
    */

    return TRUE;
}

int trajectory_read(trajectory_t* t, int fd)
{
    /* int i = 0; */

    read(fd, &t->trajectory_id, sizeof(trajectory_id_t));
    read(fd, &t->n_samples, sizeof(int));
    t->samples = malloc(sizeof(sample_t) * t->n_samples);
    if(t->samples == NULL)
    {
        printf("Unable to allocate memory for trajectory.\n");
        return FALSE;
    }
    memset(t->samples, 0, sizeof(sample_t) * t->n_samples);
    read(fd, t->samples, sizeof(sample_t) * t->n_samples);
    
    /*
    for(; i < t->n_samples; i++)
        if(!sample_read(t->samples + i, fd))
            return FALSE;
    */
    return TRUE;
}

int samples_write(sample_t* s, int fd)
{
    /* write(fd, s, sizeof(sample_t)); */
    return TRUE;
}

int sample_read(sample_t* s, int fd)
{
    /* read(fd, s, sizeof(sample_t)); */
    return TRUE;
}

void sample_map(sample_t* in, sample_t* out, map_level_t* level)
{
    out->x = (in->x >> level->x) << level->x;
    out->y = (in->y >> level->y) << level->y;
    out->t = (in->t >> level->t) << level->t;
}

int trajectory_map(trajectory_t* in, trajectory_t* out, map_level_t* level)
{
    int i = 0;
    
    out->trajectory_id = in->trajectory_id;
    out->n_samples = in->n_samples;
    if(out->samples == NULL)
    {
        out->samples = malloc(sizeof(sample_t) * out->n_samples);
        if(out->samples == NULL)
        {
            printf("Cannot allocated memory for mapped samples.\n");
            return FALSE;
        }
        memset(out->samples, 0, sizeof(sample_t) * out->n_samples);
    }
    for(; i < in->n_samples; i++)
        sample_map(in->samples + i, out->samples + i, level);
    return TRUE;
}

map_t* map_create(dataset_t* dataset, map_level_t* level)
{
    map_t* map;
    int i = 0;

    map = malloc(sizeof(map_t));
    if(map == NULL)
    {
        printf("Cannot allocate memory for map.\n");
        return NULL;
    }
    memset(map, 0, sizeof(map_t));
    map->level = *level;
    map->n_trajectories = dataset->n_trajectories;
    map->trajectories = malloc(sizeof(trajectory_t) * map->n_trajectories);
    if(map->trajectories == NULL)
    {
        printf("Cannot allocate memory for trajectories.\n");
        map_destroy(map);
        return NULL;
    }
    memset(map->trajectories, 0, sizeof(trajectory_t) * map->n_trajectories);
    for(; i < map->n_trajectories; i++)
    {
        if(!trajectory_map(dataset->trajectories + i, map->trajectories + i, level))
        {
            printf("Could not map trajectory %d.\n", i);
            map_destroy(map);
            return NULL;
        }
    }
    return map;
}

void map_destroy(map_t* map)
{
    int i = 0;
    if(map != NULL && map->n_trajectories)
    {
        for(; i < map->n_trajectories; i++)
            trajectory_destroy(map->trajectories + i);
        free(map->trajectories);
        free(map);
    }
}

void map_save(const char* filename, map_t* map)
{
    int fd = -1;
    int i = 0;

    if((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
    {
        printf("Unable to open map file %s.\n", filename);
        return;
    }

    write(fd, &map->level, sizeof(map_level_t));
    write(fd, &map->n_trajectories, sizeof(int));
    for(; i < map->n_trajectories; i++)
        trajectory_write(map->trajectories + i, fd);
    close(fd);
}

map_t* map_load(const char* filename)
{
    int fd  = -1;
    int i = 0;
    map_t* map;

    if((fd = open(filename, O_RDONLY)) == -1)
    {
        printf("Unable to open map file %s.\n", filename);
        return NULL;
    }

    map = malloc(sizeof(map_t));
    if(map == NULL)
    {
        printf("Cannot allocate memory for map.\n");
        return NULL;
    }
    memset(map, 0, sizeof(map_t));

    read(fd, &map->level, sizeof(map_level_t));
    read(fd, &map->n_trajectories, sizeof(int));
    map->trajectories = malloc(sizeof(trajectory_t) * map->n_trajectories);
    if(map->trajectories == NULL)
    {
        printf("Cannot allocate memory for trajectories.\n");
        map_destroy(map);
        return NULL;
    }
    memset(map->trajectories, 0, sizeof(trajectory_t) * map->n_trajectories);
    for(; i < map->n_trajectories; i++)
    {
        if(!trajectory_read(map->trajectories + i, fd))
        {
            printf("Could not read trajectory %d.\n", i);
            map_destroy(map);
            return NULL;
        }
    }
    close(fd);
    return map;
}

void map_print(map_t* map)
{
    int i = 0;

    for(; i < map->n_trajectories; i++)
        trajectory_print(map->trajectories + i);
}

void frequent_itemset_destroy(void* fi)
{
    if(fi)
    {
        if(((frequent_itemset_t*)fi)->items)
            free(((frequent_itemset_t*)fi)->items);
        free(fi);    
    }
}
    
void frequent_itemset_print(frequent_itemset_t* fi)
{
    int i = 0;
    printf("%d: ", fi->support);
    for(; i < fi->n_items - 1; i++)
        printf("%d, ", fi->items[i]);
    printf("%d\n", fi->items[i]);
}

frequent_itemset_t* frequent_itemset_from_line(char* line)
{
    char* tmp;
    char* sep = " ";
    char* item;
    char* state;
    int c = 0;
    char* endPtr;
    long item_value;
    frequent_itemset_t* fi;

    if(line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
    tmp = malloc(sizeof(char) * (strlen(line) + 1));
    strcpy(tmp, line);
    for(item = strtok_r(tmp, sep, &state); item; item = strtok_r(NULL, sep, &state))
    {
        strtol(item, &endPtr, 10);
        if(*item != '\0' && *endPtr == '\0')
            c++;
    }
    free(tmp);
    
    fi = malloc(sizeof(frequent_itemset_t));
    if(fi == NULL)
    {
        printf("Cannot allocate memory for frequent itemset.\n");
        return NULL;
    }
    memset(fi, 0, sizeof(frequent_itemset_t));
    fi->n_items = c;
    fi->items = malloc(sizeof(int) * fi->n_items);
    c = 0;
    for(item = strtok_r(line, sep, &state); item; item = strtok_r(NULL, sep, &state))
    {
        item_value = strtol(item, &endPtr, 10);
        if(*item != '\0' && *endPtr == '\0')
        {
            fi->items[c] = item_value;
            c++;
        }
        else if(item[0] == '(' && item[strlen(item) - 1] == ')')
        {
            item[0] = ' ';
            item[strlen(item) - 1] = '\0';
            fi->support = strtol(item, &endPtr, 10);
        }
    }
    return fi;
}

int subsequence_match(int* seq, size_t seq_s, int* sub, size_t sub_s)
{
    int i = 0;
    int c = 0;

    qsort(seq, seq_s, sizeof(int), int_cmp);
    qsort(sub, sub_s, sizeof(int), int_cmp);
    if(seq_s < sub_s)
        return FALSE;
    else
    {
        for(; i < seq_s; i++)
        {
            if(seq[i] == sub[c])
            {
                c++;
                if(c == sub_s)
                    return TRUE;
            }
        }
    }
    return FALSE;
}

void clique_destroy(void* clique)
{
    if(clique)
    {
        if(((clique_t*)clique)->trajectories)
            free(((clique_t*)clique)->trajectories);
        free(clique);
    }
}

clique_t* clique_create(frequent_itemset_t* fi, enumerated_map_t* emap)
{
    int i = 0;
    clique_t* clique;
    enumerated_trajectory_t* t;
    list_t* l;
    void* l_status;

    l = list_create();
    for(; i < emap->n_trajectories; i++)
    {
        t = emap->trajectories + i;
        if(subsequence_match(t->sample_ids, t->n_sample_ids, fi->items, fi->n_items))
            list_append(l, t);
    }
    clique = malloc(sizeof(clique_t));
    if(clique == NULL)
    {
        printf("Cannot allocate memory for new clique.\n");
        list_destroy(l, NULL);
        return NULL;
    }
    memset(clique, 0, sizeof(clique_t));
    clique->frequent_itemset_id = fi->id;
    clique->n_trajectories = list_length(l);
    clique->trajectories = malloc(sizeof(int) * clique->n_trajectories);
    if(clique->trajectories == NULL)
    {
        printf("Cannot allocate memory for matched trajectories.\n");
        list_destroy(l, NULL);
        clique_destroy(clique);
        return NULL;
    }
    i = 0;
    for(t = list_iterate(l, &l_status); l_status; t = list_iterate(NULL, &l_status))
        clique->trajectories[i++] = t->trajectory_id;
    list_destroy(l, NULL);
    return clique;
}

void clique_print(clique_t* clique)
{
    printf("%d: ", clique->frequent_itemset_id);
    if(clique->n_trajectories)
    {
        int i;
        printf("Matching trajectories: ");
        for(i = 0; i < clique->n_trajectories - 1; i++)
            printf("%d, ", clique->trajectories[i]);
        printf("%d\n", clique->trajectories[i]);
    }
}

char* line_read(FILE* f)
{
    char buf[256];
    char* line = NULL;
    size_t s = 0;

    memset(buf, 0, sizeof(buf) * sizeof(char));
    do
    {
        if(fgets(buf, sizeof(buf), f) == NULL)
            return line;
        s += strlen(buf);
        if(line)
            line = realloc(line, sizeof(char) * (s + 1));
        else
        {
            line = malloc(sizeof(char) * (s + 1));
            memset(line, 0, sizeof(char) * (s + 1));
        }
        if(line)
            strncat(line, buf, strlen(buf));
    }
    while(strlen(buf) == 255 && buf[254] != '\n');

    return line;
}
        
void line_destroy(char* line)
{
    if(line)
        free(line);
}

enumerated_trajectory_t* enumerated_trajectory_from_line(char* line)
{
    enumerated_trajectory_t* t;
    char* tmp;
    char* sep = " ";
    char* tok;
    char* state;
    int c = 0;
    char* endPtr;
    long sample;

    if(line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
    
    tmp = malloc(sizeof(char) * (strlen(line) + 1));
    strcpy(tmp, line);
    for(tok = strtok_r(tmp, sep, &state); tok; tok = strtok_r(NULL, sep, &state))
    {
        strtol(tok, &endPtr, 10);
        if(*tok != '\0' && *endPtr == '\0')
            c++;
    }
    free(tmp);
    
    t = malloc(sizeof(enumerated_trajectory_t));
    if(t == NULL)
    {
        printf("Cannot allocate memory for enumerated trajectory.\n");
        return NULL;
    }
    memset(t, 0, sizeof(enumerated_trajectory_t));
    t->n_sample_ids = c;
    t->sample_ids = malloc(sizeof(int) * t->n_sample_ids);
    c = 0;
    for(tok = strtok_r(line, sep, &state); tok; tok = strtok_r(NULL, sep, &state))
    {
        sample = strtol(tok, &endPtr, 10);
        if(*tok != '\0' && *endPtr == '\0')
        {
            t->sample_ids[c] = sample;
            c++;
        }
    }
    return t;
}

enumerated_map_t* enumerated_map_load(char* filename)
{
    FILE* in;
    char* line;
    enumerated_map_t* emap;
    enumerated_trajectory_t* t;
    list_t* trajectories;
    int c = 0;

    if((in = fopen(filename, "r")) == NULL)
    {
        printf("Cannot open enumerated map file %s.\n", filename);
        return NULL;
    }
    trajectories = list_create();
    while((line = line_read(in)) != NULL)
    {
        t = enumerated_trajectory_from_line(line);
        t->trajectory_id = c++;
        list_append(trajectories, t);
        line_destroy(line);
    }
    fclose(in);

    emap = malloc(sizeof(enumerated_map_t));
    if(emap == NULL)
    {
        printf("Cannot allocate memory for enumerated map.\n");
        /* XXX deallocate individual trajectories here, as the list has the
         * only references to the blocks */
        list_destroy(trajectories, free);
    }
    emap->n_trajectories = list_length(trajectories);
    emap->trajectories = list_to_array(trajectories, sizeof(enumerated_trajectory_t));
    list_destroy(trajectories, free);
    
    return emap;
}

frequent_itemset_list_t* frequent_itemset_list_load(char* filename, int min_length)
{
    FILE* in;
    list_t* l;
    char* line;
    frequent_itemset_t* fi;
    frequent_itemset_list_t* fil;
    int i = 0;

    if((in = fopen(filename, "r")) == NULL)
    {
        printf("Cannot open frequent itemset list file %s.\n", filename);
        return NULL;
    }
    l = list_create();
    while((line = line_read(in)) != NULL)
    {
        fi = frequent_itemset_from_line(line);
        fi->id = i++;
        if(fi->n_items >= min_length)
            list_append(l, fi);
        else
            frequent_itemset_destroy(fi);
        line_destroy(line);
    }
    fclose(in);
    fil = malloc(sizeof(frequent_itemset_list_t));
    if(fil == NULL)
    {
        printf("Cannot allocate memory for frequent itemset list.\n");
        list_destroy(l, frequent_itemset_destroy);
        return NULL;
    }
    memset(fil, 0, sizeof(frequent_itemset_list_t));
    fil->n_frequent_itemsets = list_length(l);
    fil->frequent_itemsets = list_to_array(l, sizeof(frequent_itemset_t));
    list_destroy(l, free);
    return fil;
}

void frequent_itemset_list_destroy(frequent_itemset_list_t* fil)
{
    if(fil)
    {
        if(fil->frequent_itemsets)
        {
            int i = 0;
            for(; i < fil->n_frequent_itemsets; i++)
                free(fil->frequent_itemsets[i].items);
            free(fil->frequent_itemsets);
        }
        free(fil);
    }
}

clique_list_t* clique_list_create(frequent_itemset_list_t* fil, enumerated_map_t* emap)
{
    int i = 0;
    list_t* l;
    clique_list_t* cl;

    l = list_create();
    for(; i < fil->n_frequent_itemsets; i++)
        list_append(l, clique_create(fil->frequent_itemsets + i, emap));
    cl = malloc(sizeof(clique_list_t));
    if(cl == NULL)
    {
        printf("Cannot allocate memory for clique list.\n");
        list_destroy(l, clique_destroy);
        return NULL;
    }
    cl->n_cliques = list_length(l);
    cl->cliques = list_to_array(l, sizeof(clique_t));
    list_destroy(l, free);
    return cl;
}

void clique_list_destroy(clique_list_t* cl)
{
    if(cl)
    {
        if(cl->cliques)
        {
            int i = 0;
            for(; i < cl->n_cliques; i++)
                free(cl->cliques[i].trajectories);
            free(cl->cliques);
        }
        free(cl);
    }
}

void clique_list_save(clique_list_t* cl, char* filename)
{
    FILE* out;
    int i = 0;
    int j;

    if((out = fopen(filename, "w")) == NULL)
    {
        printf("Cannot write clique list file %s.\n", filename);
        return;
    }
    for(; i < cl->n_cliques; i++)
    {
        fprintf(out, "%d: ", cl->cliques[i].frequent_itemset_id);
        for(j = 0; j < cl->cliques[i].n_trajectories - 1; j++)
            fprintf(out, "%d ", cl->cliques[i].trajectories[j]);
        fprintf(out, "%d\n", cl->cliques[i].trajectories[j]);
    }
    fclose(out);
}

clique_list_t* clique_list_load(char* filename)
{
    FILE* in;
    int i = 0;
    char* tok;
    char* line;
    char* tmp;
    char* status;
    char* endPtr;
    clique_list_t* cl;
    clique_t* c;
    list_t* cliques;
    list_t* trajectories;
    void* l_status;

    if((in = fopen(filename, "r")) == NULL)
    {
        printf("Cannot open clique list file %s.\n", filename);
        return NULL;
    }

    cliques = list_create();
    while((line = line_read(in)) != NULL)
    {
        c = malloc(sizeof(clique_t));
        if(c == NULL)
        {
            printf("Unable to allocate memory for clique.\n");
            line_destroy(line);
            continue;
        }
        memset(c, 0, sizeof(clique_t));
        if(line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';
        tmp = malloc(sizeof(char) * (strlen(line) + 1));
        strcpy(tmp, line);
        tok = strtok_r(tmp, ":", &status);
        c->frequent_itemset_id = strtol(tok, &endPtr, 10);
        if(*tok == '\0' || *endPtr != '\0')
        {
            printf("Format error (FI ID): %s\n", line);
            line_destroy(line);
            clique_destroy(c);
            free(tmp);
            continue;
        }
        trajectories = list_create();
        for(tok = strtok_r(NULL, " ", &status); tok; tok = strtok_r(NULL, " ", &status))
            list_append(trajectories, tok);

        c->n_trajectories = list_length(trajectories);
        c->trajectories = malloc(sizeof(int) * c->n_trajectories);
        if(c->trajectories == NULL)
        {
            printf("Cannot allocate memory for trajectories.\n");
            line_destroy(line);
            clique_destroy(c);
            free(tmp);
            list_destroy(trajectories, NULL);
            continue;
        }
        i = 0;
        for(tok = list_iterate(trajectories, &l_status); l_status; tok = list_iterate(NULL, &l_status))
        {
            c->trajectories[i++] = strtol(tok, &endPtr, 10);
            if(*tok == '\0' || *endPtr != '\0')
                break;
        }
        list_destroy(trajectories, NULL);
        if(l_status)
        {
            printf("Format error (TR ID): %s\n", line);
            line_destroy(line);
            clique_destroy(c);
            free(tmp);
            continue;
        }
        list_append(cliques, c);
        line_destroy(line);
        free(tmp);
    }
    fclose(in);
    cl = malloc(sizeof(clique_list_t));
    if(cl == NULL)
    {
        printf("Cannot allocate memory for clique list.\n");
        list_destroy(cliques, clique_destroy);
        return NULL;
    }
    cl->n_cliques = list_length(cliques);
    cl->cliques = list_to_array(cliques, sizeof(clique_t));
    list_destroy(cliques, free);
    return cl;
}

void clique_list_print(clique_list_t* cl)
{
    int i = 0;
    for(; i < cl->n_cliques; i++)
        clique_print(cl->cliques + i);
}

void matrix_destroy(matrix_t* matrix)
{
    if(matrix)
    {
        if(matrix->matrix)
            free(matrix->matrix);
        free(matrix);
    }
}

matrix_t* matrix_create(dataset_t* data, clique_list_t* cl, size_t weight_size,
                        void* user_data,
                        void (*weight_function)(clique_t*, int, int, void*, void*))
{
    int c_i = 0;
    int t1;
    int t2;
    matrix_t* matrix;
    void* w;

    matrix = malloc(sizeof(matrix_t));
    if(matrix == NULL)
    {
        printf("Cannot allocate memory for matrix structure.\n");
        return NULL;
    }
    memset(matrix, 0, sizeof(matrix_t));
    matrix->weight_size = weight_size;
    matrix->n_trajectories = data->n_trajectories;
    matrix->matrix = malloc(matrix->weight_size * matrix->n_trajectories * matrix->n_trajectories);
    if(matrix->matrix == NULL)
    {
        printf("Cannot allocate memory for matrix data.\n");
        matrix_destroy(matrix);
        return NULL;
    }
    memset(matrix->matrix, 0, matrix->weight_size * matrix->n_trajectories * matrix->n_trajectories);
    for(c_i = 0; c_i < cl->n_cliques; c_i++)
    {
        for(t1 = 0; t1 < cl->cliques[c_i].n_trajectories; t1++)
        {
            for(t2 = 0; t2 < cl->cliques[c_i].n_trajectories; t2++)
            {
                w = matrix->matrix + (matrix->weight_size * ((matrix->n_trajectories * cl->cliques[c_i].trajectories[t2]) + cl->cliques[c_i].trajectories[t1]));
                weight_function(cl->cliques + c_i, t1, t2, w, user_data);
            }
        }
    }
    return matrix;
}

void group_list_destroy(group_list_t* groups)
{
    if(groups)
    {
        if(groups->groups)
        {
            int i = 0;
            for(; i < groups->n_groups; i++)
                if(groups->groups[i].trajectories)
                    free(groups->groups[i].trajectories);
            free(groups->groups);
        }
        free(groups);
    }
}

group_list_t* group_list_from_clique_list(clique_list_t* cliques)
{
    int i = 0;
    group_list_t* groups;

    groups = malloc(sizeof(group_list_t));
    if(groups == NULL)
    {
        printf("Cannot allocate memory for group list.\n");
        return NULL;
    }
    memset(groups, 0, sizeof(group_list_t));
    groups->n_groups = cliques->n_cliques;
    groups->groups = malloc(sizeof(group_t) * groups->n_groups);
    if(groups->groups == NULL)
    {
        printf("Cannot allocate memory to store groups.\n");
        group_list_destroy(groups);
        return NULL;
    }
    memset(groups->groups, 0, sizeof(group_t) * groups->n_groups);
    for(; i < cliques->n_cliques; i++)
    {
        groups->groups[i].n_trajectories = cliques->cliques[i].n_trajectories;
        groups->groups[i].trajectories = malloc(sizeof(int) * groups->groups[i].n_trajectories);
        if(groups->groups[i].trajectories == NULL)
        {
            printf("Cannot allocate memory for group trajectories.\n");
            group_list_destroy(groups);
            return NULL;
        }
        memcpy(groups->groups[i].trajectories, cliques->cliques[i].trajectories, sizeof(int) * groups->groups[i].n_trajectories);
    }
    return groups;
}

int int_cmp(const void* i1, const void* i2)
{
    if(*((int*)i1) > *((int*)i2))
        return 1;
    else if(*((int*)i1) < *((int*)i2))
        return -1;
    return 0;
}

group_t group_merge(group_t* g1, group_t* g2)
{
    int* t;
    int last = -1;
    int i;
    list_t* l;
    group_t g;
  
    g.group_id = g1->group_id;
    g.n_trajectories = 0;
    g.trajectories = NULL;
    t = malloc(sizeof(int) * (g1->n_trajectories + g2->n_trajectories));
    if(t == NULL)
    {
        printf("Cannot allocate memory for temporary trajectory storage.\n");
        return g;
    }
    memcpy(t, g1->trajectories, sizeof(int) * g1->n_trajectories);
    memcpy(t + g1->n_trajectories, g2->trajectories, sizeof(int) * g2->n_trajectories);

    qsort(t, g1->n_trajectories + g2->n_trajectories, sizeof(int), int_cmp);
    l = list_create();
    for(i = 0; i < g1->n_trajectories + g2->n_trajectories; i++)
        if(*(t + i) != last)
        {
            list_append(l, t + i);
            last = *(t + i);
        }
    g.n_trajectories = list_length(l);
    g.trajectories = list_to_array(l, sizeof(int));
    list_destroy(l, NULL);
    free(t);
    return g;
}

void group_list_merge(group_list_t* groups, double (*compute_strength)(group_list_t*, int, int, void*), void* user_data)
{
    int i;
    int j;
    int c1;
    int c2;
    double max = 0.0;
    double cur;
    group_t* new_list;
    double* sm;
    int n_base_groups;

    /* pre-compute pairwise strength matrix */
    n_base_groups = groups->n_groups;
    sm = malloc(sizeof(double) * n_base_groups * n_base_groups);
    if(sm == NULL)
    {
        fprintf(stderr, "Cannot allocate strength matrix.\n");
        return;
    }
    memset(sm, 0, sizeof(double) * n_base_groups * n_base_groups);
    for(i = 0; i < groups->n_groups; i++)
    {
        /* label each group with a unique ID */
        groups->groups[i].group_id = i;

        for(j = 0; j < groups->n_groups; j++)
            sm[i * n_base_groups + j] = compute_strength(groups, i, j, user_data);
    }


    do
    {
        c1 = -1;
        c2 = -1;
        max = 0.0;
        for(i = 0; i < groups->n_groups; i++)
        {
            for(j = i + 1; j < groups->n_groups; j++)
            {
                cur = sm[groups->groups[i].group_id * n_base_groups + groups->groups[j].group_id];
                if(cur > max)
                {
                    c1 = i;
                    c2 = j;
                    max = cur;
                }
            }
        }
        
        if(c1 != -1 && c2 != -1)
        {
            new_list = malloc(sizeof(group_t) * (groups->n_groups - 1));
            j = 0;
            for(i = 0; i < groups->n_groups; i++)
            {
                if(i != c1 && i != c2)
                    new_list[j++] = groups->groups[i];
            }
            new_list[j] = group_merge(groups->groups + c1, groups->groups + c2);
            free(groups->groups[c1].trajectories);
            free(groups->groups[c2].trajectories);
            free(groups->groups);
            groups->groups = new_list;
            groups->n_groups--;
            for(i = 0; i < groups->n_groups - 1; i++)
                sm[new_list[j].group_id * n_base_groups + new_list[i].group_id] = 
                sm[new_list[i].group_id * n_base_groups + new_list[j].group_id] = 
                    compute_strength(groups, i, j, user_data);
        }
    }
    while(c1 != -1 && c2 != -1);
    free(sm);
}

void group_list_print(group_list_t* groups)
{
    int i = 0;

    for(; i < groups->n_groups; i++)
        group_print(groups->groups + i);
}

void group_list_save(group_list_t* groups, char* filename)
{
    int i = 0;
    int j;
    FILE* out = NULL;

    if((out = fopen(filename, "w")) == NULL)
    {
        printf("Cannot open groups output file %s.\n", filename);
        return;
    }
    for(; i < groups->n_groups; i++)
    {
        for(j = 0; j < groups->groups[i].n_trajectories - 1; j++)
            fprintf(out, "%d ", groups->groups[i].trajectories[j]);
        fprintf(out, "%d\n", groups->groups[i].trajectories[j]);
    }
    fclose(out);
}

group_list_t* group_list_load(char* filename)
{
    FILE* in;
    group_list_t* groups;
    list_t* gl;
    char* line;
    group_t* group;
    list_t* tl;
    char* tok;
    char* cstat;
    void* status;
    int i;
    
    if((in = fopen(filename, "r")) == NULL)
    {
        printf("Cannot open group input file %s.\n", filename);
        return NULL;
    }
    gl = list_create();
    while((line = line_read(in)) != NULL)
    {
        tl = list_create();
        for(tok = strtok_r(line, " ", &cstat); tok; tok = strtok_r(NULL, " ", &cstat))
            list_append(tl, tok);
        group = malloc(sizeof(group_t));
        /* XXX */
        group->n_trajectories = list_length(tl);
        group->trajectories = malloc(sizeof(int) * group->n_trajectories);
        /* XXX */
        i = 0;
        for(tok = list_iterate(tl, &status); status; tok = list_iterate(NULL, &status))
            group->trajectories[i++] = strtol(tok, NULL, 10);
        list_append(gl, group);
        list_destroy(tl, NULL);
        line_destroy(line);
    }
    fclose(in);
    groups = malloc(sizeof(group_list_t));
    /* XXX */
    groups->n_groups = list_length(gl);
    groups->groups = list_to_array(gl, sizeof(group_t));
    /* XXX */
    list_destroy(gl, free);
    return groups;
}
