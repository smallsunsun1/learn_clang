#ifndef MOC_MOC_NG
#define MOC_MOC_NG

#include "moc/qobjs.h"
#include "clang/Basic/SourceLocation.h"
#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class MocPPCallbacks;
namespace clang {
class CXXMethodDecl;
class CXXRecordDecl;
class CXXConstructorDecl;
class NamespaceDecl;
class EnumDecl;
class Preprocessor;
class Sema;
class TypeDecl;
class Type;
class QualType;
} // namespace clang

// 对应Q_RPOPERTY NOTIFY
struct NotifyDef {
  std::string Str;
  clang::SourceLocation Loc;
  clang::CXXMethodDecl *MD = nullptr;
  int notifyID = -1;
};

struct PrivateSlotDef {
  std::string ReturnType;
  std::string Name;
  std::vector<std::string> Args;
  int NumDefault = 0;
  std::string InPrivateClass;
};

struct PropertyDef {
  std::string name;
  std::string type;
  std::string member;
  std::string read;
  std::string write;
  std::string reset;
  std::string designable = "true";
  std::string editable;
  std::string store = "true";
  NotifyDef notify;
  bool constant = false;
  bool final = false;
  bool isEnum = false;
  int revision = 0;
  bool PointerHack = false;
  bool PossiblyForwardDeclared = false;
};

struct PluginData {
  std::string IID;
  QBJS::Value MetaData;
};

// 基础信息的定义
struct BaseDef {
  void addEnum(clang::EnumDecl *E, std::string Alias, bool IsFlag);
  void AddExtra(clang::CXXRecordDecl *E);

  std::vector<std::tuple<clang::EnumDecl *, std::string, bool>> Enums;
  std::vector<clang::CXXRecordDecl *> Extra;
  std::vector<std::pair<std::string, std::string>> ClassInfo;
};

struct ClassDef : public BaseDef {
  clang::CXXRecordDecl *Record = nullptr;      // 类本身的定义
  std::vector<clang::CXXMethodDecl *> Signals; // 信号
  std::vector<clang::CXXMethodDecl *> Slots;   // Slot
  std::vector<PrivateSlotDef> PrivateSlots;
  std::vector<clang::CXXMethodDecl> Methods;
  std::vector<clang::CXXConstructorDecl> Constructors;
  std::vector<std::string> Interfaces;
  PluginData Plugin;
  std::vector<PropertyDef> Properties;
  bool HasQObject = false;
  bool HasQGadget = false;
  int NotifyCount = 0;
  int PrivateSlotCount = 0;
  int RevisionPropertyCount = 0;
};

struct NamespaceDef : BaseDef {
  clang::NamespaceDecl *Namespace = nullptr;
  bool hasQNamespace = false;
};

class MocNg {
public:
  using MetaTypeSet = std::set<const clang::Type *>;
  using InterfaceMap =
      std::unordered_map<std::string, const clang::CXXRecordDecl *>;

  ClassDef parseClass(clang::CXXRecordDecl *RD, clang::Sema &Sema);
  NamespaceDef parseNamespace(clang::NamespaceDecl *ND, clang::Sema &Sema);
  std::string GetTag(clang::SourceLocation DeclLoc,
                     const clang::SourceManager &SM);
  bool ShouldRegisterMetaType(clang::QualType T);

  MetaTypeSet registered_meta_type;
  InterfaceMap interfaces;
  bool HasPlugin = false;
  std::map<clang::SourceLocation, std::string> Tags;
};

#endif /* MOC_MOC_NG */
