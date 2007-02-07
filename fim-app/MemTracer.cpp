#include "MemTracer.h"
#include "AFOPTMine.h"
#include "ArrayMine.h"
#include "parameters.h"

CMemTracer::CMemTracer()
{
	mnused_buffer = 0;
	mnmaxused_buffer = 0;

	mnAFOPTree_nodes = 0;
	mnAFOPTree_array_nodes = 0;
	mnAFOPTree_size = 0;
	mnmaxAFOPTree_size = 0;

	mnTransHeader_nodes = 0;
	mnTrans_size = 0;
	mnArrayDB_size = 0;
	mnmaxArrayDB_size = 0;

	mntwo_layer_map_size = 0;
	mnmax_two_layer_map_size = 0;

	mncfptree_size = 0;
	mnmax_cfptree_size = 0;

	mnpattern_buffer = 0;
	mnmaxpattern_buffer = 0;

	mntotal_memory = 0;
	mnmaxtotal_memory = 0;
}

void CMemTracer::IncBuffer(int size)
{
	mnused_buffer += size;
	mntotal_memory += size;

	if(mnmaxused_buffer<mnused_buffer)
		mnmaxused_buffer = mnused_buffer;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;
}

void CMemTracer::DecBuffer(int size)
{
	mnused_buffer -= size;
	mntotal_memory -= size;
}


void CMemTracer::AddOneAFOPTNode()
{
	mnAFOPTree_nodes++;
	mnAFOPTree_size += sizeof(AFOPT_NODE);
	mntotal_memory  += sizeof(AFOPT_NODE);

	if(mnmaxAFOPTree_size<mnAFOPTree_size)
		mnmaxAFOPTree_size = mnAFOPTree_size;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;
}

void CMemTracer::DelOneAFOPTNode()
{
	mnAFOPTree_nodes--;
	mnAFOPTree_size -= sizeof(AFOPT_NODE);
	mntotal_memory -= sizeof(AFOPT_NODE);

}

void CMemTracer::AddSingleBranch(int length)
{
	mnAFOPTree_array_nodes += length;
	mnAFOPTree_size  += length*sizeof(ARRAY_NODE);
	mntotal_memory  += length*sizeof(ARRAY_NODE);

	if(mnmaxAFOPTree_size<mnAFOPTree_size)
		mnmaxAFOPTree_size = mnAFOPTree_size;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;

}

void CMemTracer::DelSingleBranch(int length)
{
	mnAFOPTree_array_nodes -= length;
	mnAFOPTree_size  -= length*sizeof(ARRAY_NODE);
	mntotal_memory  -= length*sizeof(ARRAY_NODE);
}


void CMemTracer::AddOneTransHeadNode()
{
	mnTransHeader_nodes++;
	mnArrayDB_size += sizeof(TRANS_HEAD_NODE);
	mntotal_memory += sizeof(TRANS_HEAD_NODE);
	
	if(mnmaxArrayDB_size<mnArrayDB_size)
		mnmaxArrayDB_size = mnArrayDB_size;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;

}

void CMemTracer::DelOneTransHeadNode()
{
	mnTransHeader_nodes--;
	mnArrayDB_size -= sizeof(TRANS_HEAD_NODE);
	mntotal_memory -= sizeof(TRANS_HEAD_NODE);
}

void CMemTracer::AddOneTransaction(int length)
{
	mnTrans_size += length;
	mnArrayDB_size += length*sizeof(int);
	mntotal_memory += length*sizeof(int);
	
	if(mnmaxArrayDB_size<mnArrayDB_size)
		mnmaxArrayDB_size = mnArrayDB_size;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;

}

void CMemTracer::DelOneTransaction(int length)
{
	mnTrans_size -= length;
	mnArrayDB_size -= length*sizeof(int);
	mntotal_memory -= length*sizeof(int);
}

void CMemTracer::IncTwoLayerMap(int size)
{
	mntwo_layer_map_size += size;
	mntotal_memory += size;

	if(mnmax_two_layer_map_size<mntwo_layer_map_size)
		mnmax_two_layer_map_size = mntwo_layer_map_size;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;
}

void CMemTracer::DecTwoLayerMap(int size)
{
	mntwo_layer_map_size -= size;
	mntotal_memory -= size;
}


void CMemTracer::IncCFPSize(int size)
{
	mncfptree_size += size;
	mntotal_memory += size;

	if(mnmax_cfptree_size<mncfptree_size)
		mnmax_cfptree_size = mncfptree_size;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;
}

void CMemTracer::DecCFPSize(int size)
{
	mncfptree_size -= size;
	mntotal_memory -= size;
}


void CMemTracer::IncPatternBuffer(int size)
{
	mnpattern_buffer += size;
	mntotal_memory += size;

	if(mnmaxpattern_buffer<mnpattern_buffer)
		mnmaxpattern_buffer = mnpattern_buffer;

	if(mnmaxtotal_memory<mntotal_memory)
		mnmaxtotal_memory = mntotal_memory;

}

void CMemTracer::DecPatternBuffer(int size)
{
	mnpattern_buffer -= size;
	mntotal_memory -= size;
}

CMemTracer::~CMemTracer()
{
	if(mnused_buffer!=0)
		printf("Error: %d bytes memory are not released\n", mnused_buffer);

	if(mnAFOPTree_nodes!=0)
		printf("Error: %d tree nodes are not released, they occupy %d bytes\n", mnAFOPTree_nodes, mnAFOPTree_size);

	if(mnAFOPTree_array_nodes!=0)
		printf("Error: %d array nodes in tree are not released, they occupy %d bytes\n", mnAFOPTree_array_nodes, mnAFOPTree_size);

	if(mnTrans_size!=0)
		printf("Error: %d bytes for storing transactions in hyper-structure are not released\n", mnTrans_size);

	if(mnTransHeader_nodes!=0)
		printf("Error: %d header nodes in hyper-structure are not released\n", mnTransHeader_nodes);

	if(mntwo_layer_map_size!=0)
		printf("Error: %d bytes for storing item-sup-len map are not released\n", mntwo_layer_map_size);

	if(mncfptree_size!=0)
		printf("Error: %d bytes for buffering cfp-tree are not released\n", mncfptree_size);

	if(mnpattern_buffer!=0)
		printf("Error: %d bytes for buffering maximal patterns are not released\n", mnpattern_buffer);

}

void CMemTracer::PrintStatistics(FILE* out_file)
{
	fprintf(out_file, "%.fkb ", (double)mnmaxtotal_memory/(1<<10));
	fprintf(out_file, "%.fkb\t", (double)mninitial_struct_size/(1<<10));

	fprintf(out_file, "%.fkb %.fkb %.fkb\t", (double)mnmaxused_buffer/(1<<10), (double)mnmaxAFOPTree_size/(1<<10), (double)mnmaxArrayDB_size/(1<<10));

}
