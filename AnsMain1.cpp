//------------------------------------------------------------------------------

#include <stdio.h>
#include "lex.h"
#include "flat.h"
#include "ans_t.h"
#include "e.h"

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
printf("ANSWER simulator V0.2\n(%lu-bit version)\n\n",8*sizeof(void *));
Ans_t * pAns = 0;
if (argc<2) printf("...usage is AnsMain1 circuit_file [monitor_file]\n");
else try {
  pAns = new Ans_t(argc,argv);
}
catch(bad_alloc) {
  printf("\n\n Out of memory ...    \n\n");
}
catch(class E e){
  printf("\n\n Exception from %s(line %d) ...\n",e.s,e.i);
}
catch(...) {
  printf("\n\n Unhandled exception ???\n");
}

if (pAns!=0) delete pAns;

printf("ANSWER v0.2 closing down ... \nSmite any key to go\n\n");
//getchar();

return 0;
}

//------------------------------------------------------------------------------



