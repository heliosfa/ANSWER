//------------------------------------------------------------------------------
    
#include "drawpix_t.h"
#include "filename.h"
#include "macros.h"
#include "e.h"

//------------------------------------------------------------------------------

DrawPix_t::DrawPix_t(Circuit_t & rCct)
{

//rCct.Dump();

string c_name = rCct.name;
if (c_name.empty()) return;            // Paranoia......
FileName cmnd(c_name);                 // Build the GnuPlot command file name
cmnd.FNExtn("gpc");                    // Change extension to ".gpc"
FileName jpg(c_name);                  // Build the output file name
jpg.FNExtn("jpg");                     // Change extension to ".jpg"
string sfile = "ANSWER_" + cmnd.FNBase() + ".ans";
                                       // Write the GnuPlot command file
FILE * gf = fopen(cmnd.FNComplete().c_str(),"w");
if (gf==0) throw(E(__FILE__,__LINE__));

fprintf(gf,"set t jpeg size 1600,1000\n");
fprintf(gf,"set o '%s'\n",jpg.FNComplete().c_str());
fprintf(gf,"set linetype 1  lc rgb 'red'           lw 1 pt 0       \n");
fprintf(gf,"set linetype 2  lc rgb 'green'         lw 1 pt 0       \n");
fprintf(gf,"set linetype 3  lc rgb 'blue'          lw 1 pt 0       \n");
//fprintf(gf,"set linetype 4  lc rgb 'red'           lw 1 pt 0       \n");
//fprintf(gf,"set linetype 5  lc rgb 'green'         lw 1 pt 0       \n");
//fprintf(gf,"set linetype 6  lc rgb 'blue'          lw 1 pt 0       \n");
fprintf(gf,"set linetype 4  lc rgb 'dark-violet'   lw 1 pt 0       \n");
fprintf(gf,"set linetype 5  lc rgb 'sea-green'     lw 1 pt 0       \n");
fprintf(gf,"set linetype 6  lc rgb 'royalblue'     lw 1 pt 0       \n");
fprintf(gf,"set linetype 7  lc rgb 'magenta'       lw 1 pt 0       \n");
fprintf(gf,"set linetype 8  lc rgb 'orange-red'    lw 1 pt 0       \n");
fprintf(gf,"set linetype 9  lc rgb 'dark-orange'   lw 1 pt 0       \n");
fprintf(gf,"set linetype 10 lc rgb 'black'         lw 1 pt 0       \n");
fprintf(gf,"set linetype 11 lc rgb 'goldenrod'     lw 1 pt 0       \n");
fprintf(gf,"set linetype cycle 11                                  \n");

fprintf(gf,"set style line 1 lt 0 lw 1 lc rgb 'green' pt 0 \n");
fprintf(gf,"set style line 2 lt 0 lw 1 lc rgb 'red'   pt 0 \n");
fprintf(gf,"set style line 3 lt 0 lw 1 lc rgb 'blue'  pt 0 \n");
fprintf(gf,"set style line 4 lt 1 lw 2 lc rgb 'green' pt 0 \n");
fprintf(gf,"set style line 5 lt 1 lw 2 lc rgb 'red'   pt 0 \n");
fprintf(gf,"set style line 6 lt 1 lw 2 lc rgb 'blue'  pt 0 \n");

fprintf(gf,"set multiplot title 'ANSWER run: [%s] '"
           "font 'Arial, 16' layout 2,2 offset 0,0\n",
            c_name.c_str());
fprintf(gf,"set key off\n");
fprintf(gf,"set grid\n");
fprintf(gf,"set xtics\n");
fprintf(gf,"set y2tics tc rgb 'green'\n");
fprintf(gf,"set ytics tc rgb 'red'\n");
fprintf(gf,"set ytics nomirror\n");
fprintf(gf,"set xrange[0:*]\n");
fprintf(gf,"set yrange[0:*]\n");
fprintf(gf,"set y2range[0:*]\n");

// Graph at (1,1)
fprintf(gf,"set timestamp \"%%H:%%M:%%S %%a %%b %%d %%Y\"\n");
fprintf(gf,"set title 'Wallclock simulation performance' "
           "font 'Arial,14' offset 0.0,-1\n");
fprintf(gf,"set xlabel 'Wallclock time (s)'\n");
fprintf(gf,"set ylabel 'Simulated time (s)' tc rgb 'red' offset 2,0\n");
fprintf(gf,"set y2label 'Event queue size' tc rgb 'green' offset -2,0\n");
fprintf(gf,"plot '%s' u 1:2 w lines, ",sfile.c_str());
fprintf(gf,"'' u 1:3 w steps axes x1y2\n");
fprintf(gf,"unset timestamp\n");

// Graph at (2,1)
fprintf(gf,"set title 'Simulation time performance' "
           "font 'Arial,14' offset 0.0,-1\n");
fprintf(gf,"set xlabel 'Simulated time (s)'\n");
fprintf(gf,"set ylabel 'Wallclock time (s)' tc rgb 'red' offset 2,0\n");
fprintf(gf,"plot '%s' u 2:1 w lines, ",sfile.c_str());
fprintf(gf,"'' u 2:3 w steps axes x1y2\n");

// Graph at (1,2)
fprintf(gf,"set origin 0.03,0\n");
fprintf(gf,"set size 0.425,0.5\n");
double dy = 2.1;
double doff = 0.0;
double magn = 0.5;
if (rCct.sMon!=0) {
  fprintf(gf,"set title 'Spiking' font 'Arial,14' offset 0.0,-1\n");
  fprintf(gf,"set xlabel 'Simulated time (s)'\n");
  fprintf(gf,"set ylabel 'Neuron' tc rgb 'black' offset -6.5,0\n");
  fprintf(gf,"unset ytics\n");
  fprintf(gf,"unset y2tics\n");
  fprintf(gf,"unset y2label\n");
  unsigned lab  = 1;
  WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,rCct.G,i) {
    Device_t * pD = rCct.G.NodeData(i);
    if (pD==0) continue;
    if (!pD->fmon) continue;
    fprintf(gf,"set label %u '%s' at first 0.0,%e offset -7.5,0 font 'Arial,8'\n",
            lab++,pD->name.c_str(),doff+=dy);
  }
//  fprintf(gf,"set yrange[0:%u]\n",rCct.sMon+3);
  fprintf(gf,"set yrange[0:*]\n");
  doff = 0.0;
  lab = 0;
  fprintf(gf,"plot ");
  WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,rCct.G,i) {
    Device_t * pD = rCct.G.NodeData(i);
    if (pD==0) continue;
    if (!pD->fmon) continue;
    string name = c_name + "\\" + pD->name + "_" + c_name + ".ans";
    string eol = (++lab==rCct.sMon) ? "" :", \\";
    fprintf(gf,"'%s' u 1:(($2)*%e)+%e w steps%s\n",name.c_str(),magn,doff+=dy,eol.c_str());
  }
}
// Graph at (2,2)
fprintf(gf,"set size 0.425,0.5\n");
if (rCct.sMon!=0) {
  fprintf(gf,"set title 'Integrand' font 'Arial,14' offset 0.0,-1\n");
  fprintf(gf,"set xlabel 'Simulated time (s)'\n");
  fprintf(gf,"set ylabel 'Neuron' offset -6.5,0\n");
  fprintf(gf,"set yrange[0:*]\n");
  doff = 0.0;
  unsigned lab = 0;
  fprintf(gf,"plot ");
  WALKPDIGRAPHNODES(string,Device_t *,string,Device_t *,string,C_pin *,rCct.G,i) {
    Device_t * pD = rCct.G.NodeData(i);
    if (pD==0) continue;
    if (!pD->fmon) continue;
    string name = c_name + "\\" + pD->name + "_" + c_name + ".ans";
    string eol = (++lab==rCct.sMon) ? "" :", \\";
    fprintf(gf,"'%s' u 1:(($3)*%e)+%e w lines%s\n",name.c_str(),magn,doff+=dy,eol.c_str());
  }
}

fprintf(gf,"unset key\n");
fprintf(gf,"unset multiplot\n");
fprintf(gf,"exit\n");
fclose(gf);
string sss = "call D:\\Apps\\GnuPlot\\gnuplot\\bin\\gnuplot.exe " +
             cmnd.FNComplete();
int code = system(sss.c_str());
printf("\nCall to system GnuPlot %s \n",code==0 ? "OK" : "failed");
printf("GnuPlot stuff complete\n");

}

//------------------------------------------------------------------------------

DrawPix_t::~DrawPix_t()
{
}

//------------------------------------------------------------------------------

