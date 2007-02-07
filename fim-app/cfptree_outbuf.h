#ifndef OUTBUF_MANAGER
#define OUTBUF_MANAGER
#include <stdio.h>

typedef struct
{
	int support;
	int hash_bitmap;
	int child;
	union
	{
		int item;
		int *pitems;
	};
} ENTRY;

typedef struct
{
	int length; // length>1: an internal node with multiple entires; 
				// length=1: a node with only one entry, and the entry contains only one item;
				// length<-1: a node with only one entry and the entry contains |length| items;
	ENTRY *pentries;
	int pos;
	int cur_order;
} CFP_NODE;


typedef struct
{
	int level;
	bool in_mem;
	int  mem_pos;
	int  cfpnode_size;
	CFP_NODE *pcfp_node;
	int	 disk_pos;
} ACTIVE_NODE;

class CTreeOutBufManager
{
	int mnbuffer_size, mncapacity;
	int mnstart_pos, mnend_pos;
	int mnmin_inmem_level; 
	int mnwrite_start_pos, mndisk_write_start_pos;

	char **mptree_buffer;

	ACTIVE_NODE *mpactive_stack;

	FILE* mfpcfp_file, *mfpread_file;
	int mntree_size; // in bytes


	void Dump(int newnode_size);
	void DumpAll();

	unsigned int SearchInBuf(int ndisk_pos, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos);
	unsigned int SearchOnDisk(int ndisk_pos, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos);

public: 
//for pruning
	int mpat_len, mnactive_top;


	void Destroy();

	void Init(int nstack_size);
	void PrintfStatistics(FILE *fpsum_file);
	int  GetTreeSize();
	void IncTreeSize(int ninc_size);


	void InsertActiveNode(int level, CFP_NODE *pcfp_node);
	void WriteInternalNode(CFP_NODE *pcfp_node);
	void WriteLeafNode(int *pitems, int num_of_items, int nsupport, int hash_bitmap);

	void InsertCommonPrefixNode(int level, int *pitems, int num_of_items, int nsupport, int hash_bitmap);
	int WriteCommonPrefixNodes(int num_of_nodes, int base_hashmap);

	int bsearch(int mnmin_sup, ENTRY * pentries, int num_of_freqitems);
	unsigned int SearchActiveStack(int nactive_level, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos);


};

extern CTreeOutBufManager gotree_bufmanager;
extern int gnprune_times;
extern int gnmap_pruned_times;

typedef struct
{
	int nmax_len;
	int nmax_item_id;
	int nmin_item_id;
	int bitmap;
} MAP_NODE;


#endif


