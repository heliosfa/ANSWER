//------------------------------------------------------------------------------

#include <stdio.h>
#include "device_t.h"
#include "circuit_t.h"

//==============================================================================

Circuit_t * Device_t::par   = 0;
//void (* C_sink::pStop)() = 0;

//==============================================================================

Device_t::Device_t():fmon(false)
{
}

//------------------------------------------------------------------------------

Device_t::Device_t(string & s):fmon(false),name(s)
{
}

//------------------------------------------------------------------------------

void Device_t::Dump()
{
printf("----------------------------------------------------\n"
       "Base class: Device_t %s dump:\n",name.c_str());
printf("Monitor flag         %s\n",fmon ? "set" : "unset");
printf("----------------------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void Device_t::ND_cb(Device_t * const & pD)
{
if (pD==0) {
  printf("No name ");
  return;
}
printf("[%s(%s)]",pD->Name().c_str(),pD->sParam.c_str());
}

//------------------------------------------------------------------------------

void Device_t::NK_cb(string const & s)
{
printf("%s",s.c_str());
}

//==============================================================================

C_neuron::C_neuron(string n,string p)
{
sParam   = p;
name     = n;
//printf("Creating neuron %s(%s)\n",n.c_str(),p.c_str());
S        = 0.0;
Tr       = EvTime_t(0);
Lnt      = EvTime_t(0);
p_thresh = 0.0;
p_refrac = 0;
p_leak   = 0.0;
p_delay  = EvTime_t(0);

ftf      = true;
//vHist.reserve(64000);
}

//------------------------------------------------------------------------------

C_neuron::~C_neuron()
{
}

//------------------------------------------------------------------------------

void C_neuron::Dump()
{
Device_t::Dump();
printf("----------------------------------------------------\n"
       "Neuron %s dump:\n",name.c_str());
printf("  Address %p\n",this);
printf("  Parameter set name %s\n",sParam.c_str());
printf("  Static parameters:\n");
printf("    Model identifier                %u\n",p_model);
printf("    Output event height             %e\n",p_h);
printf("    Firing threshold                %e\n",p_thresh);
printf("    Refractory length               %e\n",p_refrac);
printf("    Geometric integrand leak factor %e\n",p_leak);
printf("    Output delay                    %e\n",p_delay);
printf("  Current state variables:\n");
printf("    Integrand                       %e\n",S);
printf("    Refractory to go                %e\n",Tr);
printf("    Last notified time              %e\n",Lnt);
printf("  ... Temperature device              \n");
printf("  First time flag                   %c\n",ftf?'T':'F');
printf("  Ghost map size                    %u\n",gh.size());
WALKMAP(void *,unsigned,gh,i) printf("%p(%s):%u\n",
         (*i).first,(((Device_t *)((*i).first))->Name().c_str()),(*i).second);
printf("----------------------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void C_neuron::Elaborate()
// Hoik a copy of the parameters up from the parameter bank
{
if (par->params[sParam].size()<6) throw(E(__FILE__,__LINE__));
p_model  = str2uint(par->params[sParam][0]);
p_h      = str2dble(par->params[sParam][1]);
p_thresh = str2dble(par->params[sParam][2]);
p_refrac = str2dble(par->params[sParam][3]);
p_leak   = str2dble(par->params[sParam][4]);
p_delay  = str2dble(par->params[sParam][5]);
}

//------------------------------------------------------------------------------

void C_neuron::OnImpulse(Event_t * pE)
// Choose your model....
{
switch (p_model) {
  case 0  :            return;
  case 1  : OnI_1(pE); return;
  case 2  : OnI_2(pE); return;
  case 11 : OnI_T(pE); return;
  case 12 : OnI_C(pE); return;
  default :            return;
}
}

//------------------------------------------------------------------------------

void C_neuron::OnI_1(Event_t * pE)
// Type 1 OnImpulse handler: requires the co-existance of a clock tick.
// Very simple: integrates the incoming spikes and fires if necessary.
{
if (Tr > 0.0) return;                  // If we're still resting.....
S += pE->h;                            // Integrate incoming spike and store
Save(pE->Time(),0.0,S,Tr);
if (S > p_thresh) {                    // Time to fire?
  Event_t * p0 = new Event_t();        // Create new event
  p0->Time(pE->Time()+p_delay);        // ... at some time in the future
  Tr = p_refrac+p_delay;               // Set refractory timer
  Save(p0->Time(),p_h,0.0,p_refrac);   // Write the outgoing pulse
  p0->Tag(this);                       // Where it's come from
  p0->Type(eType);                     // Event type
  p0->h = p_h;                         // Spike height
  pE->pEvPump->Inject(p0);             // Go
  S = 0.0;                             // Reset integrand
}

}

//------------------------------------------------------------------------------

void C_neuron::OnI_2(Event_t * pE)
// Type 2 OnImpulse handler: requires no clock BUT it can result in the history
// vector getting loaded out of simulation time order; hence we need to sort
// it before we write it.
{
EvTime_t dt = pE->Time() - Lnt;        // Time since last touched
Lnt = pE->Time();                      // Update last notified time
Tr -= dt;                              // Update any remaining refractory period
S *= exp(-p_leak*dt);                  // Leak the integrand
if (Tr > EvTime_t(0)) {                // If we're still resting.....
 Save(pE->Time(),0.0,S,Tr);
 return;
}
Tr = EvTime_t(0);                      // No; we're in business
S += pE->h;                            // Add the spike
if (S > p_thresh) {                    // Gonna fire?
  Tr = p_refrac;                       // Refractory period
  Event_t * p0 = new Event_t();        // New event
  p0->Time(pE->Time()+p_delay);        // New spike
  Save(pE->Time(),0.0,S,Tr);
  Save(p0->Time(),p_h,0.0,Tr-p_delay);
  p0->Tag(this);                       // Housekeeping
  p0->Type(eType);
  p0->h = p_h;
  pE->pEvPump->Inject(p0);             // Kick it, Winston
  S = 0.0;                             // Reset integrand
}
else Save(Lnt,0.0,S,Tr);
}

//------------------------------------------------------------------------------

void C_neuron::OnI_C(Event_t * pE)
// Type C OnImpulse handler: models the "Clamp" device
{
Dump();
pE->pEvPump->Dump();
                                       // Define output packet:
Event_t * p0 = new Event_t();          // New event
p0->Time(pE->Time()+p_delay);          // New time
p0->Tag(this);                         // New ID
p0->Type(eType);                       // Not a source
p0->h = p_h;
pE->pEvPump->Inject(p0);               // Push back
}

//------------------------------------------------------------------------------

void C_neuron::OnI_T(Event_t * pE)
// Type T OnImpulse handler: models the "Temperature" device
{
Dump();
pE->pEvPump->Dump();
                                       // If not TDevice, must be source
if (pE->Type()==eType) gh[pE->Tag()] = unsigned(pE->h);
unsigned Snew = 0;                     // The temperature to be....
WALKMAP(void *,unsigned,gh,i) Snew += (*i).second;
Snew /= 4;
                                       // Define output packet:
Event_t * p0 = new Event_t();          // New event
p0->h = (double)Snew;                  // New temperature
p0->Time(pE->Time()+p_delay);          // New time
p0->Tag(this);                         // New ID
p0->Type(eType);                       // Not a source
S = p0->h;                             // Store the new temperature

if(ftf||(unsigned(S)!=unsigned(p0->h)))// If first time OR temperature changed
  pE->pEvPump->Inject(p0);             // broadcast
else delete p0;
ftf = false;                           // Not first time any more
}

//------------------------------------------------------------------------------

void C_neuron::OnTick(Event_t * pE)
{
EvTime_t dt = pE->Time() - Lnt;        // Distance since last event
Tr -= dt;                              // Decrease unexpired refractory period
if (Tr < 0.0) Tr = 0.0;                // Refractory period expired
Lnt = pE->Time();                      // Last notified time

if (S>0.0) {
  S *= exp(-p_leak*dt);
 // S *= pow(p_leak,dt);                   // Handle leaky integrator
//  printf("S=%e p_leak=%e dt=%e pow()=%e\n",S,p_leak,dt,pow(p_leak,dt));
}
//vHist.push_back(Hist_t(Lnt,0.0,S,Tr)); // Store the new device state
Save(Lnt,0.0,S,Tr);
}

//------------------------------------------------------------------------------

struct lt_Hist_t
// The .LT. operator for the sort() in C_neuron::OutAns() below
{
bool operator()(C_neuron::Hist_t pa,C_neuron::Hist_t pb) const
{
return pa.t < pb.t;
}
};

//------------------------------------------------------------------------------

void C_neuron::OutAns(string & dir)
// Can't make this virtual because the definition of Hist_t is different in
// each derived class
{
if (!fmon) return;                     // Don't want to save it
                                       // Shove it in the subdirectory
string fname = dir + "\\" + name + "_" + par->name + ".ans";
FILE * fp = fopen(fname.c_str(),"w");
fprintf(fp,"# NEURON %s\n",fname.c_str());
fprintf(fp,"# Time       Pulse        Integrand    Refractory\n");
                                       // Fast model can produce out-of-order
                                       // results and confuse GnuPlot
sort(vHist.begin(),vHist.end(),lt_Hist_t());
                                       // Copy buffer to output file
WALKVECTOR(Hist_t,vHist,i) (*i).OutAns(fp);
fclose(fp);
}

//------------------------------------------------------------------------------

void C_neuron::Save(EvTime_t Lnt,double h,double S,EvTime_t Tr)
{
if (!fmon) return;                     // Don't want to save it
vHist.push_back(Hist_t(Lnt,h,S,Tr));
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

C_neuron::Hist_t::Hist_t(EvTime_t _t,double _h,double _S,double _Tr)
{
t  = _t;
h  = _h;
S  = _S;
Tr = _Tr;
}

//------------------------------------------------------------------------------

void C_neuron::Hist_t::OutAns(FILE * fp)
{
if (Tr>=EvTime_t(0)) fprintf(fp,"%e %e %e %e\n",t,h,S,double(Tr));
else               fprintf(fp,"%e %e %e 0.0\n",t,h,S);
}

//==============================================================================

C_source::C_source(string n,string p)
{
sParam   = p;
name     = n;
//printf("Creating source %s(%s)\n",n.c_str(),p.c_str());
cur_t    = 0;
c_minor  = 0;
c_major  = 0;
}

//------------------------------------------------------------------------------

C_source::~C_source()
{
}

//------------------------------------------------------------------------------

void C_source::Dump()
{
printf("----------------------------------------------------\n"
       "Source %s dump:\n",name.c_str());
printf("  Address %p\n",this);
printf("  Parameter set name %s\n",sParam.c_str());
printf("  Static parameters:\n");
printf("    Event height                    %e\n",p_h);
printf("    Start offset                    %e\n",t_start);
printf("    Minor period                    %e\n",t_minor);
printf("    Minor event count               %u\n",n_minor);
printf("    Major period                    %e\n",t_major);
printf("    Major event count               %u\n",n_major);
printf("  Current state variables:\n");
printf("    Last notified time              %e\n",cur_t);
printf("    Current minor count             %u\n",c_minor);
printf("    Current major count             %u\n",c_major);
printf("----------------------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void C_source::Elaborate()
// Hoik a copy of the parameters up from the parameter bank
{
if (par->params[sParam].size()<6) throw(E(__FILE__,__LINE__));
p_h      = str2dble(par->params[sParam][0]);
t_start  = str2dble(par->params[sParam][1]);
t_minor  = str2dble(par->params[sParam][2]);
n_minor  = str2uint(par->params[sParam][3]);
t_major  = str2dble(par->params[sParam][4]);
n_major  = str2uint(par->params[sParam][5]);
}

//------------------------------------------------------------------------------

Event_t * C_source::Get1()
{
Event_t * pE = new Event_t();
pE->h = p_h;
pE->Time(t_start);
pE->Tag(this);
pE->Type(eType);
Save(EvTime_t(0),p_h,c_minor,c_major);
return pE;
}

//------------------------------------------------------------------------------

void C_source::OnImpulse(Event_t * pE)
// Each source gets kicked by itself - so it knows to generate the next output
{
                                       // Store incident event
vHist.push_back(Hist_t(pE->Time(),p_h,c_minor,c_major));
if (++c_minor >= n_minor) {            // Increment internal state
  c_major++;
  c_minor = 0;
}
if (c_major >= n_major) return;        // We're all done? Close clock
                                       // Compute next tick
EvTime_t t = t_start + (c_minor * t_minor) + (c_major * t_major);
Event_t * p0 = new Event_t();          // New event
p0->Time(t);                           // Time
p0->Type(eType);                       // Clock tick
p0->Tag(this);                         // Where it comes from
p0->h = p_h;                           // How big it is
pE->pEvPump->Inject(p0);               // Shove it into queue
Save(p0->Time(),p_h,c_minor,c_major);
}

//------------------------------------------------------------------------------

void C_source::OutAns(string & dir)
{
if (!fmon) return;                     // Don't want to save it
string fname = dir + "\\" + name + "_" + par->name + ".ans";
FILE * fp = fopen(fname.c_str(),"w");
fprintf(fp,"# SOURCE %s\n",fname.c_str());
fprintf(fp,"# Time       Pulse        Minor count Major count\n");
WALKVECTOR(Hist_t,vHist,i) (*i).OutAns(fp);
fclose(fp);
}

//------------------------------------------------------------------------------

void C_source::Save(EvTime_t Lnt,double h,unsigned c_min,unsigned c_maj)
{
if (!fmon) return;                     // Don't want to save it
vHist.push_back(Hist_t(Lnt,h,c_min,c_maj));
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

C_source::Hist_t::Hist_t(EvTime_t _t,double _h,unsigned _c_min,unsigned _c_maj)
{
t     = _t;
h     = _h;
c_min = _c_min;
c_maj = _c_maj;
}

//------------------------------------------------------------------------------

void C_source::Hist_t::OutAns(FILE * fp)
{
fprintf(fp,"%e %e %u %u\n",t,h,c_min,c_maj);
}

//==============================================================================

C_sink::C_sink(string n,string p)
{
sParam   = p;
name     = n;
//printf("Creating sink %s(%s)\n",n.c_str(),p.c_str());
S        = 0;
p_thresh = 0;
p_leak   = 0;
Save(EvTime_t(0),S);
}

//------------------------------------------------------------------------------

C_sink::~C_sink()
{
}

//------------------------------------------------------------------------------

void C_sink::Dump()
{
printf("----------------------------------------------------\n"
       "Sink %s dump:\n",name.c_str());
printf("  Address %p\n",this);
printf("  Parameter set name %s\n",sParam.c_str());
printf("  Static parameters:\n");
printf("    Firing threshold                %e\n",p_thresh);
printf("    Geometric integrand leak factor %e\n",p_leak);
printf("  Current state variables:\n");
printf("    Integrand                       %e\n",S);
printf("    Last notified time              %e\n",Lnt);
printf("----------------------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void C_sink::Elaborate()
// Hoik a copy of the parameters up from the parameter bank
{
if (par->params[sParam].size()<2) throw(E(__FILE__,__LINE__));
p_thresh = str2dble(par->params[sParam][0]);
p_leak   = str2dble(par->params[sParam][1]);
}

//------------------------------------------------------------------------------
  
Event_t * C_sink::Get1()
{
Save(EvTime_t(0),0.0);
return 0;
}

//------------------------------------------------------------------------------

void C_sink::OnImpulse(Event_t * pE)
// All we do is consume, and possibly set the simulator stop flag
// We use type 2 logic, because I couldn't see the point in ever using type 1
{
EvTime_t dt = pE->Time() - Lnt;        // Time since last touched
Lnt = pE->Time();                      // Update last notified time
S *= pow(p_leak,dt);                   // Leak the integrand
S += pE->h;                            // Pump the integrand
//vHist.push_back(Hist_t(Lnt,S));        // Write some output
Save(Lnt,S);
if (S>p_thresh) pE->Par()->Stop();     // Gonna fire (i.e. stop)?
//  if (pStop!=0) pStop();               // *Can* we stop?
}

//------------------------------------------------------------------------------

void C_sink::OutAns(string & dir)
{
if (!fmon) return;                     // Don't want to save it
string fname = dir + "\\" + name + "_" + par->name + ".ans";
FILE * fp = fopen(fname.c_str(),"w");
fprintf(fp,"# SINK %s\n",fname.c_str());
fprintf(fp,"# Time       State\n");
WALKVECTOR(Hist_t,vHist,i) (*i).OutAns(fp);
fclose(fp);
}

//------------------------------------------------------------------------------

void C_sink::Save(EvTime_t Lnt,double S)
{
if (!fmon) return;                     // Don't want to save it
vHist.push_back(Hist_t(Lnt,S));
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

C_sink::Hist_t::Hist_t(EvTime_t _t,double _s)
{
t     = _t;
S     = _s;
}

//------------------------------------------------------------------------------

void C_sink::Hist_t::OutAns(FILE * fp)
{
fprintf(fp,"%e %e\n",t,S);
}

//==============================================================================

C_clock::C_clock(string n,string p)
{
sParam   = p;
name     = n;
//printf("Creating clock %s(%s)\n",n.c_str(),p.c_str());
now      = 0;
p_tstart = 0;
p_tick   = 0;
p_h      = 1.0;
vHist.reserve(64000);
}

//------------------------------------------------------------------------------

C_clock::~C_clock()
{
}

//------------------------------------------------------------------------------

void C_clock::Dump()
{
printf("----------------------------------------------------\n"
       "Clock %s dump:\n",name.c_str());
printf("  Address %p\n",this);
printf("  Parameter set name %s\n",sParam.c_str());
printf("  Static parameters:\n");
printf("    Event height                    %e\n",p_h);
printf("    Start offset                    %e\n",p_tstart);
printf("    Tick period                     %e\n",p_tick);
printf("  Current state variables:\n");
printf("    Time now                        %e\n",now);
printf("----------------------------------------------------\n\n");
}

//------------------------------------------------------------------------------

void C_clock::Elaborate()
// Hoik a copy of the parameters up from the parameter bank
{
if (par->params[sParam].size()<2) throw(E(__FILE__,__LINE__));
p_tstart = str2dble(par->params[sParam][0]);
p_tick   = str2dble(par->params[sParam][1]);
}

//------------------------------------------------------------------------------

Event_t * C_clock::Get1()
// Generate and return the first clock tick from this device
{
Event_t * pE = new Event_t();
pE->Time(p_tstart);
pE->Tag(this);
pE->h = p_h;
pE->Type(eType);
return pE;
}
//------------------------------------------------------------------------------

void C_clock::OnTick(Event_t * pE)
// Each clock gets kicked by itself - so it knows to generate its next tick
{
                                       // Check it's the right clcok kicking me
if (pE->Tag()!=this) return;
double dt = 1.000001;                  // Store incident event
vHist.push_back(Hist_t(pE->Time(),pE->h));
vHist.push_back(Hist_t(dt*(pE->Time()),0.0));
EvTime_t t = pE->Time() + p_tick;      // Compute next tick
Event_t * p0 = new Event_t();          // New event
p0->Time(t);                           // Time
p0->Type(eType);                       // Clock tick
p0->Tag(this);                         // Where it comes from
p0->h = p_h;
pE->pEvPump->Inject(p0);               // Shove it into queue
}

//------------------------------------------------------------------------------

void C_clock::OutAns(string & dir)
// Can't make this virtual because the definition of Hist_t is different in
// each derived class
{
if (!fmon) return;                     // Don't want to save it
string fname = dir + "\\" + name + "_" + par->name + ".ans";
FILE * fp = fopen(fname.c_str(),"w");
fprintf(fp,"# CLOCK %s\n",fname.c_str());
fprintf(fp,"# Time       Pulse height\n");
WALKVECTOR(Hist_t,vHist,i) (*i).OutAns(fp);
fclose(fp);
}

//------------------------------------------------------------------------------
/*
void C_sink::Hist_t::OutAns(FILE * fp)
{
fprintf(fp,"%e %e\n",t,S);
}
  */
//------------------------------------------------------------------------------

void C_clock::Save(EvTime_t Lnt,double S)
{
if (!fmon) return;                     // Don't want to save it
vHist.push_back(Hist_t(Lnt,S));
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

C_clock::Hist_t::Hist_t(EvTime_t _t,double _h)
{
t     = _t;
h     = _h;
}

//------------------------------------------------------------------------------

void C_clock::Hist_t::OutAns(FILE * fp)
{
fprintf(fp,"%e %e\n",t,h);
}


//==============================================================================

