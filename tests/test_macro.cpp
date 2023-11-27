#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/MacroInfo.h"
#include <iostream>

int main() {
  clang::IdentifierTable identifierTable;
  clang::IdentifierInfo &macroInfo = identifierTable.get("comment");

  if (macroInfo.getBuiltinID() != 0) {
    std::cout << "Macro is a built-in macro." << std::endl;
  } else {
    std::cout << "Macro is not a built-in macro." << std::endl;
  }

  return 0;
}