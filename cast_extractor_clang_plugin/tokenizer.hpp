//
//  tokenizer.hpp
//  Clang
//
//  Created by Costin on 11/11/2019.
//
#ifndef tokenizer_hpp
#define tokenizer_hpp
#include "nodeLocation.hpp"
#include "nodeToExtract.hpp"
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


// 
// tokenizer class 
// 
class Tokenizer
{
public:

  
  struct ExpansionInfo 
  {
    std::string MacroName;
    std::string Expansion;
    ExpansionInfo(std::string N, std::string E)
    : MacroName(std::move(N)), Expansion(std::move(E)) {}
  };
    
  using ExpArgTokens = llvm::SmallVector<Token, 2>;
    
  /// Maps unexpanded macro arguments to expanded arguments. A macro argument 
  /// may need to expanded further when it is nested inside another macro.
  class MacroArgMap : public std::map<const IdentifierInfo *, ExpArgTokens> 
  {
    public:
      void expandFromPrevMacro(const MacroArgMap &Super);
  };
    
  struct MacroNameAndArgs 
  {
    std::string Name;
    const MacroInfo *MI = nullptr;
    MacroArgMap Args;
      
    MacroNameAndArgs(std::string N, const MacroInfo *MI, MacroArgMap M)
      : Name(std::move(N)), MI(MI), Args(std::move(M)) {}
  };
        
    
  void tokenizeMacro(std::vector<std::string> &macro_tokens, SourceLocation MacroLoc, const Preprocessor &PP, const MacroArgMap &PrevArgs, llvm::SmallPtrSet<IdentifierInfo *, 8> &AlreadyProcessedTokens)
  {
    const SourceManager &SM = PP.getSourceManager();
      
    MacroNameAndArgs Info=getMacroNameAndArgs(SM.getExpansionLoc(MacroLoc),PP);
    IdentifierInfo* IDInfo = PP.getIdentifierInfo(Info.Name);
    AlreadyProcessedTokens.insert(IDInfo);
      
    // Iterate over the macro's tokens and stringify them.
    for (auto It = Info.MI->tokens_begin(), 
         E = Info.MI->tokens_end(); It != E; ++It)
    {
      Token T = *It;
        
      // If this token is not an identifier, we only need to print it.
      if (T.isNot(tok::identifier))
      {
        macro_tokens.push_back(PP.getSpelling(T));
        continue;
      }
        
      const auto *II = T.getIdentifierInfo();
      assert(II && "This token is an identifier but has no IdentifierInfo!");
          
      // If token is a macro that should be expanded inside the current macro.
      if (getMacroInfoForLocation(PP, SM, II, T.getLocation()))
      {
        tokenizeMacro(macro_tokens, T.getLocation(), PP, Info.Args, AlreadyProcessedTokens);
          
        // If this is a function-like macro, skip its arguments, as
        // getExpandedMacro() already tokenized them. If this is the case, 
        // let's first jump to the '(' token.
        auto N = std::next(It);
        if (N != E && N->is(tok::l_paren))
          It = getMatchingRParen(++It, E);
        continue;
      }
          
      // If this token is the current macro's argument, we should expand it.
      auto ArgMapIt = Info.Args.find(II);
      if (ArgMapIt != Info.Args.end()) 
      {
        for(MacroInfo::tokens_iterator ArgIt = ArgMapIt->second.begin(), 
            ArgEnd = ArgMapIt->second.end(); ArgIt != ArgEnd; ++ArgIt)
        {
          // These tokens may still be macros, if that is the case, 
          // handle it the same way we did above.
          const auto *ArgII = ArgIt->getIdentifierInfo();
          if (!ArgII) 
          {
            macro_tokens.push_back(PP.getSpelling(*ArgIt));
            continue;
          }

          const auto *MI = PP.getMacroInfo(ArgII);
          if (!MI) 
          {
            macro_tokens.push_back(PP.getSpelling(*ArgIt));
            continue;
          }
                  
          tokenizeMacro(macro_tokens, ArgIt->getLocation(),
                        PP, Info.Args, AlreadyProcessedTokens);
          // Peek the next token if it is a tok::l_paren. This way we can 
          // decide if this is the application or just a reference to a 
          // function macro symbol:
          //
          // #define apply(f) ...
          // #define func(x) ...
          // apply(func)
          // apply(func(42))
          auto N = std::next(ArgIt);
          if (N != ArgEnd && N->is(tok::l_paren))
            ArgIt = getMatchingRParen(++ArgIt, ArgEnd);
        }
        continue;
      }
          
      // If control reached here, then this token isn't a macro identifier, 
      // nor an unexpanded macro argument that we need to handle, token it.
      macro_tokens.push_back(PP.getSpelling(T));
    }
    AlreadyProcessedTokens.erase(IDInfo);
  }
    
    
  static const MacroInfo *getMacroInfoForLocation(const Preprocessor &PP, const SourceManager &SM, const IdentifierInfo *II, SourceLocation Loc)
  {
    const MacroDirective *MD = PP.getLocalMacroDirectiveHistory(II);
    if (!MD)
      return nullptr;
        
    return MD->findDirectiveAtLoc(Loc, SM).getMacroInfo();
  }
    
  static MacroNameAndArgs getMacroNameAndArgs(SourceLocation ExpanLoc, 
                                              const Preprocessor &PP) 
  {
        
    const SourceManager &SM = PP.getSourceManager();
    const LangOptions &LangOpts = PP.getLangOpts();
        
    // First, we create a Lexer to lex *at the expansion location* the tokens
    // referring to the macro's name and its arguments.
    std::pair<FileID, unsigned> LocInfo = SM.getDecomposedLoc(ExpanLoc);
    const llvm::MemoryBuffer *MB = SM.getBuffer(LocInfo.first);
    const char *MacroNameTokenPos = MB->getBufferStart() + LocInfo.second;
        
    Lexer RawLexer(SM.getLocForStartOfFile(LocInfo.first), LangOpts, 
                  MB->getBufferStart(), MacroNameTokenPos, MB->getBufferEnd());
        
    // Acquire the macro's name.
    Token TheTok;
    RawLexer.LexFromRawLexer(TheTok);
        
    std::string MacroName = PP.getSpelling(TheTok);
        
    const auto *II = PP.getIdentifierInfo(MacroName);
    assert(II && "Failed to acquire the IndetifierInfo for the macro!");
    
    const MacroInfo *MI = getMacroInfoForLocation(PP, SM, II, ExpanLoc);
    assert(MI && "The macro must've been defined at it's expansion location!");

    // We should always be able to obtain the MacroInfo in a given TU, but if
    // we're running the analyzer with CTU, the Preprocessor won't contain the
    // directive history (or anything for that matter) from another TU.
    // TODO: assert when we're not running with CTU.
    if (!MI)
      return { MacroName, MI, {} };
        
    // Acquire the macro's arguments.
    //
    // The rough idea here is to lex from the first left parentheses to the 
    // last right parentheses, and map the macro's unexpanded arguments to 
    // what they will be expanded to. An expanded macro argument may contain 
    // several tokens (like '3 + 4'), so we'll lex until we find a tok::comma 
    // or tok::r_paren, at which point we start lexing next argument or finish.
    ArrayRef<const IdentifierInfo *> MacroArgs = MI->params();
    if (MacroArgs.empty())
      return { MacroName, MI, {} };
        
    RawLexer.LexFromRawLexer(TheTok);
    // When this is a token which expands to another macro function then its
    // parentheses are not at its expansion locaiton. For example:
    //
    // #define foo(x) int bar() { return x; }
    // #define apply_zero(f) f(0)
    // apply_zero(foo)
    //               ^
    //               This is not a tok::l_paren, but foo is a function.
    if (TheTok.isNot(tok::l_paren))
      return { MacroName, MI, {} };
        
    MacroArgMap Args;
    
    // When the macro's argument is a function call, like
    //   CALL_FN(someFunctionName(param1, param2))
    // we will find tok::l_paren, tok::r_paren, and tok::comma that not divide
    // actual macro arguments, or do not represent the macro argument's closing
    // parentheses, so we'll count how many parentheses aren't closed yet.
    // If ParanthesesDepth
    //   * = 0, then there are no more arguments to lex.
    //   * = 1, then if we find a tok::comma, we can start lexing the next arg.
    //   * > 1, then tok::comma is a part of the current arg.
    int ParenthesesDepth = 1;
    
    // If we encounter __VA_ARGS__, we will lex until the closing tok::r_paren,
    // even if we lex a tok::comma and ParanthesesDepth == 1.
    const IdentifierInfo *__VA_ARGS__II = PP.getIdentifierInfo("__VA_ARGS__");
        
    for (const IdentifierInfo *UnexpArgII : MacroArgs) 
    {
      MacroArgMap::mapped_type ExpandedArgTokens;
      
      // One could simply not supply a single argument to __VA_ARGS__ -- this
      // results in a preprocessor warning, but is not an error:
      //   #define VARIADIC(ptr, ...) \
      //     someVariadicTemplateFunction(__VA_ARGS__)
      //
      //   int *ptr;
      //   VARIADIC(ptr); // Note that there are no commas, this isn't just an
      //                  // empty parameter -- no parameters for '...'.
      // In any other case, ParenthesesDepth mustn't be 0 here.
      if (ParenthesesDepth != 0) 
      {
          
        // Lex the first token of the next macro parameter.
        RawLexer.LexFromRawLexer(TheTok);
          
        while (!(ParenthesesDepth == 1 &&
              (UnexpArgII == __VA_ARGS__II ? false : TheTok.is(tok::comma)))) 
        {
          assert(TheTok.isNot(tok::eof) &&
                 "EOF encountered while looking for expanded macro args!");
              
              
              
          if (TheTok.is(tok::l_paren))
            ++ParenthesesDepth;
        
          if (TheTok.is(tok::r_paren))
            --ParenthesesDepth;
        
          if (ParenthesesDepth == 0)
            break;
        
         if (TheTok.is(tok::raw_identifier))
            PP.LookUpIdentifierInfo(TheTok);
              
          ExpandedArgTokens.push_back(TheTok);
          RawLexer.LexFromRawLexer(TheTok);
        }
      } 
      else 
      {
        assert(UnexpArgII == __VA_ARGS__II);
      }
            
      Args.emplace(UnexpArgII, std::move(ExpandedArgTokens));
    }
        
    assert(TheTok.is(tok::r_paren) && "Expanded macro argument acquisition failed! After the end of the loop this token should be ')'!");
        
    return { MacroName, MI, Args };
  }
    
  static MacroInfo::tokens_iterator getMatchingRParen(
                                              MacroInfo::tokens_iterator It,
                                              MacroInfo::tokens_iterator End) 
  {
    assert(It->is(tok::l_paren) && "This token should be '('!");
      
    // Skip until we find the closing ')'.
    int ParenthesesDepth = 1;
    while (ParenthesesDepth != 0) 
    {
      ++It;
          
      assert(It->isNot(tok::eof) && "Encountered EOF while attempting to skip macro arguments!");
      assert(It != End && "End of the macro definition reached before finding ')'!");
          
      if (It->is(tok::l_paren))
        ++ParenthesesDepth;
          
      if (It->is(tok::r_paren))
        --ParenthesesDepth;
    }
    return It;
  }

  vector<vector<string>> tokenizeStmts(ASTContext *Context, CompilerInstance *CI, nodeToExtract *lNTE) 
  {
    SourceLocation current, end, next, beginning;
    vector<vector<string>> stmtsAsTokens;
    vector<string> stmtAsTokens;
    SourceManager &SM = Context->getSourceManager();
    const LangOptions &LO = Context->getLangOpts();
    Token tok;
    
    stmtAsTokens.clear();
    current = lNTE->getSpanBeginRHS();
    end = lNTE->getSpanEndRHS();
    NodeLocation lLocation;

    bool okay = false;
    if (current == end)
      okay = true;
       
    while (current < end || current == end)  
    {
      while(isWhitespace(*SM.getCharacterData(current))) 
      {
        current = current.getLocWithOffset(1);
      }
      beginning = Lexer::GetBeginningOfToken(current, SM, LO);


      // MACRO tokenizer
      if(current.isMacroID())
      {
        lNTE->setExtraInfo("Macro RHS;");
        std::string rhs_string = lNTE->getSrcValue();
        if (rhs_string.size() == 1)
        {
          stmtAsTokens.push_back(rhs_string);
          break;
        }
          
        Preprocessor &PP = CI->getPreprocessor();
          
        llvm::SmallString<200> ExpansionBuf;
        llvm::raw_svector_ostream OS(ExpansionBuf);
        llvm::SmallPtrSet<IdentifierInfo*, 8> AlreadyProcessedTokens;
        
        std::vector<std::string> macro_tokens;
        std::map<unsigned int, unsigned int> index_of_token;
        tokenizeMacro(macro_tokens, SM.getExpansionLoc(current), PP, MacroArgMap{}, AlreadyProcessedTokens);
        std::string full_token_string;
        for (unsigned int i = 0; i < macro_tokens.size(); i++) 
        {
          // cout << "tst " << macro_tokens.at(i) << " "  << full_token_string.size() << " " << i << '\n'; //
          index_of_token.insert(
           std::pair<unsigned int, unsigned int>(full_token_string.size(), i));
          
          full_token_string += macro_tokens.at(i);
        }

        rhs_string.erase(remove(rhs_string.begin(), rhs_string.end(), ' '), rhs_string.end());
          
        std::size_t token_start_pos = full_token_string.find(rhs_string);
          
        bool found = false;
        while(!found && token_start_pos != std::string::npos)
        {
          for (auto& x: index_of_token) 
          {
            if(x.first == token_start_pos)
            {
              found = true;
              break;
            }
          } 
          if(!found)
            token_start_pos = full_token_string.find(
                                              rhs_string, token_start_pos + 1);
        }

        if(token_start_pos!=std::string::npos)
        {
          for (unsigned int i = index_of_token.at(token_start_pos); i < index_of_token.at(token_start_pos + rhs_string.size()); i++)
          {
              // cout << macro_tokens.at(i) << " ";
              stmtAsTokens.push_back(macro_tokens.at(i));
          }
        }
        else
        {
          lNTE->setExtraInfo("Macro Tokeniser failed;");
        }
        break;
      }

      // tokenizer for everything else than MACRO
      if(beginning == current) 
      {
        if(!Lexer::getRawToken(beginning, tok, SM, LO)) 
        {
          next = Lexer::getLocForEndOfToken(beginning, 0, SM, LO);
          CharSourceRange csr = CharSourceRange::getCharRange(beginning,next.getLocWithOffset(0));
          llvm::StringRef ref = Lexer::getSourceText(csr, SM, LO);
          stmtAsTokens.push_back(ref.str());
          current = next;
          continue;
        }
      }
      current = current.getLocWithOffset(1);
    }
    stmtsAsTokens.push_back(stmtAsTokens);
  return stmtsAsTokens;
  }

};

#endif /* tokenizer_hpp */

