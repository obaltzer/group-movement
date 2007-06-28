#ifndef __BASE_H
#define __BASE_H

#define TRUE 1
#define FALSE 0

/** Section: Data Types **/

/**
 * Type: trajectory_id_t
 * 
 * Type used to store the trajectory identification number. 
 */
typedef int trajectory_id_t;

/**
 * Type: group_id_t
 * 
 * Group identification number type. 
 */
typedef int group_id_t;

/** 
 * Structure: sample_s
 *
 * Represents a sample of a trajectory.
 */
struct sample_s
{
    /**
     * Variable: x
     *
     * The x-coordinate of the sample.
     */
    int x;
    
    /**
     * Variable: y
     *
     * The y-coordinate of the sample.
     */
    int y;

    /**
     * Variable: t
     *
     * The time-coordinate of the sample.
     */
    int t;
};

/**
 * Type: sample_t
 *
 * A type of structure <sample_s>.
 */
typedef struct sample_s sample_t;

/**
 * Structure: trajectory_s
 *
 * Represents a trajectory as a collection of samples.
 */
struct trajectory_s
{
    /**
     * Variable: trajectory_id
     *
     * Stores the identifaction of the trajectory.
     */
    trajectory_id_t trajectory_id;

    /**
     * Variable: n_samples
     *
     * The number of samples in the trajectory.
     */
    int n_samples;

    /**
     * Pointer: samples
     *
     * Pointer to an array of samples of type <sample_t>.
     */
    sample_t* samples;
};

/**
 * Type: trajectory_t
 *
 * A type of structure <trajectory_s>.
 */
typedef struct trajectory_s trajectory_t;

/**
 * Structure: group_s
 *
 * Represents a group of trajectories.
 */
struct group_s
{
    /**
     * Variable: group_id
     *
     * Identification of the group.
     */
    group_id_t group_id;

    /**
     * Variable: n_trajectories
     *
     * The number of trajectories in the group.
     */
    int n_trajectories;

    /**
     * Pointer: trajectories
     *
     * Pointer to an array of trajectory indentifiers of type
     * <trajectory_id_t>. The actual trajectories will be stored in the
     * dataset structure <dataset_s> and the trajectory identifier is used
     * to reference those.
     */
    trajectory_id_t* trajectories;
};

/**
 * Type: group_t
 *
 * A type of structure <group_s>.
 */
typedef struct group_s group_t;

/**
 * Structure: dataset_s
 *
 * Represents a dataset consisting of trajectories and potentially a set of
 * previously known groups.
 */
struct dataset_s
{
    /**
     * Variable: grid_size
     *
     * Stores the maximum extent of the grid for all dimensions.
     */
    int grid_size;
    
    /**
     * Variable: max_t
     *
     * The maximum time value.
     */
    int max_t;

    /**
     * Variable: n_trajectories
     *
     * The number of trajectories in the dataset.
     */
    int n_trajectories;

    /**
     * Pointer: trajectories
     *
     * A pointer to the array of trajectories of size <n_trajectories>.
     */
    trajectory_t* trajectories;

    /**
     * Variable: n_groups
     *
     * The number of known groups for this dataset.
     */
    int n_groups;

    /**
     * Pointer: groups
     *
     * A pointer to the array of groups of size <n_groups>.
     */
    group_t* groups;
};

/**
 * Type: dataset_t
 *
 * A type for structure <dataset_s>.
 */
typedef struct dataset_s dataset_t;

/**
 * Structure: map_level_s
 *
 * Represents the levels for each of the dimensions.
 */
struct map_level_s
{
    /**
     * Variable: x
     *
     * Level of the x dimension.
     */
    int x;

    /**
     * Variable: y
     *
     * Level of the y dimension.
     */
    int y;

    /**
     * Variable: t
     *
     * Level of the time dimensions.
     */
    int t;
};

/**
 * Type: map_level_t
 *
 * Type of structure <map_level_s>.
 */
typedef struct map_level_s map_level_t;

/**
 * Structure: map_s
 *
 * Represents a set of trajectories mapped to a specific level.
 */
struct map_s
{
    /**
     * Variable: level
     *
     * The dimension levels to which the trajectories are mapped to.
     */
    map_level_t level;

    /**
     * Varibale: n_trajectories
     *
     * The number of trajectories.
     */
    int n_trajectories;

    /**
     * Pointer: trajectories
     *
     * Pointer to an array of trajectories of size <n_trajectories>.
     */
    trajectory_t* trajectories;
};

/**
 * Type: map_t
 *
 * Type of structure <map_s>.
 */
typedef struct map_s map_t;

/**
 * Structure: unique_samples_s
 *
 * Represents a list of unique samples.
 */
struct unique_samples_s
{
    /**
     * Variable: n_samples
     *
     * The number of unique samples.
     */
    int n_samples;

    /**
     * Pointer: samples
     *
     * Pointer to an array of samples of size <n_samples>.
     */
    sample_t* samples;
};

/**
 * Type: unique_samples_t
 *
 * Type of structure <unique_samples_s>.
 */
typedef struct unique_samples_s unique_samples_t;

/**
 * Structure: enumerated_trajectory_s
 *
 * Represents a trajectory whose samples have been replaced by single
 * unique identifiers. The enumerated trajectories are being used for
 * frequent itemset mining.
 *
 * See also:
 *
 *   <unique_samples_s>
 */
struct enumerated_trajectory_s
{
    /**
     * Variable: trajectory_id
     *
     * The identifier of the original trajectory this enumerated trajectory
     * is based on.
     */
    trajectory_id_t trajectory_id;

    /**
     * Variable: n_sample_id
     *
     * Number of unique samples in this trajectory.
     */
    int n_sample_ids;

    /**
     * Pointer: sample_ids
     *
     * Pointer to an array of indices referring to samples in a list of
     * unique samples <unique_samples_s>.
     */
    int* sample_ids;
};

/**
 * Type: enumerated_trajectory_t
 *
 * Type of structure <enumerated_trajectory_s>.
 */
typedef struct enumerated_trajectory_s enumerated_trajectory_t;

/**
 * Structure: enumerated_map_s
 *
 * Represents a map at a specific level with all its trajectories being
 * converted into enumerated trajectories.
 *
 * See also:
 *  
 *  <enumerated_trajectory_s>
 *  <map_s>
 */
struct enumerated_map_s
{
    /**
     * Variable: n_trajectories
     *
     * Number of enumerated trajectories in this enumerated map.
     */
    int n_trajectories;

    /**
     * Pointer: trajectories
     *
     * Pointer to an array of enumerated trajectories of size
     * <n_trajectories>.
     */
    enumerated_trajectory_t* trajectories;
};

/**
 * Type: enumerated_map_t
 *
 * Type of structure enumerated_map_s.
 */
typedef struct enumerated_map_s enumerated_map_t;

/**
 * Structure: frequent_itemset_s
 *
 * Represents a frequent itemset. Essentially a set of mapped samples that
 * occur together <support> times.
 */
struct frequent_itemset_s
{
    /**
     * Variable: id
     *
     * Index of the frequent itemset in the frequent itemset file.
     */
    int id;

    /**
     * Variable: support
     *
     * Support of the frequent itemset, i.e. the given set of mapped
     * samples occurs in <support> trajectories.
     */
    int support;

    /**
     * Variable: n_items
     *
     * The number of items in this set.
     */
    int n_items;

    /**
     * Pointer: items
     *
     * Pointer to an array of indices pointing into a list of unique
     * samples (<unique_samples_s>).
     */
    int* items;
};

/**
 * Type: frequent_itemset_t
 *
 * Type of structure <frequent_itemset_s>.
 */
typedef struct frequent_itemset_s frequent_itemset_t;

/**
 * Structure: frequent_itemset_list_s
 *
 * Represents a list of frequent itemsets, i.e. a frequent itemset file.
 */
struct frequent_itemset_list_s
{
    /**
     * Variable: n_frequent_itemsets
     *
     * Number of frequent itemsets.
     */
    int n_frequent_itemsets;

    /**
     * Pointer: frequent_itemsets
     *
     * Pointer to an array of frequent itemsets of size
     * <n_frequent_itemsets>.
     */
    frequent_itemset_t* frequent_itemsets;
};

/**
 * Type: frequent_itemset_list_t
 *
 * Type of structure <frequent_itemset_list_s>.
 */
typedef struct frequent_itemset_list_s frequent_itemset_list_t;

/**
 * Structure: clique_s
 *
 * Represents a clique of trajectories. That is the set of trajectories
 * that support a frequent itemset.
 */
struct clique_s
{
    /**
     * Variable: frequent_itemset_id
     *
     * Index of the frequent itemset in the frequent itemset list
     * (<frequent_itemset_list_s>) that is supported by the trajectories in
     * the clique.
     */
    int frequent_itemset_id;

    /**
     * Variable: n_trajectories
     *
     * The number of the trajectories in the clique.
     */
    int n_trajectories;

    /**
     * Pointer: trajectories
     *
     * Pointer to a an array of trajectory indices in the dataset's
     * trajectory list.
     */
    int* trajectories;
};

/**
 * Type: clique_t
 *
 * Type of structure <clique_s>.
 */
typedef struct clique_s clique_t;

/**
 * Structure: clique_list_s
 *
 * Represents a list of cliques.
 */
struct clique_list_s
{
    /**
     * Variable: n_cliques
     *
     * Number of cliques in the list.
     */
    int n_cliques;

    /**
     * Pointer: cliques
     *
     * Pointer to an array of cliques of size <n_cliques>.
     */
    clique_t* cliques;
};

/**
 * Type: clique_list_t
 *
 * Type of structure <clique_list_s>.
 */
typedef struct clique_list_s clique_list_t;

/**
 * Structure: matrix_s
 *
 * Represents the weight matrix for trajectory associations.
 */
struct matrix_s
{
    /**
     * Variable: weight_size
     *
     * The datatype size of each weight.
     */
    size_t weight_size;

    /**
     * Variable: n_trajectories
     *
     * Number of trajectories connected in the matrix.
     */
    int n_trajectories;
    
    /**
     * Pointer: matrix
     *
     * Pointer to an <n_trajectories> x <n_trajectories> matrix of weight
     * values.
     */
    void* matrix;
};

/**
 * Type: matrix_t
 *
 * Type of structure <matrix_s>.
 */
typedef struct matrix_s matrix_t;

/**
 * Structure: group_list_s
 *
 * Represents a list of groups.
 */
struct group_list_s
{
    /**
     * Variable: n_groups
     *
     * Number of groups in the list.
     */
    int n_groups;

    /**
     * Pointer: groups
     *
     * Pointer to an array of groups of size <n_groups>.
     */
    group_t* groups;
};

/**
 * Type: group_list_t
 *
 * Type of structure <group_list_s>.
 */
typedef struct group_list_s group_list_t;

/** Section: Functions **/

/**
 * Function: unique_samples_create
 *
 * Create a list of unique samples used in a mapped dataset. Memory for
 * this list is dynamically allocated and should be released with
 * <unique_samples_destroy>.
 *
 * Parameters:
 *
 *   map - a mapped set of trajectories
 *
 * Returns:
 *
 *   A pointer to a list of unique samples (<unique_samples_t>).
 *
 * See also:
 *
 *   <map_create>
 *   <unique_samples_destroy>
 */
unique_samples_t* unique_samples_create(map_t* map);

/**
 * Function: unique_samples_destroy
 *
 * Releases memory allocated by a unique samples list.
 *
 * Parameters:
 *
 *   samples - the list of unique samples that is to be released
 *
 * See also:
 *
 *   <unique_samples_create>
 */
void unique_samples_destroy(unique_samples_t* samples);

/**
 * Function: unique_samples_index
 *
 * Return the index of a <sample> in the unique samples list (<samples>).
 *
 * Parameters:
 *
 *   samples - pointer to the list of unique samples
 *   sample  - pointer to the sample whose index in the list is queried
 *
 * Returns:
 *
 *   The index of the sample in the unique samples list.
 */
int unique_samples_index(unique_samples_t* samples, sample_t* sample);

/**
 * Function: unique_samples_save
 *
 * Saves the list of unique samples in a file.
 *
 * Parameters:
 *
 *   samples  - the list of unique samples to be saved
 *   filename - the name of the file to which the samples should be saved
 */
void unique_samples_save(unique_samples_t* samples, const char* filename);

/**
 * Function: unique_samples_load
 *
 * Loads a list of unique samples from a file. Memory is allocated
 * accordingly and needs to be released with <unique_samples_destroy>.
 *
 * Parameters:
 *
 *   filename - name of the file which contains the unique samples list
 *
 * Returns:
 *   
 *   A pointer to a unique samples list.
 *
 * Note:
 *
 *   *THIS FUNCTION IS NOT IMPLEMENTED*
 */
unique_samples_t* unique_samples_load(const char* filename);

/**
 * Function: sample_compare
 *
 * Compares two samples lexicographically.
 *
 * Parameters:
 * 
 *   s1 - pointer to the first sample 
 *   s2 - pointer to the second sample
 *
 * Returns:
 *
 *   - -1 if <s1> is lexicographically smaller than <s2>
 *   - 0 if <s1> is equivalent to <s2>
 *   - 1 if <s1> is lexicographically larger than <s2>
 */
int sample_compare(const void* s1, const void* s2);

/**
 * Function: enumerated_map_create
 *
 * Creates a map of enumerated trajectories from a given map and a list of
 * unique samples. Memory is dynamically allocated and should be released
 * using <enumerated_map_destroy>.
 *
 * Parameters:
 *
 *   samples - list of unique samples used in the map
 *   map     - the map to be enumerated
 *
 * Returns:
 *  
 *   A pointer to a map whose trajectories are enumerated.
 */
enumerated_map_t* enumerated_map_create(unique_samples_t* samples, map_t* map);

/**
 * Function: enumerate_trajectory
 *
 * Enumerates the samples of a trajectory. The parameter e must point to a
 * memory location that is already allocated to hold an enumerated
 * trajectory.
 *
 * Parameters:
 *
 *   t - the original trajectory
 *   e - pointer to where where the enumerated trajectory will be stored
 *   u - pointer to the list of unique samples
 *
 * Returns:
 *
 *   TRUE (1) if successful otherwise FALSE (0).
 */
int enumerate_trajectory(trajectory_t* t, enumerated_trajectory_t* e, unique_samples_t* u);

/**
 * Function: enumerated_map_destroy
 *
 * Releases the memory allocated for an enumerated map.
 *
 * Parameters:
 *
 *   map - pointer to the enumerated map to be released
 */
void enumerated_map_destroy(enumerated_map_t* map);

/**
 * Function: enumerated_map_print
 *
 * Prints an enumerated map to stdout.
 *
 * Parameters:
 * 
 *   map - the map to be printed
 */
void enumerated_map_print(enumerated_map_t* map);

/**
 * Function: enumerated_trajectory_print
 *
 * Prints an enumerated trajectory to stdout.
 *
 * Parameters:
 *
 *   t - pointer to the enumerated trajectory
 */
void enumerated_trajectory_print(enumerated_trajectory_t* t);

/**
 * Function: enumerated_map_save
 *
 * Saves an enumerated to a file.
 *
 * Parameters:
 *
 *   map      - the enumerated to be saved
 *   filename - name of the file to which the map is to be saved
 *
 * Returns:
 *
 *   TRUE (1) in success otherwise FALSE (0)
 */
int enumerated_map_save(enumerated_map_t* map, const char* filename);

/**
 * Function: dataset_load
 *
 * Loads a dataset from file. Memory is allocated accordingly and must be
 * released with <dataset_destroy>.
 *
 * Parameters:
 *
 *   filename - name of the file from which to load the dataset
 *
 * Returns:
 *
 *   Pointer to the loaded dataset or NULL if an error occured.
 */
dataset_t* dataset_load(const char* filename);

/**
 * Function: dataset_save
 *
 * Writes a dataset to file.
 *
 * Parameters:
 *
 *   data   - pointer to the dataset
 *   output - name of the file to save the dataset to
 */
void dataset_save(dataset_t* data, char* output);

/**
 * Function: dataset_destroy
 *
 * Destroys a previously allocated dataset.
 *
 * Parameters:
 *
 *   dataset_ref - a reference (pointer) to a dataset pointer
 */
void dataset_destroy(dataset_t** dataset_ref);

/**
 * Function: dataset_print
 *
 * Prints a dataset to stdout.
 *
 * Parameters:
 * 
 *   dataset - the dataset to be printed
 */
void dataset_print(dataset_t* dataset);

/**
 * Function: trajectory_destroy
 *
 * Releases memory allocated for a trajectory.
 *
 * Parameters:
 *
 *   trajectory_ref - pointer to the trajectory to be released
 */
void trajectory_destroy(trajectory_t* trajectory_ref);

/**
 * Function: trajectory_print
 *
 * Prints a trajectory to stdout.
 *
 * Parameters:
 *
 *   trajectory - the trajectory to be printed
 */
void trajectory_print(trajectory_t* trajectory);

/**
 * Function: trajectory_write
 *
 * Writes a binary representation of a trajectory to a file descriptor.
 *
 * Parameters:
 *
 *   t  - the trajectory to write
 *   id - the file descriptor to write to
 *
 * Returns:
 *
 *   TRUE (1)
 */
int trajectory_write(trajectory_t* t, int fd);

/**
 * Function: trajectory_map
 *
 * Maps a trajectory to a specified level.
 *
 * Parameters:
 *
 *   in    - the original trajectory
 *   out   - pointer to the output trajectory
 *   level - level to which the trajectory will be mapped
 *
 * Returns:
 *
 *   TRUE (1) on success otherwise FALSE (0)
 */
int trajectory_map(trajectory_t* in, trajectory_t* out, map_level_t* level);

/**
 * Function: trajectory_read
 *
 * Reads the binary description of a trajectory from a file descriptor.
 *
 * Parameters:
 *
 *   t  - the trajectory in which to store the data
 *   fd - file descriptor from which to read
 *
 * Returns:
 *
 *  TRUE (1) on success otherwise false
 */
int trajectory_read(trajectory_t* t, int fd);

/**
 * Function: sample_destroy
 *
 * Releases the memory allocated by a single sample.
 *
 * Parameters:
 *
 *   sample_ref - pointer to the sample
 */
void sample_destroy(sample_t* sample_ref);

/**
 * Function: sample_fprint
 *
 * Writes a text representation of a sample to a file.
 *
 * Parameters:
 *
 *   file   - handle of the file to which to write
 *   sample - the sample to print
 */
void sample_fprint(FILE* file, sample_t* sample);

/**
 * Function: sample_print
 *
 * Prints a sample to stdout.
 *
 * Parameters:
 *
 *   sample - the sample to print
 */
void sample_print(sample_t* sample);

/**
 * Function: sample_map
 *
 * Maps a sample from its original resolution to the specified level.
 *
 * Parameters:
 *
 *   in    - the original sample
 *   out   - pointer to allocated memory to hold the resulting sample
 *   level - level to which the sample is to be mapped
 */
void sample_map(sample_t* in, sample_t* out, map_level_t* level);

/**
 * Function: group_print
 *
 * Prints a group to stdout.
 *
 * Parameters:
 *
 *   group - group to print
 */
void group_print(group_t* group);

/**
 * Function: group_destroy
 *
 * Releases the memory allocated by a group.
 *
 * Parameters:
 *
 *   group_ref - pointer to the group to release
 */
void group_destroy(group_t* group_ref);

/**
 * Function: map_create
 *
 * Creates a map from an original dataset by mapping it to the specified
 * level. Memory is dynamically allocated as necessary and must be freed
 * using <map_destroy>.
 *
 * Parameters:
 *
 *   dataset - original dataset
 *   level   - target level of the map
 *
 * Returns:
 *
 *   A pointer to a new map of type <map_t>.
 */
map_t* map_create(dataset_t* dataset, map_level_t* level);

/**
 * Function: map_save
 *
 * Saves the specified map to a file.
 *
 * Parameters:
 *
 *   filename - name of the file to save the map to
 *   map      - pointer to the map to be saved
 */
void map_save(const char* filename, map_t* map);

/**
 * Function: map_load
 *
 * Loads a map from file. Memory is allocated as necessary and must be
 * freed using <map_destroy>.
 *
 * Parameters:
 *
 *   filename - name of the map file to load
 *
 * Returns:
 *
 *   A pointer to a map of type <map_t>.
 */
map_t* map_load(const char* filename);

/**
 * Function: map_destroy
 *
 * Releases the memory allocated for a map.
 *
 * Parameters:
 *
 *   map - the map that is to be released
 */
void map_destroy(map_t* map);

/**
 * Function: map_print
 *
 * Prints a map to stdout.
 *
 * Parameters:
 *
 *   map - the map to print
 */
void map_print(map_t* map);

/**
 * Function: line_read
 *
 * Reads a single line from file. Memory is allocated as necessary and must
 * be released with <line_destroy>.
 *
 * Parameters:
 *
 *   f - the file handle from which to read the line
 *
 * Returns:
 *
 *   A pointer to a string containing the line.
 */
char* line_read(FILE* f);

/**
 * Function: line_destroy
 *
 * Releases the memory that was previously allocated for a line.
 *
 * Parameters:
 *
 *   line - the line to be destroyed
 */
void line_destroy(char* line);

/**
 * Function: frequent_itemset_from_line
 *
 * Creates a frequent itemset from a line. Memory is allocated as necessary
 * and must be released using <frequent_itemset_destroy>.
 *
 * Parameter:
 *
 *   line - the string to parse
 *
 * Returns:
 *
 *   A pointer to a frequent itemset of type <frequent_itemset_t>.
 */
frequent_itemset_t* frequent_itemset_from_line(char* line);

/**
 * Function: frequent_itemset_destroy
 *
 * Releases the memory allocated for a frequent itemset.
 *
 * Parameters:
 *
 *   fi - pointer to the frequent itemset
 */
void frequent_itemset_destroy(void* fi);

/**
 * Function: frequent_itemset_print
 *
 * Prints a frequent itemset to stdout.
 *
 * Parameters:
 *
 *   fi - the frequent itemset to print
 */
void frequent_itemset_print(frequent_itemset_t* fi);

/**
 * Function: clique_destroy
 *
 * Releases the memory that was previously allocated for a clique.
 *
 * Parameters:
 *
 *   clique - the clique to be released
 */
void clique_destroy(void* clique);

/**
 * Function: clique_create
 *
 * Creates the clique for the specified frequent itemset and trajectory
 * information provided by the enumerated map. Memory is allocated as
 * necessary and must be released using <clique_destroy>.
 *
 * Parameters:
 * 
 *   fi   - the frequent itemset for which to create the clique
 *   emap - enumerated map with trajectory information
 *
 * Returns:
 *
 *   Pointer to a clique of trajectories of type <clique_t>.
 */
clique_t* clique_create(frequent_itemset_t* fi, enumerated_map_t* emap);

/**
 * Function: clique_print
 *
 * Prints a text representation of a clique to stdout.
 *
 * Parameters:
 *
 *   clique - the clique to print
 */
void clique_print(clique_t* clique);

/**
 * Function: enumerated_trajectory_from_line
 *
 * Creates an enumerated trajectory from a text representation stored in a
 * string. Memory is allocated as necessary and must be released with
 * <enumerated_trajectory_destroy>.
 *
 * Parameters:
 *
 *   line - the string to parse as an individual line
 * 
 * Returns:
 *
 *   A pointer to an enumerated trajectory of type
 *   <enumerated_trajectory_t>.
 */
enumerated_trajectory_t* enumerated_trajectory_from_line(char* line);

/**
 * Function: enumerated_map_load
 *
 * Loads an enumerated trajectory map from file. Memory is allocated as
 * necessary and must be released with <enumerated_map_destroy>.
 *
 * Parameters:
 *
 *   filename - name of the file to load the map from
 *
 * Returns:
 *
 *   A pointer to an enumerated map of type <enumerated_map_t>.
 */
enumerated_map_t* enumerated_map_load(char* filename);

/**
 * Function: frequent_itemset_list_load
 *
 * Reads a list of frequent itemsets from a file and filters the loaded
 * frequent itemsets by length. Memory is allocated as necessary and must
 * be released with <frequent_itemset_destroy>.
 *
 * Parameters;
 *
 *   filename   - name of the file from which the frequent itemset list
 *                should be loaded
 *   min_length - minimum number of items a frequent itemset must have to
 *                be loaded
 *
 * Returns:
 *
 *   A pointer to a list of frequent itemsets of type
 *   <frequent_itemset_list_t>.
 */
frequent_itemset_list_t* frequent_itemset_list_load(char* filename, int min_length);

/**
 * Function: frequent_itemset_list_destroy
 *
 * Releases the memory that was previously allocated for a list of frequent
 * itemsets.
 *
 * Parameters:
 *
 *   fil - the list of frequent itemsets to be released
 */
void frequent_itemset_list_destroy(frequent_itemset_list_t* fil);

/**
 * Function: clique_list_create
 *
 * Creates a list of cliques from the list of frequent itemsets and the
 * enumerated map that are provided. For each frequent itemset in the list
 * it will individually create a clique and collect them in a list that is
 * returned. Memory is allocated as necessary and must be released with
 * <clique_list_destroy>.
 *
 * Parameters:
 *
 *   fil  - frequent itemset list for which to create the clique list
 *   emap - map of enumerated trajectories to reverse map frequent itemsets
 *          to the trajectories they match
 *
 * Returns:
 *
 *   A list of cliques of type <clique_list_t>.
 */
clique_list_t* clique_list_create(frequent_itemset_list_t* fil, enumerated_map_t* emap);

/**
 * Function: clique_list_destroy
 *
 * Releases the memory for a previously allocated list of cliques.
 *
 * Parameters:
 *
 *   cl - clique list to be released
 */
void clique_list_destroy(clique_list_t* cl);

/**
 * Function: clique_list_save
 *
 * Saves a list of cliques to a text file.
 *
 * Parameters:
 *
 *   cl       - clique list to be saved
 *   filename - name of the file to which to save the cliques
 */
void clique_list_save(clique_list_t* cl, char* filename);

/**
 * Function: clique_list_load
 *
 * Loads a list of cliques from a file. Memory is allocated as necessary
 * and must be released with <clique_list_destroy>.
 *
 * Parameters:
 *
 *   filename - name of the file from which to load the cliqyes
 *
 * Returns:
 *
 *   A pointer to a list of cliques of type <clique_list_t>.
 */
clique_list_t* clique_list_load(char* filename);

/**
 * Function: clique_list_print
 *
 * Prints a list of cliques to stdout.
 *
 * Parameters:
 *
 *   cl - list of cliques to be printed
 */
void clique_list_print(clique_list_t* cl);

/**
 * Function: matrix_destroy
 *
 * Releases the memory for a previously alloced weight matrix.
 *
 * Parameters:
 * 
 *   matrix - the matrix to be released
 */
void matrix_destroy(matrix_t* matrix);

/**
 * Function: matrix_create
 *
 * Constructs an initial weight matrix for a list of cliques using the
 * specified function to compute a weight between two trajectories. Memory
 * is allocated as necessary and must be released with <matrix_destroy>.
 * 
 * Parameters:
 *
 *   data            - original dataset
 *   cliques         - list of cliques for which to construct the matrix
 *   weight_size     - size in bytes of the weight datatype
 *   weight_function - function to compute the weight for a pair of
 *                     trajectories
 *
 * Returns:
 *
 *   A pointer to a matrix structure of type <matrix_t>.
 */
matrix_t* matrix_create(dataset_t* data, clique_list_t* cl, size_t weight_size, void* user_data,
                        void (*weight_function)(clique_t*, int, int, void*, void*));

/**
 * Function: group_list_destroy
 *
 * Releases the memory that was previously allocated for a group list.
 *
 * Parameters:
 * 
 *   groups - the groups list to be released
 */
void group_list_destroy(group_list_t* groups);

/**
 * Function: group_list_from_clique_list
 *
 * Converts a list of cliques into a list of groups. This is necessary for
 * the group merging process when originally detected cliques are merged
 * into groups. Memory is allocated as necessary and must be released with
 * <group_list_destroy>.
 *
 * Parameters:
 *
 *   cliques - the list of cliques that will be converted into a list of
 *             groups
 *
 * Returns:
 *
 *   A pointer to a list of groups of tupe <group_list_t>.
 */
group_list_t* group_list_from_clique_list(clique_list_t* cliques);

/**
 * Function: group_merge
 *
 * Merges two groups into one. Memory is only being allocated on the stack
 * and returned by value.
 *
 * Parameters:
 *
 *   g1 - first group to merge
 *   g2 - second group to merge
 *
 * Returns:
 *
 *   A value of a merged group of type <group_t>.
 */
group_t group_merge(group_t* g1, group_t* g2);

/**
 * Function: group_list_merge
 *
 * Merges the groups in a group list based on their strength until no more
 * groups can be merged. The algorithm will always merge the two groups
 * with the highest pair-wise strength. To compute the strength of two
 * groups the strength function uses any auxiliary information passed to it
 * as user data. 
 *
 * Parameters:
 *
 *   groups           - list of groups to be merged
 *
 *   compute_strength - function to compute the strength by which two
 *                      groups can be potentially merged
 *   
 *   update_user_data - a call-back function called after two groups have
 *                      been merged to update the user-specified data
 *   
 *   user_data        - pointer to the user-specified data
 */
void group_list_merge(group_list_t* groups, double (*compute_strength)(group_list_t*, int, int, void*), void (*update_user_data)(int, int, int, double, void*), void* user_data);

/**
 * Function: group_list_print
 *
 * Prints a list of groups to stdout.
 *
 * Parameters:
 *
 *   groups - the list of groups to print
 */
void group_list_print(group_list_t* groups);

/**
 * Function: group_list_save
 *
 * Saves a list of groups to file.
 *
 * Parameters:
 *
 *   groups   - the list of groups to save
 *   filename - name of the file to save the groups to
 */
void group_list_save(group_list_t* groups, char* filename);

/**
 * Function: group_list_load
 *
 * Loads a list of groups from file. Memory is allocated as necessary and
 * must be released with <group_list_destroy>.
 *
 * Parameters:
 *
 *   filename - name of the file from which to load the groups
 *
 * Returns:
 *
 *   Pointer to the list of groups.
 */
group_list_t* group_list_load(char* filename);

/**
 * Function: int_cmp
 *
 * Compares the integers the pointers reference.
 *
 * Parameters:
 *
 *   i1 - pointer to the first integer
 *   i2 - pointer to the second integer
 *
 * Return:
 *
 *   - -1 if *i1 < *i2
 *   - 0 if *i1 == *i2
 *   - 1 if *i1 > *i2
 */
int int_cmp(const void* i1, const void* i2);

#endif
