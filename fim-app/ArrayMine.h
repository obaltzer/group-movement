#ifndef _ARRAY_H
#define _ARRAY_H

#include "Global.h"
#include "patternsetmanager.h"
#include "cfptree_outbuf.h"

class CArrayMine
{
public:
	void DepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems);
	void CFIDepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, CFP_NODE *pcfp_node);
	void MFIDepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, LOCAL_PATTERN_SET *psubexistmfi, LOCAL_PATTERN_SET *pnewmfi, CHash_Map *pitem_order_map);

	void CountFreqItems(TRANS_HEAD_NODE *pcond_db, int* pitem_sup_map);
	void BuildNewCondDB(TRANS_HEAD_NODE *pcond_db, HEADER_TABLE pnewheader_table, int* pitem_order_map, int num_of_freqitems, int *Buckets);
	void BuildNewCondDB(TRANS_HEAD_NODE *pcond_db, HEADER_TABLE pnewheader_table, int* pitem_order_map);
	void PushRight(HEADER_TABLE pheader_table, int item_pos, int max_push_pos);
	void BucketCount(TRANS_HEAD_NODE *pcond_db, int *pitem_order_map, int *Buckets);

};

extern CArrayMine goArrayMiner;

#endif


