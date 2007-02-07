#ifndef _MINER_H
#define _MINER_H

#include "Global.h"	

class CScanDBMine
{
public:
	int ScanDBCountFreqItems(ITEM_COUNTER **ppfreqitems);

	void ScanDBBuildCondDB(HEADER_TABLE pheader_table, int *pitem_order_map, int num_of_freqitems, int *Buckets);
	void ScanDBBuildCondDB(HEADER_TABLE pheader_table, int *pitem_order_map);

	void ScanDBBucketCount(int *pitem_order_map, int *Buckets);

};

extern CScanDBMine  goDBMiner;

#endif


