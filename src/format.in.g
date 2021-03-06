/* *************************************************************************
                          format.in.g  -   interpreter for formatted input
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002 by Marc Schellens
    email                : m_schellens@hotmail.com
 ***************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

header "pre_include_cpp" {
#include "includefirst.hpp"
}

header "post_include_cpp" {
    // gets inserted after the antlr generated includes in the cpp file
}

header {

#include <fstream>
#include <sstream>

#include "envt.hpp"
#include "io.hpp"             // StreamInfo()

#include "fmtnode.hpp"
//#include "getfmtast.hpp"    // for FMTNodeFactory;    
}

options {
	language="Cpp";
//	genHashLines = true;
	genHashLines = false;
	namespaceStd="std";         // cosmetic option to get rid of long defines
	namespaceAntlr="antlr";     // cosmetic option to get rid of long defines
}	

// the format Parser *********************************************
class FMTIn extends TreeParser;

options {
	importVocab = FMT;	// use vocab generated by format lexer
	buildAST = false;
  	ASTLabelType = "RefFMTNode";
    defaultErrorHandler = false;
//    defaultErrorHandler = true;
//    codeGenBitsetTestThreshold=999;
//    codeGenMakeSwitchThreshold=1;
}

{
public:
    FMTIn( RefFMTNode fmt, std::istream* is_, EnvT* e_, int parOffset, 
           BaseGDL* prompt_)
    : antlr::TreeParser(), 
    noPrompt( true), 
    ioss(), 
    is(is_),  
    prompt( prompt_), e( e_), nextParIx( parOffset),
    valIx(0), termFlag(false), nElements(0)
    {
        nParam = e->NParam();

        NextPar();

        format( fmt);
        
        SizeT nextParIxComp = nextParIx;
        SizeT valIxComp = valIx;

        // format reversion
        while( actPar != NULL)
        {
            format_reversion( reversionAnker);
            
            if( (nextParIx == nextParIxComp) && (valIx == valIxComp))   
                throw GDLException("Infinite format loop detected.");
        }
    }
    
private:
    void NextPar()
    {
        valIx = 0;

        restart:
        if( nextParIx < nParam)
        {
            BaseGDL** par = &e->GetPar( nextParIx);
            if( (*par) != NULL)
            {
                if( e->GlobalPar( nextParIx))
                { // defined global
                    actPar = *par;
                    nElements = actPar->ToTransfer();
                }
                else
                { // defined local
                    if( prompt != NULL)
                    { // prompt keyword there -> error
                        throw GDLException( e->CallingNode(),
                            "Expression must be named variable "
                            "in this context: "+e->GetParString( nextParIx));
                    }
                    else
                    { // prompt not there -> put out or ignore
                        if( is == &std::cin) 
                        {
                            (*par)->ToStream( std::cout);
                            std::cout << std::flush;
                            noPrompt = false;
                        }

                        nextParIx++;
                        goto restart;
                    }
                }
            }
            else
            { // undefined
                if( e->LocalPar( nextParIx))
                throw GDLException( e->CallingNode(),
                    "Internal error: Input: UNDEF is local.");

                nElements = 1;
                (*par) = new DFloatGDL( 0.0);
                actPar = *par;
            }
        } 
        else 
        {
            actPar = NULL;
            nElements = 0;
        }
        nextParIx++;
    }

    void NextVal( SizeT n=1)
    {
//        std::cout << "NextVal("<<n<<")" << std::endl;

        valIx += n;
        if( valIx >= nElements)
            NextPar();

//        std::cout << "valIx:     " << valIx << std::endl;
//        std::cout << "nElements: " << nElements << std::endl;
    }
    
    void GetLine()
    {
	    if( is == &std::cin && noPrompt)
        {
            if( prompt != NULL) 
            {
                prompt->ToStream( std::cout);
                std::cout << std::flush;
            }
            else 
            {
                std::cout << ": " << std::flush;
            }
        }
        else 
        {
            if( is->eof())
            throw GDLIOException( e->CallingNode(), 
                                  "End of file encountered. "+
                                  StreamInfo( is));
        }

//        std::string retStr;
//        getline( *is, retStr);
//        ioss.str( retStr);
        
        std::string initStr("");
        ioss.str( initStr);
//        ioss.seekg( 0);
//        ioss.seekp( 0);
        ioss.rdbuf()->pubseekpos(0,std::ios_base::in | std::ios_base::out);
        ioss.clear();
        is->get( *ioss.rdbuf());

        if ( (is->rdstate() & std::ifstream::failbit ) != 0 )
        {
            if ( (is->rdstate() & std::ifstream::eofbit ) != 0 )
            throw GDLException( e->CallingNode(),
                "End of file encountered. "+
			    StreamInfo( is));
      
            if ( (is->rdstate() & std::ifstream::badbit ) != 0 )
            throw GDLException( e->CallingNode(),
                "Error reading line. "+
			    StreamInfo( is));
      
            is->clear();
            is->get();   // remove delimiter
            return;     // assuming rdbuf == ""
        }

        if( !is->good())
        { 
           if( !is->eof()) 
              throw GDLException( e->CallingNode(), "Error 1 reading data. "+
              StreamInfo( is));
        }

        if( !is->eof()) is->get(); // remove delimiter

        //***
//        std::cout << "FMTIn::GetLine: " << ioss.str() << "." << std::endl;
//        std::cout << "tellg: " << ioss.tellg() << std::endl;
    }

    bool noPrompt;

    std::stringstream ioss;
    std::istream* is;
    BaseGDL* prompt;

    EnvT*    e;
    SizeT   nextParIx;
    SizeT   valIx;

    bool termFlag;

    SizeT   nParam;
    BaseGDL* actPar;
    SizeT nElements;

    RefFMTNode reversionAnker;
}

format
    : #(fmt:FORMAT 
            { goto realCode; } // fool ANTLR 
            q (f q)+ // this gets never executed
            {
                realCode:

                reversionAnker = #fmt;
                
                RefFMTNode blk = _t; // q (f q)+

                // as later format_recursive is used, this loop only
                // loops once (ie. could be eliminated - left here in
                // case of later changes)
                for( int r = #fmt->getRep(); r > 0; r--)
                {
                    GetLine(); 

                    q( blk);
                    _t = _retTree;

                    for (;;) 
                    {
                        if( _t == static_cast<RefFMTNode>(antlr::nullAST))
                            _t = ASTNULL;

                        switch ( _t->getType()) {
                        case FORMAT:
                        case STRING:
                        case CSTRING:
                        case TL:
                        case TR:
                        case TERM:
                        case NONL:
                        case Q: case T: case X: case A:
                        case F: case D: case E: case G:
                        case I: case O: case B: case Z: case ZZ: case C:
                            {
                                f(_t);
                                if( actPar == NULL && termFlag) goto endFMT;
                                _t = _retTree;
                                q(_t);
                                _t = _retTree;
                                break; // out of switch
                            }
                        default:
                            goto endFMT;
                        }
                    }
                    
                    endFMT: // end of one repetition
                    if( actPar == NULL && termFlag) break;
                }
            }
        )
    ;

format_recursive // don't read in a new line
    : #(fmt:FORMAT 
            { goto realCode; } // fool ANTLR 
            q (f q)+ // this gets never executed
            {
                realCode:

                reversionAnker = #fmt;
                
                RefFMTNode blk = _t; // q (f q)+

                for( int r = #fmt->getRep(); r > 0; r--)
                {
                    //    GetLine(); // the difference to format 

                    q( blk);
                    _t = _retTree;

                    for (;;) 
                    {
                        if( _t == static_cast<RefFMTNode>(antlr::nullAST))
                            _t = ASTNULL;

                        switch ( _t->getType()) {
                        case FORMAT:
                        case STRING:
                        case CSTRING:
                        case TL:
                        case TR:
                        case TERM:
                        case NONL:
                        case Q: case T: case X: case A:
                        case F: case D: case E: case G:
                        case I: case O: case B: case Z: case ZZ: case C:
                            {
                                f(_t);
                                if( actPar == NULL && termFlag) goto endFMT;
                                _t = _retTree;
                                q(_t);
                                _t = _retTree;
                                break; // out of switch
                            }
                        default:
                            goto endFMT;
                        }
                    }
                    
                    endFMT: // end of one repetition
                    if( actPar == NULL && termFlag) break;
                }
            }
        )
    ;

format_reversion
    : format // use the non-recursive format here 
        { goto realCode; } 
            q (f q)* // this gets never executed
        {
            realCode:

            q( _t);
            _t = _retTree;

            for (;;) 
            {
                if( _t == static_cast<RefFMTNode>(antlr::nullAST))
                _t = ASTNULL;

                switch ( _t->getType()) {
                case FORMAT:
                case STRING:
                case CSTRING:
                case TL:
                case TR:
                case TERM:
                case NONL:
                case Q: case T: case X: case A:
                case F: case D: case E: case G:
                case I: case O: case B: case Z: case ZZ: case C:
                    {
                        f(_t);
                        if( actPar == NULL) goto endFMT;
                        _t = _retTree;
                        q(_t);
                        _t = _retTree;
                        break; // out of switch
                    }
                default:
                    goto endFMT;
                }
            }
            endFMT:
        }
    ;

q
    : (s:SLASH 
            {
                for( int r=s->getRep(); r > 0; r--) GetLine();
            }
        )?
    ;

f_csubcode // note: IDL doesn't allow hollerith strings inside C()
    : s:STRING // ignored on input
//    | CSTRING  // ignored on input
    | tl:TL 
        { 
            SizeT actP  = ioss.tellg(); 
            int    tlVal = tl->getW();
            if( tlVal > actP)
                ioss.seekg( 0);
            else
                ioss.seekg( actP - tlVal);
        }
    | tr:TR 
        { 
            int    tlVal = tl->getW();
            ioss.seekg( tlVal, std::ios_base::cur);
        }
    ;

f
{
    RefFMTNode actNode;
}
    : TERM { termFlag = true; }
    | NONL // ignored on input
    | Q 
        {
            SizeT nLeft = ioss.rdbuf()->in_avail();
            std::istringstream iossTmp( i2s( nLeft));
            int r = 1;
            do {
                SizeT tCount = actPar->IFmtA( &iossTmp, valIx, r, 0);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | t:T
        { 
            int    tVal = t->getW();
            assert( tVal >= 1);
            ioss.seekg( tVal-1, std::ios_base::beg);
        }
    | f_csubcode
    | x
    | format_recursive // following are repeatable formats
    | a:A 
        {
            if( actPar == NULL) break;

            int r = a->getRep();
            int w = a->getW();
            do {
                SizeT tCount = actPar->IFmtA( &ioss, valIx, r, w);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | (   ff:F { actNode = ff;}
        | ee:E { actNode = ee;}
        | g:G { actNode = g;}
            //  | d:D // D is transformed to F
        )
        {
            if( actPar == NULL) break;
            
            int r = actNode->getRep();
            int w = actNode->getW();
//             if( w <= 0) 
//                 if( actPar->Type() == FLOAT) 
//                 w = 15; // set default
//                 else
//                 w = 25;
            do {
                SizeT tCount = actPar->IFmtF( &ioss, valIx, r, w);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | i:I
        {
            if( actPar == NULL) break;
            
            int r = i->getRep();
            int w = i->getW();
            do {
                SizeT tCount = actPar->IFmtI( &ioss, valIx, r, w,
                                               BaseGDL::DEC);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | o:O
        {
            if( actPar == NULL) break;
            
            int r = o->getRep();
            int w = o->getW();
            do {
                SizeT tCount = actPar->IFmtI( &ioss, valIx, r, w, 
                                               BaseGDL::OCT);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | b:B
        {
            if( actPar == NULL) break;
            
            int r = b->getRep();
            int w = b->getW();
            do {
                SizeT tCount = actPar->IFmtI( &ioss, valIx, r, w, 
                                               BaseGDL::BIN);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | (   z:Z { actNode = z;}
        | zz:ZZ { actNode = zz;}
        )
        {
            if( actPar == NULL) break;
            
            int r = actNode->getRep();
            int w = actNode->getW();
            do {
                SizeT tCount = actPar->IFmtI( &ioss, valIx, r, w,
                                               BaseGDL::HEX);
                r -= tCount;
                NextVal( tCount);
                if( actPar == NULL) break;
            } while( r>0);
        }
    | #(c:C (csubcode)+) 
    ;   

csubcode
    : c1:CMOA
    | c2:CMoA
    | c3:CmoA
    | c4:CHI
    | c5:ChI
    | c6:CDWA
    | c7:CDwA
    | c8:CdwA
    | c9:CAPA
    | c10:CApA
    | c11:CapA
    | c12:CMOI
    | c13:CDI 
    | c14:CYI
    | c15:CMI
    | c16:CSI
    | c17:CSF
    | x
    | f_csubcode
    ;

x
    : tl:X 
        {
            if( _t != static_cast<RefFMTNode>(antlr::nullAST))
            {
                int    tlVal = #tl->getW();
                ioss.seekg( tlVal, std::ios_base::cur);
            }
        }
    ;
