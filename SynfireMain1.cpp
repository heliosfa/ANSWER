//------------------------------------------------------------------------------

#include <stdio.h>
#include "pdigraph.hpp"
#include "macros.h"
#include "flat.h"
#include "filename.h"
#include <string>
#include <vector>
using namespace std;

// Real programmers can write FORTRAN in any language.....

//==============================================================================
// Global data declares
// "Template argument cannot have static or local linkage" - pathetic

struct N_t {                           // Neuron structure
  N_t(unsigned _r,unsigned _x,unsigned _y):
    r(_r),x(_x),y(_y){
      char buf[256];
      sprintf(buf,"R%02uX%03uY%03u",r,x,y);
      name = string(buf);
    }
  N_t(string n):r(0),x(0),y(0) { name = n; }
  unsigned r,x,y,p;
  string   name;
  string   param;
  void     Dump() { printf("[%s:%s]\n",name.c_str(),param.c_str()); }
  string   Name() { return name; }
};

struct Ne_pt {                         // Body neuron parameters
  Ne_pt():name("p0"),h(2.0),th(1.9),rf(0.001),dc(20),fd(0.002){}
  string   name;                       // Parameter set name
  unsigned Mt;                         // Internal (ANSWER) model type
  double   h;                          // Neuron output spike size
  double   th;                         // Neuron firing threshold
  double   rf;                         // Neuron refractory period
  double   dc;                         // Leak rate
  double   fd;                         // Fire delay
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %u %e %e %e %e %e\n",name.c_str(),Mt,h,th,rf,dc,fd);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    fprintf(fp,"*neuron : %s(\"\",\"%32b%32b%32b%32b%32b%32b\")\n",name.c_str());
    fprintf(fp,"#neuron : %s(%u,%e,%e,%e,%e,%e)\n",name.c_str(),Mt,h,th,rf,dc,fd);
  }
} Ne_p;

struct So_pt {                         // Source parameters
  So_pt():name("pSRC"),h(2.0),ts(0.02),Tmin(1.0),Nmin(1),Tmaj(100000.0),Nmaj(1){}
  string   name;                       // Parameter set name
  double   h;                          // Height of output pulse
  double   ts;                         // Start delay
  double   Tmin;                       // T minor - spike spacing within bursts
  unsigned Nmin;                       // N minor - number of spikes in a burst
  double   Tmaj;                       // T major - spacing between the beginnings of bursts
  unsigned Nmaj;                       // N major - number of bursts
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %e %e %e %u %e %u\n",name.c_str(),h,ts,Tmin,Nmin,Tmaj,Nmaj);
  }
  string sPrnt() {                     // For Loader103
    char buf[256];
    sprintf(buf,"%s(%e,%e,%e,%u,%e,%u)",name.c_str(),h,ts,Tmin,Nmin,Tmaj,Nmaj);
    return string(buf);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    fprintf(fp,"*device : %s (\"\",\"%32b%32b%32b%32b%32b%32b\")\n",name.c_str());
  }
} So_p;

struct Si_pt {                         // Sink parameters
  Si_pt():name("pSINK"),th(1.0),dc(1e-5){}
  string   name;                       // Parameter set name
  double   th;                         // Neuron firing (ie. killing) threshold
  double   dc;                         // Leak rate
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %e %e\n",name.c_str(),th,dc);
  }
  string sPrnt() {                     // For Loader103
    char buf[256];
    sprintf(buf,"%s(%e,%e)",name.c_str(),th,dc);
    return string(buf);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    fprintf(fp,"*device : %s (\"\",\"%32b%32b%32b%32b%32b%32b\")\n",name.c_str());
  }
} Si_p;

struct Cl_pt {                         // Clock parameters
  Cl_pt():name("pCLK"),ts(0.001),tick(0.00005){}
  string   name;                       // Parameter set name
  double   ts;                         // Start delay
  double   tick;                       // Tick interval
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %e %e\n",name.c_str(),ts,tick);
  }
} Cl_p;
                                       // Circuit graph
pdigraph<string,N_t *,int,int,int,int> G;
unsigned Rsize[] = {3,5,7,11,13,17,19};// Synfire ring sizes
vector<string> vMon;                   // List of nodes to be monitored
string sInput("Input");
string sOutput("Output");
string sSource("Source");
string sSink("Sink");
unsigned Mt;                           // Ring body neuron model type

//==============================================================================
// Forward routine declares

void AD_cb(int const &);
void AK_cb(int const &);
void BuildG(unsigned,unsigned,double,unsigned);
void ND_cb(N_t * const &);
void NK_cb(string const &);
void WriteANSWER(string,string,unsigned);
void WriteUIF(string,unsigned);

//==============================================================================

int main(int argc, char* argv[])
{
printf("Hello, %d-bit world\n",sizeof(unsigned)*8);
if (argc<6) {
  printf("Requires: "
         "root_filename ring_count ring_width pool_connect_prob neuron_type\n"
         "Hit any key to boogey\n");
  getchar();
  return 1;
}
long t0 = mTimer();                    // Start wallclcok
printf("[Synfire ring builder 1 launched as %s\n",argv[0]);
FileName Fn(argv[1]);
Fn.FNExtn("cct");                      // Force the circuit extension
string cname = Fn.FNComplete();        // File for ANSWER circuit
Fn.FNExtn("mon");                      // Force the monitor list extension
string mname = Fn.FNComplete();        // File for ANSWER monitor list
Fn.FNExtn("ecct");                     // Force UIF circuit file extension
string ename = Fn.FNComplete();

unsigned R   = str2uint(argv[2]);      // Ring count
unsigned W   = str2uint(argv[3]);      // Ring width
double   Pr  = str2dble(argv[4]);      // Pool-pool connection probability
         Mt  = str2uint(argv[5]);      // Neuron model type

if (R*sizeof(unsigned)>sizeof(Rsize)) {// Save the user from themself
  R = sizeof(Rsize)/sizeof(unsigned);
  printf("R truncated to %u\n",R);
}
if ((Mt==0)||(Mt>2)) {
  Mt = 1;
  printf("Mt overriden to type 1\n");
}

printf("\nSynfire ring build parameters:\n");
printf("Pool width %u\n",W);
printf("Pool-pool interconnect probability %5.2f\n",Pr);
printf("\n%u rings:\n",R);
for(unsigned i=0;i<R;i++) printf("Ring %u size %u\n",i,Rsize[i]);
printf("Neuron model type %u\n",Mt);

G.SetND_CB(ND_cb);                     // Digraph dump callbacks
G.SetNK_CB(NK_cb);
G.SetAD_CB(AD_cb);
G.SetAK_CB(AK_cb);

BuildG(R,W,Pr,Mt);                     // Populate the graph
t0 = mTimer(t0);                       // End of time
printf("... synfire ring built in %ld wall ms]\n\n",t0);
WriteANSWER(cname,mname,R);            // Dump out in ANSWER format
WriteUIF(ename,R);                     // Guess
                                       // Tidy up
WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) delete G.NodeData(i);

printf("\nSmite any key to decamp\n");
getchar();
return 0;
}

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

void BuildG(unsigned R,unsigned W,double Pr,unsigned Mt)
// Populate the circuit graph AND build the monitor list for the ANSWER simulator
{
unsigned index=0;                      // ID for the arcs
vector<vector<N_t *> > vvN;            // Rectangular matrix of neurons
vector<N_t *> vN;                      // Column vector
vector<pair<N_t *,N_t *> > p2N;        // Somewhere to store the I/O
string sp0("p0");                      // Bulk parameter set
string spx("px");                      // Collector parameter set at the end

N_t * pN;                              // Neuron pointer
for (unsigned r=0;r<R;r++) {           // One ring at a time
  vvN.clear();                         // Clear the ring matrix
  printf("Ring %u, size %u\n",r,Rsize[r]);
  for (unsigned x=0;x<Rsize[r];x++) {  // One pool (column) at a time
    vN.clear();                        // Clear the pool vector
    printf("Column %u\n",x);
    for (unsigned w=0;w<W;w++) {
      pN = new N_t(r,x,w);
      pN->param = sp0;
//      pN->Dump();
      vN.push_back(pN);                 // Store the new neuron in the column
    }
    vMon.push_back(pN->Name());         // Store the lst one to be monitored
    vvN.push_back(vN);                  // Store the completed column
  }
  for(vector<vector<N_t *> >::iterator i=vvN.begin();i!=vvN.end();i++) {
    WALKVECTOR(N_t *,(*i),j) {
//      printf("Inserting %s\n",(*j)->Name().c_str());
      G.InsertNode((*j)->Name(),*j);    // Shove into main graph
    }
  }
                                        // Save I/O neurons
  p2N.push_back(pair<N_t *,N_t *>(vvN[0][0],vvN[Rsize[r]-1][0]));

  for(unsigned x=0;x<Rsize[r]-1;x++) {  // Cross link columns in the matrix
    WALKVECTOR(N_t *,vvN[x],j) {
      WALKVECTOR(N_t *,vvN[x+1],k) {
        if (!P(Pr)) continue;
//        printf("Linking %s to %s\n",(*j)->Name().c_str(),(*k)->Name().c_str());
        G.InsertArc(index++,(*j)->Name(),(*k)->Name());
      }
    }
  }
  WALKVECTOR(N_t *,vvN[0],j) {          // Close the loop in the matrix
    WALKVECTOR(N_t *,vvN[Rsize[r]-1],k) {
      if (!P(Pr)) continue;
//      printf("...and linking %s to %s\n",
 //            (*k)->Name().c_str(),(*j)->Name().c_str());
      G.InsertArc(index++,(*k)->Name(),(*j)->Name());
    }
  }
}
// So now we have a set of disjoint rings; join them up:
printf("...join the rings\n");
                                        // Connect up all the I/O
pN = new N_t(sInput);                   // Input node
pN->param = sp0;                        // Parameter set
G.InsertNode(sInput,pN);                // Insert into graph
vMon.push_back(sInput);                 // Insert into monitor list
pN = new N_t(sOutput);                  // Output
pN->param = spx;                        // Parameters are different
G.InsertNode(sOutput,pN);               // Insert into graph
vMon.push_back(sOutput);                // Insert into monitor list

for(vector<pair<N_t *,N_t *> >::iterator i=p2N.begin();i!=p2N.end();i++) {
  printf("....and linking %s to %s\n",sInput.c_str(),(*i).first->Name().c_str());
  G.InsertArc(index++,sInput,(*i).first->Name());
  printf("....and linking %s to %s\n",(*i).second->Name().c_str(),sOutput.c_str());
  G.InsertArc(index++,(*i).second->Name(),sOutput);
}

//G.Dump();

printf("\nSynfire ring set complete\n");
printf("\nTotal nodes %u\nTotal arcs %u\n",G.SizeNodes(),G.SizeArcs());
}

//------------------------------------------------------------------------------

void ND_cb(N_t * const & pD)
{
printf("[NK:%s]",pD->Name().c_str());
}

//------------------------------------------------------------------------------

void NK_cb(string const & u)
{
printf("[NK:%s]",u.c_str());
}

//------------------------------------------------------------------------------

void WriteANSWER(string cname,string mname,unsigned R)
{
long t0 = mTimer();
printf("\n[Writing ANSWER circuit description files...\n");

string clk  = "clk";
//string srce = "srce";
//string sink = "sink";

FILE * fp = fopen(cname.c_str(),"w");
                                       // Devices
WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) {
  N_t * N = G.NodeData(i);             // Walk the devices
  fprintf(fp,"%s %s ",N->Name().c_str(),N->param.c_str());  // Driving node....
  vector<string> vNk;
  G.DrivenNodes(N->Name(),vNk);        // Extract driven nodes
  if (N->Name()==sOutput) fprintf(fp,"%s ",sSink.c_str());
  WALKVECTOR(string,vNk,j) fprintf(fp,"%s ",(*j).c_str());
  fprintf(fp,"\n");
}
                                       // Parameter sets
Ne_p.Mt = Mt;                          // Neuron model type
Ne_p.APrnt(fp);                        // Print parameter set
                                       // For the output neuron:
Ne_p.name = "px";                      // Modify name
Ne_p.th = 0.95 * Ne_p.h * R;           // Modify threshold
Ne_p.dc = 1000.0;                      // Modify leak rate
Ne_p.fd = 0.000002;                    // Modify fire delay
Ne_p.APrnt(fp);                        // And print it
if (Mt==1) {                           // Clock needed?
  fprintf(fp,"*%s %s\n",clk.c_str(),Cl_p.name.c_str());// Clock device
  Cl_p.APrnt(fp);                      // Clock parameters
}
                                       // Connect source device
fprintf(fp,">%s %s %s\n",sSource.c_str(),So_p.name.c_str(),sInput.c_str());
So_p.APrnt(fp);                        // Source parameters
fprintf(fp,"<%s %s\n",sSink.c_str(),Si_p.name.c_str());// Connect sink device
Si_p.APrnt(fp);                        // Sink parameters

unsigned D=1;                          // Calculate ample simulation time
for(unsigned i=0;i<R;i++) D*=Rsize[i];
double stop = So_p.ts + (double(D) + 2.0 * Ne_p.fd) * 1.05;
fprintf(fp,"== %e\n",stop);            // Simulation stop time
fclose(fp);                            // End of circuit file
printf("Monitor list (%s) contains %u devices\n",mname.c_str(),vMon.size());
                                       // Now write the monitor list
vMon.push_back(clk);
vMon.push_back(sSource);
vMon.push_back(sSink);
fp = fopen(mname.c_str(),"w");
WALKVECTOR(string,vMon,i) fprintf(fp,"%s\n",(*i).c_str());
fclose(fp);
t0 = mTimer(t0);
printf("...ANSWER circuit description files built in %ld wall ms]\n\n",t0);
}

//------------------------------------------------------------------------------

void WriteUIF(string ename,unsigned R)
{
long t0 = mTimer();
printf("\n[Writing Loader103 circuit description files...\n");
FILE * fp = fopen(ename.c_str(),"w");

fprintf(fp,"[Circuit(\"%s\")]\n",ename.c_str());
fprintf(fp,"*pin : Sy1(\"\",\"\")\n");
fprintf(fp,"#pin : Sy1()\n");

So_p.UPrnt(fp);
Si_p.UPrnt(fp);
fprintf(fp,"%s : %s = %s\n",sSource.c_str(),So_p.sPrnt().c_str(),sInput.c_str());
fprintf(fp,"%s : %s \n",sSink.c_str(),Si_p.sPrnt().c_str());

Ne_p.name = "p0";                      // Default model name
Ne_p.th = 1.0;                         // Reset the threshold
Ne_p.UPrnt(fp);                        // Write to UIF as default model
char sp0[64];                          // Infinitely long string buffer
                                       // Modify threshold for the output neuron
sprintf(sp0," p0(,,%e) = %s",0.95*Ne_p.h*R,sSink.c_str());

WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) {
  N_t * N = G.NodeData(i);             // Walk the devices
  string sType;
  char eq('=');
  if (N->Name()==sOutput) {            // Special case for the output device
    sType = sp0;
    eq=' ';
  }
  fprintf(fp,"%s :%s%c ",N->Name().c_str(),sType.c_str(),eq);  // Driving node....
  vector<string> vNk;
  G.DrivenNodes(N->Name(),vNk);        // Extract driven nodes
  WALKVECTOR(string,vNk,j) {
    char c = (*j)==vNk.back()?' ':',';
    fprintf(fp,"%s%c ",(*j).c_str(),c);
  }
  fprintf(fp,"\n");
}

fclose(fp);
t0 = mTimer(t0);
printf("...Loader103 circuit description files built in %ld wall ms]\n\n",t0);
}

//------------------------------------------------------------------------------
