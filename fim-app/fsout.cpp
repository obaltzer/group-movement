#include "fsout.h"

FSout::FSout(char *filename)
{
  out = fopen(filename,"wt");
}

FSout::~FSout()
{
  if(out) fclose(out);
}

int FSout::isOpen()
{
  if(out) return 1;
  else return 0;
}

void FSout::printSet(int length, int *iset, int support)
{
  for(int i=0; i<length; i++) 
    fprintf(out, "%d ", iset[i]);
  fprintf(out, "(%d)\n", support);
}

