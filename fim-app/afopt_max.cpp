#include <string.h>
#include <time.h>


#include "Global.h"
#include "ScanDBMine.h"
#include "AFOPTMine.h"
#include "ArrayMine.h"
#include "patternsetmanager.h"
#include "parameters.h"
#include "fsout.h"
#include "data.h"

//#include "TimeTracer.h"
//#include "MemTracer.h"

int gntotal_lookaheads;
int gnsuccessful_lookaheads;
int gnprunedby_existmfi;

PatSetManager goPatSetManager;
MPATTERN *gppatterns;

void MineMaxPats();

int main(int argc, char *argv[])
{

	if(argc==4)
		goparameters.bresult_name_given = true;
	else if(argc==3)
		goparameters.bresult_name_given = false;
	else 
	{
		printf("Usage:\n");
		printf("\t%s data_filename min_sup(absolute number) [output_filename]\n", argv[0]);
		return 0;
	}

	strcpy(goparameters.data_filename, argv[1]);
	goparameters.nmin_sup = atoi(argv[2]);
	gnmin_sup = goparameters.nmin_sup;
	if(gnmin_sup<0)
		printf("Please specify a positive number for minimum support threshold\n");
	else if(gnmin_sup==0)
		gnmin_sup = 1;

	goparameters.SetResultName(argv[3]);

	MineMaxPats();

//	printf("Total: %d\n", gntotal_patterns);
	return 0;
}


void MineMaxPats()
{
	ITEM_COUNTER *pfreqitems;
	int *pitem_order_map;
	int *Buckets;
	HEADER_TABLE pheader_table;
	int i, num_of_countitems, ispruned;
	double dtotal_occurences;
	CHash_Map item_order_map;

	LOCAL_PATTERN_SET existmfi, newmfi, *psubexistmfi;

	existmfi.pheader = NULL;
	existmfi.ptail = NULL;
	newmfi.pheader = NULL;
	newmfi.ptail = NULL;

	if(goparameters.bresult_name_given)
		gpfsout = new FSout(goparameters.pat_filename);
	gpdata = new Data(goparameters.data_filename);

	gntotal_freqitems = 0;
	gntotal_patterns = 0;
	gnmax_pattern_len = 0;
	gntotal_singlepath = 0;

	gndepth = 0;
	gntotal_call = 0;

	gppatterns = NULL;

	clock_t start_mining, start;

	start_mining = clock();
	pfreqitems = NULL;
	goDBMiner.ScanDBCountFreqItems(&pfreqitems);
	//goTimeTracer.mdInitial_count_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	gppat_counters = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	for(i=0;i<gnmax_trans_len;i++)
		gppat_counters[i] = 0;

	goAFOPTMiner.Init(MIN(gnmax_trans_len, gntotal_freqitems+1));

	gpheader_itemset = new int[gntotal_freqitems];
	//goMemTracer.IncBuffer(gntotal_freqitems*sizeof(int));
	while(gntotal_freqitems>0 && pfreqitems[gntotal_freqitems-1].support==gndb_size)
	{
		gpheader_itemset[gndepth] = pfreqitems[gntotal_freqitems-1].item;
		gndepth++;
		gntotal_freqitems--;
	}

	if(gntotal_freqitems==0)
	{
		if(gndepth>0)
		{
			gntotal_patterns++;
			gnmax_pattern_len = gndepth;
			OutputOneMaxPat(gpheader_itemset, gndepth, pfreqitems[0].support);
		}
		delete gpdata;
	}
	else if(gntotal_freqitems==1)
	{
		gpheader_itemset[gndepth] = pfreqitems[0].item;
		OutputOneMaxPat(1, pfreqitems[0].support);
	}
	else if(gntotal_freqitems>1)
	{
		pitem_order_map = new int[gnmax_item_id];
		for(i=0;i<gnmax_item_id;i++)
			pitem_order_map[i] = -1;
		for(i=0;i<gntotal_freqitems;i++)
			pitem_order_map[pfreqitems[i].item] = i;

		if(gntotal_freqitems<MAX_BUCKET_SIZE)
		{
			Buckets = new int[(1<<gntotal_freqitems)];
			//goMemTracer.IncBuffer(sizeof(int)*(1<<gntotal_freqitems));
			memset(Buckets, 0, sizeof(int)*(1<<gntotal_freqitems));
			goDBMiner.ScanDBBucketCount(pitem_order_map, Buckets);
			bucket_count(Buckets, gntotal_freqitems, pfreqitems, &existmfi, &newmfi);
			delete []Buckets;
			//goMemTracer.DecBuffer(sizeof(int)*(1<<gntotal_freqitems));
			delete gpdata;
			delete []pitem_order_map;
		}
		else 
		{
			Buckets = new int[(1<<MAX_BUCKET_SIZE)];
			//goMemTracer.IncBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

			start= clock();
			pheader_table = new HEADER_NODE[gntotal_freqitems];
			//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*gntotal_freqitems);
			dtotal_occurences = 0;
			for(i=0;i<gntotal_freqitems;i++)
			{
				pheader_table[i].item = pfreqitems[i].item;
				pheader_table[i].nsupport = pfreqitems[i].support;
				pheader_table[i].order = i;
				pheader_table[i].pafopt_conddb = NULL;
				dtotal_occurences += pfreqitems[i].support;
			}
			if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD || 
				dtotal_occurences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD || 
				gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD)
			{
				for(i=0;i<gntotal_freqitems;i++)
					pheader_table[i].flag = AFOPT_FLAG;
			}
			else 
			{
				for(i=0;i<gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD;i++)
					pheader_table[i].flag = 0;
				for(i=gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD;i<gntotal_freqitems;i++)
					pheader_table[i].flag = AFOPT_FLAG;
			}

			memset(Buckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
			goDBMiner.ScanDBBuildCondDB(pheader_table, pitem_order_map, gntotal_freqitems, Buckets);
			//goTimeTracer.mdInitial_construct_time = (double)(clock()-start)/CLOCKS_PER_SEC;
			//goMemTracer.mninitial_struct_size = //goMemTracer.mnAFOPTree_size;
			delete gpdata;
			delete []pitem_order_map;

			item_order_map.init(gntotal_freqitems);
			for(i=0;i<gntotal_freqitems;i++)
				item_order_map.insert(pfreqitems[i].item, i);
			//goMemTracer.IncBuffer(item_order_map.mnmemory_size);
			num_of_countitems = gntotal_freqitems-MAX_BUCKET_SIZE;
			psubexistmfi = new LOCAL_PATTERN_SET[num_of_countitems+1];
			//goMemTracer.IncPatternBuffer((num_of_countitems+1)*sizeof(LOCAL_PATTERN_SET));
			for(i=0;i<num_of_countitems+1;i++)
			{
				psubexistmfi[i].pheader = NULL;
				psubexistmfi[i].ptail = NULL;
			}
			if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD || 
				dtotal_occurences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD || 
				gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD)
				ispruned = goAFOPTMiner.MFIDepthAFOPTGrowth(pheader_table, gntotal_freqitems, 0, psubexistmfi, &newmfi, &item_order_map);
			else
			{
				goArrayMiner.MFIDepthArrayGrowth(pheader_table, gntotal_freqitems, psubexistmfi, &newmfi, &item_order_map);
				ispruned = goAFOPTMiner.MFIDepthAFOPTGrowth(pheader_table, gntotal_freqitems, gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD, psubexistmfi, &newmfi, &item_order_map);
			}
			if(!ispruned)
			{
				bucket_count(Buckets, MAX_BUCKET_SIZE, &(pfreqitems[num_of_countitems]), &(psubexistmfi[num_of_countitems]), &newmfi);
				goPatSetManager.DeleteLocalPatternSet(&(psubexistmfi[num_of_countitems]));
			}
			delete []psubexistmfi;
			//goMemTracer.DecPatternBuffer((num_of_countitems+1)*sizeof(LOCAL_PATTERN_SET));
			//goMemTracer.DecBuffer(item_order_map.mnmemory_size);
			item_order_map.destroy();

			delete []Buckets;
			//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));
			delete []pheader_table;
			//goMemTracer.DecBuffer(sizeof(HEADER_NODE)*gntotal_freqitems);
		}
	}
	gntotal_freqitems += gndepth;

	delete []gpheader_itemset;
	//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(int));
	goPatSetManager.DeleteLocalPatternSet(&newmfi);
	goPatSetManager.DeletePatterns(gppatterns);

	delete []pfreqitems;
	//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(ITEM_COUNTER));
	if(goparameters.bresult_name_given)
		delete gpfsout;

	//goTimeTracer.mdtotal_running_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;
	PrintSummary();

	delete []gppat_counters;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));

}


void CArrayMine::MFIDepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, LOCAL_PATTERN_SET *psubexistmfi, LOCAL_PATTERN_SET *pnewmfi, CHash_Map *pitem_order_map)
{
	int	i, k;
	ITEM_COUNTER * pnewfreqitems;
	int *pitem_suporder_map, *pitemset;
	int num_of_newfreqitems;
	HEADER_TABLE pnewheader_table;
	int num_of_countitems, ispruned, norig_depth;
	int *Buckets;

	LOCAL_PATTERN_SET *pnewsubexistmfi, subnewmfi;
	CHash_Map newitem_order_map;

	clock_t start;

	gntotal_call++;

	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));
	Buckets = new int[(1<<MAX_BUCKET_SIZE)];
	//goMemTracer.IncBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

	pitemset = NULL;
	for(k=0;k<num_of_freqitems-BUILD_TREE_ITEM_THRESHOLD;k++)
	{
		norig_depth = gndepth;
		subnewmfi.pheader = NULL;
		subnewmfi.ptail = NULL;
		gpheader_itemset[gndepth] = pheader_table[k].item;
		gndepth++;

		start = clock();
		for(i=k+1;i<num_of_freqitems;i++)
			pitem_suporder_map[i] = 0;
		CountFreqItems(pheader_table[k].parray_conddb, pitem_suporder_map);
		//goTimeTracer.mdHStruct_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
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

		while(num_of_newfreqitems>0 && pnewfreqitems[num_of_newfreqitems-1].support==pheader_table[k].nsupport)
		{
			gpheader_itemset[gndepth] = pnewfreqitems[num_of_newfreqitems-1].item;
			gndepth++;
			pitem_suporder_map[pnewfreqitems[num_of_newfreqitems-1].order] = -1;
			num_of_newfreqitems--;
		}

		if(num_of_newfreqitems==0)
		{
			if(psubexistmfi[k].pheader==NULL || !goPatSetManager.PrunedByExistMFI(NULL, 0, &psubexistmfi[k]))
				goPatSetManager.AddMFI(NULL, 0, pheader_table[k].nsupport, &subnewmfi); 

		}
		else if(num_of_newfreqitems==1)
		{
			pitemset = &(gpheader_itemset[gndepth]);
			pitemset[0] = pnewfreqitems[0].item;
			if(psubexistmfi[k].pheader==NULL || !goPatSetManager.PrunedByExistMFI(pitemset, 1, &psubexistmfi[k]))
			{
				goPatSetManager.AddMFI(pitemset, 1, pnewfreqitems[0].support, &subnewmfi);
			}
		}
		else if(num_of_newfreqitems>1)
		{
			if(num_of_newfreqitems <= MAX_BUCKET_SIZE )
			{
				start = clock();
				memset(Buckets, 0, sizeof(int)*(1<<num_of_newfreqitems));
				BucketCount(pheader_table[k].parray_conddb, pitem_suporder_map, Buckets);
				bucket_count(Buckets, num_of_newfreqitems, pnewfreqitems, &(psubexistmfi[k]), &subnewmfi);
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
				}
				if(num_of_newfreqitems<=BUILD_TREE_ITEM_THRESHOLD )
				{
					for(i=0;i<num_of_newfreqitems;i++)
						pnewheader_table[i].flag = AFOPT_FLAG;
				}
				else
				{
					for(i=0;i<num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD;i++)
						pnewheader_table[i].flag = 0;
					for(i=num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD;i<num_of_newfreqitems;i++)
						pnewheader_table[i].flag = AFOPT_FLAG;
				}

				memset(Buckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
				BuildNewCondDB(pheader_table[k].parray_conddb, pnewheader_table, pitem_suporder_map, num_of_newfreqitems, Buckets); 
				//goTimeTracer.mdHStruct_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;

				newitem_order_map.init(num_of_newfreqitems);
				for(i=0;i<num_of_newfreqitems;i++)
					newitem_order_map.insert(pnewheader_table[i].item, i);
				//goMemTracer.IncBuffer(newitem_order_map.mnmemory_size);
				num_of_countitems = num_of_newfreqitems-MAX_BUCKET_SIZE;
				pnewsubexistmfi = new LOCAL_PATTERN_SET[num_of_countitems+1];
				//goMemTracer.IncPatternBuffer((num_of_countitems+1)*sizeof(LOCAL_PATTERN_SET));
				for(i=0;i<num_of_countitems+1;i++)
				{
					pnewsubexistmfi[i].pheader = NULL;
					pnewsubexistmfi[i].ptail = NULL;
				}
				if(psubexistmfi[k].pheader!=NULL)
					goPatSetManager.ProjectMFIs(&(psubexistmfi[k]), pnewsubexistmfi, num_of_newfreqitems, &newitem_order_map);
				if(num_of_newfreqitems<=BUILD_TREE_ITEM_THRESHOLD)
					ispruned = goAFOPTMiner.MFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0, pnewsubexistmfi, &subnewmfi, &newitem_order_map);
				else
				{
					goArrayMiner.MFIDepthArrayGrowth(pnewheader_table, num_of_newfreqitems, pnewsubexistmfi, &subnewmfi, &newitem_order_map);
					ispruned = goAFOPTMiner.MFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD, pnewsubexistmfi, &subnewmfi, &newitem_order_map);
				}
				if(!ispruned)
				{
					bucket_count(Buckets, MAX_BUCKET_SIZE, &(pnewfreqitems[num_of_countitems]), &(pnewsubexistmfi[num_of_countitems]), &subnewmfi);
					goPatSetManager.DeleteLocalPatternSet(&(pnewsubexistmfi[num_of_countitems]));
				}
				delete []pnewsubexistmfi;
				//goMemTracer.DecPatternBuffer((num_of_countitems+1)*sizeof(LOCAL_PATTERN_SET));
				//goMemTracer.DecBuffer(newitem_order_map.mnmemory_size);
				newitem_order_map.destroy();

				delete []pnewheader_table;
				//goMemTracer.DecBuffer(num_of_newfreqitems*sizeof(HEADER_NODE));
			}
		}
		if(subnewmfi.pheader!=NULL)
			goPatSetManager.PushNewMFIRight(&subnewmfi, k, pnewmfi, psubexistmfi, num_of_freqitems, pitem_order_map);
		if(psubexistmfi[k].pheader!=NULL)
			goPatSetManager.PushMFIRight(&(psubexistmfi[k]), psubexistmfi, k, num_of_freqitems, pitem_order_map); 

		start = clock();
		PushRight(pheader_table, k, num_of_freqitems-MAX_BUCKET_SIZE);
		//goTimeTracer.mdHStruct_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;

		gndepth = norig_depth;
	}

	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
	delete []Buckets;
	//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

	return;
}


int CAFOPTMine::MFIDepthAFOPTGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos, LOCAL_PATTERN_SET *psubexistmfi, LOCAL_PATTERN_SET *pnewmfi, CHash_Map *pitem_order_map)
{
	int	 i;
	AFOPT_NODE	*ptreenode, *pcurroot;
	ARRAY_NODE *pitem_list;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map, *pitemset;
	int num_of_newfreqitems, num_of_countitems;
	bool issinglepath, isSumedByLeftSib;
	int order, depth, npbyclosed, support, ispruned;
	HEADER_TABLE pnewheader_table;
	clock_t start;

	LOCAL_PATTERN_SET *pnewsubexistmfi, subnewmfi;
	CHash_Map newitem_order_map;
	int *Buckets;

	gntotal_call++;

	pitemset = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));
	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));
	Buckets = new int[(1<<MAX_BUCKET_SIZE)];
	//goMemTracer.IncBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

	isSumedByLeftSib = false;
	for(order=nstart_pos;order<num_of_freqitems-MAX_BUCKET_SIZE;order++)
	{
		pcurroot = pheader_table[order].pafopt_conddb;
		gpheader_itemset[gndepth] = pheader_table[order].item;
		gndepth++;
		subnewmfi.pheader = NULL;
		subnewmfi.ptail = NULL;

		if(isSumedByLeftSib)
			isSumedByLeftSib = false;
		else if(pcurroot==NULL || pcurroot->flag == -1)
		{
			if(psubexistmfi[order].pheader==NULL)
				goPatSetManager.AddMFI(NULL, 0, pheader_table[order].nsupport, pnewmfi); 
		}
		else
		{
			//[begin] pruning by existing mfis
			if(psubexistmfi[order].pheader!=NULL)
			{
				for(i=order+1;i<num_of_freqitems;i++)
					pitemset[i-order-1] = pheader_table[i].item;
				if(goPatSetManager.PrunedByExistMFI(pitemset, num_of_freqitems-order-1, &psubexistmfi[order]))
				{
					gnprunedby_existmfi++;

					for(i=order;i<num_of_freqitems-MAX_BUCKET_SIZE;i++)
					{
						if(pheader_table[i].pafopt_conddb!=NULL)
							DestroyTree(pheader_table[i].pafopt_conddb);
					}

					delete []pnewfreqitems;
					//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
					delete []pitemset;
					//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
					delete []pitem_suporder_map;
					//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
					delete []Buckets;
					//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

					for(i=order;i<num_of_freqitems-MAX_BUCKET_SIZE+1;i++)
						goPatSetManager.DeleteLocalPatternSet(&psubexistmfi[i]);
					
					gndepth--;
					return 1;
				}
			}
			//[end]

			
			//[begin] lookahead
			ptreenode = pheader_table[order].pafopt_conddb;
			gntotal_lookaheads++;
			for(i=order+1;i<num_of_freqitems && ptreenode->frequency>=gnmin_sup;i++)
			{
				if(ptreenode->flag == -1)
					break;
				if(ptreenode->flag>0)
				{
					if(ptreenode->flag+i==num_of_freqitems)
					{
						pitem_list = ptreenode->pitemlist;
						for(int j=0;j<ptreenode->flag;j++)
						{
							if(pitem_list[j].order==i && pitem_list[j].support>=gnmin_sup)
								i++;
							else 
								break;
						}
					}
					break;
				}
				if(ptreenode->pchild->order==i)
					ptreenode = ptreenode->pchild;
				else 
					break;
			}

			if(i==num_of_freqitems && ptreenode->frequency>=gnmin_sup)
			{
				gnsuccessful_lookaheads++;
				for(i=order+1;i<num_of_freqitems;i++)
					pitemset[i-order-1] = pheader_table[i].item;
				goPatSetManager.AddMFI(pitemset, num_of_freqitems-order-1, ptreenode->frequency, pnewmfi);

				for(i=order;i<num_of_freqitems-MAX_BUCKET_SIZE;i++)
				{
					if(pheader_table[i].pafopt_conddb!=NULL)
						DestroyTree(pheader_table[i].pafopt_conddb);
				}

				delete []pnewfreqitems;
				//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));				
				delete []pitemset;
				//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
				delete []pitem_suporder_map;
				//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
				delete []Buckets;
				//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

				for(i=order;i<num_of_freqitems-MAX_BUCKET_SIZE+1;i++)
					goPatSetManager.DeleteLocalPatternSet(&psubexistmfi[i]);

				gndepth--;
				return 1;
			}
			//[end] lookahead

 
			issinglepath = false;
			depth = 0;
			ptreenode = pheader_table[order].pafopt_conddb;
			support = pheader_table[order].nsupport;

			//[begin] single path trimming			
			if(ptreenode->flag>0)
			{
				issinglepath = true;
				for(i=0;i<ptreenode->flag;i++)
				{
					if(ptreenode->pitemlist[i].support<gnmin_sup)
						break;
					pitemset[depth] = pheader_table[ptreenode->pitemlist[i].order].item;
					depth++;
					support = ptreenode->pitemlist[i].support;
				}
			}
			else
			{
				ptreenode = ptreenode->pchild;
 				while(ptreenode!=NULL && ptreenode->prightsibling==NULL && ptreenode->frequency>=gnmin_sup)
				{
					pitemset[depth] = pheader_table[ptreenode->order].item;
					depth++;
					pcurroot = ptreenode;
					support = ptreenode->frequency;
					if(ptreenode->flag==0)
						ptreenode = ptreenode->pchild;
					else 
						break;
				}

				if(ptreenode==NULL || (ptreenode->prightsibling==NULL && ptreenode->frequency<gnmin_sup))
				{
					issinglepath = true;
				}
				else if(ptreenode->flag>0 && ptreenode->prightsibling==NULL)
				{
					issinglepath = true;
					for(i=0;i<ptreenode->flag;i++)
					{
						if(ptreenode->pitemlist[i].support<gnmin_sup)
							break;
						pitemset[depth] = pheader_table[ptreenode->pitemlist[i].order].item;
						depth++;
						support = ptreenode->pitemlist[i].support;
					}
				}
			}
			//[end] single path trimming

			if(issinglepath)
			{
				if((depth==0 && psubexistmfi[order].pheader==NULL) || (depth>0 &&!goPatSetManager.PrunedByExistMFI(pitemset, depth, &psubexistmfi[order])))
					goPatSetManager.AddMFI(pitemset, depth, support, &subnewmfi);
			}
			else 
			{
				npbyclosed = 0;
				for(i=0;i<depth;i++)
					gpheader_itemset[gndepth+i] = pitemset[i];

				start = clock();
				for(i=order+1;i<num_of_freqitems;i++)
					pitem_suporder_map[i] = 0;
				CountFreqItems(pcurroot, pitem_suporder_map);
				//goTimeTracer.mdAFOPT_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
				num_of_newfreqitems = 0;
				for(i=order+1;i<num_of_freqitems;i++)
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

				for(i=num_of_newfreqitems-1;i>=0;i--)
				{
					if(pnewfreqitems[i].support==pcurroot->frequency)
					{
						gpheader_itemset[gndepth+depth+npbyclosed] = pnewfreqitems[i].item;
						pitemset[depth+npbyclosed] = pnewfreqitems[i].item;
						npbyclosed++;
						pitem_suporder_map[pnewfreqitems[i].order] = -1;
					}
					else break;
				}
				num_of_newfreqitems -= npbyclosed;

				if(num_of_newfreqitems==0)
				{
					if(depth+npbyclosed==0)
					{
						if(psubexistmfi[order].pheader==NULL)
							goPatSetManager.AddMFI(pitemset, depth+npbyclosed, pcurroot->frequency, pnewmfi); 
					}
					else if(psubexistmfi[order].pheader==NULL || !goPatSetManager.PrunedByExistMFI(pitemset, depth+npbyclosed, &psubexistmfi[order]))
						goPatSetManager.AddMFI(pitemset, depth+npbyclosed, pcurroot->frequency, &subnewmfi); 
				}
				else if(num_of_newfreqitems==1)
				{
					pitemset[depth+npbyclosed] = pnewfreqitems[0].item;
					if(psubexistmfi[order].pheader==NULL || !goPatSetManager.PrunedByExistMFI(pitemset, depth+npbyclosed+1, &psubexistmfi[order]))
						goPatSetManager.AddMFI(pitemset, depth+npbyclosed+1, pnewfreqitems[0].support, &subnewmfi);
				}
				else if(num_of_newfreqitems<=MAX_BUCKET_SIZE)
				{
					gndepth = gndepth + depth +npbyclosed;
					start = clock();
					memset(Buckets, 0, sizeof(int)*(1<<num_of_newfreqitems));
					BucketCountAFOPT(pcurroot, pitem_suporder_map, Buckets);
					bucket_count(Buckets, num_of_newfreqitems, pnewfreqitems, &psubexistmfi[order], &subnewmfi);
					//goTimeTracer.mdBucket_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
					gndepth = gndepth- depth - npbyclosed;
				}
				else 
				{
					for(i=0;i<num_of_newfreqitems;i++)
						pitemset[depth+npbyclosed+i] = pnewfreqitems[i].item;
					if(psubexistmfi[order].pheader==NULL || !goPatSetManager.PrunedByExistMFI(pitemset, depth+npbyclosed+num_of_newfreqitems, &psubexistmfi[order]))
					{
						gndepth = gndepth + depth +npbyclosed;

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
						memset(Buckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
						BuildNewTree(pcurroot, pnewheader_table, pitem_suporder_map, num_of_newfreqitems, Buckets); 
						//goTimeTracer.mdAFOPT_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;

						newitem_order_map.init(num_of_newfreqitems);
						for(i=0;i<num_of_newfreqitems;i++)
							newitem_order_map.insert(pnewfreqitems[i].item, i);
						//goMemTracer.IncBuffer(newitem_order_map.mnmemory_size);
						num_of_countitems = num_of_newfreqitems-MAX_BUCKET_SIZE;
						pnewsubexistmfi = new LOCAL_PATTERN_SET[num_of_countitems+1];
						//goMemTracer.IncPatternBuffer((num_of_countitems+1)*sizeof(LOCAL_PATTERN_SET));
						for(i=0;i<num_of_countitems+1;i++)
						{
							pnewsubexistmfi[i].pheader = NULL;
							pnewsubexistmfi[i].ptail = NULL;
						}
						if(psubexistmfi[order].pheader!=NULL)
							goPatSetManager.ProjectMFIs(&(psubexistmfi[order]), pnewsubexistmfi, num_of_newfreqitems, &newitem_order_map);
						ispruned = MFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0, pnewsubexistmfi, &subnewmfi, &newitem_order_map);
						if(!ispruned)
						{
							bucket_count(Buckets, MAX_BUCKET_SIZE, &(pnewfreqitems[num_of_countitems]), &(pnewsubexistmfi[num_of_countitems]), &subnewmfi);
							goPatSetManager.DeleteLocalPatternSet(&(pnewsubexistmfi[num_of_countitems]));
						}
						delete []pnewsubexistmfi;
						//goMemTracer.DecPatternBuffer((num_of_countitems+1)*sizeof(LOCAL_PATTERN_SET));
						//goMemTracer.DecBuffer(newitem_order_map.mnmemory_size);
						newitem_order_map.destroy();
						delete []pnewheader_table;
						//goMemTracer.DecBuffer(num_of_newfreqitems*sizeof(HEADER_NODE));						

						gndepth = gndepth- depth - npbyclosed;
					}
				}
			}
		}
		if(subnewmfi.pheader!=NULL)
			goPatSetManager.PushNewMFIRight(&subnewmfi, order, pnewmfi, psubexistmfi, num_of_freqitems, pitem_order_map);
		if(psubexistmfi[order].pheader!=NULL)
			goPatSetManager.PushMFIRight(&(psubexistmfi[order]), psubexistmfi, order, num_of_freqitems, pitem_order_map); 


		pcurroot = pheader_table[order].pafopt_conddb;
		if(pcurroot!=NULL && order<num_of_freqitems-1 && pheader_table[order+1].pafopt_conddb==NULL && 
		  (pcurroot->flag==0 && pcurroot->pchild->order==order+1 && pcurroot->pchild->frequency==pheader_table[order+1].nsupport ||
		  pcurroot->flag>0 && pcurroot->pitemlist[0].order==order+1 && pcurroot->pitemlist[0].support==pheader_table[order+1].nsupport))
			isSumedByLeftSib = true;


		start = clock();
		if(pcurroot!=NULL && pcurroot->flag!=-1)
			PushRight(pheader_table, order, num_of_freqitems-MAX_BUCKET_SIZE);
		else if(pcurroot!=NULL)
		{
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
		}
		//goTimeTracer.mdAFOPT_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
		gndepth--;
	}

	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitemset;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
	delete []pitem_suporder_map;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
	delete []Buckets;
	//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

	return 0;
}




typedef struct
{
	int nitem_order;
	int nmax_nextorder;
} BC_STACK_NODE;

int bucket_count(int *Buckets, int no_of_freqitems, ITEM_COUNTER* pfreqitems, LOCAL_PATTERN_SET *pexistmfis, LOCAL_PATTERN_SET *pnewmfi)
{
	int i, j, t;
	int k;
	int num_buckets;

	num_buckets = (1 << no_of_freqitems);

	for(i=0;i<no_of_freqitems;i++)
	{
		k = 0;
		while(k<num_buckets)
		{
			t = (1<<i);
			for(j=k;j<k+t;j++)
				Buckets[j] = Buckets[j] + Buckets[j+t];
			k += (t<<1);
		}
	}

	bool changed = false;
	BC_STACK_NODE *pstack;
	int top = 0, nmax_order, depth, nsupport;
	pstack = new BC_STACK_NODE[no_of_freqitems+1];
	//goMemTracer.IncBuffer((no_of_freqitems+1)*sizeof(BC_STACK_NODE));
	int* pitemset;
	pitemset = new int[no_of_freqitems];
	//goMemTracer.IncBuffer(sizeof(int)*no_of_freqitems);

	t = 0;
	k = 0;
	nsupport = 0;
	pstack[top].nitem_order = -1;
	pstack[top].nmax_nextorder = no_of_freqitems;
	top++;
	depth = 0;
	while(top>0)
	{
		if(t<pstack[top-1].nmax_nextorder)
		{
			k += (1<<t);
			if(Buckets[k]>=gnmin_sup)
			{
				nsupport = Buckets[k];
				gpheader_itemset[gndepth+depth] = pfreqitems[t].item;
				depth++;
				pstack[top].nitem_order = t;
				pstack[top].nmax_nextorder = no_of_freqitems;
				top++;
				t++;
				changed = true;
			}
			else 
			{
				k -= (1<<t);
				t++;
			}
		}
		else if(top>1)
		{
			if(changed)
			{
				for(i=0;i<depth;i++)
					pitemset[i] = gpheader_itemset[gndepth+i];
				if(!goPatSetManager.PrunedByExistMFI(pitemset, depth, pnewmfi) && !goPatSetManager.PrunedByExistMFI(pitemset, depth, pexistmfis))
				{
					goPatSetManager.AddMFI(pitemset, depth, nsupport, pnewmfi);

				}
			}

			if(pstack[1].nitem_order+top-1>=no_of_freqitems)
			{
				depth = depth - (top-1);
				break;
			}
			depth--;
			top--;
			k -= (1<<pstack[top].nitem_order);
			t = pstack[top].nitem_order+1;
			nmax_order = pstack[top].nitem_order;
			if(nmax_order==(no_of_freqitems-1))
			{
				while(top>1 && pstack[top].nitem_order-pstack[top-1].nitem_order==1)
				{
					depth--;
					top--;
					k -= (1<<pstack[top].nitem_order);
					t = pstack[top].nitem_order+1;
				}
				nmax_order = pstack[top].nitem_order;
				if(top>1)
				{
					depth--;
					top--;
					k -= (1<<pstack[top].nitem_order);
					t = pstack[top].nitem_order+1;
				}
				for(i=0;i<top;i++)
				{
					if(pstack[i].nmax_nextorder>nmax_order)
						pstack[i].nmax_nextorder = nmax_order;
				}
			}
			changed = false;
		}
		else break;

	}

	delete []pstack;
	//goMemTracer.DecBuffer(sizeof(BC_STACK_NODE)*(no_of_freqitems+1));
	delete []pitemset;
	//goMemTracer.DecBuffer(sizeof(int)*no_of_freqitems);

	return 0;
}

