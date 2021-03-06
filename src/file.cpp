/***************************************************************************
                                 file.cpp  -  file related library functions 
                             -------------------
    begin                : July 22 2004
    copyright            : (C) 2004 by Marc Schellens
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

#ifndef _MSC_VER
#	include <libgen.h>
#	include <sys/types.h>
#endif

#include <sys/stat.h>

#ifndef _MSC_VER
#	include <unistd.h> 
#endif

#include "basegdl.hpp"
#include "str.hpp"


//#ifdef HAVE_LIBWXWIDGETS

#include "envt.hpp"
#include "file.hpp"
#include "objects.hpp"

#include <climits> // PATH_MAX

// #include <wx/utils.h>
// #include <wx/file.h>
// #include <wx/dir.h>
#ifndef _WIN32
#	include <fnmatch.h>
#	include <glob.h> // glob in MinGW does not working..... why?
#else
#	include <shlwapi.h>
#endif

#ifndef _MSC_VER
#	include <dirent.h>
#else
#	include <io.h>

#	define access _access

#	define R_OK    4       /* Test for read permission.  */
#	define W_OK    2       /* Test for write permission.  */
//#	define   X_OK    1       /* execute permission - unsupported in windows*/
#	define F_OK    0       /* Test for existence.  */

#	define PATH_MAX 255

#	include <direct.h>


#	if !defined(S_ISDIR)

#	define __S_ISTYPE(mode, mask)	(((mode) & S_IFMT) == (mask))
#	define S_ISDIR(mode)	 __S_ISTYPE((mode), S_IFDIR)
#	define S_ISREG(mode)    __S_ISTYPE((mode), S_IFREG)

#endif


#endif

// workaround for HP-UX. A better solution is needed i think
//#if defined(__hpux__) || defined(__sun__)
#if !defined(GLOB_TILDE)
#  define GLOB_TILDE 0
#endif
#if !defined(GLOB_BRACE)
#  define GLOB_BRACE 0
#endif
//#if defined(__hpux__) || defined(__sun__) || defined(__CYGWIN__) || defined(__OpenBSD__)
#if !defined(GLOB_ONLYDIR)
#  define GLOB_ONLYDIR 0
#endif
#if !defined(GLOB_PERIOD)
#  define GLOB_PERIOD 0
#endif

#ifdef _MSC_VER

/*
    Implementation of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003 and July 2012.
    Rights:  See end of file.
*/

#include <errno.h>
#include <io.h> /* _findfirst and _findnext set errno iff they return -1 */
#include <stdlib.h>
#include <string.h>

struct dirent
{
    char *d_name;
};

#ifdef __cplusplus
extern "C"
{
#endif

typedef ptrdiff_t handle_type; /* C99's intptr_t not sufficiently portable */

struct DIR
{
    handle_type         handle; /* -1 for failed rewind */
    struct _finddata_t  info;
    struct dirent       result; /* d_name null iff first time */
    char                *name;  /* null-terminated char string */
};


DIR *opendir(const char *name)
{
    DIR *dir = 0;

    if(name && name[0])
    {
        size_t base_length = strlen(name);
        const char *all = /* search pattern must end with suitable wildcard */
            strchr("/\\", name[base_length - 1]) ? "*" : "/*";

        if((dir = (DIR *) malloc(sizeof *dir)) != 0 &&
           (dir->name = (char *) malloc(base_length + strlen(all) + 1)) != 0)
        {
            strcat(strcpy(dir->name, name), all);

            if((dir->handle =
                (handle_type) _findfirst(dir->name, &dir->info)) != -1)
            {
                dir->result.d_name = 0;
            }
            else /* rollback */
            {
                free(dir->name);
                free(dir);
                dir = 0;
            }
        }
        else /* rollback */
        {
            free(dir);
            dir   = 0;
            errno = ENOMEM;
        }
    }
    else
    {
        errno = EINVAL;
    }

    return dir;
}

int closedir(DIR *dir)
{
    int result = -1;

    if(dir)
    {
        if(dir->handle != -1)
        {
            result = _findclose(dir->handle);
        }
        free(dir->name);
        free(dir);
    }
    if(result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }
    return result;
}

struct dirent *readdir(DIR *dir)
{
    struct dirent *result = 0;

    if(dir && dir->handle != -1)
    {
        if(!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1)
        {
            result         = &dir->result;
            result->d_name = dir->info.name;
        }
    }
    else
    {
        errno = EBADF;
    }
    return result;
}

void rewinddir(DIR *dir)
{
    if(dir && dir->handle != -1)
    {
        _findclose(dir->handle);
        dir->handle = (handle_type) _findfirst(dir->name, &dir->info);
        dir->result.d_name = 0;
    }
    else
    {
        errno = EBADF;
    }
}

#ifdef __cplusplus
}
#endif
/*
    Copyright Kevlin Henney, 1997, 2003, 2012. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.
    This software is supplied "as is" without express or implied warranty.
    But that said, if there are any problems please get in touch.
*/
#endif

namespace lib {

  using namespace std;

  string PathSeparator()
  {
#ifdef _WIN32
    string PathSep="\\"; //"
#else
    string PathSep="/";//"
#endif
    return PathSep;
  }

  DString GetCWD()
  {
    SizeT bufSize = PATH_MAX;
    char *buf = new char[ bufSize];
    for(;;)
      {
	char* value = getcwd( buf, bufSize);
	if( value != NULL)
	  break;
	delete[] buf;
	if( bufSize > 32000) 
	  throw GDLException("Cannot get CWD.");
	bufSize += PATH_MAX;
	buf = new char[ bufSize];
      }

    DString cur( buf);
    delete[] buf;
    
    return cur;
  }

  void cd_pro( EnvT* e)
  {
    if( e->KeywordPresent( 0)) // CURRENT
      {
	DString cur = GetCWD();
	e->SetKW( 0, new DStringGDL( cur));
      }

    SizeT nParam=e->NParam(); 
    if( nParam == 0) return;
    
    DString dir;
    e->AssureScalarPar<DStringGDL>( 0, dir);
   
    WordExp( dir);

     
//     // expand tilde
//     if( dir[0] == '~')
//       {
// 	char* homeDir = getenv( "HOME");
// 	if( homeDir != NULL)
// 	  {
// 	    dir = string( homeDir) + dir.substr(1);
// 	  }
//       }

    int success = chdir( dir.c_str());
 
    if( success != 0)
      e->Throw( "Unable to change current directory to: "+dir+".");
  }

  bool FindInDir( const DString& dirN, const DString& pat)
  {
    DString root = dirN;
    AppendIfNeeded( root, "/");

    DIR* dir = opendir( dirN.c_str());
    if( dir == NULL) return false;

    struct stat    statStruct;

    for(;;)
      {
	struct dirent* entry = readdir( dir);
	if( entry == NULL) break;
	
	DString entryStr( entry->d_name);
	if( entryStr != "." && entryStr != "..")
	  {
	    DString testFile = root + entryStr;
#ifdef _WIN32
	    int actStat = stat( testFile.c_str(), &statStruct);
#else
	    int actStat = lstat( testFile.c_str(), &statStruct);
#endif

	    if( S_ISDIR(statStruct.st_mode) == 0)

	      { // only test non-dirs

#ifdef _WIN32
#	ifdef _UNICODE
		TCHAR *tchr1 = new TCHAR[entryStr.size()+1];
		TCHAR *tchr2 = new TCHAR[pat.size() + 1];
		tchr1[entryStr.size()] = 0;
		tchr2[pat.size()] = 0;
		int match = 1 - PathMatchSpec(tchr1, tchr2);
		delete tchr1;
		delete tchr2;
#	else
		int match = 1 - PathMatchSpec(entryStr.c_str(), pat.c_str());
#	endif
#else

		int match = fnmatch( pat.c_str(), entryStr.c_str(), 0);

#endif
		if( match == 0)
		  {
		    closedir( dir);
		    return true;
		  }
	      }
	  }
      }

    closedir( dir);
    return false;
  }
  
  void ExpandPathN( FileListT& result,
		    const DString& dirN, 
		    const DString& pat,
		    bool all_dirs ) {
  // expand "+"

  int fnFlags = 0;

  //    fnFlags |= FNM_PERIOD;
  //    fnFlags |= FNM_NOESCAPE;

  DString root = dirN;
  AppendIfNeeded( root, "/" );

  struct stat statStruct;

  FileListT recurDir;

  bool notAdded = true;

  DIR* dir = opendir( dirN.c_str( ) );

  if ( dir == NULL ) return;
  int debug = 0;
  if ( debug ) cout << "ExpandPathN: " << dirN << endl;
  if ( all_dirs )
    notAdded = false;
  for (;; ) {
    struct dirent* entry = readdir( dir );
    if ( entry == NULL ) break;

    DString entryStr( entry->d_name );
    if ( entryStr != "." && entryStr != ".." ) {
      DString testDir = root + entryStr;
      if ( debug ) cout << "testing " << testDir <<"... ";
#ifdef _WIN32
      int actStat = stat( testDir.c_str( ), &statStruct );
#else
      int actStat = lstat( testDir.c_str( ), &statStruct );
#endif
      if ( actStat == 0 ) {
        if ( S_ISDIR( statStruct.st_mode ) != 0 ) {
          recurDir.push_back( testDir );
          if ( debug ) cout << "..dir: " << testDir << endl;
          ;
        }
        //GD Dec 2014 added test: if directory is a symlink, or if a tested file is a symlink. Note the use of 'stat'
        //instead of 'lstat' below.
        else if ( S_ISLNK( statStruct.st_mode ) != 0 ) 
        { //dir link or file ?
          struct stat lnkStatStruct;
          int lnkActStat = stat( testDir.c_str( ), &lnkStatStruct );
          if ( S_ISDIR( lnkStatStruct.st_mode ) != 0 ) 
          {
            recurDir.push_back( testDir );
            if ( debug ) cout << "..symlinkDir: " << testDir << endl;
          } 
          else if ( notAdded ) 
          {
#ifdef _WIN32
#ifdef _UNICODE
            TCHAR *tchr1 = new TCHAR[entryStr.size( ) + 1];
            TCHAR *tchr2 = new TCHAR[pat.size( ) + 1];
            tchr1[entryStr.size( )] = 0;
            tchr2[pat.size( )] = 0;
            int match = 1 - PathMatchSpec( tchr1, tchr2 );
            delete tchr1;
            delete tchr2;

#else
            int match = 1 - PathMatchSpec( entryStr.c_str( ), pat.c_str( ) );
#endif
#else
            int match = fnmatch( pat.c_str( ), entryStr.c_str( ), 0 );
#endif
            if ( debug ) cout << "symlinkEntry: " << entryStr << " match " << pat <<": "<< match << "\n";
            if ( match == 0 ) notAdded = false;
          }
        }
        else if ( notAdded ) 
        {
#ifdef _WIN32
#ifdef _UNICODE
          TCHAR *tchr1 = new TCHAR[entryStr.size( ) + 1];
          TCHAR *tchr2 = new TCHAR[pat.size( ) + 1];
          tchr1[entryStr.size( )] = 0;
          tchr2[pat.size( )] = 0;
          int match = 1 - PathMatchSpec( tchr1, tchr2 );
          delete tchr1;
          delete tchr2;

#else
          int match = 1 - PathMatchSpec( entryStr.c_str( ), pat.c_str( ) );
#endif
#else
          int match = fnmatch( pat.c_str( ), entryStr.c_str( ), 0 );
#endif
          if ( debug ) cout << "Entry: " << entryStr << " match " << pat <<": "<< match << "\n";
          if ( match == 0 ) notAdded = false;
        }
      }
    }
//    if ( debug ) cout << " notAdded? " << notAdded << endl;
  }

  int c = closedir( dir );
  if ( c == -1 ) return;

  // recursive search
  SizeT nRecur = recurDir.size( );
  for ( SizeT d = 0; d < nRecur; ++d ) {
    ExpandPathN( result, recurDir[d], pat, all_dirs );
  }

  if ( !notAdded )
    result.push_back( dirN );
}

  void ExpandPath( FileListT& result,
		    const DString& dirN, 
		    const DString& pat,
		    bool all_dirs)
  {
	int debug=0;
	if(debug) 	cout << " ExpandPath(,dirN.pat,bool) " << dirN << endl;
    if( dirN == "") 
      return;

    if( StrUpCase( dirN) == "<GDL_DEFAULT>" ||
	StrUpCase( dirN) == "<IDL_DEFAULT>")
      {
	// result.push_back( the default path here);
	return;
      }
    
    if( dirN[0] != '+' && dirN[0] != '~')
      {
	result.push_back( dirN);
	return;
      }
    
    if( dirN.length() == 1) {
      // dirN == "+" 
      if (dirN[0] == '+') return;
    }

    // dirN == "+DIRNAME"

#ifdef _WIN32
    DString initDir = dirN;
    if(dirN[0] == '+') 
		 initDir = dirN.substr(1);
        
#else

    // do first a glob because of '~'

    int flags = GLOB_TILDE | GLOB_NOSORT;

    glob_t p;

    int offset_tilde=0;
    if (dirN[0] == '+') offset_tilde=1;
    int gRes = glob( dirN.substr(offset_tilde).c_str(), flags, NULL, &p);
    if( gRes != 0 || p.gl_pathc == 0)
      {
		globfree( &p);
		return;
      }



    DString initDir = p.gl_pathv[ 0];
    globfree( &p);

#endif
 //   cout << "ExpandPath: initDir:" << initDir << dirN << endl;
    if (dirN[0] == '+')
      { 
	ExpandPathN( result, initDir, pat, all_dirs);
      } 
    else
      {
	result.push_back(initDir);
      }

  }

  BaseGDL* expand_path( EnvT* e)
  {
    e->NParam( 1);

    DString s;
    e->AssureStringScalarPar( 0, s);

    FileListT sArr;
    

    static int all_dirsIx = e->KeywordIx( "ALL_DIRS");
    bool all_dirs = e->KeywordSet( all_dirsIx);

    static int arrayIx = e->KeywordIx( "ARRAY");
    bool array = e->KeywordSet( arrayIx);

    static int countIx = e->KeywordIx( "COUNT");

    DString pattern;
    if(e->KeywordPresent( "PATTERN")) {
        static int typeIx = e->KeywordIx( "PATTERN");
	e->AssureStringScalarKWIfPresent( typeIx, pattern);
        }
    else      pattern = "*.pro";

    SizeT d;
    long   sPos=0;
   #ifdef _WIN32
      char pathsep[]=";";
    #else
      char pathsep[]=":";
    #endif
    do
      {
	d=s.find(pathsep[0],sPos);
	string act = s.substr(sPos,d-sPos);
	
	ExpandPath( sArr, act, pattern, all_dirs);
	
	sPos=d+1;
      }
    while( d != s.npos);

    SizeT nArr = sArr.size();

    if( e->KeywordPresent( countIx)) 
      {
	e->SetKW( countIx, new DLongGDL( nArr));
      }

    if( nArr == 0)
      return new DStringGDL( "");

    if( array)
      {
	DStringGDL* res = new DStringGDL( dimension( nArr), BaseGDL::NOZERO);
	for( SizeT i=0; i<nArr; ++i)
	  (*res)[ i] = sArr[i];
	// GJ (*res)[ i] = sArr[nArr-i-1];
	return res;
      }

    // set the path
    DString cat = sArr[0];
    // GJ DString cat = sArr[nArr-1];
    for( SizeT i=1; i<nArr; ++i)
      //GJ      cat += pathsep + sArr[nArr-i-1];
      cat += pathsep + sArr[i];
    return new DStringGDL( cat);
  }


  void PatternSearch( FileListT& fL, const DString& dirN, const DString& pat,
		      bool accErr,
		      bool quote,
		      bool match_dot,
		      const DString& prefixIn)
  {
    int fnFlags = 0;

#ifndef _WIN32

    if( !match_dot)
      fnFlags |= FNM_PERIOD;

    if( !quote)
      fnFlags |= FNM_NOESCAPE;

#endif

    DString root = dirN;
    if( root != "")
      {
	 long endR; 
	 for( endR = root.length()-1; endR >= 0; --endR)
	   {
	     if( root[ endR] != '/')
	       break;
	   }
	 if( endR >= 0)
	   root = root.substr( 0, endR+1) + "/";
	 else
	   root = "/";
      }

     DString prefix = root;
//     DString prefix = prefixIn;
//     if( prefix != "")
//       {
// 	 long endR; 
// 	 for( endR = prefix.length()-1; endR >= 0; --endR)
// 	   {
// 	     if( prefix[ endR] != '/')
// 	       break;
// 	   }
// 	 if( endR >= 0)
// 	   prefix = prefix.substr( 0, endR+1) + "/";
// 	 else
// 	   prefix = "/";
//       }

    struct stat    statStruct;

    FileListT recurDir;
    
    DIR* dir;
    if( root != "")
      dir = opendir( dirN.c_str());
    else
      dir = opendir( ".");
    if( dir == NULL) {
      if( accErr)
	throw GDLException( "FILE_SEARCH: Error opening dir: "+dirN);
      else
	return;
    }

    for(;;)
      {
	struct dirent* entry = readdir( dir);
	if( entry == NULL)
	  break;

	DString entryStr( entry->d_name);
	if( entryStr != "." && entryStr != "..")
	  {
	    if( root != "") // dirs for current ("") already included
	      {
		DString testDir = root + entryStr;
#ifdef _WIN32
		int actStat = stat( testDir.c_str(), &statStruct);
#else
		int actStat = lstat( testDir.c_str(), &statStruct);
#endif

		if( S_ISDIR(statStruct.st_mode) != 0)
		    recurDir.push_back( testDir);
	      }



	    // dirs are also returned if they match

#ifdef _WIN32
#	ifdef _UNICODE
		TCHAR *tchr1 = new TCHAR[entryStr.size() + 1];
		TCHAR *tchr2 = new TCHAR[pat.size() + 1];
		tchr1[entryStr.size()] = 0;
		tchr2[pat.size()] = 0;
		int match = 1 - PathMatchSpec(tchr1, tchr2);
		delete tchr1;
		delete tchr2;
#	else
	    int match = 1 - PathMatchSpec(entryStr.c_str(), pat.c_str());
#	endif
#else
	    int match = fnmatch( pat.c_str(), entryStr.c_str(), fnFlags);
#endif
	    if( match == 0)
	      fL.push_back( prefix + entryStr);
	  }
      }

    int c = closedir( dir);
    if( c == -1) {
      if( accErr)
	throw GDLException( "FILE_SEARCH: Error closing dir: "+dirN);
      else
	return;
    }
    // recursive search
    SizeT nRecur = recurDir.size();
    for( SizeT d=0; d<nRecur; ++d)
      {

	PatternSearch( fL, recurDir[d], pat, accErr, quote, 
		       match_dot,
		       /*prefix +*/ recurDir[d]);
      }
  }

// Make s string case-insensitive for glob()
DString makeInsensitive(const DString &s)
{
	DString insen="";
	char coupleBracket[5]={'[',0,0,']',0};
	char couple[3]={0};
	bool bracket=false;
	
	for(size_t i=0;i<s.size();i++) 
		if((s[i]>='A' && s[i]<='Z') || (s[i]>='a' && s[i]<='z'))
		{
			char m,M;
			if(s[i]>='a' && s[i]<='z')
				m=s[i],M=m+'A'-'a';
			else
				M=s[i],m=M-'A'+'a';

			if(bracket) // If bracket is open, then don't add bracket
				couple[0]=m,couple[1]=M,insen+=couple;
			else // else [aA]
				coupleBracket[1]=m,coupleBracket[2]=M,insen+=coupleBracket;
		}
		else
		{
			if(s[i]=='[')
			{
				bracket=false;
				for(size_t ii=i;ii<s.size();ii++) // Looking for matching right bracket
					if(s[ii]==']') { bracket=true; break; }

				if(bracket) insen+=s[i];
				else insen+="[[]";
			}
			else if(s[i]==']' && s[(!i?0:i-1)]!='[')
				bracket=false, insen+=s[i];
			else
				insen+=s[i];
		}
	return insen;
}

#ifndef _WIN32
  void FileSearch( FileListT& fL, const DString& s, 
		   bool environment,
		   bool tilde,
		   bool accErr,
		   bool mark,
		   bool noSort,
		   bool quote,
		   bool dir,
		   bool period,
                   bool forceAbsPath,
		   bool fold_case)
  {
    int flags = 0;
    DString st;

    if( environment)
      flags |= GLOB_BRACE;
    
    if( tilde)
      flags |= GLOB_TILDE;

    if( accErr)
      flags |= GLOB_ERR;
    
    if( mark && !dir) // only mark directory if not in dir mode
      flags |= GLOB_MARK;

    if( noSort)
      flags |= GLOB_NOSORT;

#if !defined(__APPLE__) && !defined(__FreeBSD__)
    if( !quote) // n/a on OS X
      flags |= GLOB_NOESCAPE;

    if( dir) // simulate with lstat()
      flags |= GLOB_ONLYDIR;

    if( period) // n/a on OS X
      flags |= GLOB_PERIOD;
#else
    struct stat    statStruct;
#endif
    if( fold_case)
	st=makeInsensitive(s);
   else
	st=s;

    glob_t p;
    int gRes;
    if (!forceAbsPath)
    {
      if (st != "") gRes = glob(st.c_str(), flags, NULL, &p);
      else gRes = glob("*", flags, NULL, &p);
    }
    else 
    {
      int debug=0;
      if (debug) {
	cout << "st : " << st << endl;
	cout << "st.size() : " << st.size() << endl;
      }
      
      string pattern;
      if (st == ""){
	pattern = GetCWD();
	pattern.append("/*");
	gRes = glob(pattern.c_str(), flags, NULL, &p);
      } else {
	if (
	    st.at(0) != '/' && 
	    !(tilde && st.at(0) == '~') && 
	    !(environment && st.at(0) == '$')
	    ) 
	  { 
	    pattern = GetCWD();
	    pattern.append("/");
	    if(!( st.size() ==1 && st.at(0) == '.')) pattern.append(st);
	    
	    if (debug) cout << "patern : " << pattern << endl;
	    
	    gRes = glob(pattern.c_str(), flags, NULL, &p);
	  }
	else 
	  {
	    gRes = glob(st.c_str(), flags, NULL, &p);
	  }
      }
      if (debug) {
	cout << "gRes : " << gRes << endl;
	cout << "st out : " << st << endl;
      }
    }

#ifndef __APPLE__
    if( accErr && (gRes == GLOB_ABORTED || gRes == GLOB_NOSPACE))
      throw GDLException( "FILE_SEARCH: Read error: "+s);
#else
    if( accErr && (gRes != 0 && p.gl_pathc > 0)) // NOMATCH is no error
      throw GDLException( "FILE_SEARCH: Read error: "+s);
#endif      

    if( gRes == 0)
      for( SizeT f=0; f<p.gl_pathc; ++f)
	{
#ifndef __APPLE__
	  fL.push_back( p.gl_pathv[ f]);
#else
	  if( !dir)
	    fL.push_back( p.gl_pathv[ f]);
	  else
	    { // push only if dir
	      int actStat = lstat( p.gl_pathv[ f], &statStruct);
	      if( S_ISDIR(statStruct.st_mode) != 0)
		fL.push_back( p.gl_pathv[ f]);
	    }
#endif      
	}
    globfree( &p);

    if( st == "" && dir)
      fL.push_back( "");
  }


  // AC 16 May 2014 : preliminary (and no MSwin support !)
  // revised by AC on June 28 
  // PRINT, FILE_expand_path([['','.'],['$PWD','src/']])
  // when the path is wrong, wrong output ...
#if defined (__MINGW32__)
//  This is includced here even though the
//  the whole block is exluded for _WIN32
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH) 
// ref:http://sourceforge.net/p/mingw/patches/256/ Keith Marshall 2005-12-02
#endif

  BaseGDL* file_expand_path( EnvT* e)
  {
    // always 1
    SizeT nParam=e->NParam(1);

    // accepting only strings as parameters
    BaseGDL* p0 = e->GetParDefined(0);
    if( p0->Type() != GDL_STRING)
      e->Throw("String expression required in this context: " + e->GetParString(0));
    DStringGDL* p0S = static_cast<DStringGDL*>(p0);

    SizeT nPath = p0S->N_Elements();

    //    cout << "nPath :" << nPath  << endl;

    DStringGDL* res = new DStringGDL(p0S->Dim(), BaseGDL::NOZERO);
    for( SizeT r=0; r<nPath ; ++r)
      {
	string tmp=(*p0S)[r];

	if (tmp.length() == 0) {
	  char* cwd;
	  char buff[PATH_MAX + 1];
	  cwd = getcwd( buff, PATH_MAX + 1 );
	  if( cwd != NULL ){
	    (*res)[r]= string(cwd);
	  } 
	  else {
	    (*res)[r]=""; //( errors are not managed ...)
	  }
	} else {
	  WordExp(tmp);
	  char *symlinkpath =const_cast<char*> (tmp.c_str());
	  char actualpath [PATH_MAX+1];
	  char *ptr;
	  ptr = realpath(symlinkpath, actualpath);
	  if( ptr != NULL ){
	    (*res)[r] =string(ptr);
	  }else {
	    //( errors are not managed ...)
	    (*res)[r] = tmp ;
	  }
	}
      }
    return res;
  }

  // not finished yet
  BaseGDL* file_search( EnvT* e)
  {
    SizeT nParam=e->NParam(); // 0 -> "*"
    
    DStringGDL* pathSpec;
    DString     recurPattern;

    SizeT nPath = 0;

    if( nParam > 0)
      {
	BaseGDL* p0 = e->GetParDefined( 0);
	pathSpec = dynamic_cast<DStringGDL*>( p0);
	if( pathSpec == NULL)
	  e->Throw( "String expression required in this context.");

	nPath = pathSpec->N_Elements();

	if( nParam > 1)
	  e->AssureScalarPar< DStringGDL>( 1, recurPattern);
      }

    // unix defaults
    bool tilde = true;
    bool environment = true;
    bool fold_case = false;

    // keywords
    // next three have default behaviour
    static int tildeIx = e->KeywordIx( "EXPAND_TILDE");
    bool tildeKW = e->KeywordPresent( tildeIx);
    if( tildeKW) tilde = e->KeywordSet( tildeIx);

    static int environmentIx = e->KeywordIx( "EXPAND_ENVIRONMENT");
    bool environmentKW = e->KeywordPresent( environmentIx);
    if( environmentKW) 
      {
	environment = e->KeywordSet( environmentIx);
	if( environment) // only warn when expiclitely set
	  Warning( "FILE_SEARCH: EXPAND_ENVIRONMENT not supported.");
      }

    static int fold_caseIx = e->KeywordIx( "FOLD_CASE");
    bool fold_caseKW = e->KeywordPresent( fold_caseIx);
    if( fold_caseKW) fold_case = e->KeywordSet( fold_caseIx);

    // 
    static int countIx = e->KeywordIx( "COUNT");
    bool countKW = e->KeywordPresent( countIx);

    static int accerrIx = e->KeywordIx( "ISSUE_ACCESS_ERROR");
    bool accErr = e->KeywordSet( accerrIx);

    static int markIx = e->KeywordIx( "MARK_DIRECTORY");
    bool mark = e->KeywordSet( markIx);

    static int nosortIx = e->KeywordIx( "NOSORT");
    bool noSort = e->KeywordSet( nosortIx);

    static int quoteIx = e->KeywordIx( "QUOTE");
    bool quote = e->KeywordSet( quoteIx);

    static int match_dotIx = e->KeywordIx( "MATCH_INITIAL_DOT");
    bool match_dot = e->KeywordSet( match_dotIx);

    static int match_all_dotIx = e->KeywordIx( "MATCH_ALL_INITIAL_DOT");
    bool match_all_dot = e->KeywordSet( match_all_dotIx);

    static int fully_qualified_pathIx = e->KeywordIx( "FULLY_QUALIFY_PATH");
    bool forceAbsPath = e->KeywordSet( fully_qualified_pathIx);

    if( match_all_dot)
      Warning( "FILE_SEARCH: MATCH_ALL_INITIAL_DOT keyword ignored (not supported).");

    bool onlyDir = nParam > 1;

    FileListT fileList;
    int debug=0;
    if (debug) cout << "nPath: " << nPath << endl;

    if( nPath == 0)
      FileSearch( fileList, "", 
		  environment, tilde, 
		  accErr, mark, noSort, quote, onlyDir, match_dot, forceAbsPath, fold_case);
    else
      FileSearch( fileList, (*pathSpec)[0],
		  environment, tilde, 
		  accErr, mark, noSort, quote, onlyDir, match_dot, forceAbsPath, fold_case);
    
    for( SizeT f=1; f < nPath; ++f) 
      FileSearch( fileList, (*pathSpec)[f],
		  environment, tilde, 
		  accErr, mark, noSort, quote, onlyDir, match_dot, forceAbsPath, fold_case);

    DLong count = fileList.size();

    if (debug) cout << "Count : " << count << endl;
    //    cout << fileList << endl;

    if( onlyDir)
      { // recursive search for recurPattern
	FileListT fileOut;
	
	for( SizeT f=0; f<count; ++f) // ok for count == 0
	  {
	    //	    cout << "Looking in: " << fileList[f] << endl;
	    PatternSearch( fileOut, fileList[f], recurPattern, accErr, quote,
			   match_dot,
			   fileList[f]);
	  }	

	DLong pCount = fileOut.size();
	
	if( countKW)
	  e->SetKW( countIx, new DLongGDL( pCount));

	if( pCount == 0)
	  return new DStringGDL("");

	if( !noSort)
	  sort( fileOut.begin(), fileOut.end());
    
	// fileOut -> res
	DStringGDL* res = new DStringGDL( dimension( pCount), BaseGDL::NOZERO);
	for( SizeT r=0; r<pCount; ++r)
	  (*res)[r] = fileOut[ r];

	return res;
      }

    if( countKW)
      e->SetKW( countIx, new DLongGDL( count));

    if( count == 0)
      return new DStringGDL("");
    
    if( !noSort)
      sort( fileList.begin(), fileList.end());

    // fileList -> res
    DStringGDL* res = new DStringGDL( dimension( count), BaseGDL::NOZERO);
    for( SizeT r=0; r<count; ++r)
      (*res)[r] = fileList[ r];

    return res;
  }
#endif


  BaseGDL* file_basename( EnvT* e)
  {

    SizeT nParams=e->NParam( 1);

    // accepting only strings as parameters
    BaseGDL* p0 = e->GetParDefined(0);
    if( p0->Type() != GDL_STRING)
      e->Throw("String expression required in this context: " + e->GetParString(0));
    DStringGDL* p0S = static_cast<DStringGDL*>(p0);

    BaseGDL* p1;
    DStringGDL* p1S;
    bool DoRemoveSuffix = false;

    if (nParams == 2) {
      // shall we remove a suffix ?
      p1 = e->GetPar(1);
      if( p1 == NULL || p1->Type() != GDL_STRING)
	e->Throw("String expression required in this context: " + e->GetParString(1));
      p1S = static_cast<DStringGDL*>(p1);
      if (p1S->N_Elements() == 1) {
	if ((*p1S)[0].length() >0) DoRemoveSuffix=true;
      }
      if (p1S->N_Elements() > 1) 
	e->Throw(" Expression must be a scalar or 1 element array in this context: " + e->GetParString(1));
    }
    
    dimension resDim;
    resDim=p0S->Dim();
    DStringGDL* res = new DStringGDL(resDim, BaseGDL::NOZERO);

    for (SizeT i = 0; i < p0S->N_Elements(); i++) {

      //tmp=strdup((*p0S)[i].c_str());
      const string& tmp=(*p0S)[i];

      //      cout << ">>"<<(*p0S)[i].c_str() << "<<" << endl;
      if (tmp.length() > 0) {

#ifdef _WIN32
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath( tmp.c_str(),drive,dir,fname,ext);
	string bname = string(fname)+"."+ext;
#else
	char buf[ PATH_MAX+1];
	strncpy(buf, tmp.c_str(), PATH_MAX+1);
	string bname = basename(buf);
#endif

	(*res)[i] = bname;
      } 
      else
	{
	  (*res)[i]="";
	}
    }

    // managing suffixe
    if (DoRemoveSuffix) {
      
      string suffixe=(*p1S)[0];
      int suffLength=(*p1S)[0].length();
      
      static int fold_caseIx = e->KeywordIx( "FOLD_CASE");
      bool fold_case = e->KeywordSet( fold_caseIx);
      
      if (fold_case) suffixe=StrUpCase(suffixe);

      //cout << "suffixe :"<< suffixe << endl;

      
      string tmp1, fin_tmp;
      for (SizeT i = 0; i < p0S->N_Elements(); i++) {
	tmp1=(*res)[i];
	
	// Strickly greater : if equal, we keep it !
	if (tmp1.length() > suffLength) {
	  fin_tmp=tmp1.substr(tmp1.length()-suffLength);
	  
	  if (fold_case) fin_tmp=StrUpCase(fin_tmp);
	  
	  if (fin_tmp.compare(suffixe) == 0) {
	      (*res)[i]=tmp1.substr(0,tmp1.length()-suffLength);
	  }	 	  
	}
      }
      
    }

    return res;
  }


  BaseGDL* file_dirname( EnvT* e)
  {
    // accepting only strings as parameters
    BaseGDL* p0 = e->GetParDefined(0);
    if( p0->Type() != GDL_STRING)
      e->Throw("String expression required in this context: " + e->GetParString(0));
    DStringGDL* p0S = static_cast<DStringGDL*>(p0);

    dimension resDim;
    resDim=p0S->Dim();
    DStringGDL* res = new DStringGDL(resDim, BaseGDL::NOZERO);

    for (SizeT i = 0; i < p0S->N_Elements(); i++) {
      //tmp=strdup((*p0S)[i].c_str());
      const string& tmp = (*p0S)[i];

#ifdef _WIN32
   char path_buffer[_MAX_PATH];
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];

   _splitpath( tmp.c_str(),drive,dir,fname,ext);
   string dname = string(drive)+":"+dir;
#else
	char buf[ PATH_MAX+1];
	strncpy(buf, tmp.c_str(), PATH_MAX+1);
	string dname = dirname(buf);
#endif
   (*res)[i] = dname;

    }
    
    if (e->KeywordSet("MARK_DIRECTORY")) {
      for (SizeT i = 0; i < p0S->N_Elements(); i++) {
	(*res)[i]=(*res)[i] + PathSeparator();
      }
    }
    
    return res;

}

  BaseGDL* file_same( EnvT* e)
  {
    // assuring right number of parameters
    SizeT nParam=e->NParam(2); 

    // accepting only strings as parameters
    BaseGDL* p0 = e->GetParDefined(0);
    DStringGDL* p0S = dynamic_cast<DStringGDL*>(p0);
    if (p0S == NULL) e->Throw("String expression required in this context: " + e->GetParString(0));

    BaseGDL* p1 = e->GetParDefined(1);
    DStringGDL* p1S = dynamic_cast<DStringGDL*>(p1);
    if (p1S == NULL) e->Throw("String expression required in this context: " + e->GetParString(1));

    // no empty strings accepted
    {
      int empty = 0;
      for (SizeT i = 0; i < p0S->N_Elements(); i++) empty += (*p0S)[i].empty();
      for (SizeT i = 0; i < p1S->N_Elements(); i++) empty += (*p1S)[i].empty();
      if (empty != 0) e->Throw("Null filename not allowed.");
    }

    // allocating memory for the comparison result
    DByteGDL* res;
    {
      dimension resDim;
      if (p0S->Rank() == 0 || p1S->Rank() == 0) {
        resDim = (p0S->N_Elements() > p1S->N_Elements() ? p0S : p1S)->Dim(); 
      } else {
        resDim = (p0S->N_Elements() < p1S->N_Elements() ? p0S : p1S)->Dim(); 
      }
      res = new DByteGDL(resDim); // zero
    }

    // comparison loop
    for (SizeT i = 0; i < res->N_Elements(); i++)
    {
      // deciding which filename to compare
      SizeT p0idx = p0S->Rank() == 0 ? 0 : i;
      SizeT p1idx = p1S->Rank() == 0 ? 0 : i;

      // checking for lexically identical paths
      if ((*p0S)[p0idx].compare((*p1S)[p1idx]) == 0)
      {
        (*res)[i] = 1;
        continue;
      }

      // expanding if needed (tilde, shell variables, etc)
      const char *file0, *file1;
      string tmp0, tmp1;
      if (!e->KeywordSet(e->KeywordIx("NOEXPAND_PATH"))) 
      {
        tmp0 = (*p0S)[p0idx];
        WordExp(tmp0);
        tmp1 = (*p1S)[p1idx];
        WordExp(tmp1);
        // checking again for lexically identical paths (e.g. ~/ and $HOME)
        if (tmp0.compare(tmp1) == 0)
        {
          (*res)[i] = 1;
          continue;
        }
        file0 = tmp0.c_str();
        file1 = tmp1.c_str();
      }
      else
      {
        file0 = (*p0S)[p0idx].c_str();
        file1 = (*p1S)[p1idx].c_str();
      }

      // checking for the same inode/device numbers
      struct stat statStruct;
      dev_t file0dev;
      ino_t file0ino;    
      int ret = stat(file0, &statStruct);
      if (ret != 0) continue;
      file0dev = statStruct.st_dev;
      file0ino = statStruct.st_ino;
      ret = stat(file1, &statStruct);
      if (ret != 0) continue;
      (*res)[i] = (file0dev == statStruct.st_dev && file0ino == statStruct.st_ino);

    }

    return res;

  }
  BaseGDL* file_test( EnvT* e)
  {
    SizeT nParam=e->NParam( 1); 
    
    BaseGDL* p0 = e->GetParDefined( 0);

    DStringGDL* p0S = dynamic_cast<DStringGDL*>( p0);
    if( p0S == NULL)
      e->Throw( "String expression required in this context: "+
		e->GetParString(0));

    static int directoryIx = e->KeywordIx( "DIRECTORY");
    bool directory = e->KeywordSet( directoryIx);

    static int executableIx = e->KeywordIx( "EXECUTABLE");
    bool executable = e->KeywordSet( executableIx);

    static int readIx = e->KeywordIx( "READ");
    bool read = e->KeywordSet( readIx);

    static int writeIx = e->KeywordIx( "WRITE");
    bool write = e->KeywordSet( writeIx);

    static int zero_lengthIx = e->KeywordIx( "ZERO_LENGTH");
    bool zero_length = e->KeywordSet( zero_lengthIx);

    static int get_modeIx = e->KeywordIx( "GET_MODE");
    bool get_mode = e->KeywordPresent( get_modeIx);

    static int regularIx = e->KeywordIx( "REGULAR");
    bool regular = e->KeywordSet( regularIx);

    static int block_specialIx = e->KeywordIx( "BLOCK_SPECIAL");
    bool block_special = e->KeywordSet( block_specialIx);

    static int character_specialIx = e->KeywordIx( "CHARACTER_SPECIAL");
    bool character_special = e->KeywordSet( character_specialIx);

    static int named_pipeIx = e->KeywordIx( "NAMED_PIPE");
    bool named_pipe = e->KeywordSet( named_pipeIx);

    static int socketIx = e->KeywordIx( "SOCKET");
    bool socket = e->KeywordSet( socketIx);

    static int symlinkIx = e->KeywordIx( "SYMLINK");
    bool symlink = e->KeywordSet( symlinkIx);
    
    static int dSymlinkIx = e->KeywordIx( "DANGLING_SYMLINK");
    bool dsymlink = e->KeywordSet( dSymlinkIx);

    static int noexpand_pathIx = e->KeywordIx( "NOEXPAND_PATH");
    bool noexpand_path = e->KeywordSet( noexpand_pathIx);

    DLongGDL* getMode = NULL; 
    if( get_mode)
      {
	getMode = new DLongGDL( p0S->Dim()); // zero
	e->SetKW( get_modeIx, getMode);
      }
    
    DLongGDL* res = new DLongGDL( p0S->Dim()); // zero

//     bool doStat = 
//       zero_length || get_mode || directory || 
//       regular || block_special || character_special || 
//       named_pipe || socket || symlink;

    SizeT nEl = p0S->N_Elements();

    for( SizeT f=0; f<nEl; ++f)
      {
	string actFile;

    if ( !noexpand_path ) {
      string tmp = (*p0S)[f];
      WordExp( tmp );
      if ( tmp.length( ) > 1 && tmp[ tmp.length( ) - 1] == '/' )
        actFile = tmp.substr( 0, tmp.length( ) - 1 );
      else
        actFile = tmp;
    }
    else {
       actFile = (*p0S)[f];
    }
	struct stat statStruct,statStruct2;
#ifdef _WIN32
	int actStat = stat( actFile.c_str(), &statStruct2);
#else
	int actStat = lstat( actFile.c_str(), &statStruct2);
#endif
	
	if( actStat != 0) 	  continue;
//be more precise in case of symlinks --- use stat to find the state of the symlinked file instead:
     bool isASymLink = S_ISLNK(statStruct2.st_mode) ; 
     actStat = stat( actFile.c_str(), &statStruct);
     bool isADanglingSymLink = (actStat != 0 && isASymLink); //is a dangling symlink!
     if (isADanglingSymLink) isASymLink = FALSE;
     
	if( read && access( actFile.c_str(), R_OK) != 0)  continue;
	if( write && access( actFile.c_str(), W_OK) != 0)  continue;
	if( zero_length && statStruct.st_size != 0) 	  continue;

#ifndef _WIN32

	if( executable && access( actFile.c_str(), X_OK) != 0)	  continue;

	if( get_mode)
	  (*getMode)[ f] = statStruct.st_mode & 
	                   (S_IRWXU | S_IRWXG | S_IRWXO);

	if( block_special && S_ISBLK(statStruct.st_mode) == 0) 	  continue;

	if( character_special && S_ISCHR(statStruct.st_mode) == 0) continue;

	if( named_pipe && S_ISFIFO(statStruct.st_mode) == 0)       continue;

	if( socket && S_ISSOCK(statStruct.st_mode) == 0) 	  continue;

	if( symlink && !isASymLink ) 	  continue;
	if( dsymlink && !isADanglingSymLink ) 	  continue;

#endif

	if( directory && S_ISDIR(statStruct.st_mode) == 0) 
	  continue;

	if( regular && S_ISREG(statStruct.st_mode) == 0) 
	  continue;

	(*res)[ f] = 1;

      }
    return res;
  }


  BaseGDL* file_info( EnvT* e)
  {
    SizeT nParam=e->NParam( 1); 
    DStringGDL* p0S = dynamic_cast<DStringGDL*>(e->GetParDefined(0));
    if( p0S == NULL)
      e->Throw( "String expression required in this context: "+
		e->GetParString(0));

    bool noexpand_path = e->KeywordSet(e->KeywordIx( "NOEXPAND_PATH"));

    DStructGDL* res = new DStructGDL(
      FindInStructList(structList, "FILE_INFO"), 
      p0S->Rank() == 0 ? dimension(1) : p0S->Dim()
    ); 

    int tName = tName = res->Desc()->TagIndex("NAME");
    int tExists, tRead, tWrite, tExecute, tRegular, tDirectory, tBlockSpecial, 
      tCharacterSpecial, tNamedPipe, tSetuid, tSetgid, tSocket, tStickyBit, 
      tSymlink, tDanglingSymlink, tMode, tAtime, tCtime, tMtime, tSize;
    int indices_known = false;

    SizeT nEl = p0S->N_Elements();

    for (SizeT f = 0; f < nEl; f++)
    {
      // NAME
      string actFile;
      string tmp;
      if ( !noexpand_path ) {
        string tmp = (*p0S)[f];
        WordExp( tmp );
        if ( tmp.length( ) > 1 && tmp[ tmp.length( ) - 1] == '/' )
          actFile = tmp.substr( 0, tmp.length( ) - 1 );
        else
          actFile = tmp;
      }
      else {
        actFile = (*p0S)[f];
      }
	  *(res->GetTag(tName, f)) = DStringGDL(actFile.c_str());

        // stating the file (and moving on to the next file if failed)
	struct stat statStruct,statStruct2;
#ifdef _WIN32
	int actStat = stat(actFile, &statStruct2);
#else
	int actStat = lstat(actFile.c_str(), &statStruct2);
#endif
//be more precise in case of symlinks --- use stat to find the state of the symlinked file instead:
     bool isASymLink = S_ISLNK(statStruct2.st_mode);
     actStat = stat( actFile.c_str(), &statStruct);
     bool isADanglingSymLink = (actStat != 0 && isASymLink); //is a dangling symlink!
	

        // checking struct tag indices (once)

        if (!indices_known) 

        {

          tExists =           res->Desc()->TagIndex("EXISTS"); 
          tRead =             res->Desc()->TagIndex("READ"); 
          tWrite =            res->Desc()->TagIndex("WRITE"); 
          tRegular =          res->Desc()->TagIndex("REGULAR"); 
          tDirectory =        res->Desc()->TagIndex("DIRECTORY");

#ifndef _MSC_VER

          tBlockSpecial =     res->Desc()->TagIndex("BLOCK_SPECIAL");
          tCharacterSpecial = res->Desc()->TagIndex("CHARACTER_SPECIAL");
          tNamedPipe =        res->Desc()->TagIndex("NAMED_PIPE");
          tExecute =          res->Desc()->TagIndex("EXECUTE"); 
          tSetuid =           res->Desc()->TagIndex("SETUID");
          tSetgid =           res->Desc()->TagIndex("SETGID");
	      tSocket =           res->Desc()->TagIndex("SOCKET");
          tStickyBit =        res->Desc()->TagIndex("STICKY_BIT");
          tSymlink =          res->Desc()->TagIndex("SYMLINK");
          tDanglingSymlink =  res->Desc()->TagIndex("DANGLING_SYMLINK");
          tMode =             res->Desc()->TagIndex("MODE");

#endif

          tAtime =            res->Desc()->TagIndex("ATIME");
          tCtime =            res->Desc()->TagIndex("CTIME");
          tMtime =            res->Desc()->TagIndex("MTIME");
          tSize =             res->Desc()->TagIndex("SIZE");

          indices_known = true;

        }
     // DANGLING_SYMLINK good place
#ifndef _WIN32
        if (isADanglingSymLink) { 
          // warning: statStruct now describes the linked file
          *(res->GetTag(tDanglingSymlink, f)) = DByteGDL(1);
        }
#endif
     if( actStat != 0 ) continue;

       // EXISTS (would not reach here if stat failed)
        *(res->GetTag(tExists, f)) = DByteGDL(1);
        
        // READ, WRITE, EXECUTE

        *(res->GetTag(tRead, f)) =    DByteGDL(access(actFile.c_str(), R_OK) == 0);

        *(res->GetTag(tWrite, f)) =   DByteGDL(access(actFile.c_str(), W_OK) == 0);

#ifndef _MSC_VER

        *(res->GetTag(tExecute, f)) = DByteGDL(access(actFile.c_str(), X_OK) == 0);

#endif



        // REGULAR, DIRECTORY, BLOCK_SPECIAL, CHARACTER_SPECIAL, NAMED_PIPE, SOCKET

        *(res->GetTag(tRegular, f)) =          DByteGDL(S_ISREG( statStruct.st_mode) != 0);

        *(res->GetTag(tDirectory, f)) =        DByteGDL(S_ISDIR( statStruct.st_mode) != 0);

#ifndef _MSC_VER

        *(res->GetTag(tBlockSpecial, f)) =     DByteGDL(S_ISBLK( statStruct.st_mode) != 0);

        *(res->GetTag(tCharacterSpecial, f)) = DByteGDL(S_ISCHR( statStruct.st_mode) != 0);

        *(res->GetTag(tNamedPipe, f)) =        DByteGDL(S_ISFIFO(statStruct.st_mode) != 0);
#ifndef __MINGW32__
        *(res->GetTag(tSocket, f)) =           DByteGDL(S_ISSOCK(statStruct.st_mode) != 0);
#endif

#endif  

        // SETUID, SETGID, STICKY_BIT

#ifndef WIN32

        *(res->GetTag(tSetuid, f)) =           DByteGDL((S_ISUID & statStruct.st_mode) != 0);
        *(res->GetTag(tSetgid, f)) =           DByteGDL((S_ISGID & statStruct.st_mode) != 0);
        *(res->GetTag(tStickyBit, f)) =        DByteGDL((S_ISVTX & statStruct.st_mode) != 0);

        // MODE

        *(res->GetTag(tMode, f)) = DLongGDL(
          statStruct.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX)
        );

#endif

        // ATIME, CTIME, MTIME
        *(res->GetTag(tAtime, f)) = DLong64GDL(statStruct.st_atime);
        *(res->GetTag(tCtime, f)) = DLong64GDL(statStruct.st_ctime);
        *(res->GetTag(tMtime, f)) = DLong64GDL(statStruct.st_mtime);

        // SIZE
	*(res->GetTag(tSize, f)) = DLong64GDL(statStruct.st_size);

        // SYMLINK
#ifndef _WIN32

        if (isASymLink)
        {
          *(res->GetTag(tSymlink, f)) = DByteGDL(1);
        }
#endif
      }

    return res;

  }

  void file_mkdir( EnvT* e)
  {
    // sanity checks
    SizeT nParam=e->NParam( 1);
    for (int i=0; i<nParam; i++)
    {
      if (dynamic_cast<DStringGDL*>(e->GetParDefined(i)) == NULL) 
        e->Throw( "All arguments must be string scalars/arrays, argument " + i2s(i+1) + " is: " + e->GetParString(i));
    }

    static int noexpand_pathIx = e->KeywordIx( "NOEXPAND_PATH");
    bool noexpand_path = e->KeywordSet( noexpand_pathIx);
#ifdef _MSC_VER
    string cmd = "md"; // windows always creates all of the non-existing directories
#else
    string cmd = "mkdir -p";
#endif
    for (int i=0; i<nParam; i++)
    {
      DStringGDL* pi = dynamic_cast<DStringGDL*>(e->GetParDefined(i));
      for (int j=0; j<pi->N_Elements(); j++)
      {
        string tmp = (*pi)[j];
	//	cout<<tmp<<"--tmp\n";
         if (!noexpand_path) WordExp(tmp);
	 tmp="'"+tmp+"'";
        cmd.append(" " + tmp);
      }
    }
#ifndef _MSC_VER
    cmd.append(" 2>&1 | awk '{print \"% FILE_MKDIR: \" $0; exit 1}'");
#endif
    // SA: calling system(), mkdir and awk is surely not the most efficient way, 
    //     but copying a bunch of code from coreutils does not seem elegant either
    //    system("echo 'hello world'");
    if (system(cmd.c_str()) != 0) e->Throw("failed to create a directory (or execute mkdir).");
  }

}

//#endif
