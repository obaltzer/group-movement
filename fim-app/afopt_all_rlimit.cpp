#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#include "ScanDBMine.h"
#include "AFOPTMine.h"
#include "ArrayMine.h"
#include "parameters.h"
#include "fsout.h"
#include "data.h"

//#include "TimeTracer.h"
//#include "MemTracer.h"


void MineAllPats();

int bucket_count(int *Buckets, int no_of_freqitems, ITEM_COUNTER* pfreqitems);
void OutputSinglePath(ITEM_COUNTER* pfreqitems, int length);
int *gpbuckets;

int main(int argc, char *argv[])
{
        struct rlimit cpu_limit = { 300, 300 };
        struct rlimit mem_limit = { 268435456, 268435456};
        setrlimit(RLIMIT_CPU, &cpu_limit  );
        setrlimit(RLIMIT_AS, &mem_limit  );
 	if(argc!=3 && argc!=4)
	{
		printf("Usage\n");
		printf("\t%s  data_filename  min_sup(absolute number) [output_filename]\n", argv[0]);
		return 0;
	}	

	strcpy(goparameters.data_filename, argv[1]);
	goparameters.nmin_sup = atoi(argv[2]);
	gnmin_sup = goparameters.nmin_sup;
	if(gnmin_sup<0)
		printf("Please specify a positive number for minimum support threshold\n");
	else if(gnmin_sup<1)
		gnmin_sup = 1;

	if(argc==4)
	{
		goparameters.bresult_name_given = true;
		strcpy(goparameters.pat_filename, argv[3]);
	}
	else 
		goparameters.bresult_name_given = false;

	MineAllPats();

//	printf("Total %d\n", gntotal_patterns);
	return 0;
}

//---------------------------------------------
//mining frequent itemsets from database
//---------------------------------------------
void MineAllPats()
{
	ITEM_COUNTER* pfreqitems;
	int *pitem_order_map;
	double ntotal_occurrences;

	clock_t start, start_mining;;

	int i;
	if(goparameters.bresult_name_given)
		gpfsout = new FSout(goparameters.pat_filename);

	start_mining = clock();

	gntotal_patterns = 0;
	gnmax_pattern_len = 0;
	gntotal_singlepath = 0;

	gndepth = 0;
	gntotal_call = 0;
	gpdata = new Data(goparameters.data_filename);

	//count frequent items in original database
	pfreqitems = NULL;
	start = clock();
	goDBMiner.ScanDBCountFreqItems(&pfreqitems);
	//goTimeTracer.mdInitial_count_time = (double)(clock()-start)/CLOCKS_PER_SEC;

	gppat_counters = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	for(i=0;i<gnmax_trans_len;i++)
		gppat_counters[i] = 0;

	if(gntotal_freqitems==0)
		delete gpdata;
	else if(gntotal_freqitems==1)
	{
		gnmax_pattern_len = 1;
		OutputOnePattern(pfreqitems[0].item, pfreqitems[0].support);
		delete gpdata;
	}
	else if(gntotal_freqitems>1)
	{
		gpheader_itemset = new int[gntotal_freqitems];
		//goMemTracer.IncBuffer(gntotal_freqitems*sizeof(int));

		//an array which maps an item to its frequency order; the most infrequent item has order 0
		pitem_order_map = new int[gnmax_item_id];
		for(i=0;i<gnmax_item_id;i++)
			pitem_order_map[i] = -1;
		for(i=0;i<gntotal_freqitems;i++)
			pitem_order_map[pfreqitems[i].item] = i;

		//if the number of frequent items in a conditonal database is smaller than maximal bucket size, use bucket counting technique
		if(gntotal_freqitems <= MAX_BUCKET_SIZE)
		{
			gpbuckets = new int[(1<<gntotal_freqitems)];
			memset(gpbuckets, 0, sizeof(int)*(1<<gntotal_freqitems));
			goDBMiner.ScanDBBucketCount(pitem_order_map, gpbuckets);
			bucket_count(gpbuckets, gntotal_freqitems, pfreqitems);
			delete []gpbuckets;
			delete gpdata;
			delete []pitem_order_map;
		}
		else
		{
			HEADER_TABLE pheader_table;

			goAFOPTMiner.Init(MIN(gnmax_trans_len, gntotal_freqitems+1));
			gpbuckets = new int[(1<<MAX_BUCKET_SIZE)];
			//goMemTracer.IncBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

			//initialize header table
			start = clock();
			pheader_table = new HEADER_NODE[gntotal_freqitems];
			//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*gntotal_freqitems);
			ntotal_occurrences = 0;
			for(i=0;i<gntotal_freqitems;i++)
			{
				pheader_table[i].item = pfreqitems[i].item;
				pheader_table[i].nsupport = pfreqitems[i].support;
				pheader_table[i].parray_conddb = NULL;
				pheader_table[i].order = i;
				ntotal_occurrences += pfreqitems[i].support;
			}

			//to choose a proper representation format for each conditional database
			if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD || 
				ntotal_occurrences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD ||
				gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD )
			{
				for(i=0;i<gntotal_freqitems;i++)
					pheader_table[i].flag = AFOPT_FLAG;
			}
			else
			{
				for(i=0;i<gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD;i++)
					pheader_table[i].flag = 0;
				for(i=MAX(0, gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD);i<gntotal_freqitems;i++)
					pheader_table[i].flag = AFOPT_FLAG;
			}

			//scan database to construct conditional databases and do bucket counting
			memset(gpbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
			goDBMiner.ScanDBBuildCondDB(pheader_table, pitem_order_map, gntotal_freqitems, gpbuckets);
			//goTimeTracer.mdInitial_construct_time = (double)(clock()-start)/CLOCKS_PER_SEC;
			//goMemTracer.mninitial_struct_size = //goMemTracer.mnArrayDB_size+//goMemTracer.mnAFOPTree_size+sizeof(int)*(1<<MAX_BUCKET_SIZE);			
			bucket_count(gpbuckets, MAX_BUCKET_SIZE, &(pfreqitems[gntotal_freqitems-MAX_BUCKET_SIZE]));

			delete []pitem_order_map;
			delete gpdata;
			
			//mining frequent itemsets in depth first order
			if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD ||
				ntotal_occurrences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD ||
				gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD)
				goAFOPTMiner.DepthAFOPTGrowth(pheader_table, gntotal_freqitems, 0);
			else
			{
				goArrayMiner.DepthArrayGrowth(pheader_table, gntotal_freqitems);
				goAFOPTMiner.DepthAFOPTGrowth(pheader_table, gntotal_freqitems, gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD);
			}

			delete []pheader_table;
			//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(HEADER_NODE));
			delete []gpbuckets;
			//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

		}
		delete []gpheader_itemset;
		//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(int));
	}
	delete []pfreqitems;
	//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(ITEM_COUNTER));
	
	if(goparameters.bresult_name_given)
		delete gpfsout;
	//goTimeTracer.mdtotal_running_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	PrintSummary();
	delete []gppat_counters;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));
}

//------------------------------------------------------------------------------------
//mining frequent itemsets from a conditional database represented by array structure
//------------------------------------------------------------------------------------
void CArrayMine::DepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems)
{
	int	t, k;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map;
	int num_of_newfreqitems;
	HEADER_TABLE pnewheader_table;

	clock_t start;

	gntotal_call++;

	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));

	if(gnmax_pattern_len<gndepth+1)
		gnmax_pattern_len = gndepth+1;

	for(k=0;k<num_of_freqitems-BUILD_TREE_ITEM_THRESHOLD;k++)
	{
		OutputOnePattern(pheader_table[k].item, pheader_table[k].nsupport);

		if(pheader_table[k].parray_conddb != NULL)
		{
			gpheader_itemset[gndepth] = pheader_table[k].item;
			gndepth++;

			//scan the conditional database to count frequent items
			for(t=k;t<num_of_freqitems;t++)
				pitem_suporder_map[t] = 0;
			start = clock();
			CountFreqItems(pheader_table[k].parray_conddb, pitem_suporder_map);
			//goTimeTracer.mdHStruct_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			num_of_newfreqitems = 0;
			for(t=k;t<num_of_freqitems;t++)
			{
				if(pitem_suporder_map[t]>=gnmin_sup)
				{
					pnewfreqitems[num_of_newfreqitems].item = pheader_table[t].item;
					pnewfreqitems[num_of_newfreqitems].support = pitem_suporder_map[t];
					pnewfreqitems[num_of_newfreqitems].order = t;
					num_of_newfreqitems++;
				}
				pitem_suporder_map[t] = -1;
			}
			if(num_of_newfreqitems>0)
			{
				qsort(pnewfreqitems, num_of_newfreqitems, sizeof(ITEM_COUNTER), comp_item_freq_asc);
				for(t=0;t<num_of_newfreqitems;t++)
					pitem_suporder_map[pnewfreqitems[t].order] = t;
			}


			if(num_of_newfreqitems==1)
			{
				if(gnmax_pattern_len<gndepth+1)
					gnmax_pattern_len = gndepth+1;
				OutputOnePattern(pnewfreqitems[0].item, pnewfreqitems[0].support);
			}
			else if(num_of_newfreqitems>1)
			{ 
				if(num_of_newfreqitems <= MAX_BUCKET_SIZE)
				{
					start = clock();
					memset(gpbuckets, 0, sizeof(int)*(1<<num_of_newfreqitems));
					BucketCount(pheader_table[k].parray_conddb, pitem_suporder_map, gpbuckets);
					bucket_count(gpbuckets, num_of_newfreqitems, pnewfreqitems);
					//goTimeTracer.mdBucket_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
				}
				else 
				{
					start = clock();
					pnewheader_table = new HEADER_NODE[num_of_newfreqitems];
					//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);

					for(t=0;t<num_of_newfreqitems;t++)
					{
						pnewheader_table[t].item = pnewfreqitems[t].item;
						pnewheader_table[t].nsupport = pnewfreqitems[t].support;
						pnewheader_table[t].pafopt_conddb = NULL;
						pnewheader_table[t].order = t;
					}
					if(num_of_newfreqitems<=BUILD_TREE_ITEM_THRESHOLD )
					{
						for(t=0;t<num_of_newfreqitems;t++)
							pnewheader_table[t].flag = AFOPT_FLAG;
					}
					else
					{
						for(t=0;t<num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD;t++)
							pnewheader_table[t].flag = 0;
						for(t=num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD;t<num_of_newfreqitems;t++)
							pnewheader_table[t].flag = AFOPT_FLAG;
					}

					memset(gpbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
					BuildNewCondDB(pheader_table[k].parray_conddb, pnewheader_table, pitem_suporder_map, num_of_newfreqitems, gpbuckets); 
					//goTimeTracer.mdHStruct_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;
					bucket_count(gpbuckets, MAX_BUCKET_SIZE, &(pnewfreqitems[num_of_newfreqitems-MAX_BUCKET_SIZE]));

					if(num_of_newfreqitems<=BUILD_TREE_ITEM_THRESHOLD)
						goAFOPTMiner.DepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0);
					else 
					{
						DepthArrayGrowth(pnewheader_table, num_of_newfreqitems);
						goAFOPTMiner.DepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD);
					}

					delete []pnewheader_table;
					//goMemTracer.DecBuffer(num_of_newfreqitems*sizeof(HEADER_NODE));
				}
			}

			start = clock();
			PushRight(pheader_table, k, num_of_freqitems-MAX_BUCKET_SIZE);
			//if(gndepth==1)
				//goTimeTracer.mdInitial_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			//else 
				//goTimeTracer.mdHStruct_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			gndepth--;
		}
	}

	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));

	return;
}

//-----------------------------------------------------------------------------------
//mining frequent itemsets from a conditional database represented by an AFOPT-tree
//------------------------------------------------------------------------------------
void CAFOPTMine::DepthAFOPTGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos)
{
	int	i;
	AFOPT_NODE *pcurroot;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map;
	int num_of_newfreqitems;
	HEADER_TABLE pnewheader_table;
	int k, nend_pos;
	ARRAY_NODE *pitem_list;

	clock_t start;

	gntotal_call++;

	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));

	if(gnmax_trans_len<gndepth+1)
		gnmax_trans_len = gndepth+1;
	nend_pos = num_of_freqitems-MAX_BUCKET_SIZE;
	for(k=nstart_pos;k<nend_pos;k++)
	{
		OutputOnePattern(pheader_table[k].item, pheader_table[k].nsupport);
		pcurroot = pheader_table[k].pafopt_conddb;
		if(pcurroot==NULL)
			continue;

  		gpheader_itemset[gndepth] = pheader_table[k].item;
		gndepth++;

		//if the tree contains only one single branch, then directly enumerate all frequent itemsets from the tree
		if(pcurroot->flag>0)
		{
			pitem_list = pcurroot->pitemlist;
			for(i=0;i<pcurroot->flag && pitem_list[i].support>=gnmin_sup;i++)
			{
				pnewfreqitems[i].item = pheader_table[pitem_list[i].order].item;
				pnewfreqitems[i].support = pitem_list[i].support;
			}

			OutputSinglePath(pnewfreqitems, i);

			if(pcurroot->flag>1 && pitem_list[0].order<nend_pos)
			{
				InsertTransaction(pheader_table, pitem_list[0].order, &(pitem_list[1]), pcurroot->flag-1);
			}
			delete []pitem_list;
			//goMemTracer.DelSingleBranch(pcurroot->flag);
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
			pheader_table[k].pafopt_conddb = NULL;
		}
		else if(pcurroot->flag==0)
		{

			//count frequent items from AFOPT-tree
			for(i=k+1;i<num_of_freqitems;i++)
				pitem_suporder_map[i] = 0;
			start = clock();
			CountFreqItems(pcurroot, pitem_suporder_map);
			//goTimeTracer.mdAFOPT_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			num_of_newfreqitems = 0;
			for(i=k+1;i<num_of_freqitems;i++)
			{
				if(pitem_suporder_map[i]>=gnmin_sup)
				{
					pnewfreqitems[num_of_newfreqitems].item = pheader_table[i].item;
					pnewfreqitems[num_of_newfreqitems].support = pitem_suporder_map[i];
					pnewfreqitems[num_of_newfreqitems].order = i;
					num_of_newfreqitems++;
				}
				pitem_suporder_map[i] = -1;
			}
			if(num_of_newfreqitems>0)
			{
				qsort(pnewfreqitems, num_of_newfreqitems, sizeof(ITEM_COUNTER), comp_item_freq_asc);
				for(i=0;i<num_of_newfreqitems;i++)
					pitem_suporder_map[pnewfreqitems[i].order] = i;
			}


			if(num_of_newfreqitems==1)
			{
				if(gnmax_trans_len<gndepth+1)
					gnmax_trans_len = gndepth+1;
				OutputOnePattern(pnewfreqitems[0].item, pnewfreqitems[0].support);
			}
			else if(num_of_newfreqitems>1)
			{
				if(num_of_newfreqitems<=MAX_BUCKET_SIZE)
				{
					start = clock();
					memset(gpbuckets, 0, sizeof(int)*(1<<num_of_newfreqitems));
					BucketCountAFOPT(pcurroot, pitem_suporder_map, gpbuckets);
					bucket_count(gpbuckets, num_of_newfreqitems, pnewfreqitems);
					//goTimeTracer.mdBucket_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
				}
				else 
				{
					start = clock();
					pnewheader_table = new HEADER_NODE[num_of_newfreqitems];
					//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);
					for(i=0;i<num_of_newfreqitems;i++)
					{
						pnewheader_table[i].item = pnewfreqitems[i].item;
						pnewheader_table[i].nsupport = pnewfreqitems[i].support;
						pnewheader_table[i].pafopt_conddb = NULL;
						pnewheader_table[i].order = i;
						pnewheader_table[i].flag = AFOPT_FLAG;
					}

					memset(gpbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
					BuildNewTree(pcurroot, pnewheader_table, pitem_suporder_map, num_of_newfreqitems, gpbuckets); 
					//goTimeTracer.mdAFOPT_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;
					bucket_count(gpbuckets, MAX_BUCKET_SIZE, &(pnewfreqitems[num_of_newfreqitems-MAX_BUCKET_SIZE]));

					DepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0);
					delete []pnewheader_table;
					//goMemTracer.DecBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);
				}
			}

			start = clock();
			PushRight(pheader_table, k, nend_pos);
			//if(gndepth==1)
				//goTimeTracer.mdInitial_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			//else
				//goTimeTracer.mdAFOPT_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
		}
		else 
		{
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
			pheader_table[k].pafopt_conddb = NULL;
		}
		gndepth--;
	}
	

	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));

	return;
}


typedef struct 
{
	int frequency;
	int next;
	int item;
} SNODE;

int get_combnum(int n, int m)
{
	int i, ncombnum;

	if(m==n || m==0)
		return 1;
	if(m==1 || m==n-1)
		return n;

	if(m>n-m)
		m = n-m;
	ncombnum = 1; 
	for(i=0;i<m;i++)
		ncombnum = ncombnum*(n-i)/(i+1);

	return ncombnum;
}


void OutputSinglePath(ITEM_COUNTER* pfreqitems, int length)
{
	SNODE*  pstack;
	int top;
	int next;
	int i;

	gntotal_patterns += (1<<length)-1;
	for(i=1;i<=length;i++)
		gppat_counters[gndepth+i] += get_combnum(length, i);
	if(gnmax_pattern_len<gndepth+length)
		gnmax_pattern_len = gndepth+length;

	if(length<=0)
		return;

	if(goparameters.bresult_name_given)
	{
		pstack = new SNODE[length+1];
		//goMemTracer.IncBuffer((length+1)*sizeof(SNODE));
		top = 0;

		pstack[top].item = -1;
		pstack[top].frequency = -1;
		pstack[top].next = top;

		top++;
		while(top>0)
		{
			top--;
			next = pstack[top].next;
			if(next<length)
			{
				pstack[top].next++;
				top++;
				pstack[top].item = pfreqitems[next].item;
				pstack[top].frequency = pfreqitems[next].support;
				pstack[top].next = next+1;

				if(top>0)
				{
					for(i=1;i<=top;i++)
						gpheader_itemset[gndepth+i-1] = pstack[i].item;
					gpfsout->printSet(gndepth+top, gpheader_itemset, pstack[top].frequency);
				}
				top++;
			}
			else if(next==length)
			{
				while(top>=0 && pstack[top].next==length)
					top--;
				top++;
			}

		}
		delete []pstack;
		//goMemTracer.DecBuffer((length+1)*sizeof(SNODE));
	}
}

int bucket_count(int *Buckets, int no_of_freqitems, ITEM_COUNTER* pfreqitems)
{
	int i, j;
	unsigned int k;
	int num_of_freqpats, npat_len;

	int num_buckets;

	num_buckets = (1 << no_of_freqitems);
		
	for(i=0;i<no_of_freqitems;i++)
	{
		k = 1 << i;
		for(j=0;j<num_buckets;j++)
		{
			if(!(k & j))
			{
				Buckets[j] = Buckets[j] + Buckets[j+(1 << i)];
			}
		}
	}

	num_of_freqpats = 0;
	for(i=1;i<num_buckets;i++)
	{
		if(Buckets[i]>=gnmin_sup)
		{
			npat_len = 0;
			for(j=0;j<no_of_freqitems;j++)
			{
				if(i & (1 << j))
				{
					gpheader_itemset[gndepth+npat_len] = pfreqitems[j].item;
					npat_len++;
				}
			}
			if(npat_len>0)
			{
				if(goparameters.bresult_name_given)
					gpfsout->printSet(gndepth+npat_len, gpheader_itemset, Buckets[i]);
				gntotal_patterns++;
				gppat_counters[gndepth+npat_len]++;
				if(gnmax_pattern_len < gndepth+npat_len)
					gnmax_pattern_len = gndepth+npat_len;
				num_of_freqpats++;
			}
		}
	}

	return num_of_freqpats;
}



