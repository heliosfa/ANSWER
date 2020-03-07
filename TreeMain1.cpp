//------------------------------------------------------------------------------
/* Type BC tree builder: behaviour depends on the value of the fifth parameter
(a flag):
If flag = "B" we make two IDENTICAL trees, then flip one of them over and stitch
the leaves of each tree together. This means the plane of symmetry goes across
the edges joining the two leaf planes, and there are two planes in the middle of
the system that have the same number of devices.
Number of devices = 2*(1-w^(d+1))/(1-w)

If flag = "C" the contracting tree is one plane smaller than the expanding tree,
and the plane of symmetry goes along the line of devices in the middle of the
structure (i.e. the leaf level of the expanding tree).
Number of devices = (((w^d)*(w+1))-2)/(w-1)
*/

#include <stdio.h>
#include "pdigraph.hpp"
#include "macros.h"
#include "flat.h"
#include "filename.h"
#include <string>
#include <vector>
using namespace std;

//==============================================================================
// "Template arguments cannot have static or local linkage" - pathetic.
// Who designed this damn language?

struct N_t {
  N_t(unsigned _id,unsigned _x,unsigned _y,unsigned _t):
    id(_id),x(_x),y(_y),t(_t){}
  N_t():id(0),x(0),y(0),t(0){}
  unsigned id,x,y,t;
  void Dump() { printf("[%3u: %3u %3u (%1u)]\n",id,x,y,t);}
} N;

pdigraph<unsigned,N_t,int,int,int,int> G;

//------------------------------------------------------------------------------

void AD_cb(int const & i)
{
printf("[AD:%d]",i);
}

//------------------------------------------------------------------------------

void AK_cb(int const & i)
{
printf("[AK:%d]",i);
}

//------------------------------------------------------------------------------

void ND_cb(N_t const & pD)
{
printf("[ND:%u %u %u (%u)]",pD.id,pD.x,pD.y,pD.t);
}

//------------------------------------------------------------------------------

void NK_cb(unsigned const & u)
{
printf("[NK:%u]",u);
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
printf("---------------------------\n");
printf("Tree generator: Hello, %d-bit world\n",sizeof(void *)*8);
if (argc<6) {
  printf("Requires: cct_filename mon_filename fanout depth type\n"
         "Hit any key to go\n");
  getchar();
  return 1;
}
long t0;                               // Wallclock timers
long t1 = mTimer();
printf("Tree builder 1 (built %s %s)\nlaunched as %s\n",
       string(__TIME__).c_str(),string(__DATE__).c_str(),argv[0]);
printf("at %s %s\n",GetTime(),GetDate());
FileName Fn(argv[1]);
Fn.FNExtn("cct");                      // Force the circuit extension
string cname = Fn.FNComplete();        // File for circuit
Fn.FNComplete(argv[2]);                // Monitor list file
Fn.FNExtn("mon");                      // Force the monitor list extension
string mname = Fn.FNComplete();        // File for monitor list
unsigned F = str2uint(argv[3]);        // Fanout
unsigned D = str2uint(argv[4]);        // Depth
char ttyp = argv[5][0];

printf("Circuit file: %s  Monitor file: %s\nFanout: %u Depth: %u Type: %c\n\n",
        cname.c_str(),mname.c_str(),F,D,ttyp);

G.SetND_CB(ND_cb);
G.SetNK_CB(NK_cb);
G.SetAD_CB(AD_cb);
G.SetAK_CB(AK_cb);

unsigned index = 0;                    // Unique label for entities
unsigned xc;                           // Local in-row counter
unsigned out;                          // Seed node for EACH tree

vector<N_t> vRowA,vRowB;               // Two rows playing ping-pong
vector<N_t> * pRow1 = &vRowA;          // Pointers - these we swap
vector<N_t> * pRow2 = &vRowB;
vector<N_t> vRowEdge[2];               // Store the leaf row of each tree
vector<unsigned> vMon;                 // Device monitor list

for(unsigned tree=0;tree<2;tree++) {   // Build two trees:
  vRowA.clear();
  vRowB.clear();
  N_t N = N_t(out=index++,0,0,tree);   // Seed node. Note save the lower subtree
                                       // root (out) for the sink device at end
  vMon.push_back(N.id);                // Monitor it
  pRow1->push_back(N);                 // Shove into generating vector
  G.InsertNode(N.id,N);                // Shove into main graph
  for(unsigned y=0;y<D;y++) {          // For each row....
                                       // Type C: symmetry plane through devices
    if ((ttyp!='B')&&(y==D-1)&&(tree==1)) continue;
    printf("[%s row %u...",tree==0?"  Expanding":"Contracting",y);
    t0 = mTimer();                     // Real time clock
    xc=0;
    WALKVECTOR(N_t,(*pRow1),i) {       // Walk the generator row
      for(unsigned x=0;x<F;x++) {      // Walk each node child(ren)
        N = N_t(index++,xc++,y,tree);  // Create new node
        pRow2->push_back(N);           // Shove into generator child
        G.InsertNode(N.id,N);          // Shove node into graph
                                       // Arc direction depends on tree type
        if (tree==0) G.InsertArc(index++,i->id,N.id);
        else         G.InsertArc(index++,N.id,i->id);
      }
    }
    vMon.push_back(N.id);              // Monitor the last node in each row
    pRow1->clear();                    // Kill generator row
    swap(pRow1,pRow2);                 // Swap generator/generator child rows
    t0 = mTimer(t0);                   // Delta real time
    printf(" ... %7u nodes built in %7ld msecs]\n",pRow1->size(),t0);
  }
  vRowEdge[tree] = *pRow1;             // Save the last level of the tree
}

unsigned Fx = F;                       // Stride in bottom subtree
                                       // If type B, the last row of the
                                       // contracting tree has the wrong
                                       // parameter set
if (ttyp=='B') {
  WALKVECTOR(N_t,(*pRow1),i) G.FindNode(i->id)->t=0;
  Fx = 1;                              // Stride in top row of bottom subtree
}
                                       // Stitch the two trees together :
                                       // If it's a C tree (symmetry about
                                       // devices), vRowEdge[1] will have fewer
                                       // devices than vRowEdge[0].
printf("[Stitching subtrees together...");
t0 = mTimer();
for(unsigned i=0;i<vRowEdge[0].size();i++)
  G.InsertArc(index++,vRowEdge[0][i].id,vRowEdge[1][i/Fx].id);
printf(" ... %u edges in %ld msecs]\n",vRowEdge[0].size(),mTimer(t0));

unsigned sink = index;                 // Save the sink node
N = N_t(index++,0,0,0);                // Tack it onto the end
G.InsertNode(N.id,N);
vMon.push_back(N.id);                  // Monitor it
G.InsertArc(index++,out,N.id);         // Connect it into the graph

printf("\nTotal tree build time %ld ms\n",mTimer(t1));
printf("\nDouble sided tree (%s) complete (fanout %u, rows %u):\n",
       cname.c_str(),F,D);
printf("Total nodes %u, arcs %u\n",G.SizeNodes(),G.SizeArcs());
                                       // Write the circuit file
FILE * fp = fopen(cname.c_str(),"w");
                                       // Devices
WALKPDIGRAPHNODES(unsigned,N_t,int,int,int,int,G,i) {
  N = G.NodeData(i);                   // Walk the devices
  if (N.id!=sink) fprintf(fp,"d%04u p%1u ",N.id,N.t);  // Driving node....
  else fprintf(fp,"<d%04u psnk ",N.id);
  vector<unsigned> vNk;
  G.DrivenNodes(N.id,vNk);             // Extract driven nodes
  WALKVECTOR(unsigned,vNk,j) fprintf(fp,"d%04u ",*j);
  fprintf(fp,"\n");
}
                                       // Parameter sets
unsigned n_type = 2;                   // Internal model type
double h  = 2.0;                       // Neuron output spike size
double th = 1.0;                       // Neuron firing threshold
double rf = 0.001;                     // Neuron refractory period
double dc = 0.95;                      // Leak rate
double fd = 0.0005;                    // Fire delay
fprintf(fp,"!p0 %u %e %e %e %e %e\n",n_type,h,th,rf,dc,fd);
fprintf(fp,"!p1 %u %e %e %e %e %e\n",n_type,h,th*double(F),rf,dc,fd);
double ts = 0.001;                     // Start delay
//string clk = "clk";
//fprintf(fp,"*%s pclk\n",clk.c_str());  // Clock device
//fprintf(fp,"!pclk %e 0.00005\n",ts);   // Clock parameters
string src = "src";
fprintf(fp,">%s psrc d0000\n",src.c_str());       // Source device
fprintf(fp,"!psrc %e %e 1.0 1 100000 1\n",h,ts);  // Source parameters
//fprintf(fp,"<%s psnk\n","?????????");              // Sink device
fprintf(fp,"!psnk 1.0 0.95\n");        // Sink parameters
//double stop = ts + ((double(D)+2.0)*fd*2.05);
double stop = 100000.0;
fprintf(fp,"== %e\n",stop);            // Simulation stop time
fclose(fp);                            // End of circuit file

                                       // Now write the monitor list
printf("Monitor list (%s) contains %u devices\n",mname.c_str(),vMon.size()+2);
fp = fopen(mname.c_str(),"w");
fprintf(fp,"%s\n",src.c_str());
//fprintf(fp,"%s\n",clk.c_str());
WALKVECTOR(unsigned,vMon,i) fprintf(fp,"d%04u\n",*i);
fclose(fp);                            // End of monitor list

printf("Total elapsed wallclock %ld msecs\n",mTimer(t1));
printf("---------------------------\n");
printf("\nSmite any key to absquatulate\n");
//getchar();
return 0;
}

//------------------------------------------------------------------------------
