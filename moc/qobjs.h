#ifndef MOC_QOBJS
#define MOC_QOBJS

#include <map>
#include <string>
#include <vector>

namespace llvm {
namespace yaml {

class Node;
}

class raw_ostream;
} // namespace llvm

namespace QBJS {
enum Type {
  Null = 0x0,
  Bool = 0x1,
  Double = 0x2,
  String = 0x3,
  Array = 0x4,
  Object = 0x5,
  Undefined = 0x80
};

struct Value {
  Value() = default;
  Value(std::string S) : T(String), Str(std::move(S)) {}
  Value(double D) : T(Double), D(D) {}
  Value(bool B) : T(Bool), D(B ? 1. : -1.) {}
  Type T = Undefined;
  std::map<std::string, Value> Props; // for Object
  std::vector<Value> Elems;           // For Array
  std::string Str;
  double D = 0.;
  mutable int S = -1;

  int Size() const {
    if (S < 0)
      S = ComputeSize();
    return S;
  }
  int ComputeSize() const;
};

struct Stream {
  Stream(llvm::raw_ostream &OS) : OS(OS) {}
  Stream &operator<<(const Value &);

private:
  int Col = 0;
  llvm::raw_ostream &OS;
  Stream &operator<<(const std::string &Str);
  Stream &operator<<(uint32_t);
  Stream &operator<<(uint16_t);
  Stream &operator<<(unsigned char);
};

bool Parse(llvm::yaml::Node *Node, Value &Root);
} // namespace QBJS

#endif /* MOC_QOBJS */
