#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>


#include "Global.h"
#include "ScanDBMine.h"
#include "AFOPTMine.h"
#include "ArrayMine.h"
#include "parameters.h"
#include "fsout.h"
#include "data.h"
#include "cfptree_outbuf.h"

//#include "MemTracer.h"
//#include "TimeTracer.h"

ITEM_COUNTER *gpfreqitems;
int  gntree_level;
int *gpitem_order_map;

MAP_NODE** gpitem_sup_map;
int gnprune_times;
int gnmap_pruned_times;

CTreeOutBufManager gotree_bufmanager;

void MineClosedPats();

int inline PrunedByExistCFI(int nsuffix_len, int nsupport);
void update_suplen_map(int nsuffix_len, int nsupport);
int OutputSingleClosePath(int* pitem_sups, int length, int nroot_sup);
int OutputCommonPrefix(int* pitem_sups, int length, int *bitmap, int nroot_sup);

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
	if(goparameters.nmin_sup<0)
	{
		printf("Please specify a positive number for minimum support threshold\n");
		return 0;
	}
	else if(goparameters.nmin_sup==0)
		goparameters.nmin_sup = 1;
	gnmin_sup = goparameters.nmin_sup;

	goparameters.SetResultName(argv[3]);

	MineClosedPats();

//	printf("Total: %d\n", gntotal_patterns);
	return 0;
}

void MineClosedPats()
{
	int i, base_bitmap;
	clock_t start_mining, start;
	double dtotal_occurences;
	CFP_NODE cfp_node;

	if(goparameters.bresult_name_given)
		gpfsout = new FSout(goparameters.pat_filename);

	gntotal_freqitems = 0;
	gnmax_pattern_len = 0;
	gntotal_singlepath = 0;
	gntotal_patterns = 0;

	gntree_level = 0; 
	gnfirst_level = 0;
	gnfirst_level_depth = 0;
	gntotal_call = 0;

	gpdata = new Data(goparameters.data_filename);

	gpfreqitems = NULL;
	start_mining = clock();
	goDBMiner.ScanDBCountFreqItems(&gpfreqitems);
	//goTimeTracer.mdInitial_count_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	goAFOPTMiner.Init(MIN(gnmax_trans_len, gntotal_freqitems+1));

	goparameters.SetBufParameters(sizeof(ENTRY));
	gotree_bufmanager.Init(gnmax_trans_len);

	gpheader_itemset = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	gndepth = 0;
	gppat_counters = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	for(i=0;i<gnmax_trans_len;i++)
		gppat_counters[i] = 0;

	gnprune_times = 0;
	gnmap_pruned_times = 0;
	
	while(gntotal_freqitems>0 && gpfreqitems[gntotal_freqitems-1].support==gndb_size)
	{
		gpheader_itemset[gndepth] = gpfreqitems[gntotal_freqitems-1].item;
		gndepth++;
		gntotal_freqitems--;
	}

	base_bitmap = 0;
	if(gndepth>0)
	{
		gnfirst_level = 1;
		gnfirst_level_depth = gndepth;

		OutputOneClosePat(gndb_size);
		for(i=0;i<gntotal_freqitems;i++)
			base_bitmap = base_bitmap | (1<<(gpfreqitems[i].item%INT_BIT_LEN));
		gotree_bufmanager.InsertCommonPrefixNode(gntree_level, gpheader_itemset, gndepth, gndb_size, base_bitmap);
		gntree_level++;
	}

	if(gntotal_freqitems==0)
		delete gpdata;
	else if(gntotal_freqitems==1)
	{
		gpheader_itemset[gndepth] = gpfreqitems[0].item;
		OutputOneClosePat(1, gpfreqitems[0].support);
		gotree_bufmanager.WriteLeafNode(&(gpfreqitems[0].item), 1, gpfreqitems[0].support, 0);
		delete gpdata;
	}
	else if(gntotal_freqitems>1)
	{
		HEADER_TABLE pheader_table;

		gpitem_order_map = new int[gnmax_item_id];
		for(i=0;i<gnmax_item_id;i++)
			gpitem_order_map[i] = -1;
		for(i=0;i<gntotal_freqitems;i++)
			gpitem_order_map[gpfreqitems[i].item] = i;

		gpitem_sup_map = new MAP_NODE*[gntotal_freqitems];
		//goMemTracer.IncTwoLayerMap(gntotal_freqitems*sizeof(MAP_NODE*));
		for(i=0;i<gntotal_freqitems;i++)
			gpitem_sup_map[i] = NULL;
		
		pheader_table = new HEADER_NODE[gntotal_freqitems];
		//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*gntotal_freqitems);
		dtotal_occurences = 0;
		for(i=0;i<gntotal_freqitems;i++)
		{
			pheader_table[i].item = gpfreqitems[i].item;
			pheader_table[i].nsupport = gpfreqitems[i].support;
			pheader_table[i].order = i;
			pheader_table[i].pafopt_conddb = NULL;
			dtotal_occurences += gpfreqitems[i].support;
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
		start= clock();
		goDBMiner.ScanDBBuildCondDB(pheader_table, gpitem_order_map);
		//goTimeTracer.mdInitial_construct_time = (double)(clock()-start)/CLOCKS_PER_SEC;
		//goMemTracer.mninitial_struct_size = //goMemTracer.mnAFOPTree_size+//goMemTracer.mnArrayDB_size;
		delete gpdata;

		cfp_node.pos = gotree_bufmanager.GetTreeSize();
		cfp_node.cur_order = 0;
		cfp_node.length = gntotal_freqitems;
		cfp_node.pentries = new ENTRY[cfp_node.length];
		//goMemTracer.IncCFPSize(cfp_node.length*sizeof(ENTRY));
		for(i=0;i<cfp_node.length;i++)
		{
			cfp_node.pentries[i].item = pheader_table[i].item;
			cfp_node.pentries[i].support = pheader_table[i].nsupport;
			cfp_node.pentries[i].hash_bitmap = 0;
			cfp_node.pentries[i].child = 0;
		}
		gotree_bufmanager.InsertActiveNode(gntree_level, &cfp_node);
		gntree_level++;

		if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD || 
			dtotal_occurences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD || 
			gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD)
		{
			goAFOPTMiner.CFIDepthAFOPTGrowth(pheader_table, gntotal_freqitems, 0, &cfp_node);
		}
		else 
		{
			goArrayMiner.CFIDepthArrayGrowth(pheader_table, gntotal_freqitems, &cfp_node);
			goAFOPTMiner.CFIDepthAFOPTGrowth(pheader_table, gntotal_freqitems, gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD, &cfp_node);
		}

		gntree_level--;
		gotree_bufmanager.WriteInternalNode(&cfp_node);
		delete []cfp_node.pentries;
		//goMemTracer.DecCFPSize(cfp_node.length*sizeof(ENTRY));

		delete []pheader_table;
		//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(HEADER_NODE));

		delete []gpitem_order_map;
		for(i=0;i<gntotal_freqitems;i++)
		{
			if(gpitem_sup_map[i]!=NULL)
			{
				printf("The two-layer map for item %d should be empty\n", gpfreqitems[i].item);
				delete []gpitem_sup_map[i];
				//goMemTracer.DecTwoLayerMap(MIN(SUP_MAXLEN_MAP_SIZE, gpfreqitems[i].support-(int)gnmin_sup+1)*sizeof(MAP_NODE));
			}
		}
		delete []gpitem_sup_map;
		//goMemTracer.DecTwoLayerMap(gntotal_freqitems*sizeof(MAP_NODE*));
	}

	if(gndepth>0)
	{
		gotree_bufmanager.WriteCommonPrefixNodes(1, base_bitmap);
		gntree_level--;
	}

	if(goparameters.bresult_name_given)
		delete gpfsout;

	delete []gpheader_itemset;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));
	gntotal_freqitems += gndepth;
	delete []gpfreqitems;
	//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(ITEM_COUNTER));
	gotree_bufmanager.Destroy();

	//goTimeTracer.mdtotal_running_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	PrintSummary();
	delete []gppat_counters;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));

}

void CArrayMine::CFIDepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, CFP_NODE *pcfp_node)
{
	int	k, i;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map;
	int num_of_newfreqitems, npbyclosed;
	int* pitemset, *pitem_sups;
	HEADER_TABLE pnewheader_table;
	clock_t start;
	CFP_NODE newcfp_node;

	int bitmap, base_bitmap, nclass_pos;

	gntotal_call++;

	bitmap = 0; 
	pitem_sups = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(sizeof(int)*gnmax_trans_len);
	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems+1];
	//goMemTracer.IncBuffer((num_of_freqitems+1)*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));	

	for(k=0;k<num_of_freqitems-BUILD_TREE_ITEM_THRESHOLD;k++)
	{
		gpheader_itemset[gndepth] = pheader_table[k].item;
		gndepth++;
		pitemset = &(gpheader_itemset[gndepth]);

		nclass_pos = PrunedByExistCFI(0, pheader_table[k].nsupport);
		if(nclass_pos!=0)
		{
			pcfp_node->pentries[k].hash_bitmap = nclass_pos;
		}
		else if(pheader_table[k].parray_conddb!=NULL)
		{
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

			npbyclosed = 0;
			while(num_of_newfreqitems>0 && pnewfreqitems[num_of_newfreqitems-1].support==pheader_table[k].nsupport)
			{
				gpheader_itemset[gndepth+npbyclosed] = pnewfreqitems[num_of_newfreqitems-1].item;
				npbyclosed++;
				pitem_suporder_map[pnewfreqitems[num_of_newfreqitems-1].order] = -1;
				num_of_newfreqitems--;
			}

			base_bitmap = 0;
			for(i=0;i<num_of_newfreqitems;i++)
				base_bitmap = base_bitmap | (1<< (pnewfreqitems[i].item%INT_BIT_LEN));
			bitmap = 0;
			for(i=0;i<npbyclosed;i++)
				bitmap = bitmap | (1<<pitemset[i]%INT_BIT_LEN);
			if(npbyclosed+num_of_newfreqitems>0)
			{
				pcfp_node->pentries[k].child = gotree_bufmanager.GetTreeSize();
				pcfp_node->pentries[k].hash_bitmap = bitmap | base_bitmap;
			}

			if(npbyclosed>0 && num_of_newfreqitems==0)
				gotree_bufmanager.WriteLeafNode(pitemset, npbyclosed, pheader_table[k].nsupport, 0);
			else if(npbyclosed>0 && num_of_newfreqitems>0)
			{
				gotree_bufmanager.InsertCommonPrefixNode(gntree_level, pitemset, npbyclosed, pheader_table[k].nsupport, base_bitmap);
				gntree_level++;
			}

			if(npbyclosed>0)
				update_suplen_map(npbyclosed, pheader_table[k].nsupport);
			gndepth += npbyclosed;
			OutputOneClosePat(pheader_table[k].nsupport);

			if(num_of_newfreqitems==1)
			{
				gpheader_itemset[gndepth] = pnewfreqitems[0].item;
				nclass_pos = PrunedByExistCFI(1, pnewfreqitems[0].support);
				if(nclass_pos==0)
				{
					OutputOneClosePat(1, pnewfreqitems[0].support);
					gotree_bufmanager.WriteLeafNode(&(pnewfreqitems[0].item), 1, pnewfreqitems[0].support, 0);
				}
				else 
				{
					if(npbyclosed==0)
					{
						pcfp_node->pentries[k].child = 0;
						pcfp_node->pentries[k].hash_bitmap = 0;
					}
					else 
						pcfp_node->pentries[k].hash_bitmap = bitmap;
					base_bitmap = 0;
				}
			}
			else if(num_of_newfreqitems>1)
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
				BuildNewCondDB(pheader_table[k].parray_conddb, pnewheader_table, pitem_suporder_map); 
				//goTimeTracer.mdHStruct_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;

				newcfp_node.pos = gotree_bufmanager.GetTreeSize();
				newcfp_node.cur_order = 0;
				newcfp_node.length = num_of_newfreqitems;
				newcfp_node.pentries = new ENTRY[newcfp_node.length];
				//goMemTracer.IncCFPSize(newcfp_node.length*sizeof(ENTRY));
				for(i=0;i<newcfp_node.length;i++)
				{
					newcfp_node.pentries[i].item = pnewfreqitems[i].item;
					newcfp_node.pentries[i].support = pnewfreqitems[i].support;
					newcfp_node.pentries[i].hash_bitmap = 0;
					newcfp_node.pentries[i].child = 0;
				}
				gotree_bufmanager.InsertActiveNode(gntree_level, &newcfp_node);
				gntree_level++;
				if(num_of_newfreqitems<=BUILD_TREE_ITEM_THRESHOLD)
					goAFOPTMiner.CFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0, &newcfp_node);
				else
				{
					goArrayMiner.CFIDepthArrayGrowth(pnewheader_table, num_of_newfreqitems, &newcfp_node);
					goAFOPTMiner.CFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, num_of_newfreqitems-BUILD_TREE_ITEM_THRESHOLD, &newcfp_node);
				}
				gntree_level--;
				gotree_bufmanager.WriteInternalNode(&newcfp_node);
				delete []newcfp_node.pentries;
				//goMemTracer.DecCFPSize(newcfp_node.length*sizeof(ENTRY));

				delete []pnewheader_table;
				//goMemTracer.DecBuffer(num_of_newfreqitems*sizeof(HEADER_NODE));
			}
			gndepth -= npbyclosed;
			if(npbyclosed>0 && num_of_newfreqitems>0)
			{
				gotree_bufmanager.WriteCommonPrefixNodes(1, base_bitmap);
				gntree_level--;
			}
		}
		gndepth--;

		if(gndepth==gnfirst_level_depth && gpitem_sup_map[k]!=NULL)
		{
			delete []gpitem_sup_map[k];
			//goMemTracer.DecTwoLayerMap(MIN(SUP_MAXLEN_MAP_SIZE, pheader_table[k].nsupport-(int)gnmin_sup+1)*sizeof(MAP_NODE));
			gpitem_sup_map[k] = NULL;
		}
		start = clock();
		PushRight(pheader_table, k, num_of_freqitems);
		//goTimeTracer.mdHStruct_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;

		pcfp_node->cur_order = k+1;
	}

	delete []pnewfreqitems;
	//goMemTracer.DecBuffer((1+num_of_freqitems)*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
	delete []pitem_sups;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));

	return;
}

void CAFOPTMine::CFIDepthAFOPTGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos, CFP_NODE *pcfp_node)
{
	int	 i;
	AFOPT_NODE	*ptreenode, *pcurroot;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map;
	int num_of_newfreqitems, order, len, npbyclosed;
	int* pitemset, *pitem_sups;
	clock_t start;
	bool issinglepath, isSumedByLeftSib;
	HEADER_TABLE pnewheader_table;

	CFP_NODE newcfp_node;
	int bitmap, base_bitmap, num_of_prefix_node, nclass_pos;

	gntotal_call++;

	pitem_sups = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));
	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));

	isSumedByLeftSib = false;
	order = nstart_pos;
	while(order<num_of_freqitems)
	{
		pcurroot = pheader_table[order].pafopt_conddb;
		num_of_prefix_node = 0;
		gpheader_itemset[gndepth] = pheader_table[order].item;
		gndepth++;
		pitemset = &(gpheader_itemset[gndepth]);

		if(isSumedByLeftSib)
		{
			isSumedByLeftSib = false;
			nclass_pos = -1;
		}
		else 
			nclass_pos = PrunedByExistCFI(0, pheader_table[order].nsupport);

		if(nclass_pos)
			pcfp_node->pentries[order].hash_bitmap = nclass_pos;
		else if(pcurroot==NULL || pcurroot->flag==-1)
			OutputOneClosePat(pheader_table[order].nsupport);
		else
		{
			issinglepath = false;
			len = 0;
			//[begin] single path testing
			ptreenode = pcurroot;
			if(ptreenode->flag>0)
			{
				issinglepath = true;
				for(i=0;i<ptreenode->flag;i++)
				{
					if(ptreenode->pitemlist[i].support<gnmin_sup)
						break;
					gpheader_itemset[gndepth+len] = pheader_table[ptreenode->pitemlist[i].order].item;
					pitem_sups[len] = ptreenode->pitemlist[i].support;
					len++;
				}
			}
			else
			{
				ptreenode = ptreenode->pchild;
 				while(ptreenode!=NULL && ptreenode->prightsibling==NULL && ptreenode->frequency>=gnmin_sup)
				{
					gpheader_itemset[gndepth+len] = pheader_table[ptreenode->order].item;
					pitem_sups[len] = ptreenode->frequency;
					len++;
					pcurroot = ptreenode;
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
						gpheader_itemset[gndepth+len] = pheader_table[ptreenode->pitemlist[i].order].item;
						pitem_sups[len] = ptreenode->pitemlist[i].support;
						len++;
					}
				}
			}
			//[end] single path testing

			if(issinglepath)
			{
				if(len==0 || pheader_table[order].nsupport>pitem_sups[0])
					OutputOneClosePat(pheader_table[order].nsupport);
				
				if(len==1)
				{
					nclass_pos = PrunedByExistCFI(1, pitem_sups[0]);
					if(nclass_pos==0)
					{
						pcfp_node->pentries[order].child = gotree_bufmanager.GetTreeSize();
						pcfp_node->pentries[order].hash_bitmap = (1<<pitemset[0]%INT_BIT_LEN);
						gotree_bufmanager.WriteLeafNode(pitemset, 1, pitem_sups[0], 0);
						OutputOneClosePat(1, pitem_sups[0]);
					}
				}
				else if(len>1)
				{
					pcfp_node->pentries[order].child = gotree_bufmanager.GetTreeSize();
					bitmap = OutputSingleClosePath(pitem_sups, len, pheader_table[order].nsupport);
					if(bitmap==0)
						pcfp_node->pentries[order].child = 0;
					else 
						pcfp_node->pentries[order].hash_bitmap = bitmap;
				}
			}
			else 
			{
				npbyclosed = 0;

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
						gpheader_itemset[gndepth+len+npbyclosed] = pnewfreqitems[i].item;
						pitem_sups[len+npbyclosed] = pnewfreqitems[i].support;
						npbyclosed++;
						pitem_suporder_map[pnewfreqitems[i].order] = -1;
					}
					else 
						break;
				}
				num_of_newfreqitems -= npbyclosed;

				num_of_prefix_node = 0;
				if(len+npbyclosed==0 || pitem_sups[0]<pheader_table[order].nsupport)
					OutputOneClosePat(pheader_table[order].nsupport);
				if(num_of_newfreqitems+len+npbyclosed>0)
					pcfp_node->pentries[order].child = gotree_bufmanager.GetTreeSize();

				base_bitmap = 0;
				for(i=0;i<num_of_newfreqitems;i++)
					base_bitmap = base_bitmap | (1<< (pnewfreqitems[i].item%INT_BIT_LEN));
				if(len+npbyclosed>0 && num_of_newfreqitems==0)
				{
					bitmap = OutputSingleClosePath(pitem_sups, len+npbyclosed, pheader_table[order].nsupport);
					if(bitmap==0)
						pcfp_node->pentries[order].child = 0;
					else 
						pcfp_node->pentries[order].hash_bitmap = bitmap;
				}
				else if(len+npbyclosed>0 && num_of_newfreqitems>0)
				{
					bitmap = base_bitmap;
					num_of_prefix_node = OutputCommonPrefix(pitem_sups, len+npbyclosed, &bitmap, pheader_table[order].nsupport);
				}

				if(num_of_prefix_node<0)
				{
					if(bitmap==0)
						pcfp_node->pentries[order].child = 0;
					else
						pcfp_node->pentries[order].hash_bitmap = bitmap;
				}
				else if(num_of_newfreqitems>0)
				{
					bitmap = 0;
					for(i=0;i<len+npbyclosed;i++)
						bitmap = bitmap | (1<<(pitemset[i]%INT_BIT_LEN));
					pcfp_node->pentries[order].hash_bitmap = bitmap | base_bitmap;

					if(num_of_newfreqitems==1)
					{
						gndepth += len+npbyclosed;
						gpheader_itemset[gndepth] = pnewfreqitems[0].item;
						nclass_pos = PrunedByExistCFI(1, pnewfreqitems[0].support);
						if(nclass_pos==0)
						{
							OutputOneClosePat(1, pnewfreqitems[0].support);
							gotree_bufmanager.WriteLeafNode(&(pnewfreqitems[0].item), 1, pnewfreqitems[0].support, 0);
						}
						else 
						{
							if(len+npbyclosed==0)
							{
								pcfp_node->pentries[order].child = 0;
								pcfp_node->pentries[order].hash_bitmap = 0;
							}
							else 
								pcfp_node->pentries[order].hash_bitmap = bitmap;
							base_bitmap = 0;
						}
						gndepth = gndepth-len-npbyclosed;

					}
					else if(num_of_newfreqitems>1)
					{
						gndepth += len+npbyclosed;

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
						BuildNewTree(pcurroot, pnewheader_table, pitem_suporder_map);
						//goTimeTracer.mdAFOPT_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;

						newcfp_node.pos = gotree_bufmanager.GetTreeSize();
						newcfp_node.cur_order = 0;
						newcfp_node.length = num_of_newfreqitems;
						newcfp_node.pentries = new ENTRY[newcfp_node.length];
						//goMemTracer.IncCFPSize(newcfp_node.length*sizeof(ENTRY));
						for(i=0;i<newcfp_node.length;i++)
						{
							newcfp_node.pentries[i].item = pnewfreqitems[i].item;
							newcfp_node.pentries[i].support = pnewfreqitems[i].support;
							newcfp_node.pentries[i].hash_bitmap = 0;
							newcfp_node.pentries[i].child = 0;
						}
						gotree_bufmanager.InsertActiveNode(gntree_level, &newcfp_node);
						gntree_level++;
						CFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0, &newcfp_node);
						gntree_level--;
						gotree_bufmanager.WriteInternalNode(&newcfp_node);
						delete []newcfp_node.pentries;
						//goMemTracer.DecCFPSize(newcfp_node.length*sizeof(ENTRY));

						delete []pnewheader_table;
						//goMemTracer.DecBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);

						gndepth = gndepth-len-npbyclosed;
					}

					if(len+npbyclosed>0 && num_of_prefix_node>0)
					{
						gotree_bufmanager.WriteCommonPrefixNodes(num_of_prefix_node, base_bitmap);
						gntree_level -= num_of_prefix_node;
					}
				}
			}
		}
		gndepth--;

		if(gndepth==gnfirst_level_depth && gpitem_sup_map[order]!=NULL)
		{
			delete []gpitem_sup_map[order];
			//goMemTracer.DecTwoLayerMap(MIN(SUP_MAXLEN_MAP_SIZE, pheader_table[order].nsupport-(int)gnmin_sup+1)*sizeof(MAP_NODE));
			gpitem_sup_map[order] = NULL;
		}

		pcurroot = pheader_table[order].pafopt_conddb;
		if(pcurroot!=NULL && order<num_of_freqitems-1 && pheader_table[order+1].pafopt_conddb==NULL && 
		  (pcurroot->flag==0 && pcurroot->pchild->order==order+1 && pcurroot->pchild->frequency==pheader_table[order+1].nsupport ||
		  pcurroot->flag>0 && pcurroot->pitemlist[0].order==order+1 && pcurroot->pitemlist[0].support==pheader_table[order+1].nsupport))
			isSumedByLeftSib = true;

		start = clock();
		if(pcurroot!=NULL && pcurroot->flag!=-1)
			PushRight(pheader_table, order, num_of_freqitems);
		else if(pcurroot!=NULL)
		{
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
		}
		//goTimeTracer.mdAFOPT_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;

		order++;
		pcfp_node->cur_order++;
	}

	delete []pitem_sups;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));

	return;
}

int OutputSingleClosePath(int* pitem_sups, int length, int nroot_sup)
{
	int i, nprev_i, j;
	int *pitemset;
	int base_bitmap, bitmap;
	int num_of_nodes;
	int nclass_pos;

	if(length<=0)
		return 0;

	pitemset = &(gpheader_itemset[gndepth]);

	nprev_i = 0;
	i = 0;
	num_of_nodes = 0;
	nclass_pos = 0;
	while(i<length)
	{
		nprev_i = i;
		while(i<length-1 && pitem_sups[i]==pitem_sups[i+1])
		{
			i++;
		}
		
		if(pitem_sups[i]==nroot_sup)
		{
			nclass_pos = 0;
			update_suplen_map(i+1, pitem_sups[i]);
		}
		else 
			nclass_pos = PrunedByExistCFI(i+1, pitem_sups[i]);
		if(nclass_pos==0)
		{
			OutputOneClosePat(i+1, pitem_sups[i]);
			if(i<length-1)
			{
				gotree_bufmanager.InsertCommonPrefixNode(gntree_level, &(pitemset[nprev_i]), (i+1-nprev_i), pitem_sups[i], 0);
				gntree_level++;
				num_of_nodes++;
			}
			else
				break;
		}
		else
			break;
		i++;
	}

	base_bitmap = 0;
	if(nclass_pos==0)
	{
		gotree_bufmanager.WriteLeafNode(&(pitemset[nprev_i]), (i+1-nprev_i), pitem_sups[i], 0);
		for(j=nprev_i;j<i+1;j++)
			base_bitmap = base_bitmap | (1<<pitemset[j]%INT_BIT_LEN);
	}

	if(num_of_nodes>0)
	{
		bitmap = gotree_bufmanager.WriteCommonPrefixNodes(num_of_nodes, base_bitmap);
		gntree_level -= num_of_nodes;
	}
	else 
		bitmap = base_bitmap;

	return bitmap;
}

int OutputCommonPrefix(int* pitem_sups, int length, int *bitmap, int nroot_sup)
{
	int i, nprev_i, j;
	int *pitemset;
	int base_bitmap;
	int num_of_nodes;
	int nclass_pos;

	if(length<=0)
		return 0;

	num_of_nodes = 0;

	pitemset = &(gpheader_itemset[gndepth]);

	i = 0;
	while(i<length)
	{
		nprev_i = i;
		while(i<length-1 && pitem_sups[i]==pitem_sups[i+1])
		{
			i++;
		}

		if(pitem_sups[i]==nroot_sup)
		{
			nclass_pos = 0;
			update_suplen_map(i+1, pitem_sups[i]);
		}
		else 
			nclass_pos = PrunedByExistCFI(i+1, pitem_sups[i]);
		if(nclass_pos==0)
		{
			OutputOneClosePat(i+1, pitem_sups[i]);
			base_bitmap = (*bitmap);
			for(j=i+1;j<length;j++)
				base_bitmap = base_bitmap | (1<<pitemset[j]%INT_BIT_LEN);
			gotree_bufmanager.InsertCommonPrefixNode(gntree_level, &(pitemset[nprev_i]), (i+1-nprev_i), pitem_sups[i], base_bitmap);
			gntree_level++;
			num_of_nodes++;
		}
		else
		{
			if(num_of_nodes>0)
			{
				(*bitmap) = gotree_bufmanager.WriteCommonPrefixNodes(num_of_nodes, 0);
				gntree_level -= num_of_nodes;
			}
			else
				(*bitmap) = 0;

			num_of_nodes = -1;
			break; 
		}

		i++;
	}

	return num_of_nodes;
}

void update_suplen_map(int nsuffix_len, int nsupport)
{
	int i, npat_len, order, nmap_size, offset;
	int nmax_item_id, nmin_item_id, bitmap;
	clock_t start;

	start = clock();
	npat_len = gndepth+nsuffix_len;

	nmax_item_id = gpheader_itemset[gnfirst_level_depth];
	nmin_item_id = gpheader_itemset[gnfirst_level_depth];
	bitmap = 0;
	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		if(gpheader_itemset[i]>nmax_item_id)
			nmax_item_id = gpheader_itemset[i];
		if(gpheader_itemset[i]<nmin_item_id)
			nmin_item_id = gpheader_itemset[i];
		bitmap = bitmap | (1<< (gpheader_itemset[i]%INT_BIT_LEN));
	}

	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		order = gpitem_order_map[gpheader_itemset[i]];
		if(order<0)
			printf("Error[update_suplen_map]: cannot find item %d in item-order-map\n", gpheader_itemset[i]);
		else 
		{
			nmap_size = MIN(SUP_MAXLEN_MAP_SIZE, gpfreqitems[order].support-(int)gnmin_sup+1);
			offset = (nsupport-(int)gnmin_sup)%nmap_size;
			if(gpitem_sup_map[order]==NULL)
			{
				gpitem_sup_map[order] = new MAP_NODE[nmap_size];
				//goMemTracer.IncTwoLayerMap(nmap_size*sizeof(MAP_NODE));
				memset(gpitem_sup_map[order], 0, sizeof(MAP_NODE)*nmap_size);
				gpitem_sup_map[order][offset].nmax_len = npat_len;
				gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				gpitem_sup_map[order][offset].bitmap = bitmap;
			}
			else 
			{
				if(gpitem_sup_map[order][offset].nmax_len<npat_len)
					gpitem_sup_map[order][offset].nmax_len = npat_len;
				if(gpitem_sup_map[order][offset].nmax_item_id<nmax_item_id)
					gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				if(gpitem_sup_map[order][offset].nmin_item_id>nmin_item_id)
					gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				gpitem_sup_map[order][offset].bitmap = gpitem_sup_map[order][offset].bitmap | bitmap;
			}
		}
	}

	//goTimeTracer.mdpruning_time += (double)(clock()-start)/CLOCKS_PER_SEC;
}

int inline PrunedByExistCFI(int nsuffix_len, int nsupport)
{
	unsigned int found;
	int npat_len, i, order, nmap_size, offset, nmax_item_id, nmin_item_id, bitmap;
	clock_t start;

	start = clock();
	gnprune_times++;
	npat_len = gndepth+nsuffix_len;
	found = 1;

	nmin_item_id = gpheader_itemset[gnfirst_level_depth];
	nmax_item_id = gpheader_itemset[gnfirst_level_depth];
	bitmap = 0;
	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		if(gpheader_itemset[i]>nmax_item_id)
			nmax_item_id = gpheader_itemset[i];
		if(gpheader_itemset[i]<nmin_item_id)
			nmin_item_id = gpheader_itemset[i];
		bitmap = bitmap | (1<< (gpheader_itemset[i]%INT_BIT_LEN));
	}

	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		order = gpitem_order_map[gpheader_itemset[i]];
		if(order<0)
			printf("Error[PrunedByExistCFI]: cannot find item %d in item-order-map\n", gpheader_itemset[i]);
		else 
		{
			nmap_size = MIN(SUP_MAXLEN_MAP_SIZE, gpfreqitems[order].support-(int)gnmin_sup+1);
			offset = (nsupport-(int)gnmin_sup)%nmap_size;
			if(gpitem_sup_map[order]==NULL)
			{
				found = 0;
				gpitem_sup_map[order] = new MAP_NODE[nmap_size];
				//goMemTracer.IncTwoLayerMap(nmap_size*sizeof(MAP_NODE));
				memset(gpitem_sup_map[order], 0, sizeof(MAP_NODE)*nmap_size);
				gpitem_sup_map[order][offset].nmax_len = npat_len;
				gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				gpitem_sup_map[order][offset].bitmap = bitmap;
			}
			else 
			{
				if(gpitem_sup_map[order][offset].nmax_len<npat_len)
				{
					found = 0;
					gpitem_sup_map[order][offset].nmax_len = npat_len;
				}
				else if(gpitem_sup_map[order][offset].nmax_len==npat_len)
					found = 0;

				if(gpitem_sup_map[order][offset].nmax_item_id<nmax_item_id)
				{
					found = 0;
					gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				}
				if(gpitem_sup_map[order][offset].nmin_item_id>nmin_item_id)
				{
					found = 0;
					gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				}
				if(found)
				{
					if((gpitem_sup_map[order][offset].bitmap & bitmap) != bitmap)
						found = 0;
				}
				gpitem_sup_map[order][offset].bitmap = gpitem_sup_map[order][offset].bitmap | bitmap;
			}
		}
	}

	if(found==0)
	{
		gnmap_pruned_times++;
		return 0;
	}

	found = 0;
	gotree_bufmanager.mpat_len = gnfirst_level_depth;

	if(npat_len-gnfirst_level_depth==0)
	{
		printf("Error with pattern length\n");
	}
	else if(npat_len>gnfirst_level_depth)
		found = gotree_bufmanager.SearchActiveStack(gnfirst_level, &(gpheader_itemset[gnfirst_level_depth]), npat_len-gnfirst_level_depth, nsupport, gndb_size, 0);
	else
	{
		printf("Error with level\n");
	}		

	if(found)
	{
		if(npat_len>gotree_bufmanager.mpat_len)
			printf("Error: pattern length are not consistent %d %d\n", npat_len, gotree_bufmanager.mpat_len);
	}
	else if(gotree_bufmanager.mpat_len!=gnfirst_level_depth)
		printf("Error with the length of the pattern\n");

	//goTimeTracer.mdpruning_time += (double)(clock()-start)/CLOCKS_PER_SEC;

	return found;

}



