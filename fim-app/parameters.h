#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#define MAX_FILENAME_LEN	200

#define CFP_TREE_FILENAME		"temp.cfp"

#define DEFAULT_TREE_OUTBUF_SIZE		(1<<27)
#define DEFAULT_TREE_BUF_PAGE_SIZE		20

#define ITEM_SUP_MAP_SIZE	10000

#define SUP_MAXLEN_MAP_SIZE 10000


class CParameters
{
public:
	char data_filename[MAX_FILENAME_LEN];
	int nmin_sup;
	bool bresult_name_given;
	char pat_filename[MAX_FILENAME_LEN];

	char tree_filename[MAX_FILENAME_LEN];

	int ntree_outbuf_size;
	int ntree_buf_page_size;
	int ntree_buf_num_of_pages;

	void SetResultName(char *result_name);
	void SetBufParameters(int entry_size);

};

extern CParameters goparameters;

#endif


