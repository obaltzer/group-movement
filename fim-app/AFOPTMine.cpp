#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "parameters.h"
#include "AFOPTMine.h"
//#include "MemTracer.h"

//-------------------------------------------------------------------------------
//Count frequent items from an AFOPT-tree by depth-first traversal of the tree.
//It is implemented using stack.
//-------------------------------------------------------------------------------
void CAFOPTMine::CountFreqItems(AFOPT_NODE* proot, int *pitem_sup_map)
{
	AFOPT_NODE *ptreenode, *pnextchild;

	int source_top;
	int i;
	ARRAY_NODE* pitem_list;
	
	ptreenode = proot;
	source_top = 0;
	mpsource_stack[source_top].pnext_child = ptreenode->pchild;
	mpsource_stack[source_top].pnode = ptreenode;
	source_top++;

	while(source_top>0)
	{
		source_top--;
		pnextchild = mpsource_stack[source_top].pnext_child;
		if(pnextchild!=NULL)
		{
			pitem_sup_map[pnextchild->order] += pnextchild->frequency;
			mpsource_stack[source_top].pnext_child = pnextchild->prightsibling;
			source_top++;
			if(pnextchild->flag==0)
			{
				mpsource_stack[source_top].pnext_child = pnextchild->pchild;
				mpsource_stack[source_top].pnode = pnextchild;
				source_top++;
			}
			else if(pnextchild->flag>0) 
			{
				pitem_list = pnextchild->pitemlist;
				for(i=0;i<pnextchild->flag;i++)
				{
					pitem_sup_map[pitem_list[i].order] += pitem_list[i].support;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------------
//Build new conditional databases from an AFOPT-tree by depth-first traversal of the tree, 
//and at the same time bucket counting the most frequent items. Implemented using stack;
//----------------------------------------------------------------------------------------
void CAFOPTMine::BuildNewTree(AFOPT_NODE *proot, HEADER_TABLE pnewheader_table, int *pitem_order_map, int num_of_newfreqitems, int *Buckets)
{
	AFOPT_NODE *ptreenode;
	int i, order;
	int source_top, ntrans_len, ntemp_len, ntemp_k;
	unsigned int k;
	int num_of_countitems;
	
	num_of_countitems = num_of_newfreqitems-MAX_BUCKET_SIZE;

	source_top = 0;
	mpsource_stack[source_top].pnext_child = proot->pchild;
	mpsource_stack[source_top].frequency = proot->frequency;
	mpsource_stack[source_top].flag = 0;
	source_top++;
	ntrans_len = 0;

	k = 0;
	while(source_top>0)
	{
		source_top--;
		ptreenode = mpsource_stack[source_top].pnext_child;
		if(ptreenode!=NULL)
		{
			mpsource_stack[source_top].frequency -= ptreenode->frequency;
			mpsource_stack[source_top].pnext_child = ptreenode->prightsibling;
			source_top++;

			if(ptreenode->flag==0)
			{
				mpsource_stack[source_top].pnext_child = ptreenode->pchild;
				mpsource_stack[source_top].frequency = ptreenode->frequency;
				order = pitem_order_map[ptreenode->order];
				if(order>=0)
				{
					mpsource_stack[source_top].flag = 1;
					mpbranch[ntrans_len] = order;
					ntrans_len++;
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
				}
				else 
					mpsource_stack[source_top].flag = 0;
				source_top++;
			}
			else if(ptreenode->flag==-1) //the node has no children
			{
				order = pitem_order_map[ptreenode->order];
				if(order>=0)
				{
					mpbranch[ntrans_len] = order;
					ntrans_len++;
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
				}

				if(k>0)
					Buckets[k] += ptreenode->frequency;

				if(ntrans_len>1)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc); 
					if(mptransaction[0]<num_of_countitems)
					{
						InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, ptreenode->frequency);
					}
				}

				if(order>=0)
				{
					ntrans_len--;
					if(order>=num_of_countitems)
						k -= (1<<(order-num_of_countitems));
				}

				//pop out nodes whose children are all visited
				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
					{
						ntrans_len--;
						if(mpbranch[ntrans_len]>=num_of_countitems)
							k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
					}
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
			else //the node points to a single branch
			{
				ntemp_len = ntrans_len;
				ntemp_k = k;

				order = pitem_order_map[ptreenode->order];
				if(order>=0)
				{
					mpbranch[ntrans_len] = order;
					ntrans_len++;
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
				}
				if(ntrans_len>0)
					mpfrequencies[ntrans_len-1] = ptreenode->frequency;

				for(i=0;i<ptreenode->flag;i++)
				{
					order = pitem_order_map[ptreenode->pitemlist[i].order];
					if(order>=0)
					{
						mpbranch[ntrans_len] = order;
						mpfrequencies[ntrans_len] = ptreenode->pitemlist[i].support;
						ntrans_len++;
						if(order>=num_of_countitems)
							k += (1<<(order-num_of_countitems));
					}
				}

				if(ntrans_len>0)
				{
					mpfrequencies[ntrans_len] = 0;
					while(ntrans_len>0 && ntrans_len>=ntemp_len)
					{
						memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
						qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
						if(ntrans_len>1 && mptransaction[0]<num_of_countitems)
						{
							InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len]);
						}
						if(k>0)
							Buckets[k] += mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len];

						if(mpfrequencies[ntrans_len-1]==ptreenode->frequency)
						{
							ntrans_len = ntemp_len;
							k = ntemp_k;
							break;
						}
						ntrans_len--;
						if(mpbranch[ntrans_len]>=num_of_countitems)
							k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
						while(ntrans_len>ntemp_len && mpfrequencies[ntrans_len-1]==mpfrequencies[ntrans_len])
						{
							ntrans_len--;
							if(mpbranch[ntrans_len]>=num_of_countitems)
								k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
						}
					}
				}

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
					{
						ntrans_len--;
						if(mpbranch[ntrans_len]>=num_of_countitems)
							k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
					}
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
		}
		else
		{
			if(mpsource_stack[source_top].frequency>0)
			{
				if(k>0)
					Buckets[k] += mpsource_stack[source_top].frequency;
				if(ntrans_len>1)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
					if(mptransaction[0]<num_of_countitems)
					{
						InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpsource_stack[source_top].frequency);
					}
				}
				mpsource_stack[source_top].frequency = 0;
			}

			while(source_top>=0 && mpsource_stack[source_top].frequency==0)
			{
				if(mpsource_stack[source_top].pnext_child!=NULL)
				{
					printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
					exit(-1);
				}
				if(mpsource_stack[source_top].flag)
				{
					ntrans_len--;
					if(mpbranch[ntrans_len]>=num_of_countitems)
						k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
				}
				source_top--;
			}
			if(source_top>=0)
				source_top++;
		}
	}
}

//-----------------------------------------------------------------
//build new conditional databases from an AFOPT-tree;
//-----------------------------------------------------------------
void CAFOPTMine::BuildNewTree(AFOPT_NODE *proot, HEADER_TABLE pnewheader_table, int *pitem_order_map)
{
	AFOPT_NODE *ptreenode;
	int i;
	int source_top, ntrans_len, ntemp_len;
	
	source_top = 0;
	mpsource_stack[source_top].pnext_child = proot->pchild;
	mpsource_stack[source_top].frequency = proot->frequency;
	mpsource_stack[source_top].flag = 0;
	source_top++;
	ntrans_len = 0;

	while(source_top>0)
	{
		source_top--;
		ptreenode = mpsource_stack[source_top].pnext_child;
		if(ptreenode!=NULL)
		{
			mpsource_stack[source_top].frequency -= ptreenode->frequency;
			mpsource_stack[source_top].pnext_child = ptreenode->prightsibling;
			source_top++;

			if(ptreenode->flag==0)
			{
				mpsource_stack[source_top].pnext_child = ptreenode->pchild;
				mpsource_stack[source_top].frequency = ptreenode->frequency;
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpsource_stack[source_top].flag = 1;
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				else 
					mpsource_stack[source_top].flag = 0;
				source_top++;
			}
			else if(ptreenode->flag==-1)
			{
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}

				if(ntrans_len>1)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc); 
					InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, ptreenode->frequency);
				}

				if(pitem_order_map[ptreenode->order]>=0)
					ntrans_len--;

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
			else
			{
				ntemp_len = ntrans_len;

				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				if(ntrans_len>0)
					mpfrequencies[ntrans_len-1] = ptreenode->frequency;

				for(i=0;i<ptreenode->flag;i++)
				{
					if(pitem_order_map[ptreenode->pitemlist[i].order]>=0)
					{
						mpbranch[ntrans_len] = pitem_order_map[ptreenode->pitemlist[i].order];
						mpfrequencies[ntrans_len] = ptreenode->pitemlist[i].support;
						ntrans_len++;
					}
				}

				if(ntrans_len>1)
				{
					mpfrequencies[ntrans_len] = 0;
					while(ntrans_len>0 && ntrans_len>=ntemp_len)
					{
						if(ntrans_len>1)
						{
							memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
							qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
							InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len]);
						}
						if(mpfrequencies[ntrans_len-1]==ptreenode->frequency)
						{
							ntrans_len = ntemp_len;
							break;
						}
						ntrans_len--;
						while(ntrans_len>ntemp_len && mpfrequencies[ntrans_len-1]==mpfrequencies[ntrans_len])
							ntrans_len--;
					}
				}
				else 
					ntrans_len = ntemp_len;

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
		}
		else
		{
			if(mpsource_stack[source_top].frequency>0)
			{
				if(ntrans_len>0)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
					InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpsource_stack[source_top].frequency);
				}
				mpsource_stack[source_top].frequency = 0;
			}

			while(source_top>=0 && mpsource_stack[source_top].frequency==0)
			{
				if(mpsource_stack[source_top].pnext_child!=NULL)
				{
					printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
					exit(-1);
				}
				if(mpsource_stack[source_top].flag)
					ntrans_len--;
				source_top--;
			}
			if(source_top>=0)
				source_top++;
		}
	}
}


//---------------------------------------------------------
//bucket count all the frequent itemsets in an AFOPT-tree; 
//---------------------------------------------------------
void CAFOPTMine::BucketCountAFOPT(AFOPT_NODE *proot, int* pitem_order_map, int *Buckets)
{
	int i;
	unsigned int k;
	AFOPT_NODE *ptreenode;

	int source_top, ntrans_len, ntemp_len;

	source_top = 0;
	mpsource_stack[source_top].pnext_child = proot->pchild;
	mpsource_stack[source_top].frequency = proot->frequency;
	mpsource_stack[source_top].flag = 0;
	source_top++;
	ntrans_len = 0;

	while(source_top>0)
	{
		source_top--;
		ptreenode = mpsource_stack[source_top].pnext_child;
		if(ptreenode!=NULL)
		{
			mpsource_stack[source_top].frequency -= ptreenode->frequency;
			mpsource_stack[source_top].pnext_child = ptreenode->prightsibling;
			source_top++;

			if(ptreenode->flag==0)//the node has child nodes
			{
				mpsource_stack[source_top].pnext_child = ptreenode->pchild;
				mpsource_stack[source_top].frequency = ptreenode->frequency;
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpsource_stack[source_top].flag = 1;
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				else 
					mpsource_stack[source_top].flag = 0;
				source_top++;
			}
			else if(ptreenode->flag==-1) //the node has no child nodes
			{
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}

				if(ntrans_len>0)
				{
					k = 0;
					for(i=0;i<ntrans_len;i++)
						k += (1<<mpbranch[i]);
					Buckets[k] += ptreenode->frequency;
				}

				if(pitem_order_map[ptreenode->order]>=0)
					ntrans_len--;

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BucketCountAFOPT]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
			else // the node points to a single branch
			{
				ntemp_len = ntrans_len;

				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				if(ntrans_len>0)
					mpfrequencies[ntrans_len-1] = ptreenode->frequency;

				for(i=0;i<ptreenode->flag;i++)
				{
					if(pitem_order_map[ptreenode->pitemlist[i].order]>=0)
					{
						mpbranch[ntrans_len] = pitem_order_map[ptreenode->pitemlist[i].order];
						mpfrequencies[ntrans_len] = ptreenode->pitemlist[i].support;
						ntrans_len++;
					}
				}

				if(ntrans_len>0)
				{
					k = 0;
					for(i=0;i<ntrans_len;i++)
						k += (1<<mpbranch[i]);
					mpfrequencies[ntrans_len] = 0;
					while(ntrans_len>0 && ntrans_len>=ntemp_len)
					{
						Buckets[k] += (mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len]);
						if(mpfrequencies[ntrans_len-1]==ptreenode->frequency)
						{
							ntrans_len = ntemp_len;
							break;
						}
						ntrans_len--;
						k -= (1<<mpbranch[ntrans_len]);						
						while(ntrans_len>ntemp_len && mpfrequencies[ntrans_len-1]==mpfrequencies[ntrans_len])
						{
							ntrans_len--;
							k -= (1<<mpbranch[ntrans_len]);
						}
					}
				}

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BucketCountAFOPT]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
		}
		else
		{
			if(mpsource_stack[source_top].frequency>0)
			{
				if(ntrans_len>0)
				{
					k = 0;
					for(i=0;i<ntrans_len;i++)
						k += (1 << mpbranch[i]);
					Buckets[k] += mpsource_stack[source_top].frequency;
				}
				mpsource_stack[source_top].frequency = 0;
			}

			while(source_top>=0 && mpsource_stack[source_top].frequency==0)
			{
				if(mpsource_stack[source_top].pnext_child!=NULL)
				{
					printf("Error[BucketCountAFOPT]: the frequency of the item is 0 while it still has some children\n");
					exit(-1);
				}
				if(mpsource_stack[source_top].flag)
					ntrans_len--;
				source_top--;
			}
			if(source_top>=0)
				source_top++;
		}
	}

}

//-------------------------------------------------------------------------------
//Push right an AFOPT-tree. The children of the root are merged with corresponding 
//siblings of the root
//-------------------------------------------------------------------------------
void CAFOPTMine::PushRight(HEADER_TABLE pheader_table, int nitem_pos, int nmax_push_pos)
{
	ARRAY_NODE	*pitem_list;
	AFOPT_NODE *psource_root, *psource_child, *pnext_source_child;

	psource_root = pheader_table[nitem_pos].pafopt_conddb;
	if(psource_root->flag==-1)
	{
		printf("Error with flag\n");
	}
	else if(psource_root->flag>0)
	{
		pitem_list = psource_root->pitemlist;
		if(psource_root->flag>1 && pitem_list[0].order<nmax_push_pos)
		{
			InsertTransaction(pheader_table, pitem_list[0].order, &(pitem_list[1]), psource_root->flag-1);
		}
		
		delete []pitem_list;
		//goMemTracer.DelSingleBranch(psource_root->flag);
	}
	else 
	{
		psource_child = psource_root->pchild;
		while(psource_child!=NULL)
		{
			pnext_source_child = psource_child->prightsibling;
			if(psource_child->order<nmax_push_pos)
			{
				if(pheader_table[psource_child->order].pafopt_conddb==NULL)
				{
					psource_child->prightsibling = NULL;
					pheader_table[psource_child->order].pafopt_conddb = psource_child;
				}
				else if(psource_child->flag>=0)
					MergeTwoTree(psource_child, pheader_table[psource_child->order].pafopt_conddb);
				else 
				{
					delete psource_child;
					//goMemTracer.DelOneAFOPTNode();
				}
			}
			else	
				DestroyTree(psource_child);
			psource_child = pnext_source_child;
		}
	}
	
	delete psource_root;
	//goMemTracer.DelOneAFOPTNode();
	pheader_table[nitem_pos].pafopt_conddb = NULL;

}

//-------------------------------------------------------------------------------
//Merge two trees togather
//-------------------------------------------------------------------------------
void CAFOPTMine::MergeTwoTree(AFOPT_NODE *psource_root, AFOPT_NODE *pdest_root)
{
	ARRAY_NODE	*pitem_list;
	int nlen;
	AFOPT_NODE *psource_child, *pnext_source_child, *pdest_child, *pprev_dest_child;

	pdest_root->frequency += psource_root->frequency;

	pprev_dest_child = NULL;
	pnext_source_child = NULL;
	if(psource_root->flag==-1)//if the source root has no child, output error message;
	{
		printf("Error with flag\n");
	}
	else if(psource_root->flag>0) //the source root points to a single branch, insert this single branch into destination tree
	{
		InsertTransaction(pdest_root, psource_root->pitemlist, psource_root->flag);
		delete []psource_root->pitemlist;
		//goMemTracer.DelSingleBranch(psource_root->flag);
	}
	else if(pdest_root->flag==-1) //the source root has an AFOPT-node as a child and the destination tree has no child, adjust child pointer
	{
		pdest_root->pchild = psource_root->pchild;
		pdest_root->flag = 0;
	}
	else if(pdest_root->flag>0) //the destination root has a single branch, first exchange child pointer, then insert the single branch
	{
		pitem_list = pdest_root->pitemlist;
		nlen = pdest_root->flag;
		pdest_root->pchild = psource_root->pchild;
		pdest_root->flag = 0;
		InsertTransaction(pdest_root, pitem_list, nlen);
		delete []pitem_list;
		//goMemTracer.DelSingleBranch(nlen);
	}
	else //both source root and destination root points to AFOPT-node, recursively call itself
	{
		psource_child = psource_root->pchild;
		pdest_child = pdest_root->pchild;
		while(psource_child!=NULL)
		{
			while(pdest_child!=NULL && pdest_child->order<psource_child->order)
			{
				pprev_dest_child = pdest_child;
				pdest_child = pdest_child->prightsibling;
			}

			if(pdest_child==NULL)
			{
				pprev_dest_child->prightsibling = psource_child;
				break;
			}
			else if(psource_child->order==pdest_child->order)
			{
				pnext_source_child = psource_child->prightsibling;
				if(psource_child->flag>=0)
					MergeTwoTree(psource_child, pdest_child);
				else 
				{
					pdest_child->frequency += psource_child->frequency;
					delete psource_child;
					//goMemTracer.DelOneAFOPTNode();
				}
				psource_child = pnext_source_child;
				pprev_dest_child = pdest_child;
				pdest_child = pdest_child->prightsibling;
			}
			else 
			{
				pnext_source_child = psource_child->prightsibling;
				psource_child->prightsibling = pdest_child;
				if(pprev_dest_child==NULL)
					pdest_root->pchild = psource_child;
				else 
					pprev_dest_child->prightsibling = psource_child;
				pprev_dest_child = psource_child;
				psource_child = pnext_source_child;
			}
		}
	}
	
	delete psource_root;
	//goMemTracer.DelOneAFOPTNode();

}

//-------------------------------------------------------------------------------
//insert a transaction into an AFOPT-tree
//-------------------------------------------------------------------------------
void CAFOPTMine::InsertTransaction(HEADER_TABLE pheader_table, int nitem_pos, int* ptransaction, int length, int frequency)
{
	int i, j, order, orig_frequency, list_len , t;
	AFOPT_NODE *pnewnode, *pcurnode, *pprevnode, *ptnode;
	ARRAY_NODE *pitem_list, *pnewitem_list;

	if(pheader_table[nitem_pos].pafopt_conddb==NULL)
	{
		pnewnode = new AFOPT_NODE;
		//goMemTracer.AddOneAFOPTNode();
		pnewnode->order = nitem_pos;
		pnewnode->frequency = 0;
		pnewnode->flag = -1;
		pnewnode->pchild = NULL;
		pnewnode->prightsibling = NULL;
		pheader_table[nitem_pos].pafopt_conddb = pnewnode;
	}

	pprevnode = NULL;
	pcurnode = pheader_table[nitem_pos].pafopt_conddb;
	orig_frequency = pcurnode->frequency;
	pcurnode->frequency += frequency;
	
	for(i=0;i<length;i++)
	{
		if(pcurnode->flag == -1)
		{
			pnewitem_list = new ARRAY_NODE[length-i];
			//goMemTracer.AddSingleBranch(length-i);
			for(j=0;j<length-i;j++)
			{
				pnewitem_list[j].order = ptransaction[j+i];
				pnewitem_list[j].support = frequency;
			}
			pcurnode->pitemlist = pnewitem_list;
			pcurnode->flag = length-i;
			break;
		}
		else if(pcurnode->flag == 0)
		{
			order = ptransaction[i];
			if(pcurnode->pchild->order > order)
			{
				//goMemTracer.AddOneAFOPTNode();
				pnewnode = new AFOPT_NODE;
				pnewnode->frequency = frequency;
				pnewnode->prightsibling = pcurnode->pchild;
				pnewnode->order = order;
				pcurnode->pchild = pnewnode;
				if(i==length-1)
				{
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
				}
				else
				{
					pnewitem_list = new ARRAY_NODE[length-i-1];
					//goMemTracer.AddSingleBranch(length-i-1);
					for(j=0;j<length-i-1;j++)
					{
						pnewitem_list[j].order = ptransaction[j+i+1];
						pnewitem_list[j].support = frequency;
					}
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = length-i-1;
					break;
				}
			}
			else 
			{
				pcurnode = pcurnode->pchild;
				while(pcurnode!=NULL && pcurnode->order<order)
				{
					pprevnode = pcurnode;
					pcurnode = pcurnode->prightsibling;
				}

				if(pcurnode!=NULL && pcurnode->order==order)
				{
					pcurnode->frequency += frequency;
				}
				else 
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = frequency;
					pnewnode->prightsibling = pcurnode;
					pnewnode->order = order;
					pprevnode->prightsibling = pnewnode;
					if(i==length-1)
					{
						pnewnode->pchild = NULL;
						pnewnode->flag = -1;
					}
					else
					{
						pnewitem_list = new ARRAY_NODE[length-i-1];
						//goMemTracer.AddSingleBranch(length-i-1);
						for(j=0;j<length-i-1;j++)
						{
							pnewitem_list[j].order = ptransaction[j+i+1];
							pnewitem_list[j].support = frequency;
						}
						pnewnode->pitemlist = pnewitem_list;
						pnewnode->flag = length-i-1;
						break;
					}				
				}
			}
		}
		else 
		{
			pitem_list = pcurnode->pitemlist;
			list_len = pcurnode->flag;

			j = 0;
			while(j<list_len && (i+j< length) && pitem_list[j].order == ptransaction[i+j])
			{
				pitem_list[j].support += frequency;
				j++;
			}

			if(i+j==length)
			{
				break;
			}
			else if(j==list_len)
			{
				pnewitem_list = new ARRAY_NODE[length-i];
				//goMemTracer.AddSingleBranch(length-i);
				for(t=0;t<j;t++)
					pnewitem_list[t] = pitem_list[t];
				for(t=j;t<length-i;t++)
				{
					pnewitem_list[t].order = ptransaction[i+t];
					pnewitem_list[t].support = frequency;
				}
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);
				pcurnode->pitemlist = pnewitem_list;
				pcurnode->flag = length-i;
				break;
			}
			else 
			{
				for(t=0;t<j;t++)
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = pitem_list[t].support;
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
					pnewnode->prightsibling = NULL;
					pnewnode->order = pitem_list[t].order;
					pcurnode->pchild = pnewnode;
					pcurnode->flag = 0;
					pcurnode = pnewnode;
				}

				pnewnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				pnewnode->frequency = pitem_list[j].support;
				pnewnode->order = pitem_list[j].order;
				pnewnode->prightsibling = NULL;
				if(j==list_len-1)
				{
					pnewnode->flag = -1;
					pnewnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[list_len-1-j];
					//goMemTracer.AddSingleBranch(list_len-j-1);
					for(t=0;t<list_len-j-1;t++)
						pnewitem_list[t] = pitem_list[t+j+1];
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = list_len-1-j;
				}

				ptnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				ptnode->frequency = frequency;
				ptnode->order = ptransaction[i+j];
				ptnode->prightsibling = NULL;
				if(i+j == length-1)
				{
					ptnode->flag = -1;
					ptnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[length-1-j-i];
					//goMemTracer.AddSingleBranch(length-1-j-i);
					for(t=0;t<length-1-j-i;t++)
					{
						pnewitem_list[t].order = ptransaction[t+j+1+i];
						pnewitem_list[t].support = frequency;
					}
					ptnode->pitemlist = pnewitem_list;
					ptnode->flag = length-1-i-j;
				}

				if(pnewnode->order<ptnode->order)
				{
					pcurnode->pchild = pnewnode;
					pnewnode->prightsibling = ptnode;
				}
				else 
				{
					pcurnode->pchild = ptnode;
					ptnode->prightsibling = pnewnode;
				}
				pcurnode->flag = 0;
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);

				break;
			}
		}
	}
}

//-------------------------------------------------------------------------------
//insert a single branch in an AFOPT-tree
//-------------------------------------------------------------------------------
void CAFOPTMine::InsertTransaction(HEADER_TABLE pheader_table, int nitem_pos, ARRAY_NODE* ptransaction, int length)
{
	int i, j, order, orig_frequency, list_len , t;
	AFOPT_NODE *pnewnode, *pcurnode, *pprevnode, *ptnode;
	ARRAY_NODE *pitem_list, *pnewitem_list;

	if(pheader_table[nitem_pos].pafopt_conddb==NULL)
	{
		pnewnode = new AFOPT_NODE;
		//goMemTracer.AddOneAFOPTNode();
		pnewnode->order = nitem_pos;
		pnewnode->frequency = 0;
		pnewnode->flag = -1;
		pnewnode->pchild = NULL;
		pnewnode->prightsibling = NULL;
		pheader_table[nitem_pos].pafopt_conddb = pnewnode;
	}

	pprevnode = NULL;
	pcurnode = pheader_table[nitem_pos].pafopt_conddb;
	pcurnode->frequency += ptransaction[0].support;
	
	for(i=0;i<length;i++)
	{
		if(pcurnode->flag == -1)
		{
			pnewitem_list = new ARRAY_NODE[length-i];
			//goMemTracer.AddSingleBranch(length-i);
			for(j=0;j<length-i;j++)
			{
				pnewitem_list[j].order = ptransaction[j+i].order;
				pnewitem_list[j].support = ptransaction[j+i].support;
			}
			pcurnode->pitemlist = pnewitem_list;
			pcurnode->flag = length-i;
			break;
		}
		else if(pcurnode->flag == 0)
		{
			order = ptransaction[i].order;
			if(pcurnode->pchild->order > order)
			{
				//goMemTracer.AddOneAFOPTNode();
				pnewnode = new AFOPT_NODE;
				pnewnode->frequency = ptransaction[i].support;
				pnewnode->prightsibling = pcurnode->pchild;
				pnewnode->order = order;
				pcurnode->pchild = pnewnode;
				if(i==length-1)
				{
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
				}
				else
				{
					pnewitem_list = new ARRAY_NODE[length-i-1];
					//goMemTracer.AddSingleBranch(length-i-1);
					for(j=0;j<length-i-1;j++)
					{
						pnewitem_list[j].order = ptransaction[j+i+1].order;
						pnewitem_list[j].support = ptransaction[j+i+1].support;
					}
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = length-i-1;
					break;
				}
			}
			else 
			{
				pcurnode = pcurnode->pchild;
				while(pcurnode!=NULL && pcurnode->order<order)
				{
					pprevnode = pcurnode;
					pcurnode = pcurnode->prightsibling;
				}

				if(pcurnode!=NULL && pcurnode->order==order)
				{
					pcurnode->frequency += ptransaction[i].support;
				}
				else 
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = ptransaction[i].support;
					pnewnode->prightsibling = pcurnode;
					pnewnode->order = order;
					pprevnode->prightsibling = pnewnode;
					if(i==length-1)
					{
						pnewnode->flag = -1;
						pnewnode->pchild = NULL;
					}
					else
					{
						pnewitem_list = new ARRAY_NODE[length-i-1];
						//goMemTracer.AddSingleBranch(length-i-1);
						for(j=0;j<length-i-1;j++)
						{
							pnewitem_list[j].order = ptransaction[j+i+1].order;
							pnewitem_list[j].support = ptransaction[j+i+1].support;
						}
						pnewnode->pitemlist = pnewitem_list;
						pnewnode->flag = length-i-1;
						break;
					}				
				}
			}
		}
		else 
		{
			pitem_list = pcurnode->pitemlist;
			list_len = pcurnode->flag;
			orig_frequency = pcurnode->frequency;

			j = 0;
			while(j<list_len && (i+j< length) && pitem_list[j].order == ptransaction[i+j].order)
			{
				pitem_list[j].support += ptransaction[i+j].support;
				j++;
			}

			if(i+j==length)
			{
				break;
			}
			else if(j==list_len)
			{
				pnewitem_list = new ARRAY_NODE[length-i];
				//goMemTracer.AddSingleBranch(length-i);
				for(t=0;t<j;t++)
					pnewitem_list[t] = pitem_list[t];
				for(t=j;t<length-i;t++)
				{
					pnewitem_list[t] = ptransaction[i+t];
				}
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);
				pcurnode->pitemlist = pnewitem_list;
				pcurnode->flag = length-i;
				break;
			}
			else 
			{
				for(t=0;t<j;t++)
				{
					pnewnode = new AFOPT_NODE;
					//goMemTracer.AddOneAFOPTNode();
					pnewnode->frequency = pitem_list[t].support;
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
					pnewnode->prightsibling = NULL;
					pnewnode->order = pitem_list[t].order;
					pcurnode->pchild = pnewnode;
					pcurnode->flag = 0;
					pcurnode = pnewnode;
				}

				pnewnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				pnewnode->frequency = pitem_list[j].support;
				pnewnode->order = pitem_list[j].order;
				pnewnode->prightsibling = NULL;
				if(j==list_len-1)
				{
					pnewnode->flag = -1;
					pnewnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[list_len-1-j];
					//goMemTracer.AddSingleBranch(list_len-j-1);
					for(t=0;t<list_len-j-1;t++)
						pnewitem_list[t] = pitem_list[t+j+1];
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = list_len-1-j;
				}

				ptnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				ptnode->frequency = ptransaction[i+j].support;
				ptnode->order = ptransaction[i+j].order;
				ptnode->prightsibling = NULL;
				if(i+j == length-1)
				{
					ptnode->flag = -1;
					ptnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[length-1-j-i];
					//goMemTracer.AddSingleBranch(length-1-j-i);
					for(t=0;t<length-1-j-i;t++)
					{
						pnewitem_list[t].order = ptransaction[t+j+1+i].order;
						pnewitem_list[t].support = ptransaction[t+j+1+i].support;
					}
					ptnode->pitemlist = pnewitem_list;
					ptnode->flag = length-1-i-j;
				}

				if(pnewnode->order<ptnode->order)
				{
					pcurnode->pchild = pnewnode;
					pnewnode->prightsibling = ptnode;
				}
				else 
				{
					pcurnode->pchild = ptnode;
					ptnode->prightsibling = pnewnode;
				}
				pcurnode->flag = 0;
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);

				break;
			}
		}
	}
}

//-------------------------------------------------------------------------------
//insert a single branch in an AFOPT-tree. 
//This procedure is called by MergeTwoTree procedure
//-------------------------------------------------------------------------------
void CAFOPTMine::InsertTransaction(AFOPT_NODE *ptreeroot, ARRAY_NODE* ptransaction, int length)
{
	int i, j, order, orig_frequency, list_len , t;
	AFOPT_NODE *pnewnode, *pcurnode, *pprevnode, *ptnode;
	ARRAY_NODE *pitem_list, *pnewitem_list;


	pprevnode = NULL;
	pcurnode = ptreeroot;
//	pcurnode->frequency += ptransaction[0].support;
	
	for(i=0;i<length;i++)
	{
		if(pcurnode->flag == -1)
		{
			pnewitem_list = new ARRAY_NODE[length-i];
			//goMemTracer.AddSingleBranch(length-i);
			for(j=0;j<length-i;j++)
			{
				pnewitem_list[j].order = ptransaction[j+i].order;
				pnewitem_list[j].support = ptransaction[j+i].support;
			}
			pcurnode->pitemlist = pnewitem_list;
			pcurnode->flag = length-i;
			break;
		}
		else if(pcurnode->flag == 0)
		{
			order = ptransaction[i].order;
			if(pcurnode->pchild->order > order)
			{
				//goMemTracer.AddOneAFOPTNode();
				pnewnode = new AFOPT_NODE;
				pnewnode->frequency = ptransaction[i].support;
				pnewnode->prightsibling = pcurnode->pchild;
				pnewnode->order = order;
				pcurnode->pchild = pnewnode;
				if(i==length-1)
				{
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
				}
				else
				{
					pnewitem_list = new ARRAY_NODE[length-i-1];
					//goMemTracer.AddSingleBranch(length-i-1);
					for(j=0;j<length-i-1;j++)
					{
						pnewitem_list[j].order = ptransaction[j+i+1].order;
						pnewitem_list[j].support = ptransaction[j+i+1].support;
					}
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = length-i-1;
					break;
				}
			}
			else 
			{
				pcurnode = pcurnode->pchild;
				while(pcurnode!=NULL && pcurnode->order<order)
				{
					pprevnode = pcurnode;
					pcurnode = pcurnode->prightsibling;
				}

				if(pcurnode!=NULL && pcurnode->order==order)
				{
					pcurnode->frequency += ptransaction[i].support;
				}
				else 
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = ptransaction[i].support;
					pnewnode->prightsibling = pcurnode;
					pnewnode->order = order;
					pprevnode->prightsibling = pnewnode;
					if(i==length-1)
					{
						pnewnode->flag = -1;
						pnewnode->pchild = NULL;
					}
					else
					{
						pnewitem_list = new ARRAY_NODE[length-i-1];
						//goMemTracer.AddSingleBranch(length-i-1);
						for(j=0;j<length-i-1;j++)
						{
							pnewitem_list[j].order = ptransaction[j+i+1].order;
							pnewitem_list[j].support = ptransaction[j+i+1].support;
						}
						pnewnode->pitemlist = pnewitem_list;
						pnewnode->flag = length-i-1;
						break;
					}				
				}
			}
		}
		else 
		{
			pitem_list = pcurnode->pitemlist;
			list_len = pcurnode->flag;
			orig_frequency = pcurnode->frequency;

			j = 0;
			while(j<list_len && (i+j< length) && pitem_list[j].order == ptransaction[i+j].order)
			{
				pitem_list[j].support += ptransaction[i+j].support;
				j++;
			}

			if(i+j==length)
			{
				break;
			}
			else if(j==list_len)
			{
				pnewitem_list = new ARRAY_NODE[length-i];
				//goMemTracer.AddSingleBranch(length-i);
				for(t=0;t<j;t++)
					pnewitem_list[t] = pitem_list[t];
				for(t=j;t<length-i;t++)
				{
					pnewitem_list[t] = ptransaction[i+t];
				}
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);
				pcurnode->pitemlist = pnewitem_list;
				pcurnode->flag = length-i;
				break;
			}
			else 
			{
				for(t=0;t<j;t++)
				{
					pnewnode = new AFOPT_NODE;
					//goMemTracer.AddOneAFOPTNode();
					pnewnode->frequency = pitem_list[t].support;
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
					pnewnode->prightsibling = NULL;
					pnewnode->order = pitem_list[t].order;
					pcurnode->pchild = pnewnode;
					pcurnode->flag = 0;
					pcurnode = pnewnode;
				}

				pnewnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				pnewnode->frequency = pitem_list[j].support;
				pnewnode->order = pitem_list[j].order;
				pnewnode->prightsibling = NULL;
				if(j==list_len-1)
				{
					pnewnode->flag = -1;
					pnewnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[list_len-1-j];
					//goMemTracer.AddSingleBranch(list_len-j-1);
					for(t=0;t<list_len-j-1;t++)
						pnewitem_list[t] = pitem_list[t+j+1];
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = list_len-1-j;
				}

				ptnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				ptnode->frequency = ptransaction[i+j].support;
				ptnode->order = ptransaction[i+j].order;
				ptnode->prightsibling = NULL;
				if(i+j == length-1)
				{
					ptnode->flag = -1;
					ptnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[length-1-j-i];
					//goMemTracer.AddSingleBranch(length-1-j-i);
					for(t=0;t<length-1-j-i;t++)
					{
						pnewitem_list[t].order = ptransaction[t+j+1+i].order;
						pnewitem_list[t].support = ptransaction[t+j+1+i].support;
					}
					ptnode->pitemlist = pnewitem_list;
					ptnode->flag = length-1-i-j;
				}

				if(pnewnode->order<ptnode->order)
				{
					pcurnode->pchild = pnewnode;
					pnewnode->prightsibling = ptnode;
				}
				else 
				{
					pcurnode->pchild = ptnode;
					ptnode->prightsibling = pnewnode;
				}
				pcurnode->flag = 0;
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);

				break;
			}
		}
	}
}

//-------------------------------------------------------------------------------
//release the space occupied by an AFOPT-tree
//-------------------------------------------------------------------------------
void CAFOPTMine::DestroyTree(AFOPT_NODE * proot)
{
	AFOPT_NODE *ptreenode, *pnextchild;
	int source_top;

	if(proot->flag == -1)
	{
		delete proot;
		//goMemTracer.DelOneAFOPTNode();
		return;
	}
	else if(proot->flag>0)
	{
		delete []proot->pitemlist;
		//goMemTracer.DelSingleBranch(proot->flag);
		delete proot;
		//goMemTracer.DelOneAFOPTNode();
		return;
	}
	
	ptreenode = proot;
	source_top = 0;

	mpsource_stack[source_top].pnext_child = ptreenode->pchild;
	mpsource_stack[source_top].pnode = ptreenode;
	source_top++;

	while(source_top>0)
	{
		source_top--;
		pnextchild = mpsource_stack[source_top].pnext_child;
		if(pnextchild!=NULL)
		{
			mpsource_stack[source_top].pnext_child = pnextchild->prightsibling;
			source_top++;
			if(pnextchild->flag==0)
			{
				mpsource_stack[source_top].pnext_child = pnextchild->pchild;
				mpsource_stack[source_top].pnode = pnextchild;
				source_top++;
			}
			else if(pnextchild->flag==-1)
			{
				delete pnextchild;
				//goMemTracer.DelOneAFOPTNode();
			}
			else 
			{
				delete []pnextchild->pitemlist;
				//goMemTracer.DelSingleBranch(pnextchild->flag);
				delete pnextchild;
				//goMemTracer.DelOneAFOPTNode();
			}
		}
		else 
		{
			delete mpsource_stack[source_top].pnode;
			//goMemTracer.DelOneAFOPTNode();
		}
	}

}

//-------------------------------------------------------------------------------
//initialize some varibles used when traversal a tree
//-------------------------------------------------------------------------------
void CAFOPTMine::Init(int nmax_depth)
{
	mpbranch = new int[nmax_depth];
	mptransaction = new int[nmax_depth];
	mpfrequencies = new int[nmax_depth+1];
	mpsource_stack = new STACK_NODE[nmax_depth];
	mpdest_stack = new STACK_NODE[nmax_depth];
}

CAFOPTMine::CAFOPTMine()
{
	mpsource_stack = NULL;
	mpdest_stack = NULL;
	mptransaction = NULL;
	mpbranch = NULL;
	mpfrequencies = NULL;
}

CAFOPTMine::~CAFOPTMine()
{
	if(mpsource_stack!=NULL)
		delete []mpsource_stack;
	if(mpdest_stack!=NULL)
		delete []mpdest_stack;
	if(mptransaction!=NULL)
		delete []mptransaction;
	if(mpbranch!=NULL)
		delete []mpbranch;
	if(mpfrequencies!=NULL)
		delete []mpfrequencies;
}

