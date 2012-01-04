/***************************************************************************
            gdleventhandler.hpp  -  global event handler routine 
                             -------------------
    begin                : February 23 2004
    copyright            : (C) 2004 by Marc Schellens
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// later this should become a thread
// right now its called in DInterpreter::NoReadline(...) or via readline

#include "includefirst.hpp"

#ifdef __APPLE__
#include <time.h>
#endif

#include "gdleventhandler.hpp"
#include "graphics.hpp"

using namespace std;

int GDLEventHandler()
{
  Graphics::HandleEvents();

#ifdef __APPLE__
  // under OS X the event loop burns to much CPU time
  struct timespec delay;
  delay.tv_sec=0;
  delay.tv_nsec = 20000000; // 20ms
  nanosleep(&delay,NULL);
#endif

  return 0;
}
