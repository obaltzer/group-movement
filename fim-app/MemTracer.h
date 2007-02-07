#ifndef _MEMTRACER_H
#define _MEMTRACER_H

#include <stdlib.h>
#include <stdio.h>

//CMemTracer class traces the memory consumed by all kinds of structures.
class CMemTracer
{
public:
	int mnused_buffer; //memory size for temporary varibles
	int mnmaxused_buffer; //memory size for temporary varibles

	int mnAFOPTree_nodes; //number of AFOPT-node
	int mnAFOPTree_array_nodes; //number of single branches in AFOPT-trees
	int mnAFOPTree_size; //total size of AFOPT-trees
	int mnmaxAFOPTree_size;//maximal size of AFOPT-trees

	int mnTransHeader_nodes; //memory size for storing transaction head nodes
	int mnTrans_size; //memory size for storing transactions
	int mnArrayDB_size; //total size of conditional databases stored using arrays
	int mnmaxArrayDB_size; //maximal size of conditional databases stored using arrays

	int mntwo_layer_map_size; //memory size for storing two layer hash maps
	int mnmax_two_layer_map_size; //maximal memory size for storing two layer hash maps

	int mncfptree_size; //memory size for storing CFP-tree
	int mnmax_cfptree_size; //maximal memory size for storing CFP-tree

	int mnpattern_buffer; //memory size for storing maximal frequent itemsets
	int mnmaxpattern_buffer; //memory size for storing maximal frequent itemsets

	int mntotal_memory; //total size of memory used
	int mnmaxtotal_memory; //maximal memory usage
	int mninitial_struct_size; 

	CMemTracer();
	~CMemTracer();

	void IncBuffer(int size);
	void DecBuffer(int size);
	
	void AddOneAFOPTNode();
	void DelOneAFOPTNode();
	void AddSingleBranch(int length);
	void DelSingleBranch(int length);

	void AddOneTransHeadNode();
	void DelOneTransHeadNode();
	void AddOneTransaction(int length);
	void DelOneTransaction(int length);

	void IncTwoLayerMap(int size);
	void DecTwoLayerMap(int size);

	void IncCFPSize(int size);
	void DecCFPSize(int size);

	void IncPatternBuffer(int size);
	void DecPatternBuffer(int size);

	void PrintStatistics(FILE* out_file);
};

extern CMemTracer  goMemTracer;

#endif
