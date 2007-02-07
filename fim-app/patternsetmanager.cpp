#include <stdlib.h>

#include "patternsetmanager.h"
#include "parameters.h"
#include "fsout.h"

//#include "MemTracer.h"

//--------------------------------------------------------------------------------------
//Add one maximal frequent itemset to the set of existing maximal frequent itemsets.
//All maximal itemsets are sorted lexicographically
//--------------------------------------------------------------------------------------
void PatSetManager::AddMFI(int* pitemset, int length, int support, LOCAL_PATTERN_SET *pnewmfi)
{
	gntotal_patterns++;
	if(gnmax_pattern_len<gndepth+length)
		gnmax_pattern_len = gndepth+length;


	int* pmfiitemset;
	MPATTERN *pnewpattern;
	PATTERN_POINTER *pnewpointer;
	int t;

	pnewpattern = new MPATTERN;
	//goMemTracer.IncPatternBuffer(sizeof(MPATTERN));

	pnewpattern->length = gndepth+length;
	pmfiitemset = new int[pnewpattern->length];
	//goMemTracer.IncPatternBuffer(pnewpattern->length*sizeof(int));

	for(t=0;t<gndepth;t++)
		pmfiitemset[t] = gpheader_itemset[t];
	for(t=0;t<length;t++)
		pmfiitemset[t+gndepth] = pitemset[t];

	qsort(pmfiitemset, gndepth+length, sizeof(int), comp_int_asc);

	if(pnewpattern->length==7 && pmfiitemset[0]==2 && pmfiitemset[1]==34 && pmfiitemset[2]==63 &&
		pmfiitemset[3]==76 && pmfiitemset[4]==85  && pmfiitemset[5]==86  && pmfiitemset[6]==90)
		printf("stop\n");

	pnewpattern->pitemset = pmfiitemset;
	pnewpattern->support = support;
	pnewpattern->next = gppatterns;
	gppatterns = pnewpattern;

	pnewpointer = new PATTERN_POINTER;
	//goMemTracer.IncPatternBuffer(sizeof(PATTERN_POINTER));
	pnewpointer->ppattern = pnewpattern;
	pnewpointer->next = NULL;

	if(pnewmfi->pheader==NULL)
	{
		pnewmfi->pheader = pnewpointer;
		pnewmfi->ptail = pnewpointer;
	}
	else 
	{
		pnewmfi->ptail->next = pnewpointer;
		pnewmfi->ptail = pnewpointer;
	}

}

//------------------------------------------------------------------------------------
//Project all itemsets in LMFI of p to the LMFIs of p's frequent extensions
//------------------------------------------------------------------------------------
void PatSetManager::ProjectMFIs(LOCAL_PATTERN_SET *pexistmfi, LOCAL_PATTERN_SET *psubexistmfi, int no_of_newfreqitems, CHash_Map *pitem_order_map)
{
	PATTERN_POINTER *ppointer, *pnewpointer;
	MPATTERN *ppattern;
	int firstitem;
	int i, no_of_countitems;
	HASH_ENTRY *pitemnode;

	no_of_countitems = no_of_newfreqitems-MAX_BUCKET_SIZE;

	if(pexistmfi->pheader!=NULL)
	{
		ppointer = pexistmfi->pheader;
		while(ppointer!=NULL)
		{
			ppattern = ppointer->ppattern;

			firstitem = no_of_newfreqitems;
			for(i=0;i<ppattern->length;i++)
			{
				pitemnode = pitem_order_map->find(ppattern->pitemset[i]);
				if(pitemnode!=NULL && pitemnode->value<firstitem)
				{
					firstitem = pitemnode->value;
				}
			}
			
			if(firstitem<no_of_countitems)
			{
				pnewpointer = new PATTERN_POINTER;
				//goMemTracer.IncPatternBuffer(sizeof(PATTERN_POINTER));
				pnewpointer->ppattern = ppattern;
				pnewpointer->next = NULL;
				if(psubexistmfi[firstitem].pheader==NULL)
					psubexistmfi[firstitem].pheader = pnewpointer;
				if(psubexistmfi[firstitem].ptail!=NULL)
					psubexistmfi[firstitem].ptail->next = pnewpointer;
				psubexistmfi[firstitem].ptail = pnewpointer;
			}
			else if(firstitem>=no_of_countitems)
			{
				pnewpointer = new PATTERN_POINTER;
				//goMemTracer.IncPatternBuffer(sizeof(PATTERN_POINTER));
				pnewpointer->ppattern = ppattern;
				pnewpointer->next = NULL;
				if(psubexistmfi[no_of_countitems].pheader==NULL)
					psubexistmfi[no_of_countitems].pheader = pnewpointer;
				if(psubexistmfi[no_of_countitems].ptail!=NULL)
					psubexistmfi[no_of_countitems].ptail->next = pnewpointer;
				psubexistmfi[no_of_countitems].ptail = pnewpointer;
			}
			ppointer = ppointer->next;
		}
	}
}

//--------------------------------------------------------------------------
//Delete LMFI of a maximal frequent itemset
//--------------------------------------------------------------------------
void PatSetManager::DeleteLocalPatternSet(LOCAL_PATTERN_SET *plocalpatternset)
{
	PATTERN_POINTER *ppointer, *pnextpointer;

	ppointer = plocalpatternset->pheader;
	while(ppointer!=NULL)
	{
		pnextpointer = ppointer->next;
		delete ppointer;
		//goMemTracer.DecPatternBuffer(sizeof(PATTERN_POINTER));
		ppointer = pnextpointer;
	}

	plocalpatternset->pheader = NULL;
	plocalpatternset->ptail = NULL;

}

//---------------------------------------------------------------------------------
//Project the set of maximal itemsets mined from Dp to p's right siblings' LMFIs
//---------------------------------------------------------------------------------
void PatSetManager::PushNewMFIRight(LOCAL_PATTERN_SET *pnewmfi, int order, LOCAL_PATTERN_SET *pmfi, LOCAL_PATTERN_SET *psubexistmfi, int no_of_freqitems, CHash_Map *pitem_order_map)
{
	PATTERN_POINTER *ppointer, *pnextpointer, *pnewpointer;
	MPATTERN *ppattern;
	int firstitem, t;
	int no_of_countitems;
	HASH_ENTRY *pitemnode;

	no_of_countitems = no_of_freqitems-MAX_BUCKET_SIZE;
	
	if(pnewmfi->pheader!=NULL)
	{
		ppointer = pnewmfi->pheader;
		while(ppointer!=NULL)
		{
			ppattern = ppointer->ppattern;
			pnextpointer = ppointer->next;

			firstitem = no_of_freqitems;
			for(t=0;t<ppattern->length;t++)
			{
				pitemnode = pitem_order_map->find(ppattern->pitemset[t]);
				if(pitemnode!=NULL && pitemnode->value > order && pitemnode->value<firstitem)
				{
					firstitem = pitemnode->value;
				}
			}

			if(firstitem<no_of_countitems)
			{
				pnewpointer = new PATTERN_POINTER;
				//goMemTracer.IncPatternBuffer(sizeof(PATTERN_POINTER));
				pnewpointer->ppattern = ppointer->ppattern;
				pnewpointer->next = NULL;
				if(psubexistmfi[firstitem].pheader==NULL)
					psubexistmfi[firstitem].pheader = pnewpointer;
				if(psubexistmfi[firstitem].ptail!=NULL)
					psubexistmfi[firstitem].ptail->next = pnewpointer;
				psubexistmfi[firstitem].ptail = pnewpointer;
			}
			else 
			{
				pnewpointer = new PATTERN_POINTER;
				//goMemTracer.IncPatternBuffer(sizeof(PATTERN_POINTER));
				pnewpointer->ppattern = ppointer->ppattern;
				pnewpointer->next = NULL;
				if(psubexistmfi[no_of_countitems].pheader==NULL)
					psubexistmfi[no_of_countitems].pheader = pnewpointer;
				if(psubexistmfi[no_of_countitems].ptail!=NULL)
					psubexistmfi[no_of_countitems].ptail->next = pnewpointer;
				psubexistmfi[no_of_countitems].ptail = pnewpointer;

			}
			ppointer = pnextpointer;
		}
	}

	pnewmfi->ptail->next = pmfi->pheader;
	pmfi->pheader = pnewmfi->pheader;
	if(pmfi->ptail == NULL)
		pmfi->ptail = pnewmfi->ptail;
}

//---------------------------------------------------------------------------------
//Project itemsets in LMFIs of p to p's right siblings' LMFIs
//---------------------------------------------------------------------------------
void PatSetManager::PushMFIRight(LOCAL_PATTERN_SET *pmfi, LOCAL_PATTERN_SET* psubexistmfi, int order, int no_of_freqitems, CHash_Map *pitem_order_map)
{
	MPATTERN *ppattern;
	PATTERN_POINTER *ppointer, *pnextpointer;
	int t, firstitem;
	HASH_ENTRY *pitemnode;
	
	int no_of_countitems = no_of_freqitems-MAX_BUCKET_SIZE;
	if(pmfi->pheader!=NULL)
	{
		ppointer = pmfi->pheader;
		while(ppointer!=NULL)
		{
			pnextpointer = ppointer->next;

			ppattern = ppointer->ppattern;
			firstitem = no_of_freqitems;
			for(t=0;t<ppattern->length;t++)
			{
				pitemnode = pitem_order_map->find(ppattern->pitemset[t]);
				if(pitemnode!=NULL && pitemnode->value>order && pitemnode->value<firstitem)
				{
					firstitem = pitemnode->value;
				}
			}

			if(firstitem<no_of_countitems)
			{
				ppointer->next = NULL;
				if(psubexistmfi[firstitem].pheader==NULL)
					psubexistmfi[firstitem].pheader = ppointer;
				if(psubexistmfi[firstitem].ptail!=NULL)
					psubexistmfi[firstitem].ptail->next = ppointer;
				psubexistmfi[firstitem].ptail = ppointer;
			}
			else if(firstitem<no_of_freqitems)
			{
				ppointer->next = NULL;
				if(psubexistmfi[no_of_countitems].pheader==NULL)
					psubexistmfi[no_of_countitems].pheader = ppointer;
				if(psubexistmfi[no_of_countitems].ptail!=NULL)
					psubexistmfi[no_of_countitems].ptail->next = ppointer;
				psubexistmfi[no_of_countitems].ptail = ppointer;
			}
			else
			{
				delete ppointer;
				//goMemTracer.DecPatternBuffer(sizeof(PATTERN_POINTER));
			}
			ppointer = pnextpointer;	
		}
	}
}

//----------------------------------------------------
//Sort all maximal frequent itemsets.
//----------------------------------------------------
void PatSetManager::SortPatterns(MPATTERN* ppatternset)
{
	MPATTERN *ppattern, *ppatternarray;
	int i;

	ppatternarray = new MPATTERN[gntotal_patterns];

	ppattern = ppatternset;
	i = 0;
	while(ppattern!=NULL)
	{
		ppatternarray[i] = (*ppattern);
		ppattern = ppattern->next;
		i++;
	}

	qsort(ppatternarray, gntotal_patterns, sizeof(MPATTERN), ComparePattern);

	delete []ppatternarray;
}

//------------------------------------------------------
//Delete all maximal itemsets
//------------------------------------------------------
void PatSetManager::DeletePatterns(MPATTERN* ppatternset)
{
	MPATTERN *ppattern, *pnextpat;

	ppattern = ppatternset;
	while(ppattern!=NULL)
	{
		pnextpat = ppattern->next;

		OutputOneMaxPat(ppattern->pitemset, ppattern->length, ppattern->support);

		delete []ppattern->pitemset;
		//goMemTracer.DecPatternBuffer(ppattern->length*sizeof(int));
		delete ppattern;
		//goMemTracer.DecPatternBuffer(sizeof(MPATTERN));
		ppattern = pnextpat;
	}
}

//---------------------------------------------------------------------------------------
//Check wheter pitemset is a subset of an itemset in pexistmfis, where pexistmfis is 
//LMFI of pitemset.
//----------------------------------------------------------------------------------------
int PatSetManager::PrunedByExistMFI(int* pitemset, int len, LOCAL_PATTERN_SET *pexistmfis)
{
	int issubset = 0;
	MPATTERN *ppattern;
	PATTERN_POINTER *ppointer;
	int t, npat_len, i;
	int* ppat_itemset;

	if(len>0)
		qsort(pitemset, len, sizeof(int), comp_int_asc);
	else if(pexistmfis->pheader!=NULL)
		return true;
	else 
		return false;

	ppointer = pexistmfis->pheader;
	while(ppointer!=NULL)
	{
		ppattern = ppointer->ppattern;
		npat_len = ppattern->length;
		if(npat_len>=len)
		{
			ppat_itemset = ppattern->pitemset;

			t = 0;
			i = 0;
			while(t<len)
			{
				while(i<npat_len && ppat_itemset[i]<pitemset[t])
					i++;
				if(i>=npat_len)
					break;
				if(ppat_itemset[i]==pitemset[t])
				{
					i++;
					t++;
				}
				else break;
			}
			if(t==len)
			{
				issubset = true;
				break;
			}
		}

		ppointer = ppointer->next;
	}

	return issubset;
}

int ComparePattern(const void *e1, const void *e2)
{
	MPATTERN pair1,pair2;
	int i;
	pair1=*(MPATTERN *) e1;
	pair2=*(MPATTERN *) e2;

	i = 0;
	while(i<pair1.length && i<pair2.length)
	{
		if(pair1.pitemset[i] == pair2.pitemset[i])
			i++;
		else if(pair1.pitemset[i] > pair2.pitemset[i])
			return 1;
		else return -1;
	}

	if(pair1.length == pair2.length)
		return 0;
	else if(i == pair1.length)
		return -1;
	else return 1;

}



