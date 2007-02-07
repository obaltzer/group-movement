#ifndef _DATA_H
#define _DATA_H

/*----------------------------------------------------------------------
  File     : data.h
  Contents : data set management
----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

class Transaction
{
public:
	
  Transaction(int l) : length(l) {t = new int[l];}
  Transaction(const Transaction &tr);
  ~Transaction(){delete [] t;}
  
  int length;
  int *t;
};

class Data
{
 public:
	
  Data(char *filename);
  ~Data();
  int isOpen();
  
  Transaction *getNextTransaction();
  
 private:
  
  FILE *in;
};

extern Data* gpdata;

#endif


