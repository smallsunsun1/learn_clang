#ifndef MOC_COMMON
#define MOC_COMMON

#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/MemoryBuffer.h"
#include <memory>

clang::FileID CreateFileIDForMemBuffer(clang::Preprocessor &PP,
                                       std::unique_ptr<llvm::MemoryBuffer> Buf,
                                       clang::SourceLocation Loc);

#endif /* MOC_COMMON */
