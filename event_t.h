//------------------------------------------------------------------------------

#ifndef __Event_t__H
#define __Event_t__H

#include "evpump.hpp"

typedef double EvTime_t;

//==============================================================================

class Event_t : public EvPump<EvTime_t>::BASE_EVENT
{
public:
             Event_t();
             Event_t(Event_t *);
virtual ~    Event_t();

virtual void Dump();

double                     h;
static EvPump<EvTime_t> *  pEvPump;

};

//==============================================================================

#endif
