#ifndef __DrawPix_tH__H
#define __DrawPix_tH__H

#include "circuit_t.h"
#include <string>
#include <vector>
using namespace std;

//==============================================================================
#ifdef _WIN32
#define GNUPLOT_BIN_DEFAULT  "gnuplot.exe"
#else
#define GNUPLOT_BIN_DEFAULT  "gnuplot"
#endif

#ifndef GNUPLOT_BIN
#define GNUPLOT_BIN     GNUPLOT_BIN_DEFAULT
#endif

class DrawPix_t
{
public:
                      DrawPix_t(Circuit_t &);
virtual ~             DrawPix_t();

};

//==============================================================================

#endif




