/***************************************************************************
  gdlwxstream.cpp  - adapted from plplot wxWidgets driver documentation
                             -------------------
    begin                : Wed Oct 16 2013
    copyright            : (C) 2013 by Marc Schellens
    email                : m_schellens@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"

#ifdef HAVE_LIBWXWIDGETS

#include <plplot/plstream.h>

#include "gdlwxstream.hpp"

#ifdef HAVE_OLDPLPLOT
#define SETOPT SetOpt
#else
#define SETOPT setopt
#endif


GDLWXStream::GDLWXStream( int width, int height )
: GDLGStream( width, height, "wxwidgets")
  , m_dc(NULL)
  , m_bitmap(NULL)
  , m_width(width), m_height(height)
  , gdlWindow(NULL)
{
  m_dc = new wxMemoryDC();
  m_bitmap = new wxBitmap( width, height, 32);
  m_dc->SelectObject( *m_bitmap);
  if( !m_dc->IsOk())
  {
    m_dc->SelectObject( wxNullBitmap );
    delete m_bitmap;
    delete m_dc;
    throw GDLException("GDLWXStream: Failed to create DC.");
  }

  SETOPT("drvopt", "hrshsym=1,backend=0,text=0" ); // do not use freetype. Backend=0 enable compatibility (sort of) with X11 behaviour in plots. To be augmented one day...
//  SETOPT("drvopt", "hrshsym=1,backend=1,text=0" ); 
//  SETOPT("drvopt", "hrshsym=1,backend=2,text=0" );
  spage( 0.0, 0.0, 0, 0, 0, 0 ); //width and height have no importance, they are recomputed inside driver anyway!
  this->plstream::init();
  plstream::cmd(PLESC_DEVINIT, (void*)m_dc );
  plstream::set_stream();
}

GDLWXStream::~GDLWXStream()
{
  m_dc->SelectObject( wxNullBitmap );
  delete m_bitmap;
  delete m_dc;
}

void GDLWXStream::SetGDLDrawPanel(GDLDrawPanel* w)
{
  gdlWindow = w;
}

void GDLWXStream::Update()
{
  if( this->valid && gdlWindow != NULL)
    gdlWindow->Update();
}

void GDLWXStream::SetSize( int width, int height )
{
  m_dc->SelectObject( wxNullBitmap );
  delete m_bitmap;
  m_bitmap = new wxBitmap( width, height, 32 );
  m_dc->SelectObject( *m_bitmap);
  if( !m_dc->IsOk())
  {
    m_dc->SelectObject( wxNullBitmap );
    delete m_bitmap;
    delete m_dc;
    throw GDLException("GDLWXStream: Failed to resize DC.");
  }
//  wxSize screenPPM = m_dc->GetPPI(); //integer. Loss of precision if converting to PPM using wxSize operators.
  wxSize size = wxSize( width, height);

  PLFLT def,cur,ofact,fact;
  plgchr(&def,&cur); //cerr<<"before: "<<def;
  ofact = 80.0/pls->ydpi;
//  cerr<<", ofact= "<<ofact; 

  plstream::cmd(PLESC_RESIZE, (void*)&size );
  m_width = width;
  m_height = height;
  plgchr(&def,&cur); // cerr<<" ,after= "<<def;
  fact = 80.0/pls->ydpi;
//  cerr<<", fact= "<<fact;
  def *= fact/ofact;
//  cerr<<", new: " <<def<<endl;
////  cerr << xp << ", " << yp << ", " << xleng * xp << ", " << yleng * yp << ", " << xoff << ", " << yoff << "," << &event << endl;
  this->RenewPlplotDefaultCharsize( def );
}

void GDLWXStream::WarpPointer(DLong x, DLong y) {
  int xx=x;
  int yy=y;
  wxPanel *p = static_cast<wxPanel*>(gdlWindow);
  p->WarpPointer(xx,gdlWindow->GetSize().y-yy);
}

void GDLWXStream::Init()
{
  this->plstream::init();

  set_stream(); // private
// test :  gdlFrame->Show();
}


void GDLWXStream::RenewPlot()
{
  plstream::cmd( PLESC_CLEAR, NULL );
  replot();
}

void GDLWXStream::GetGeometry( long& xSize, long& ySize, long& xoff, long& yoff)
{
  // plplot does not return the real size
  xSize = m_width;
  ySize = m_height;
  xoff =  0; //false with X11
  yoff =  0; //false with X11
  if (GDL_DEBUG_PLSTREAM) fprintf(stderr,"GDLWXStream::GetGeometry(%ld %ld %ld %ld)\n", xSize, ySize, xoff, yoff);
}

unsigned long GDLWXStream::GetWindowDepth() {
  return 24;
}

void GDLWXStream::Clear() {
  ::c_plbop();
}

//FALSE: REPLACE With Clear(DLong chan) as in X
void GDLWXStream::Clear(DLong bColor) {
  PLINT r0, g0, b0;
  PLINT r1, g1, b1;
  DByte rb, gb, bb;

  // Get current background color
  plgcolbg(&r0, &g0, &b0);

  // Get desired background color
  GDLCT* actCT = GraphicsDevice::GetCT();
  actCT->Get(bColor, rb, gb, bb);

  // Convert to PLINT from GDL_BYTE
  r1 = (PLINT) rb;
  g1 = (PLINT) gb;
  b1 = (PLINT) bb;
  // this mimics better the *DL behaviour.
  ::c_plbop();
  plscolbg(r1, g1, b1);

}

bool GDLWXStream::PaintImage(unsigned char *idata, PLINT nx, PLINT ny, DLong *pos,
        DLong trueColorOrder, DLong chan) {
  plstream::cmd( PLESC_FLUSH, NULL );
  wxMemoryDC temp_dc;
  temp_dc.SelectObject(*m_bitmap);
  wxImage image=m_bitmap->ConvertToImage();
  unsigned char* mem=image.GetData();
  PLINT xoff = (PLINT) pos[0]; //(pls->wpxoff / 32767 * dev->width + 1);
  PLINT yoff = (PLINT) pos[2]; //(pls->wpyoff / 24575 * dev->height + 1);

  PLINT xsize = m_width;
  PLINT ysize = m_height;
  PLINT kxLimit = xsize - xoff;
  PLINT kyLimit = ysize - yoff;
  if (nx < kxLimit) kxLimit = nx;
  if (ny < kyLimit) kyLimit = ny;

  if ( nx > 0 && ny > 0 ) {
    SizeT p = (ysize - yoff - 1)*3*xsize;
    for ( int iy = 0; iy < kyLimit; ++iy ) {
      SizeT rowStart = p;
      p += xoff*3;
      for ( int ix = 0; ix < kxLimit; ++ix ) {
        if ( trueColorOrder == 0 && chan == 0 ) {
          mem[p++] = pls->cmap0[idata[iy * nx + ix]].r;
          mem[p++] = pls->cmap0[idata[iy * nx + ix]].g;
          mem[p++] = pls->cmap0[idata[iy * nx + ix]].b;
        } else {
          if ( chan == 0 ) {
            if ( trueColorOrder == 1 ) {
              mem[p++] = idata[3 * (iy * nx + ix) + 0]; 
              mem[p++] = idata[3 * (iy * nx + ix) + 1];
              mem[p++] = idata[3 * (iy * nx + ix) + 2];
            } else if ( trueColorOrder == 2 ) {
              mem[p++] = idata[nx * (iy * 3 + 0) + ix];
              mem[p++] = idata[nx * (iy * 3 + 1) + ix];
              mem[p++] = idata[nx * (iy * 3 + 2) + ix];
            } else if ( trueColorOrder == 3 ) {
              mem[p++] = idata[nx * (0 * ny + iy) + ix];
              mem[p++] = idata[nx * (1 * ny + iy) + ix];
              mem[p++] = idata[nx * (2 * ny + iy) + ix];
            }
          } else {
            if ( chan == 1 ) {
              mem[p++] = idata[1 * (iy * nx + ix) + 0];
              p += 2;
            } else if ( chan == 2 ) {
              p ++;
              mem[p++] = idata[1 * (iy * nx + ix) + 1];
              p ++;
            } else if ( chan == 3 ) {
              p += 2;
              mem[p++] = idata[1 * (iy * nx + ix) + 2];
            }
          }
        }
      }
      p = rowStart - (xsize*3);  
    }
  }
  m_dc->DrawBitmap(image,0,0);
  image.Destroy();
  temp_dc.SelectObject( wxNullBitmap);
  *m_bitmap = m_dc->GetAsBitmap(); 
  return true;
}

bool GDLWXStream::SetGraphicsFunction( long value) {
  cerr<<"Set Graphics Function not ready for wxWindow draw panel, please contribute."<<endl;
 return true;
}

bool GDLWXStream::GetWindowPosition(long& xpos, long& ypos ) {
  cerr<<"Get Window Position not ready for wxWindow draw panel, please contribute."<<endl;
  xpos=0;
  ypos=0;
 return true;
}

bool GDLWXStream::GetScreenResolution(double& resx, double& resy) {
  wxScreenDC *temp_dc=new wxScreenDC();
  wxSize reso=temp_dc->GetPPI();
  resx = reso.x/2.54;
  resy = reso.y/2.54;
  return true;
}
bool GDLWXStream::CursorStandard(int cursorNumber)
{
 cerr<<"Cursor Setting not ready for wxWindow draw panel, please contribute."<<endl;
 return true;
}
DLong GDLWXStream::GetVisualDepth() {
return 24;
}
DString GDLWXStream::GetVisualName() {
static const char* visual="TrueColor";
return visual;
}

DByteGDL* GDLWXStream::GetBitmapData() {
    plstream::cmd( PLESC_FLUSH, NULL );
    wxMemoryDC temp_dc;
    temp_dc.SelectObject(*m_bitmap);
    wxImage image=m_bitmap->ConvertToImage();
    unsigned char* mem=image.GetData();
    if ( mem == NULL ) return NULL;    

    unsigned int nx = m_bitmap->GetWidth();
    unsigned int ny = m_bitmap->GetHeight();

    SizeT datadims[3];
    datadims[0] = nx;
    datadims[1] = ny;
    datadims[2] = 3;
    dimension datadim(datadims, (SizeT) 3);
    DByteGDL *bitmap = new DByteGDL( datadim, BaseGDL::NOZERO);
    //PADDING is 3BPP -- we revert Y to respect IDL default
    SizeT kpad = 0;
    for ( SizeT iy =0; iy < ny ; ++iy ) {
      for ( SizeT ix = 0; ix < nx; ++ix ) {
        (*bitmap)[3 * ((ny-1-iy) * nx + ix) + 0] =  mem[kpad++];
        (*bitmap)[3 * ((ny-1-iy) * nx + ix) + 1] =  mem[kpad++];
        (*bitmap)[3 * ((ny-1-iy) * nx + ix) + 2] =  mem[kpad++];
      }
    }
    image.Destroy();
    return bitmap;
}
#endif
