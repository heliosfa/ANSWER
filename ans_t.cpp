//------------------------------------------------------------------------------

#include <stdio.h>
#include "Ans_t.h"
#include "event_t.h"
#include "drawpix_t.h"
#include <fstream>

//==============================================================================

void eDecoder(void *,EvPump<EvTime_t>::BASE_EVENT *);
void TidyUp(void *,EvPump<EvTime_t>::BASE_EVENT *);
//void Stop(){EvPump<EvTime_t>::Stop();}

//==============================================================================

Ans_t::Ans_t(int argc,char * argv[])
{
printf("---------------------------\n");
printf("[Prologue ... \n");
t0 = mTimer();                         // Start the prologue wallclock
pEv = 0;                               // In case the parser chokes
Cct.Parse(string(argv[1]));            // Parse the circuit file
pEv = new EvPump<EvTime_t>(this);      // Create event pump
Event_t::pEvPump = pEv;                // Tell all the events
pEv->Order();                          // Pump is ordered, not a FIFO
pEv->Set_CB(eDecoder);                 // Event callback function
MonList(argc,argv);                    // Tag devices to monitor
PrimeQ();                              // Prime the event queue
t0 = mTimer(t0);                       // End of initialisation
printf("... prologue done in %ld wall ms]\n\n",t0);
printf("[Simulation ... \n");
t0 = mTimer();                         // Start simulation wallclcok
int code = pEv->Go();                  // And do it.....
t0 = mTimer(t0);                       // End of time
printf("Event pump stop code %d (%s)\n",code,EvPump<EvTime_t>::STOP_str[code]);
printf("... simulation done in %ld wall ms]\n\n",t0);

//Cct.Dump();

double tE = double(t0)/1000.0;         // Maximum sim time
printf("[Epilogue ... \n");
t0 = mTimer();                         // Start epilogue wallclcok
pEv->Purge(TidyUp);                    // We could stop for all sorts of reasons
Cct.OutAns();                          // Write the output files
OutAns(Cct.Name(),tE);                 // Write local statistics file
DrawPix_t DrawPix(Cct);                // Draw the pictures
t0 = mTimer(t0);
printf("... epilogue done in %ld wall ms]\n\n",t0);
printf("---------------------------\n");
}

//------------------------------------------------------------------------------

Ans_t::~Ans_t()
{
if (pEv!=0) delete pEv;                // Lose the event pump
}

//------------------------------------------------------------------------------

void Ans_t::Dump()
{
printf("----------------------------------------------------\n"
       "Ans_t dump:\n");
printf("  Circuit: %p\n",&Cct);
Cct.Dump();
printf("  Event pump: %p\n",pEv);
if (pEv!=0) pEv->Dump();
printf("  Static parameters:\n");
printf("    Wallclock %u\n",t0);
//printf("    Monitor list\n");
//WALKVECTOR(string,vMon,i) printf("%s\n",(*i).c_str());
printf("    Local history:\n");
WALKVECTOR(Hist_t,vHist,i) (*i).Dump();
printf("----------------------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void Ans_t::MonList(int argc,char * argv[])
// Extract (parse?) the list of devices to be monitored
{
if (argc<=2) return;                   // There are none
Lex Lx;
Lx.SetFile(argv[2]);                   // Heave the Lexer out of bed
Cct.sMon = 0;
for(;;) {
  Lex::tokdat Td;
  Lx.GetTokNC(Td);
  if (Td.t == Lex::Sy_STR) {           // If it's a string, process it
//vMon.push_back(Td.s);
    Device_t ** ppD = Cct.G.FindNode(Td.s);
    if (ppD==0) continue;              // Not in circuit?
    (*ppD)->fmon = true;               // Yup; set "we want to know" flag
    Cct.sMon++;                        // Increment "monitored device" counter
  }
  if (Td.t == Lex::Sy_EOF) break;      // Time to go...
}
Lx.SetFile();                          // Close the file
printf("%u devices to be monitored in %s\n",Cct.sMon,argv[2]);
}

//------------------------------------------------------------------------------

void Ans_t::OutAns(string n,double tE)
{
string fname = "ANSWER_" + n + ".ans";
/*
FILE * fp = fopen(fname.c_str(),"w");
fprintf(fp,"# ANSWER simulation statistics\n",fname.c_str());
fprintf(fp,"#  Wall        SimTim         Q size     Event rate\n");
fprintf(fp,"# (secs)       (secs)         (events)   (events/sec)\n");
WALKVECTOR(Hist_t,vHist,i)
  fprintf(fp,"  %e %e  %5u %e\n",double((*i).wt)/1000.0,(*i).st,(*i).qs,(*i).er);
fclose(fp);
*/
                                       // Gnuplot file
ofstream fs;
fs.open(fname.c_str());
fs << "# ANSWER simulation statistics\n";
fs << "#  Wall        SimTim         Q size     Event rate\n";
fs << "# (secs)       (secs)         (events)   (events/sec)\n";
WALKVECTOR(Hist_t,vHist,i)
  fs << double((*i).wt)/1000.0 <<' '<<(*i).st<<' '<<(*i).qs<<' '<<(*i).er<<'\n';
fs.close();

unsigned ec,em;                        // Statistics for the user
pEv->Stats(ec,em);
EvTime_t lpt = pEv->Lpt();
cout << "\nLast popped time " << lpt << '\n';    
printf("Simulation [0:%e] sim secs\n\n",Cct.Stop());
if (tE>1e-12) printf("Events:\n  Total  %12u (%9.2e /wall secs) \n  MaxQ   %12u"
                     "\n  Clock  %12u (%9.2e /wall secs)\n  Source %12u\n"
                     "  Neuron %12u (%9.2e /wall secs)\n",
                     ec,double(ec)/tE,em,stats.n_Clk,double(stats.n_Clk)/tE,
                     stats.n_Sou,stats.n_Neu,double(stats.n_Neu)/tE);
else          printf("Events:\n  Total  %12u (*** /wall secs) \n  MaxQ   %12u"
                     "\n  Clock  %12u (*** /wall secs)\n  Source %12u\n"
                     "  Neuron %12u (*** /wall secs)\n",
                     ec,em,stats.n_Clk,stats.n_Sou,stats.n_Neu);
}

//------------------------------------------------------------------------------

void Ans_t::PrimeQ()
// One-off routine to prime the event queue. Walk all the sources and all the
// clocks and extract the firstest ever event from them.
{
WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,Cct.G,i) {
  Event_t * Ev1 = Cct.G.NodeData(i)->Get1();
  if (Ev1==0) continue;
  pEv->Inject(Ev1);
}

}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

Ans_t::Hist_t::Hist_t(long _wt,EvTime_t _st,unsigned _qs,double _er)
{
wt     = _wt;
st     = _st;
qs     = _qs;
er     = _er;
}

//------------------------------------------------------------------------------

void Ans_t::Hist_t::Dump()
{
printf("Wallclock time = %ul\n",wt);
printf("Simulated time = "); cout << time; printf("\n");
printf("Queue size     = %u\n",qs);
printf("Event rate     = %e\n",er);
}

//==============================================================================

void eDecoder(void * par,EvPump<EvTime_t>::BASE_EVENT * pB)
// Callback routine to decode the destination of events popped from the queue.
// In this application, we have three types: timer tick, source event and
// MC packet.
{
if (pB==0) throw(E(__FILE__,__LINE__));// Dud event ?
                                       // Access the parent process
Ans_t * pAns = static_cast<Ans_t *>(par);
Event_t * pE = dynamic_cast<Event_t *>(pB);
if (pE==0) throw(E(__FILE__,__LINE__));
EvTime_t ts = pB->Time();              // Stop time reached?
if (ts > pAns->Cct.Stop())pAns->pEv->Stop();
switch (pB->Type()) {                  // Decode the event on the basis of
                                       // source device
  case C_clock::eType  : {             // It's a clock tick
    WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,
                      string,C_pin *,pAns->Cct.G,i) {
      Device_t * pD = pAns->Cct.G.NodeData(i);
                                       // Kick everything except sources
      if (dynamic_cast<C_source *>(pD)!=0) continue;
      pD->OnTick(pE);
    }
    delete pB;                         // Kill the event
    pAns->stats.n_Clk++;               // Statistics
    break;
  }
  case C_source::eType :               // Source kicks itself
    static_cast<C_source *>(pB->Tag())->OnImpulse(pE);
    pAns->stats.n_Sou++;               // Statistics
    pAns->stats.n_Neu--;               // Avoid double count on the drop-through
  case C_neuron::eType : {             // It's a AP
    void * p = pB->Tag();              // Source device
    string src = static_cast<C_neuron *>(p)->Name();
    vector<string> tgts;               // Vector of target devices
    pAns->Cct.G.DrivenNodes(src,tgts); // Load it
    WALKVECTOR(string,tgts,i) {      // Walk it, delivering impulses
      C_neuron * pN = dynamic_cast<C_neuron *>(*(pAns->Cct).G.FindNode(*i));
      if (pN!=0) pN->OnImpulse(pE);
      C_sink * pS = dynamic_cast<C_sink *>(*(pAns->Cct).G.FindNode(*i));
      if (pS!=0) pS->OnImpulse(pE);
    }
    delete pB;                         // Kill the event
    pAns->stats.n_Neu++;               // Statistics
    break;
  }
  default              : throw(E(__FILE__,__LINE__));
}
// Gather more statistics:

//static unsigned ecnt = 0;
//static long tprev = 0;
double evrate = 0.0;
//unsigned N = 100;
/*
if (++ecnt%N == 0) {            // Every N events popped:
  long now = mTimer();
  double dt = (now - tprev)/1000.0;
  evrate = 0.0;
  if (dt!=0) evrate = double(N)/dt;
  printf("now=%ld tprev=%ld evrate=%e dt=%e\n",now,tprev,evrate,dt);
  tprev = now;
}                    */
                                       // Save the statistics
pAns->vHist.push_back(Ans_t::Hist_t(mTimer(pAns->t0),ts,pAns->pEv->Size(),evrate));
}

//------------------------------------------------------------------------------

void TidyUp(void * pv,EvPump<EvTime_t>::BASE_EVENT * pBE)
// Tidy up the event pump if it's stopped before it's empty
{
//printf("                   delete %x\n",pBE);
delete pBE;
}

//==============================================================================

