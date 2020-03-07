//------------------------------------------------------------------------------

#ifndef __Circuit_t__H
#define __Circuit_t__H

#include "lex.h"
#include "e.h"
#include "device_t.h"
#include "flat.h"
#include "pdigraph.hpp"
#include "filename.h"
#include <string>
using namespace std;

//==============================================================================
      
class Circuit_t
{
public:
                             Circuit_t();
virtual ~                    Circuit_t();

void                         ChkInteg();
void                         Dump();
string                       Name() { return name; }
void                         OutAns();
void                         Parse(string);
void                         ParseClock();
void                         ParseNeuron(Lex::tokdat);
void                         ParseParams();
void                         ParseSink();
void                         ParseSource();
void                         ParseStop();
EvTime_t                     Stop() { return stop; }

unsigned                     sMon;     // Number of devices to be monitored
EvTime_t                     stop;     // Simulation stop time
string                       name;
Lex                          Lx;
pdigraph<string,Device_t *,string,Device_t *,string,C_pin *> G;
map<string,vector<string> >  params;
};

//==============================================================================

#endif
