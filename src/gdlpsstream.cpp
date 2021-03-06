/* *************************************************************************
                          gdlpsstream.cpp  -  graphic stream postscript
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002 by Marc Schellens
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"

//#include <iostream>
#include "graphicsdevice.hpp"
#include "gdlpsstream.hpp"

using namespace std;

void GDLPSStream::Init()
{
   plstream::init();
   page = 0;
}
 
void GDLPSStream::eop()
{
   if (page != 0) 
   {
     if (encapsulated) 
     {
       Warning("Warning: multi-page output violates Encapsulated PostScript specification");
     }
     else
     {
#ifdef USE_PSLIB
       Warning("Warning: multi-page PostScript not supported yet (FIXME!)");
#endif
     }
     plstream::eop();
   }
   page++;
};

bool GDLPSStream::PaintImage(unsigned char *idata, PLINT nx, PLINT ny, DLong *pos,
        DLong trueColorOrder, DLong channel) {
  //need to : check position in file Ok; update bounding box values.
  //test black and white:
  bool bw = (((*static_cast<DLongGDL*> (SysVar::D()->GetTag(SysVar::D()->Desc()->TagIndex("FLAGS"), 0)))[0] & 16) == 0); 
  long xs, ys, xo, yo;
  GDLPSStream::GetGeometry(xs, ys, xo, yo);
#define XSIZE         (int) ( pls->xlength * ( pls->xdpi / 72. ) )
#define YSIZE         (int) ( pls->ylength * ( pls->ydpi / 72. ) )
#define ENLARGE       5
#define XPSSIZE       ENLARGE * XSIZE
#define YPSSIZE       ENLARGE * YSIZE
  static DLong bitsPerPix = 8;
  
  //position of the image WILL NOT BE ACCURATE. The coordinate transforms of plplot are a mess.
  double xScale=(pls->diorot==0)?(double)YPSSIZE/(double)xs:(double)XPSSIZE/(double)xs;
  double yScale=(pls->diorot==0)?(double)XPSSIZE/(double)ys:(double)YPSSIZE/(double)ys;
  if (channel > 0) {
    cerr << "TV: Value of CHANNEL (use TRUE instead) is out of allowed range." << endl;
    return false;
  } 
  pls->bytecnt += fprintf(pls->OutFile, "\n%%BeginObject: Image\n S gsave\n");
  if (pls->diorot == 1.0) {
    pls->bytecnt += fprintf(pls->OutFile, "/ratio {%d %d div} def\n/offset {%d %d ratio mul sub 2 div} def\n %f offset %f ratio mul ratio mul add translate\n",XPSSIZE,YPSSIZE,YPSSIZE,XPSSIZE,pos[0]*xScale, pos[2]*yScale);
  } else {
    pls->bytecnt += fprintf(pls->OutFile, "%d 0 translate 90 rotate\n", XPSSIZE);
    pls->bytecnt += fprintf(pls->OutFile, "%f %f translate\n", pos[0]*xScale, pos[2]*yScale);
  }

  pls->bytecnt += fprintf(pls->OutFile, "%f %f scale\n", pos[1]*xScale, pos[3]*xScale);

#define LINEWIDTH 80

  if (trueColorOrder == 0) { 
    if  (bw) { //black and white indexed value 0->255: image
      pls->bytecnt += fprintf(pls->OutFile, "/gdlImagePixString %d string def\n", nx);
      pls->bytecnt += fprintf(pls->OutFile, "%d %d %d\n", nx, ny, bitsPerPix);

      pls->bytecnt += fprintf(pls->OutFile, "[%d 0 0 %d 0 0] {currentfile gdlImagePixString readhexstring pop} image\n", nx, ny);
      //output data in lines of LINEWIDTH chars:
      for (SizeT i = 0, k=0 ; i < nx * ny ; ++i) {
        fprintf(pls->OutFile, "%2.2X", idata[i]); k+=2; if( (k % LINEWIDTH) == 0 ) fprintf(pls->OutFile, "\n");
        }
      pls->bytecnt += (nx*ny*2 + nx*ny*2/LINEWIDTH);
    } else {
      pls->bytecnt += fprintf(pls->OutFile, "/gdlImagePixString %d string def\n", nx * 3);
      pls->bytecnt += fprintf(pls->OutFile, "%d %d %d\n", nx, ny, bitsPerPix);
      pls->bytecnt += fprintf(pls->OutFile, "[%d 0 0 %d 0 0] {currentfile gdlImagePixString readhexstring pop} false 3 colorimage\n", nx, ny);
      //output data in lines of LINEWIDTH chars:
      for (SizeT i = 0, k=0 ; i < nx * ny; ++i) {
        fprintf(pls->OutFile, "%2.2X", pls->cmap0[idata[i]].r); k+=2; if( (k % LINEWIDTH) == 0 ) fprintf(pls->OutFile, "\n");
        fprintf(pls->OutFile, "%2.2X", pls->cmap0[idata[i]].g); k+=2; if( (k % LINEWIDTH) == 0 ) fprintf(pls->OutFile, "\n");
        fprintf(pls->OutFile, "%2.2X", pls->cmap0[idata[i]].b); k+=2; if( (k % LINEWIDTH) == 0 ) fprintf(pls->OutFile, "\n");
        }
      pls->bytecnt += (nx*ny*6 + nx*ny*6/LINEWIDTH);
    }
  } else { //true color
    switch (trueColorOrder) {
      case 1:
        pls->bytecnt += fprintf(pls->OutFile, "/gdlImagePixString %d string def\n", nx * 3);
        pls->bytecnt += fprintf(pls->OutFile, "%d %d %d\n", nx, ny, bitsPerPix);
        pls->bytecnt += fprintf(pls->OutFile, "[%d 0 0 %d 0 0] {currentfile gdlImagePixString readhexstring pop} false 3 colorimage\n", nx, ny);
        break;
      case 2:
        pls->bytecnt += fprintf(pls->OutFile, "/gdlImagePixStringR %d string def /gdlImagePixStringG %d string def /gdlImagePixStringB %d string def\n", nx, nx, nx);
        pls->bytecnt += fprintf(pls->OutFile, "%d %d %d\n", nx, ny, bitsPerPix);
        pls->bytecnt += fprintf(pls->OutFile, "[%d 0 0 %d 0 0] {currentfile gdlImagePixStringR readhexstring pop} {currentfile gdlImagePixStringG readhexstring pop} {currentfile gdlImagePixStringB readhexstring pop} true 3 colorimage\n", nx, ny);
        break;
      case 3:
        pls->bytecnt += fprintf(pls->OutFile, "/gdlImagePixStringR %d string def /gdlImagePixStringG %d string def /gdlImagePixStringB %d string def\n", nx*ny, nx*ny, nx*ny);
        pls->bytecnt += fprintf(pls->OutFile, "%d %d %d\n", nx, ny, bitsPerPix);
        pls->bytecnt += fprintf(pls->OutFile, "[%d 0 0 %d 0 0] {currentfile gdlImagePixStringR readhexstring pop} {currentfile gdlImagePixStringG readhexstring pop} {currentfile gdlImagePixStringB readhexstring pop} true 3 colorimage\n",  nx, ny);
        break;
      default:
        cerr<<"you should not get here!"<<endl;
    }
    //output data in lines of LINEWIDTH chars:
    for (SizeT i = 0, k=0 ; i < nx * ny * 3; ++i) {
      fprintf(pls->OutFile, "%2.2X", idata[i]); k+=2; if( (k % LINEWIDTH) == 0 ) fprintf(pls->OutFile, "\n");
      }
    pls->bytecnt += (nx*ny*6 + nx*ny*6/LINEWIDTH);
  }
  pls->bytecnt += fprintf(pls->OutFile, "\ngrestore\n%%EndObject: Image\n       ");
  return true;
#undef LINEWIDTH
}

