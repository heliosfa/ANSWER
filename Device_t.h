//------------------------------------------------------------------------------

#ifndef __Device_t__H
#define __Device_t__H

#include "event_t.h"
#include "flat.h"
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <algorithm>
using namespace std;
class Circuit_t;

//==============================================================================

class Device_t
{
public:
                   Device_t();
                   Device_t(string &);
virtual ~          Device_t()                     {              }

virtual void       Dump();
virtual void       Elaborate()                    {              }
virtual Event_t *  Get1()                         { return 0;    }
void               SetParam(string & s)           { sParam = s;  }
void               Name(string & s)               { name = s;    }
string             Name()                         { return name; }
static void        ND_cb(Device_t * const &);
static void        NK_cb(string const &);
virtual void       OnImpulse(Event_t *)           {              }
virtual void       OnTick(Event_t *)              {              }
virtual void       OutAns(string &)               {              }

string             sParam;
string             name;
static Circuit_t * par;
bool               fmon;               // Data to be saved?
static const unsigned    eType = 0;    // Device type

};

//==============================================================================

class C_neuron : public Device_t
{
public:
                   C_neuron(string,string=string());
virtual ~          C_neuron();

virtual void       Dump();
virtual void       Elaborate();
virtual void       OnImpulse(Event_t *);
void               OnI_1(Event_t *);
void               OnI_2(Event_t *);
void               OnI_C(Event_t *);
void               OnI_T(Event_t *);
virtual void       OnTick(Event_t *);
void               OutAns(string &);
void               Save(EvTime_t,double,double,EvTime_t);

double             S;                  // Integrand
EvTime_t           Tr;                 // Refractory period left to go
EvTime_t           Lnt;                // Last notified time

static const unsigned    eType = 3;    // Device type
unsigned           p_model;            // Internal model identifier
double             p_h;                // Event height
double             p_thresh;           // Fire threshold
EvTime_t           p_refrac;           // Length of refractory period
double             p_leak;             // Geometic integrator leak factor
EvTime_t           p_delay;            // Neuron output delay

struct Hist_t {                        // History structure
  friend struct    lt_Hist_t;
  Hist_t(EvTime_t,double,double,EvTime_t);
  void             OutAns(FILE *);
  EvTime_t         t;                  // Sim time it happened
  double           h;                  // Size of OUTGOING tonker
  double           S;                  // Consequential internal integrand
  EvTime_t         Tr;                 // Consequential refractory time to go
};
vector<Hist_t>     vHist;              // ... and this is where they live

//........... For temperature device

map<void *,unsigned> gh;
bool       ftf;


};

//==============================================================================

class C_source : public Device_t
{
public:
                   C_source(string,string=string());
virtual ~          C_source();
virtual void       Dump();
virtual void       Elaborate();
virtual Event_t *  Get1();
virtual void       OnImpulse(Event_t *);
void               OutAns(string &);
void               Save(EvTime_t,double,unsigned,unsigned);

EvTime_t           cur_t;              // Last notified time

double             p_h;                // Event height
EvTime_t           t_start;            // Start offset
EvTime_t           t_minor;            // Minor period
unsigned           n_minor;            // Minor event count
EvTime_t           t_major;            // Major period
unsigned           n_major;            // Major event count

static const unsigned    eType = 1;    // Device type
private:
unsigned           c_minor;            // Minor loop count
unsigned           c_major;            // Major loop count

struct Hist_t {
  Hist_t(EvTime_t,double,unsigned,unsigned);
  void             OutAns(FILE *);
  EvTime_t         t;
  double           h;
  unsigned         c_min;
  unsigned         c_maj;
};
vector<Hist_t>     vHist;
};

//==============================================================================

class C_sink : public Device_t
{
public:
                   C_sink(string,string=string());
virtual ~          C_sink();
virtual void       Dump();
virtual void       Elaborate();
virtual Event_t *  Get1();
virtual void       OnImpulse(Event_t *);
void               OutAns(string &);
void               Save(EvTime_t,double);

EvTime_t           Lnt;                // Last notified time
double             S;                  // Integrand
double             p_thresh;           // Fire threshold
double             p_leak;             // Geometic integrator leak factor

static const unsigned    eType = 4;    // Device type
struct Hist_t {                        // History structure
  friend struct    lt_Hist_t;
  Hist_t(EvTime_t,double);
  void             OutAns(FILE *);
  EvTime_t         t;                  // Sim time it happened
  double           S;                  // Consequential internal integrand
};
vector<Hist_t>     vHist;              // ... and this is where they live
};

//==============================================================================

class C_clock : public Device_t
{
public:
                   C_clock(string,string=string());
virtual ~          C_clock();
virtual void       Dump();
virtual void       Elaborate();
virtual Event_t *  Get1();
virtual void       OnTick(Event_t *);
void               OutAns(string &);
void               Save(EvTime_t,double);

double             p_h;                // Event height
EvTime_t           now;                // Time now
EvTime_t           p_tstart;           // Start offset
EvTime_t           p_tick;             // Tick length

static const unsigned     eType = 2;   // Device type

struct Hist_t {
  Hist_t(EvTime_t,double);
  void             OutAns(FILE *);
  EvTime_t         t;
  double           h;
};
vector<Hist_t>     vHist;
};

//==============================================================================

class C_link : public Device_t
{
public:
                   C_link(string &){}
virtual ~          C_link(){}

};

//==============================================================================

class C_pin
{
public:
                   C_pin(string &){}
virtual ~          C_pin(){}

};

//==============================================================================

#endif
