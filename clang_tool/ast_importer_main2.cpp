#include "clang/AST/ASTImporter.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace tooling;
using namespace ast_matchers;

template <typename Node, typename Matcher>
Node *getFirstDecl(Matcher M, const std::unique_ptr<ASTUnit> &Unit) {
  auto MB = M.bind("bindStr");
  auto MatchRes = match(MB, Unit->getASTContext());
  Node *Result =
      const_cast<Node *>(MatchRes[0].template getNodeAs<Node>("bindStr"));
  return Result;
};

int main() {
  std::unique_ptr<ASTUnit> ToUnit = buildASTFromCode(
      R"(
      // primary template
      template <typename T>
      struct X {};
      // explicit specialization
      template<>
      struct X<int> { int i; };
      )",
      "to.cc");
  ToUnit->enableSourceFileDiagnostics();
  std::unique_ptr<ASTUnit> FromUnit = buildASTFromCode(
      R"(
      // primary template
      template <typename T>
      struct X {};
      // explicit specialization
      template<>
      struct X<int> { int i2; };
      // field mismatch:  ^^
      )",
      "from.cc");
  FromUnit->enableSourceFileDiagnostics();
  auto Matcher = classTemplateSpecializationDecl(hasName("X"));
  auto *From = getFirstDecl<ClassTemplateSpecializationDecl>(Matcher, FromUnit);
  auto *To = getFirstDecl<ClassTemplateSpecializationDecl>(Matcher, ToUnit);

  ASTImporter Importer(ToUnit->getASTContext(), ToUnit->getFileManager(),
                       FromUnit->getASTContext(), FromUnit->getFileManager(),
                       /*MinimalImport=*/false);
  llvm::Expected<Decl *> ImportedOrErr = Importer.Import(From);
  if (!ImportedOrErr) {
    llvm::Error Err = ImportedOrErr.takeError();
    llvm::errs() << "ERROR: " << Err << "\n";
    consumeError(std::move(Err));
    To->getTranslationUnitDecl()->dump();
    return 1;
  }
  return 0;
};