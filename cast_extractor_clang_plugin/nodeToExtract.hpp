//
//  nodeLocation.hpp
//  Clang
//
//  Created by Costin on 20/08/2018.
//
#ifndef nodeToExtract_hpp
#define nodeToExtract_hpp
#include "nodeLocation.hpp"
#define JSON_NOEXCEPTION
#include "json.hpp"
using json = nlohmann::json;
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Token.h"
#include "clang/Lex/TokenConcatenation.h"
#include "clang/CrossTU/CrossTranslationUnit.h"


// check if b is in a
bool rangeSubsumes(const SourceRange &a, const SourceRange &b) 
{
    return (a.getBegin() <  b.getBegin() && b.getEnd() <  a.getEnd()) || 
           (a.getBegin() == b.getBegin() && b.getEnd() <  a.getEnd()) ||
           (a.getBegin() <  b.getBegin() && b.getEnd() == a.getEnd()) ||
           (a.getBegin() == b.getBegin() && b.getEnd() == a.getEnd());
}

string getSourceRangeAsString(const SourceManager &SM, const SourceRange &sr)
{
    string buf;
    llvm::raw_string_ostream rso(buf);
    sr.getBegin().print(rso, SM);
    sr.getEnd().print(rso, SM);
    return rso.str();
}

// 
// Node from ast which needs to be extracted
// 
class nodeToExtract
{
public:
    std::vector<nodeToExtract *> children;
    std::vector<Stmt *> stmts;
    NodeLocation gLocation,gLocFuncDecl;
    SourceRange span;
    string value;
    string type;
    string SrcValue,SrcType,DstValue,DstType,ExtraInfo;

    
    NodeLocation getLocation()
    {
        return gLocation;
    }
    
    void setLocation(NodeLocation lNodeLocation)
    {
        gLocation = lNodeLocation;
    }

    NodeLocation getLocFD()
    {
        return gLocFuncDecl;
    }

    void setLocFD(NodeLocation lNodeLocation)
    {
        gLocFuncDecl = lNodeLocation;
    }
    
 
    string getType()
    {
        return type;
    }
    
    void setType(string lType)
    {
        type = lType;
    }
    
    string getValue()
    {
        return value;
    }

    void setValue(string lVal)
    {
        value = lVal;
    }

    // Locations for various util functions
    SourceLocation getSpanEnd()
    {
        return span.getEnd();
    }
    
    SourceLocation getSpanBegin()
    {
        return span.getBegin();
    }
    
    SourceRange getSpan()
    {
        return span;
    }
    
    void setSpan(SourceLocation begin, SourceLocation end) 
    {
        span = SourceRange(begin, end);
    }


    // Locations for RHS for tokeniser
    SourceLocation getSpanEndRHS()
    {
        return span.getEnd();
    }
    
    SourceLocation getSpanBeginRHS()
    {
        return span.getBegin();
    }
    
    SourceRange getSpanRHS()
    {
        return span;
    }
    
    void setSpanRHS(SourceLocation begin, SourceLocation end) 
    {
        span = SourceRange(begin, end);
    }


    void addStmt (Stmt *s)
    {
        stmts.push_back(s);
    }

    std::vector<Stmt *> getStmts ()
    {
        return stmts;
    }

    void addChild (nodeToExtract *child)
    {
        children.push_back(child);
    }
    
    std::vector<nodeToExtract *> getChildren ()
    {
        return children;
    }
    
    bool hasChildren()
    {
        if ( children.size() == 0 )
        {
            return false;
        }
        return true;
    }

    void setSrcValue(string lValue)
    {
        SrcValue = lValue;
    }

    string getSrcValue()
    {
        return SrcValue;
    }

    void setSrcType(string lType)
    {
        SrcType = lType;
    }

    string getSrcType()
    {
        return SrcType;
    }

    void setDstValue(string lValue)
    {
        DstValue = lValue;
    }

    string getDstValue()
    {
        return DstValue;
    }

    void setDstType(string lType)
    {
        DstType = lType;
    }

    string getDstType()
    {
        return DstType;
    }

    void setExtraInfo(string lValue)
    {
        if(!ExtraInfo.empty())
            ExtraInfo += " " + lValue;
        else
            ExtraInfo += lValue;
    }

    string getExtraInfo()
    {
        return ExtraInfo;
    }
};
#endif /* nodeToExtract_hpp */

