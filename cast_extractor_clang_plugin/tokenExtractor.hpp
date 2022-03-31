//
//  tokenExtractor.hpp
//  Clang
//
//  Created by Costin on 11/11/2019.
//
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTTypeTraits.h"
#include "nodeLocation.hpp"
#include "nodeToExtract.hpp"
#define JSON_NOEXCEPTION
#include "json.hpp"

using json = nlohmann::json;

 
class tokenExtractor: public RecursiveASTVisitor<tokenExtractor> 
{
	private:
	    ASTContext *Context;
	    vector <nodeToExtract *> nodesToExtract;
	public:
    	explicit tokenExtractor(ASTContext *Context,vector<nodeToExtract *> nodes):Context(Context), nodesToExtract(nodes)  {}
    

	bool isStmtSubsumed(vector<Stmt *> stmts, Stmt *s)
	{
		for(Stmt *t: stmts )
		{
			if(rangeSubsumes(t->getSourceRange(), s->getSourceRange()))
			{
				return true;
			}
		}
		return false;
	}  


	void addStmtToSnippet(Stmt *s)
	{
		for(nodeToExtract *lNTE: nodesToExtract)
		{
     
			if(!rangeSubsumes(lNTE->getSpanRHS(), s->getSourceRange())) 
			{
				continue;
			}
     

			for(nodeToExtract *lNTE: nodesToExtract) 
			{
				if(rangeSubsumes(lNTE->getSpanRHS() , s->getSourceRange()))
				{
					if(!isStmtSubsumed(lNTE->getStmts(), s)) 
					{
						lNTE->addStmt(s);
						return;
					}
				}
			}

		}
	}
    

	bool VisitStmt(Stmt *s)
	{
        if(!s->getSourceRange().isValid())
        {
            return true;
        }
        
        if (Context->getSourceManager().isInMainFile(s->getBeginLoc()))
        {
            for(nodeToExtract *lNTE: nodesToExtract)
            {
                if(rangeSubsumes(lNTE->getSpanRHS(), s->getSourceRange()))
                {
                	// if(lNTE->getType() == "Stmt ImplicitCastExpr")
                    addStmtToSnippet(s);
                }
            }
        }
 		return true;
	}

};

