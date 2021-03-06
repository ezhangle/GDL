#ifndef INC_TreeParserSharedInputState_hpp__
#define INC_TreeParserSharedInputState_hpp__

/* ANTLR Translator Generator
 * Project led by Terence Parr at http://www.jGuru.com
 * Software rights: http://www.antlr.org/license.html
 *
 * $Id: TreeParserSharedInputState.hpp,v 1.1.1.1 2004/12/09 15:10:21 m_schellens Exp $
 */

#include <antlr/config.hpp>
#include <antlr/RefCount.hpp>

#ifdef ANTLR_CXX_SUPPORTS_NAMESPACE
namespace antlr {
#endif

/** This object contains the data associated with an
 *  input AST.  Multiple parsers
 *  share a single TreeParserSharedInputState to parse
 *  the same tree or to have the parser walk multiple
 *  trees.
 */
class ANTLR_API TreeParserInputState {
public:
	TreeParserInputState() : guessing(0) {}
	virtual ~TreeParserInputState() {}

public:
	/** Are we guessing (guessing>0)? */
	int guessing; //= 0;

private:
	// we don't want these:
	TreeParserInputState(const TreeParserInputState&);
	TreeParserInputState& operator=(const TreeParserInputState&);
};

typedef RefCount<TreeParserInputState> TreeParserSharedInputState;

#ifdef ANTLR_CXX_SUPPORTS_NAMESPACE
}
#endif

#endif //INC_TreeParserSharedInputState_hpp__
