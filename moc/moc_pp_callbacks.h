#ifndef MOC_MOC_PP_CALLBACKS
#define MOC_MOC_PP_CALLBACKS

#include "clang/Lex/PPCallbacks.h"
#include <set>

class MocPPCallbacks : public clang::PPCallbacks {
public:
  MocPPCallbacks(clang::Preprocessor &PP,
                 std::map<clang::SourceLocation, std::string> &Tags)
      : PP(PP), Tags(Tags) {}
  void InjectQObjectDefs(clang::SourceLocation Loc);
  void EnterMainFile(clang::StringRef Name);

  bool IsInMainFile = false;

protected:
  void MacroUndefined(const clang::Token &MacroNameTok,
                      const clang::MacroDefinition &,
                      const clang::MacroDirective *) override;
  void FileChanged(clang::SourceLocation Loc, FileChangeReason Reason,
                   clang::SrcMgr::CharacteristicKind FileType,
                   clang::FileID PrevFID) override;
  bool FileNotFound(llvm::StringRef FileName) override;
  void InclusionDirective(clang::SourceLocation HashLoc,
                          const clang::Token &IncludeTok,
                          clang::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange,
                          clang::OptionalFileEntryRef File,
                          clang::StringRef SearchPath,
                          clang::StringRef RelativePath,
                          const clang::Module *Imported,
                          clang::SrcMgr::CharacteristicKind FileType) override;
  void Defined(const clang::Token &MacroNameTok,
               const clang::MacroDefinition &MD,
               clang::SourceRange Range) override;
  void Ifdef(clang::SourceLocation Loc, const clang::Token &MacroNameTok,
             const clang::MacroDefinition &MD) override;
  void Ifndef(clang::SourceLocation Loc, const clang::Token &MacroNameTok,
              const clang::MacroDefinition &MD) override;
  void Endif(clang::SourceLocation Loc, clang::SourceLocation IfLoc) override;
  void MacroDefined(const clang::Token &MacroNameTok,
                    const clang::MacroDirective *) override;
  void MacroExpands(const clang::Token &MacroNameTok,
                    const clang::MacroDefinition &, clang::SourceRange Range,
                    const clang::MacroArgs *) override;

private:
  clang::Preprocessor &PP;

  bool IncludeNotFoundSupressed = false;
  bool ShouldWarnHeaderNotFound = false;
  bool InQMOCRUN = false;
  std::set<std::string> PossibleTags;
  std::map<clang::SourceLocation, std::string> &Tags;
};

#endif /* MOC_MOC_PP_CALLBACKS */
