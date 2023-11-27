#include "moc/common.h"

clang::FileID CreateFileIDForMemBuffer(clang::Preprocessor &PP,
                                       std::unique_ptr<llvm::MemoryBuffer> Buf,
                                       clang::SourceLocation Loc) {
  return PP.getSourceManager().createFileID(std::move(Buf),
                                            clang::SrcMgr::C_User, 0, 0, Loc);
}