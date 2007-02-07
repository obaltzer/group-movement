#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parameters.h"
#include "Global.h"

void CParameters::SetResultName(char *result_name)
{
	if(goparameters.bresult_name_given)
		strcpy(pat_filename, result_name);

	strcat(tree_filename, CFP_TREE_FILENAME);

}

void CParameters::SetBufParameters(int entry_size)
{
	ntree_buf_page_size = (1<<DEFAULT_TREE_BUF_PAGE_SIZE);
	if((unsigned int)ntree_buf_page_size<entry_size+sizeof(int)*(gnmax_trans_len+1))
	{
		while((unsigned int)ntree_buf_page_size<entry_size+sizeof(int)*(gnmax_trans_len+1))
			ntree_buf_page_size = ntree_buf_page_size*2;
	}
	if((unsigned int)ntree_buf_page_size<entry_size*gntotal_freqitems+2*sizeof(int))
	{
		while((unsigned int)ntree_buf_page_size<entry_size*gntotal_freqitems+2*sizeof(int))
			ntree_buf_page_size = ntree_buf_page_size*2;
	}

	ntree_outbuf_size = DEFAULT_TREE_OUTBUF_SIZE;

	ntree_buf_num_of_pages = (ntree_outbuf_size-1)/ntree_buf_page_size+1;
	ntree_outbuf_size = ntree_buf_page_size*ntree_buf_num_of_pages;

}

