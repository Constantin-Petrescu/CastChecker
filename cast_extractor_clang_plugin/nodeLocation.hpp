//
//  nodeLocation.hpp
//  Clang
//
//  Created by Costin on 18/08/2018.
//
//
#ifndef nodeLocation_hpp
#define nodeLocation_hpp
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

//////////////////////////////////////////////////////////
//            BEGIN OF LOCATION HANDLER                 //
//////////////////////////////////////////////////////////

// 
// Location class for a node!
// 
class NodeLocation
{
public:
    string filename;
    int startLine;
    int endLine;
    int startColumn;
    int endColumn;
    
    template <typename T>
    bool assertIsValid(clang::ASTContext *Context, T const& lVar)
    {
        PresumedLoc lPresumedLoc = Context->getSourceManager().getPresumedLoc(lVar->getBeginLoc());
        return lPresumedLoc.isValid();
    }

    template <typename T>
    string getFilename(clang::ASTContext *Context, T const& lVar)
    {
        PresumedLoc lPresumedLoc = Context->getSourceManager().getPresumedLoc(lVar->getBeginLoc());
        return lPresumedLoc.getFilename();
    }
    
    template <typename T>
    int getLineStart(clang::ASTContext *Context, T const& lVar)
    {
        PresumedLoc lPresumedLoc = Context->getSourceManager().getPresumedLoc(lVar->getBeginLoc());
        return int(lPresumedLoc.getLine());
    }
    
    template <typename T>
    int getColumnStart(clang::ASTContext *Context, T const& lVar)
    {
        PresumedLoc lPresumedLoc = Context->getSourceManager().getPresumedLoc(lVar->getBeginLoc());
        return int(lPresumedLoc.getColumn());
    }
    
    template <typename T>
    int getLineEnd(clang::ASTContext *Context, T const& lVar)
    {
        PresumedLoc lPresumedLoc = Context->getSourceManager().getPresumedLoc(lVar->getEndLoc());
        return int(lPresumedLoc.getLine());
    }
    
    template <typename T>
    int getColumnEnd(clang::ASTContext *Context, T const& lVar)
    {
        PresumedLoc lPresumedLoc = Context->getSourceManager().getPresumedLoc(lVar->getEndLoc());
        return int(lPresumedLoc.getColumn());
    }
    
    template <typename T>
    string getLineStartString(clang::ASTContext *Context, T const& lVar)
    {
       
        return to_string(getLineStart(Context, lVar));
    }
    
    template <typename T>
    string getColumnStartString(clang::ASTContext *Context, T const& lVar)
    {
        return to_string(getColumnStart(Context, lVar));
    }
    
    template <typename T>
    string getLineEndString(clang::ASTContext *Context, T const& lVar)
    {
        
        return to_string(getLineEnd(Context, lVar));
    }
    
    template <typename T>
    string getColumnEndString(clang::ASTContext *Context, T const& lVar)
    {
        return to_string(getColumnEnd(Context, lVar));
    }

};
//////////////////////////////////////////////////////////
//              END OF LOCATION HANDLER                 //
//////////////////////////////////////////////////////////


#endif /* nodeLocation_hpp */