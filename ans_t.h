//------------------------------------------------------------------------------

#ifndef __Ans_t__H
#define __Ans_t__H

#include "evpump.hpp"
#include "circuit_t.h"

#include <string>
using namespace std;

//==============================================================================

class Ans_t
{
public:
                     Ans_t(int,char* []);
virtual ~            Ans_t();
 
void                 Dump();
void                 MonList(int,char **);
void                 OutAns(string,double);
void                 PrimeQ();

Circuit_t            Cct;              // Circuit under simulation
EvPump<EvTime_t> *   pEv;              // Event pump
long                 t0;               // Wallclock
//vector<string>       vMon;             // Devices to be monitored

struct stats_t {
  stats_t():n_Clk(0),n_Sou(0),n_Neu(0){}
  unsigned n_Clk;                      // Clock pulses
  unsigned n_Sou;                      // Source pulses
  unsigned n_Neu;                      // Neuron events
} stats;

struct Hist_t {
  Hist_t(long,EvTime_t,unsigned,double);
  void     Dump();
  long     wt;                         // Wallclock time
  EvTime_t st;                         // Simulated time
  unsigned qs;                         // Queue size
  double   er;                         // Event handling rate
};
vector<Hist_t> vHist;                  // Event history store

};

//==============================================================================

#endif
