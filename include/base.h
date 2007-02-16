#ifndef __BASE_H
#define __BASE_H

#define TRUE 1
#define FALSE 0

typedef int trajectory_id_t;
typedef int group_id_t;

struct sample_s
{
    int x;
    int y;
    int t;
};
typedef struct sample_s sample_t;

struct trajectory_s
{
    trajectory_id_t trajectory_id;
    int n_samples;
    sample_t* samples;
};
typedef struct trajectory_s trajectory_t;

struct group_s
{
    group_id_t group_id;
    int n_trajectories;
    trajectory_id_t* trajectories;
};
typedef struct group_s group_t;

struct dataset_s
{
    int grid_size;
    int max_t;
    int n_trajectories;
    trajectory_t* trajectories;
    int n_groups;
    group_t* groups;
};
typedef struct dataset_s dataset_t;

struct map_level_s
{
    int x;
    int y;
    int t;
};
typedef struct map_level_s map_level_t;

struct map_s
{
    map_level_t level;
    int n_trajectories;
    trajectory_t* trajectories;
};
typedef struct map_s map_t;

struct unique_samples_s
{
    int n_samples;
    sample_t* samples;
};
typedef struct unique_samples_s unique_samples_t;

struct enumerated_trajectory_s
{
    trajectory_id_t trajectory_id;
    int n_sample_ids;
    int* sample_ids;
};
typedef struct enumerated_trajectory_s enumerated_trajectory_t;

struct enumerated_map_s
{
    int n_trajectories;
    enumerated_trajectory_t* trajectories;
};
typedef struct enumerated_map_s enumerated_map_t;

struct frequent_itemset_s
{
    int id;
    int support;
    int n_items;
    int* items;
};
typedef struct frequent_itemset_s frequent_itemset_t;

struct frequent_itemset_list_s
{
    int n_frequent_itemsets;
    frequent_itemset_t* frequent_itemsets;
};
typedef struct frequent_itemset_list_s frequent_itemset_list_t;

struct clique_s
{
    int frequent_itemset_id;
    int n_trajectories;
    int* trajectories;
};
typedef struct clique_s clique_t;

struct clique_list_s
{
    int n_cliques;
    clique_t* cliques;
};
typedef struct clique_list_s clique_list_t;

struct matrix_s
{
    size_t weight_size;
    int n_trajectories;
    void* matrix;
};
typedef struct matrix_s matrix_t;

struct group_list_s
{
    int n_groups;
    group_t* groups;
};
typedef struct group_list_s group_list_t;

unique_samples_t* unique_samples_create(map_t* map);
void unique_samples_destroy(unique_samples_t* samples);
int unique_samples_index(unique_samples_t* samples, sample_t* sample);
void unique_samples_save(unique_samples_t* samples, const char* filename);
unique_samples_t* unique_samples_load(const char** filename);
int sample_compare(const void* s1, const void* s2);
enumerated_map_t* enumerated_map_create(unique_samples_t* samples, map_t* map);
int enumerate_trajectory(trajectory_t* t, enumerated_trajectory_t* e, unique_samples_t* u);
void enumerated_map_destroy(enumerated_map_t* map);
void enumerated_map_print(enumerated_map_t* map);
void enumerated_trajectory_print(enumerated_trajectory_t* t);
int enumerated_map_save(enumerated_map_t* map, const char* filename);
dataset_t* dataset_load(const char* filename);
void dataset_save(dataset_t* data, char* output);
void dataset_destroy(dataset_t** dataset_ref);
void dataset_print(dataset_t* dataset);
void trajectory_destroy(trajectory_t* trajectory_ref);
void trajectory_print(trajectory_t* trajectory);
int trajectory_write(trajectory_t* t, int fd);
int trajectory_map(trajectory_t* in, trajectory_t* out, map_level_t* level);
int trajectory_read(trajectory_t* t, int fd);
void sample_destroy(sample_t* sample_ref);
void sample_print(sample_t* sample);
void sample_map(sample_t* in, sample_t* out, map_level_t* level);
int sample_write(sample_t* s, int fd);
int sample_read(sample_t* s, int fd);
void group_print(group_t* group);
void group_destroy(group_t* group_ref);
map_t* map_create(dataset_t* dataset, map_level_t* level);
void map_save(const char* filename, map_t* map);
map_t* map_load(const char* filename);
void map_destroy(map_t* map);
void map_print(map_t* map);
char* line_read(FILE* f);
void line_destroy(char* line);
frequent_itemset_t* frequent_itemset_from_line(char* line);
void frequent_itemset_destroy(void* fi);
void frequent_itemset_print(frequent_itemset_t* fi);
void clique_destroy(void* clique);
clique_t* clique_create(frequent_itemset_t* fi, enumerated_map_t* emap);
void clique_print(clique_t* clique);
enumerated_trajectory_t* enumerated_trajectory_from_line(char* line);
enumerated_map_t* enumerated_map_load(char* filename);
frequent_itemset_list_t* frequent_itemset_list_load(char* filename, int min_length);
void frequent_itemset_list_destroy(frequent_itemset_list_t* fil);
clique_list_t* clique_list_create(frequent_itemset_list_t* fil, enumerated_map_t* emap);
void clique_list_destroy(clique_list_t* cl);
void clique_list_save(clique_list_t* cl, char* filename);
clique_list_t* clique_list_load(char* filename);
void clique_list_print(clique_list_t* cl);
void matrix_destroy(matrix_t* matrix);
matrix_t* matrix_create(dataset_t* data, clique_list_t* cl, size_t weight_size,
                        void (*weight_function)(dataset_t*, clique_t*, int, int, void*));
void group_list_destroy(group_list_t* groups);
group_list_t* group_list_from_clique_list(clique_list_t* cliques);
group_t group_merge(group_t* g1, group_t* g2);
void group_list_merge(group_list_t* groups, double (*compute_strength)(group_list_t*, int, int, void*), void* user_data);
void group_list_print(group_list_t* groups);
void group_list_save(group_list_t* groups, char* filename);
group_list_t* group_list_load(char* filename);
int int_cmp(const void* i1, const void* i2);

#endif
