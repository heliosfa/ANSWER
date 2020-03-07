//------------------------------------------------------------------------------

#include <stdio.h>
#include "circuit_t.h"

//==============================================================================

Circuit_t::Circuit_t()
{
stop = EvTime_t(0);
sMon = 0;
}

//------------------------------------------------------------------------------

Circuit_t::~Circuit_t()
{
                                       // Kill all the devices
WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,G,i)
  delete G.NodeData(i);
}

//------------------------------------------------------------------------------

void Circuit_t::ChkInteg()
// (Try) to make sure the circuit makes sense:
{
// 1. Device referenced but not defined?
WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,G,i) {
  Device_t * pD = G.NodeData(i);
  if (pD->sParam.empty()) throw(E(__FILE__,__LINE__));
}

printf("Neurons : %u\n",G.SizeNodes());
printf("Synapses: %u\n",G.SizeArcs());

}

//------------------------------------------------------------------------------

void Circuit_t::Dump()
{
printf("----------------------------------------\n"
       "Circuit %s dump:\n",name.c_str());
G.Dump();
printf("-------------------------\nInitial parameter sets:");
for(map<string,vector<string> >::iterator i=params.begin();i!=params.end();i++){
  printf("\nSet %s\n",(*i).first.c_str());
  WALKVECTOR(string,(*i).second,j) {
    printf("  %s\n",(*j).c_str());
  }
}
printf("-------------------------\n");

WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,G,i)
  G.NodeData(i)->Dump();
printf("Recovered stop time %e\n",stop);
printf("----------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void Circuit_t::OutAns()
// Walk the results store in each device a(as necessary) and dump it to a file
// in a scratch directory
{
string cmd = "rmdir "+name+" /s /q";   // Kill the scratch directory quietly:
printf("--> %s\n",cmd.c_str());        // Tell the user
int code = system(cmd.c_str());        // Actually do it
printf("--> System call %s\n",code==0 ? "OK" : "failed");
cmd = "mkdir " + name;                 // Make a new subdirectory
long t = Timer();
while(Timer(t)<5);                     // (Wait while the disk heads move...)
printf("--> %s\n",cmd.c_str());        // Tell the user
code = system(cmd.c_str());            // And do it
printf("--> System call %s\n",code==0 ? "OK" : "failed");

WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,G,i) {
  Device_t * pD = G.NodeData(i);
  pD->OutAns(name);
}
}

//------------------------------------------------------------------------------

void Circuit_t::Parse(string ifile)
{
FileName FN(ifile);
name = FN.FNBase();
printf("Circuit_t::Parsing %s\n",ifile.c_str());
Device_t::par = this;                  // Device->circuit backpointer
G.SetND_CB(Device_t::ND_cb);
G.SetNK_CB(Device_t::NK_cb);
G.SetAD_CB(Device_t::ND_cb);
G.SetAK_CB(Device_t::NK_cb);
Lx.SetFile((char *)ifile.c_str());     // Point the lexer at it
                                       // And away we go...
for(;;) {
  Lex::tokdat Td;
  Lx.GetTokNC(Td);                     // Get the next token...
  switch (Td.t) {
    case Lex::Sy_EOR  :                  continue;
    case Lex::Sy_EQ   : ParseStop();     break;
    case Lex::Sy_dqut :
    case Lex::Sy_STR  : ParseNeuron(Td); break;
    case Lex::Sy_LT   : ParseSink();     break;
    case Lex::Sy_GT   : ParseSource();   break;
    case Lex::Sy_mult : ParseClock();    break;
    case Lex::Sy_plng : ParseParams();   break;
    case Lex::Sy_EOF  :                  break;
    default           : Td.Dump();
                        throw(E(__FILE__,__LINE__));
  }
  if (Td.t==Lex::Sy_EOF) break;
}
Lx.SetFile();                          // Close the circuit file
ChkInteg();                            // Check circuit integrity
                                       // Load parameters from parameter library
WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,G,i)
  G.NodeData(i)->Elaborate();
}

//------------------------------------------------------------------------------

void Circuit_t::ParseClock()
// Parse the rest of the clock record
{
Lex::tokdat Td;
Lx.GetTokNC(Td);                       // Next token
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
string name = Td.s;                    // Device name
//printf("Circuit_t::ParseClock %s\n",name.c_str());
Lx.GetTokNC(Td);
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
string par = Td.s;                     // Parameters
                                       // Chuck away all else
while (Td.t!=Lex::Sy_EOR) Lx.GetTokNC(Td);
                                       // Already there?
if (G.FindNode(name)!=0) throw(E(__FILE__,__LINE__));
                                       // Nope - create it
G.InsertNode(name,new C_clock(name,par));
}

//------------------------------------------------------------------------------

void Circuit_t::ParseNeuron(Lex::tokdat Td)
// Parse the rest of the neuron record
{
string src = Td.s;                     // Source neuron name
//printf("Circuit_t::ParseNeuron %s\n",src.c_str());
Lx.GetTokNC(Td);                       // Next token
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
string par = Td.s;                     // Parameters
Device_t ** pSrc = G.FindNode(src);    // Already there?
                                       // Nope - create it
if (pSrc==0) G.InsertNode(src,new C_neuron(src,par));
else {                                 // Already there as a base class
  delete *pSrc;                        // Sling it out
  (*pSrc) = new C_neuron(src,par);     // Upcast
}

while (Td.t!=Lex::Sy_EOR) {
  Lx.GetTokNC(Td);
  if (Lex::IsStr(Td.t)) {              // It's a driven device
    string tgt = Td.s;                 // Target device exist?
    Device_t ** pD = G.FindNode(tgt);
                                       // No, create it
    if (pD==0) G.InsertNode(tgt,new Device_t(tgt));
    string sarc = UniS("axon_");
    string p_fr = UniS("p_fr");
    string p_to = UniS("p_to");
    G.InsertArc(sarc,src,tgt);
  }
}

}

//------------------------------------------------------------------------------

void Circuit_t::ParseParams()
// And the rest of the parameter record
{
Lex::tokdat Td;
Lx.GetTokNC(Td);                       // Next token
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
string name = Td.s;                    // Model name
//printf("Circuit_t::ParseParams %s\n",name.c_str());
string par;                            // Parameter name
string sign;
while (Td.t!=Lex::Sy_EOR) {            // Loop through the rest of the record
  Lx.GetTokNC(Td);
  if (Td.t==Lex::Sy_sub) sign = "-";   // Handle negative strings
  if (Lx.IsStr(Td.t)) {
    par = sign + Td.s;
    sign.clear();
    params[name].push_back(par);
  }
}
}

//------------------------------------------------------------------------------

void Circuit_t::ParseSink()
// Just the same - almost - as a neuron
{
Lex::tokdat Td;
Lx.GetTokNC(Td);                       // Pull in sink name
string sink = Td.s;                    // Sink name
//printf("Circuit_t::ParseSink %s\n",sink.c_str());
Lx.GetTokNC(Td);                       // Pull in parameter pack
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
string par = Td.s;                     // Parameters
Device_t ** pSink = G.FindNode(sink);  // Already there?
                                       // Nope - create it
if (pSink==0) G.InsertNode(sink,new C_sink(sink,par));
else {                                 // Already there as a base class
  delete *pSink;                       // Sling it out
  (*pSink) = new C_sink(sink,par);     // Upcast
}
                                       // Nothing else to do....
while (Td.t!=Lex::Sy_EOR) Lx.GetTokNC(Td);
}

//------------------------------------------------------------------------------

void Circuit_t::ParseSource()
// Just the same - almost - as a neuron
{
Lex::tokdat Td;
Lx.GetTokNC(Td);                       // Pull in source name
string src = Td.s;                     // Source name
//printf("Circuit_t::ParseSource %s\n",src.c_str());
Lx.GetTokNC(Td);                       // Pull in parameter pack
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
string par = Td.s;                     // Parameters
Device_t ** pSrc = G.FindNode(src);    // Already there?
                                       // Nope - create it
if (pSrc==0) G.InsertNode(src,new C_source(src,par));
else (*pSrc)->SetParam(par);

while (Td.t!=Lex::Sy_EOR) {
  Lx.GetTokNC(Td);
  if (Lex::IsStr(Td.t)) {              // It's a driven device
    string tgt = Td.s;                 // Target device exist?
    Device_t ** pD = G.FindNode(tgt);
                                       // No, create it
    if (pD==0) G.InsertNode(tgt,new Device_t(tgt));
    string sarc = UniS("axon_");
    string p_fr = UniS("p_fr");
    string p_to = UniS("p_to");
    G.InsertArc(sarc,src,tgt);
  }
}

}

//------------------------------------------------------------------------------

void Circuit_t::ParseStop()
// Uncool, but then the whole input thing is pretty crap.
// Found a stop command (==)
{
//printf("Circuit_t::ParseStop\n");
Lex::tokdat Td;
Lx.GetTokNC(Td);                       // Next token is the stop time
if (!Lex::IsStr(Td.t)) throw(E(__FILE__,__LINE__));
stop = str2dble(Td.s);                 // Stop time
while (Td.t!=Lex::Sy_EOR) Lx.GetTokNC(Td);
}

//==============================================================================

