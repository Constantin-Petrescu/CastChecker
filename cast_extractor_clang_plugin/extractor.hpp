#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/raw_ostream.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fstream"
#include "iostream"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "writerHandler.hpp"
#include "nodeLocation.hpp"
#include "tokenizer.hpp"
#include "tokenExtractor.hpp"
#include "nodeToExtract.hpp"
#define JSON_NOEXCEPTION
#include "json.hpp"

using namespace clang;
using namespace std;
using json = nlohmann::json;

vector <nodeToExtract *> gNodesToExtract;
    
class FindVar: public RecursiveASTVisitor<FindVar>
{
  ASTContext *Context;
  public:
  FindVar(ASTContext *Context): Context(Context){}

  bool VisitVarDecl(VarDecl *lVD)
  {
    NodeLocation lLocation;
    if(!lLocation.assertIsValid(Context, lVD))
      return true;
    if (!Context->getSourceManager().isInMainFile(lVD->getBeginLoc()))
      return true;
    if(!lVD->getAnyInitializer())
      return true;

    string bufVD, bufARG, bufRH, buf;
    llvm::raw_string_ostream lVD_out(bufVD), lARG_out(bufARG); 
    llvm::raw_string_ostream lRH_out(bufRH), lVFrom_out(buf);
    lVD->print(lVD_out, 0, false);    

    const Expr* expr_rh = lVD->getAnyInitializer();
    expr_rh->printPretty(lRH_out, NULL, PrintingPolicy(LangOptions()));
    
    const char* node_type_string = expr_rh->getStmtClassName();
    if(strcmp(node_type_string, "CXXConstCastExpr")       == 0 || 
       strcmp(node_type_string, "CXXDynamicCastExpr")     == 0 ||
       strcmp(node_type_string, "CXXStaticCastExpr")      == 0 || 
       strcmp(node_type_string, "CXXReinterpretCastExpr") == 0  )
    {
      nodeToExtract *lNTE = new nodeToExtract();
      cout<<"NameDecl:  "<< lVD_out.str() << " -- "<<lRH_out.str()<< "\n" ;        

      // Setting the location
      lLocation.filename    = lLocation.getFilename(Context,expr_rh);
      lLocation.startLine   = lLocation.getLineStart(Context, expr_rh);
      lLocation.startColumn = lLocation.getColumnStart(Context, expr_rh);
      lLocation.endLine     = lLocation.getLineEnd(Context, expr_rh);
      lLocation.endColumn   = lLocation.getColumnEnd(Context, expr_rh);
      
      // Setting the information for JSON
      lNTE->setSpan(expr_rh->getBeginLoc(),expr_rh->getEndLoc());
      lNTE->setType("VarDecl " + (string)(node_type_string));
      lNTE->setLocation(lLocation);
      lNTE->setValue(lVD_out.str());

      // Src node -- variable mapping to cast string and type
      lNTE->setDstValue(lVD->getNameAsString());
      lNTE->setDstType(lVD->getType().getAsString());

      // Src node - cast string and type
      // cases: 
      // 1: default and most common
      // 2: builtin when there is 
      // 3: type of initialiser is same with the definition type
      const Expr* sub_expr_rh = (dyn_cast<CastExpr>(expr_rh))->getSubExpr();
      if(sub_expr_rh->getType().getAsString() != (lNTE->getDstType()))
      {
        lNTE->setSrcType(sub_expr_rh->getType().getAsString());
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
      }        
      else if(strcmp(sub_expr_rh->getStmtClassName(),"ImplicitCastExpr") == 0)
      {
        const Expr* sub_sub_expr_rh = nullptr;
        sub_sub_expr_rh = (dyn_cast<CastExpr>(sub_expr_rh))->getSubExpr();
        lNTE->setSrcType(sub_sub_expr_rh->getType().getAsString());

        SourceLocation begin_sub_sub_expr_rh = sub_sub_expr_rh->getBeginLoc();
        SourceLocation end_sub_sub_expr_rh = sub_sub_expr_rh->getEndLoc();
        lNTE->setSpanRHS(begin_sub_sub_expr_rh,end_sub_sub_expr_rh);
        lNTE->setExtraInfo("Builtin casting function;");
      }    
      else
      {
        lNTE->setSrcType(sub_expr_rh->getType().getAsString());
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
        lNTE->setExtraInfo("From and To same type;");
      }
      sub_expr_rh->printPretty(lVFrom_out, NULL, PrintingPolicy(LangOptions()));
      lNTE->setSrcValue(lVFrom_out.str());
    
      gNodesToExtract.push_back(lNTE);
    }
    return true;
  }


  bool VisitCallExpr(CallExpr *lCE)
  {
    NodeLocation lLocation;
    if(!lLocation.assertIsValid(Context, lCE))
      return true;
    if (!Context->getSourceManager().isInMainFile(lCE->getBeginLoc()))
      return true;
    if(lCE->getNumArgs()<1)
      return true;

    string bufCE, bufARG, bufRH;
    llvm::raw_string_ostream lCE_out(bufCE), lARG_out(bufARG), rh_out(bufRH);
    lCE->printPretty(lCE_out, NULL, PrintingPolicy(LangOptions()));

    for (unsigned int lIt = 0; lIt < lCE->getNumArgs(); lIt++)
    {
        // cout<<"CallExpr: "<<lCE_out.str() << "\n";
      const Expr* arg = lCE->getArg(lIt);
      arg->printPretty(lARG_out, NULL, PrintingPolicy(LangOptions()));
      lCE->getCallee()->printPretty(rh_out, NULL,PrintingPolicy(LangOptions()));
        
      if(strcmp(arg->getStmtClassName(), "CXXConstCastExpr")       == 0 || 
         strcmp(arg->getStmtClassName(), "CXXDynamicCastExpr")     == 0 ||
         strcmp(arg->getStmtClassName(), "CXXStaticCastExpr")      == 0 || 
         strcmp(arg->getStmtClassName(), "CXXReinterpretCastExpr") == 0  )
      {
        nodeToExtract *lNTE = new nodeToExtract();
        
        const char* node_type_string = arg->getStmtClassName();


        cout<<"CallExpr:  "<<lCE_out.str() << "\n" ;
        cout<<"argument "<< lIt << ": " << lARG_out.str() << " type: ";
        cout<<lCE->getArg(lIt)->getStmtClassName()<<" - "<<rh_out.str()<<" \n";

        // Extraction of location
        lLocation.filename    = lLocation.getFilename(Context,arg);
        lLocation.startLine   = lLocation.getLineStart(Context, arg);
        lLocation.startColumn = lLocation.getColumnStart(Context, arg);
        lLocation.endLine     = lLocation.getLineEnd(Context, arg);
        lLocation.endColumn   = lLocation.getColumnEnd(Context, arg);
        
        // Setting the information for JSON
        lNTE->setSpan(arg->getBeginLoc(),arg->getEndLoc());
        lNTE->setType("CallExpr " + (string)(node_type_string));
        lNTE->setLocation(lLocation);
        lNTE->setValue(lCE_out.str());

        if(FunctionDecl* lFD = lCE->getDirectCallee())
        {
          for(unsigned int it_param = 0;it_param<lFD->getNumParams();it_param++)
          {
            if (lIt == it_param)
            {
              string buf2;
              llvm::raw_string_ostream l2(buf2);
              const ParmVarDecl* param_decl = lFD->getParamDecl(it_param);
              SplitQualType param_type = param_decl->getType().split();
              string type=QualType::getAsString(param_type, PrintingPolicy{{}});
              lFD->getParamDecl(it_param)->print(l2,PrintingPolicy{ {} });
              lNTE->setDstType(type);
              if(l2.str().size() > lNTE->getDstType().size())
              {
                lNTE->setDstValue(l2.str().erase(0,lNTE->getDstType().size()));
              }
              else
              {
                lNTE->setDstValue(l2.str());
                lNTE->setExtraInfo("Parameter name extracted wrong;");
              }

              NodeLocation lLocFD;
              if(lLocFD.assertIsValid(Context, lCE))
              {
                lLocFD.filename = lLocFD.getFilename(Context,arg);
                lLocFD.startLine = lLocFD.getLineStart(Context, arg);
                lLocFD.startColumn = lLocFD.getColumnStart(Context, arg);
                lLocFD.endLine = lLocFD.getLineEnd(Context, arg);
                lLocFD.endColumn = lLocFD.getColumnEnd(Context, arg);
                lNTE->setLocFD(lLocFD);
              }
            }
          }
        }
        else
        {
          lNTE->setExtraInfo("Getting functionDecl failed");
        }
        // Src node -- variable mapping to cast string and type
        // cases: 
        // 1: default and most common
        // 2: builtin when there is 
        // 3: type of initialiser is same with the definition type
        const Expr* sub_expr_arg = (dyn_cast<CastExpr>(arg))->getSubExpr();
        if(sub_expr_arg->getType().getAsString() != (lNTE->getDstType()))
        {
          lNTE->setSrcType(sub_expr_arg->getType().getAsString());
          SourceLocation begin_sub_expr_arg=sub_expr_arg->getBeginLoc();
          SourceLocation end_sub_expr_arg = sub_expr_arg->getEndLoc();
          lNTE->setSpanRHS(begin_sub_expr_arg,end_sub_expr_arg);
        }    
        else if(strcmp(sub_expr_arg->getStmtClassName(),"ImplicitCastExpr")== 0)
        {
          const Expr* sub_sub_expr_arg = nullptr;
          sub_sub_expr_arg = (dyn_cast<CastExpr>(sub_expr_arg))->getSubExpr();
          lNTE->setSrcType(sub_sub_expr_arg->getType().getAsString());
          SourceLocation begin_sub_sub_expr_arg=sub_sub_expr_arg->getBeginLoc();
          SourceLocation end_sub_sub_expr_arg = sub_sub_expr_arg->getEndLoc();
          lNTE->setSpanRHS(begin_sub_sub_expr_arg, end_sub_sub_expr_arg);
          lNTE->setExtraInfo("Builtin casting function;");
        }
        else
        {
          lNTE->setSrcType(sub_expr_arg->getType().getAsString());
          SourceLocation begin_sub_expr_arg=sub_expr_arg->getBeginLoc();
          SourceLocation end_sub_expr_arg = sub_expr_arg->getEndLoc();
          lNTE->setSpanRHS(begin_sub_expr_arg,end_sub_expr_arg);
          lNTE->setExtraInfo("From and To same type;");
        }
        
        string buf;
        llvm::raw_string_ostream lVFrom_out(buf);
        sub_expr_arg->printPretty(lVFrom_out, NULL, PrintingPolicy(LangOptions()));
        lNTE->setSrcValue(lVFrom_out.str());

        gNodesToExtract.push_back(lNTE);
      }
    }
    return true; 
  }


  bool VisitBinaryOperator(BinaryOperator *lBOp)
  {
    NodeLocation lLocation;
    if(!lLocation.assertIsValid(Context, lBOp))
      return true;
    if (!Context->getSourceManager().isInMainFile(lBOp->getBeginLoc()))
      return true;
    if(!lBOp->isAssignmentOp())
      return true;
           
    string bufBOP, bufLH, bufRH;
    llvm::raw_string_ostream lBOP_out(bufBOP), lLH_out(bufLH), lRH_out(bufRH);
    lBOp->printPretty(lBOP_out, NULL, PrintingPolicy(LangOptions()));

    const Expr* lRH = lBOp->getRHS();
    lRH->printPretty(lRH_out, NULL, PrintingPolicy(LangOptions()));
    
    const Expr* lLH = lBOp->getLHS();
    lLH->printPretty(lLH_out, NULL, PrintingPolicy(LangOptions()));

    if(strcmp(lRH->getStmtClassName(), "CXXConstCastExpr")       == 0 || 
       strcmp(lRH->getStmtClassName(), "CXXDynamicCastExpr")     == 0 || 
       strcmp(lRH->getStmtClassName(), "CXXStaticCastExpr")      == 0 || 
       strcmp(lRH->getStmtClassName(), "CXXReinterpretCastExpr") == 0  )
    {
      nodeToExtract *lNTE = new nodeToExtract();
      cout<<"binaryOperator:  "<<lBOP_out.str()<<"\n";
      
      const Expr* sub_expr_rh = (dyn_cast<CastExpr> (lRH))->getSubExpr();
      string buf;
      llvm::raw_string_ostream lVFrom_out(buf);
      sub_expr_rh->printPretty(lVFrom_out, NULL, PrintingPolicy(LangOptions()));

      // Extraction of location
      lLocation.filename = lLocation.getFilename(Context,lBOp);
      lLocation.startLine = lLocation.getLineStart(Context, lBOp);
      lLocation.startColumn = lLocation.getColumnStart(Context, lBOp);
      lLocation.endLine = lLocation.getLineEnd(Context, lBOp);
      lLocation.endColumn = lLocation.getColumnEnd(Context, lBOp);
      
      // Setting the information for JSON
      lNTE->setSpan(lBOp->getBeginLoc(),lBOp->getEndLoc());
      lNTE->setType("BinaryOperator " + (string)(lRH->getStmtClassName()));
      lNTE->setLocation(lLocation);
      lNTE->setValue(lBOP_out.str());

      // Dst node -- variable mapping to cast string and type
      lNTE->setDstValue(lLH_out.str());
      lNTE->setDstType((lLH->getType()).getAsString());

      // Src node -- variable mapping to cast string and type
      // cases: 
      // 1: default and most common by collecting the sub expresion type 
      // 2: CallExpr with same type
      // 3: builtin when there is 
      // 4: dependent type
      // 5: type of initialiser is same with the definition type
      SplitQualType param_type = sub_expr_rh->getType().split();
      string sub_rh_type=QualType::getAsString(param_type, PrintingPolicy{{}});
      if(sub_rh_type != (lLH->getType()).getAsString())
      {
        lNTE->setSrcType(sub_rh_type);
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
      }
      else if(strcmp(sub_expr_rh->getStmtClassName(),"CallExpr") == 0)
      {
        lNTE->setSrcType(sub_rh_type);
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
        lNTE->setExtraInfo("CallExpr with same type;");
      }
      else if(strcmp(sub_expr_rh->getStmtClassName(),"ImplicitCastExpr") == 0)
      {
        const Expr* sub_sub_expr_rh = nullptr;
        sub_sub_expr_rh = (dyn_cast<CastExpr>(sub_expr_rh))->getSubExpr();
        SplitQualType param_type = sub_sub_expr_rh->getType().split();
        string type=QualType::getAsString(param_type, PrintingPolicy{{}});       
        lNTE->setSrcType(type);
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
        lNTE->setExtraInfo("Builtin casting function;");
      }
      else if(sub_rh_type == "<dependent type>")
      {
        lNTE->setSrcType("<dependent type>");
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
        lNTE->setExtraInfo("Dependent type;");
      }
      else
      {
        lNTE->setSrcType(sub_rh_type);
        lNTE->setSpanRHS(sub_expr_rh->getBeginLoc(),sub_expr_rh->getEndLoc());
        lNTE->setExtraInfo("From and To same type;");
      }
      lNTE->setSrcValue(lVFrom_out.str());
      gNodesToExtract.push_back(lNTE);
    }
    return true;
  }
};

// take the vector of nodes from ast and put them into JSON desired format
json prepareJsonFromVector(nodeToExtract *lNTE, ASTContext& context, 
                           CompilerInstance &CI)
{
  json lResult, lSrc, lDst, lAst;

  lAst["node_type"] = lNTE->getType();
  lAst["tokens"] = lNTE->getValue();
  lAst["file"]   = lNTE->getLocation().filename;
  lAst["begin"]  = to_string(lNTE->getLocation().startLine) + ":" 
                 + to_string(lNTE->getLocation().startColumn);
  lAst["end"]    = to_string(lNTE->getLocation().endLine) + ":"
                 + to_string(lNTE->getLocation().endColumn);
  


  lSrc["text"] = lNTE->getSrcValue();
  lSrc["type"] = lNTE->getSrcType();
  lSrc["qualifier"] = "Use Qualifier class to do it.";

  lDst["text"] = lNTE->getDstValue();
  lDst["type"] = lNTE->getDstType();
  lDst["qualifier"] = "Use Qualifier class to do it.";
  if(lNTE->getLocFD().filename != lAst["file"] &&
     !lNTE->getLocFD().filename.empty())
  {
    lDst["begin"]  = to_string(lNTE->getLocFD().startLine) + ":"
                   + to_string(lNTE->getLocFD().startColumn);
    lDst["end"]    = to_string(lNTE->getLocFD().endLine) + ":"
                   + to_string(lNTE->getLocFD().endColumn);
    lDst["file"]   = lNTE->getLocFD().filename;  
  }
  
  Tokenizer tokenizer_rhs;
  lResult["x_children"] = tokenizer_rhs.tokenizeStmts(&context, &CI, lNTE);

  // Put this one last in case the tokenizer find is Macro
  if(!lNTE->getExtraInfo().empty())
    lAst["extra_info"] = lNTE->getExtraInfo();

  lResult["from"] = lSrc;
  lResult["to"] = lDst;
  lResult["ast_meta_info"] = lAst; 
  return lResult;
}

class PrintFunctionsConsumer : public ASTConsumer
{
    CompilerInstance &Instance;
    std::set<std::string> ParsedTemplates;
    writerHandler *writer;
    
public:
  PrintFunctionsConsumer(CompilerInstance &Instance, 
                         std::set<std::string> ParsedTemplates): 
                         Instance(Instance), 
                         ParsedTemplates(ParsedTemplates)
  {
      writer = new writerHandler();
  }
  
  void HandleTranslationUnit(ASTContext& context) override
  {
    
    // Parser for nodes which are going to be extracted
    FindVar lV(&context);
    lV.TraverseDecl(context.getTranslationUnitDecl());

    std::set<string> all_files;
    json lOutput;

    // Generating the JSON's and the filenames for writing.
    for(nodeToExtract *lNTE: gNodesToExtract) 
    {
      json result = prepareJsonFromVector(lNTE, context, Instance);
      string comment_file = lNTE->getLocation().filename;
      
      if(all_files.count(comment_file)) 
      {
        lOutput[comment_file] += result;
      }
      else
      {
        lOutput[comment_file] += result;
        all_files.insert(comment_file);
      }
    }
      
    // Writing the JSON into files.
    for (string f: all_files)
    {
      string outFile = f;
      outFile = outFile.replace(outFile.begin(), outFile.begin() + 6, "");
      writer->writeToLucidFile(outFile, lOutput[f].dump(4) + "\n");
    }
  }
};
