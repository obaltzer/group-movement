#ifndef _PATTERNSET_MANAGER_H
#define _PATTERNSET_MANAGER_H

#include "Global.h"
#include "hash_map.h"

typedef struct MPATTERN
{
	int length;
	int *pitemset;
	int support;
	MPATTERN *next;
} MPATTERN;

typedef struct PATTERN_POINTER
{
	MPATTERN *ppattern;
	PATTERN_POINTER *next;
}PATTERN_POINTER;

typedef struct 
{
	PATTERN_POINTER *pheader;
	PATTERN_POINTER *ptail;
} LOCAL_PATTERN_SET;

class PatSetManager
{
public:
	void AddMFI(int* pitemset, int length, int support, LOCAL_PATTERN_SET *pnewmfi);
	void ProjectMFIs(LOCAL_PATTERN_SET *pexistmfi, LOCAL_PATTERN_SET *psubexistmfi, int no_of_newfreqitems, CHash_Map *pitem_order_map);

	void PushNewMFIRight(LOCAL_PATTERN_SET *pnewmfi, int order, LOCAL_PATTERN_SET *pmfi, LOCAL_PATTERN_SET *psubexistmfi, int no_of_freqitems, CHash_Map *pitem_order_map);
	void PushMFIRight(LOCAL_PATTERN_SET *pmfi, LOCAL_PATTERN_SET* psubexistmfi, int order, int no_of_freqitems, CHash_Map *pitem_order_map);

	void DeleteLocalPatternSet(LOCAL_PATTERN_SET *pcondpatternset);

	int PrunedByExistMFI(int* pitemset, int len, LOCAL_PATTERN_SET *pexistmfis);

	void CheckMFI(MPATTERN *ppatternset);
	void SortPatterns(MPATTERN* ppatternset);
	void DeletePatterns(MPATTERN* ppatternset);

};

extern MPATTERN *gppatterns;
extern PatSetManager goPatSetManager;

int ComparePattern(const void *e1, const void *e2);
int bucket_count(int *Buckets, int no_of_freqitems, ITEM_COUNTER* pfreqitems, LOCAL_PATTERN_SET *pexistmfis, LOCAL_PATTERN_SET *pnewmfi);

#endif
