//===- extract_casts.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//          Base file which start the execution of extractor.hpp
// 
//===----------------------------------------------------------------------===//

 
#include "extractor.hpp"

    
class PrintFunctionNamesAction : public PluginASTAction 
{
    std::set<std::string> ParsedTemplates;
protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,llvm::StringRef) override 
    {
        return make_unique<PrintFunctionsConsumer>(CI, ParsedTemplates);
    }
    
    bool ParseArgs(const CompilerInstance &CI,
                   const std::vector<std::string> &args) override 
    {
        for (unsigned i = 0, e = args.size(); i != e; ++i) 
        {
            llvm::errs() << "Print args = " << args[i] << "\n";
            
            DiagnosticsEngine &D = CI.getDiagnostics();
            if (args[i] == "-an-error") 
            {
                unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,"invalid argument '%0'");
                D.Report(DiagID) << args[i];
                return false;
            } 
            else if (args[i] == "-parse-template") 
            {
                if (i + 1 >= e) 
                {
                    D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,"missing -parse-template argument"));
                    return false;
                }
                ++i;
                ParsedTemplates.insert(args[i]);
            }
        }
        return true;
    }
};


// Registering the plugin command to extract_casts
static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X("extract_casts", "extract casts");


