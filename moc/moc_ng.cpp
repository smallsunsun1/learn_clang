#include "moc/moc_ng.h"
#include "moc/common.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Version.h"
#include "clang/Lex/LiteralSupport.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

static clang::SourceLocation GetFromLiteral(clang::Token Tok,
                                            clang::StringLiteral *Lit,
                                            clang::Preprocessor &PP) {
  return Lit->getLocationOfByte(
      PP.getSourceManager().getFileOffset(Tok.getLocation()),
      PP.getSourceManager(), PP.getLangOpts(), PP.getTargetInfo());
}

static void parseInterfaces(ClassDef &Def, clang::Expr *Content,
                            clang::Sema &Sema) {
  clang::Preprocessor &PP = Sema.getPreprocessor();
  clang::StringLiteral *Val = llvm::dyn_cast<clang::StringLiteral>(Content);
  if (!Val) {
    PP.getDiagnostics().Report(
        Content->getExprLoc(),
        PP.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error,
                                            "Invalid Q_INTERFACES annotation"));
    return;
  }
  std::unique_ptr<llvm::MemoryBuffer> Buf =
      llvm::MemoryBuffer::getMemBufferCopy(Val->getString(), "Q_INTERFACES");
  llvm::MemoryBufferRef buffer_ref(*Buf);
  clang::Lexer Lex(
      CreateFileIDForMemBuffer(PP, std::move(Buf), Content->getExprLoc()),
      buffer_ref, PP);

  clang::Token Tok;
  bool Append = false;
  bool Error = false;
  while (true) {
    Lex.LexFromRawLexer(Tok);
    if (Tok.is(clang::tok::eof)) {
      break;
    }
    if (Tok.is(clang::tok::raw_identifier)) {
      PP.LookUpIdentifierInfo(Tok);
    }
    if (Tok.is(clang::tok::identifier)) {
      // 直接字符串相加和往后面push元素
      if (Append) {
        Def.Interfaces.back() += PP.getSpelling(Tok);
      } else {
        Def.Interfaces.push_back(PP.getSpelling(Tok));
      }
      Append = false;
      continue;
    }
    if (Append) {
      Error = true;
      break;
    }
    // ::的情况
    if (Tok.is(clang::tok::coloncolon)) {
      Def.Interfaces.back() += PP.getSpelling(Tok);
      Append = true;
      continue;
    }
    if (!Tok.is(clang::tok::colon)) {
      Error = true;
      break;
    }
  }
  if (Error || Append || !Tok.is(clang::tok::eof)) {
    PP.getDiagnostics().Report(
        GetFromLiteral(Tok, Val, PP),
        PP.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error,
                                            "parse error in Q_INTERFACES"));
  }
}

static void parsePluginMetaData(ClassDef &Def, clang::Expr *Content,
                                clang::Sema &Sema) {
  clang::Preprocessor &PP = Sema.getPreprocessor();
  clang::StringLiteral *Val = llvm::dyn_cast<clang::StringLiteral>(Content);
  if (!Val) {
    PP.getDiagnostics().Report(Content->getExprLoc(),
                               PP.getDiagnostics().getCustomDiagID(
                                   clang::DiagnosticsEngine::Error,
                                   "Invalid Q_PLUGIN_METADATA annotation"));
    return;
  }
  std::unique_ptr<llvm::MemoryBuffer> Buf =
      llvm::MemoryBuffer::getMemBufferCopy(Val->getString(),
                                           "Q_PLUGIN_METADATA");
  llvm::MemoryBufferRef buffer_ref(*Buf);
  clang::Lexer Lex(
      CreateFileIDForMemBuffer(PP, std::move(Buf), Content->getExprLoc()),
      buffer_ref, PP.getSourceManager(), PP.getLangOpts());
  clang::Token Tok;
  Lex.LexFromRawLexer(Tok);
  // clang raw_identifier
  while (Tok.is(clang::tok::raw_identifier)) {
    clang::IdentifierInfo *II = PP.LookUpIdentifierInfo(Tok);
    if (II->getName() != "IID" && II->getName() != "FILE") {
      Lex.LexFromRawLexer(Tok);
      continue;
    }
    Lex.LexFromRawLexer(Tok);
    if (!Tok.is(clang::tok::string_literal)) {
      PP.getDiagnostics().Report(
          GetFromLiteral(Tok, Val, PP),
          PP.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error,
                                              "Expected string literal"));
      return;
    }
    llvm::SmallVector<clang::Token, 4> StrToks;
    do {
      StrToks.push_back(Tok);
      Lex.LexFromRawLexer(Tok);
    } while (Tok.is(clang::tok::string_literal));
    clang::StringLiteralParser Literal(StrToks, PP);
    if (Literal.hadError)
      return;
    if (II->getName() == "IID") {
      Def.Plugin.IID = Literal.GetString();
    } else {
      llvm::StringRef Filename = Literal.GetString();
      const clang::ConstSearchDirIterator *CurDir;
      const clang::OptionalFileEntryRef File = PP.LookupFile(
          Val->getSourceRange().getBegin(), Filename, false, nullptr, nullptr,
          nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }
  }
}

void BaseDef::addEnum(clang::EnumDecl *E, std::string Alias, bool IsFlag) {
  for (const auto &I : Enums) {
    if (std::get<1>(I) == Alias) {
      return;
    }
  }
  Enums.emplace_back(E, std::move(Alias), IsFlag);
}

void BaseDef::AddExtra(clang::CXXRecordDecl *E) {
  if (!E) {
    return;
  }
  if (std::find(Extra.begin(), Extra.end(), E) != Extra.end()) {
    return;
  }
  Extra.push_back(E);
}
