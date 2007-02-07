#ifndef _FOUT_H
#define _FOUT_H

#include <stdio.h>
#include "Global.h"
#include "parameters.h"

class FSout
{
 public:

  FSout(char *filename);
  ~FSout();

  int isOpen();

  void printSet(int length, int *iset, int support);

 private:

  FILE *out;
};

extern FSout *gpfsout;

void inline OutputOnePattern(int nitem, int nsupport)
{
	gntotal_patterns++;
	gppat_counters[gndepth+1]++;

	if(goparameters.bresult_name_given)
	{
		gpheader_itemset[gndepth] = nitem;
		gpfsout->printSet(gndepth+1, gpheader_itemset, nsupport);
	}
}

void inline OutputOneClosePat(int nsupport)
{
	gntotal_patterns++;
	gppat_counters[gndepth]++;
	if(gnmax_pattern_len<gndepth)
		gnmax_pattern_len = gndepth;

	if(goparameters.bresult_name_given)
		gpfsout->printSet(gndepth, gpheader_itemset, nsupport);
}

void inline OutputOneClosePat(int length, int nsupport)
{
	gntotal_patterns++;
	gppat_counters[gndepth+length]++;
	if(gnmax_pattern_len<gndepth+length)
		gnmax_pattern_len = gndepth+length;

	if(goparameters.bresult_name_given)
		gpfsout->printSet(gndepth+length, gpheader_itemset, nsupport);
}

void inline OutputOneMaxPat(int length, int nsupport)
{
	gntotal_patterns++;
	gppat_counters[gndepth+length]++;
	if(gnmax_pattern_len<gndepth+length)
		gnmax_pattern_len = gndepth+length;

	if(goparameters.bresult_name_given)
		gpfsout->printSet(gndepth+length, gpheader_itemset, nsupport);
}


void inline OutputOneMaxPat(int *ppattern, int npat_len, int nsupport)
{
	gppat_counters[npat_len]++;

	if(goparameters.bresult_name_given)
		gpfsout->printSet(npat_len, ppattern, nsupport);
}



#endif


