#include "moc/moc_pp_callbacks.h"
#include "moc/common.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Preprocessor.h"

static const char Injected[] = R"-(
#if defined(Q_MOC_OUTPUT_REVISION) || defined(Q_MOC_RUN)

#ifdef QT_ANNOTATE_CLASS
#undef QT_ANNOTATE_CLASS
#endif
#ifdef QT_ANNOTATE_CLASS2
#undef QT_ANNOTATE_CLASS2
#endif

#ifdef Q_COMPILER_VARIADIC_MACROS
#define QT_ANNOTATE_CLASS(type, ...) \
    __extension__ _Static_assert(sizeof (#__VA_ARGS__), #type);
#else
#define QT_ANNOTATE_CLASS(type, anotation) \
    __extension__ _Static_assert(sizeof (#anotation), #type);
#endif
#define QT_ANNOTATE_CLASS2(type, a1, a2) \
    __extension__ _Static_assert(sizeof (#a1, #a2), #type);



#ifndef QT_NO_META_MACROS
# if defined(QT_NO_KEYWORDS)
#  define QT_NO_EMIT
# else
#   ifndef QT_NO_SIGNALS_SLOTS_KEYWORDS
#     undef  slots
#     define slots Q_SLOTS
#     undef  signals
#     define signals Q_SIGNALS
#   endif
# endif

# undef  Q_SLOTS
# define Q_SLOTS Q_SLOT
# undef  Q_SIGNALS
# define Q_SIGNALS public Q_SIGNAL
# undef  Q_PRIVATE_SLOT
# define Q_PRIVATE_SLOT(d, signature) QT_ANNOTATE_CLASS2(qt_private_slot, d, signature)


#undef Q_CLASSINFO
#undef Q_PLUGIN_METADATA
#undef Q_INTERFACES
#undef Q_PROPERTY
#undef Q_PRIVATE_PROPERTY
#undef Q_REVISION
#undef Q_ENUMS
#undef Q_FLAGS
#undef Q_SCRIPTABLE
#undef Q_INVOKABLE
#undef Q_SIGNAL
#undef Q_SLOT
#undef Q_ENUM
#undef Q_FLAG

#define Q_CLASSINFO(name, value)  __extension__ _Static_assert(sizeof (name, value), "qt_classinfo");
#define Q_PLUGIN_METADATA(x) QT_ANNOTATE_CLASS(qt_plugin_metadata, x)
#define Q_INTERFACES(x) QT_ANNOTATE_CLASS(qt_interfaces, x)
#ifdef Q_COMPILER_VARIADIC_MACROS
#define Q_PROPERTY(...) QT_ANNOTATE_CLASS(qt_property, __VA_ARGS__)
#else
#define Q_PROPERTY(text) QT_ANNOTATE_CLASS(qt_property, text)
#endif
#define Q_PRIVATE_PROPERTY(d, text)  QT_ANNOTATE_CLASS2(qt_private_property, d, text)

#define Q_REVISION(v) __attribute__((annotate("qt_revision:" QT_STRINGIFY2(v))))
#define Q_ENUMS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_FLAGS(x) QT_ANNOTATE_CLASS(qt_flags, x)
#define Q_ENUM_IMPL(ENUM) \
    friend Q_DECL_CONSTEXPR const QMetaObject *qt_getEnumMetaObject(ENUM) Q_DECL_NOEXCEPT { return &staticMetaObject; } \
    friend Q_DECL_CONSTEXPR const char *qt_getEnumName(ENUM) Q_DECL_NOEXCEPT { return #ENUM; }
#define Q_ENUM(x) Q_ENUMS(x) Q_ENUM_IMPL(x)
#define Q_FLAG(x) Q_FLAGS(x) Q_ENUM_IMPL(x)
#define Q_SCRIPTABLE  __attribute__((annotate("qt_scriptable")))
#define Q_INVOKABLE  __attribute__((annotate("qt_invokable")))
#define Q_SIGNAL __attribute__((annotate("qt_signal")))
#define Q_SLOT __attribute__((annotate("qt_slot")))
#endif // QT_NO_META_MACROS


#undef QT_TR_FUNCTIONS
#ifndef QT_NO_TRANSLATION
#define QT_TR_FUNCTIONS \
    static inline QString tr(const char *s, const char *c = Q_NULLPTR, int n = -1) \
    { return staticMetaObject.tr(s, c, n); } \
    QT_DEPRECATED static inline QString trUtf8(const char *s, const char *c = Q_NULLPTR, int n = -1) \
    { return staticMetaObject.tr(s, c, n); } \
    QT_ANNOTATE_CLASS(qt_qobject, "")
#else
#define QT_TR_FUNCTIONS \
    QT_ANNOTATE_CLASS(qt_qobject, "")
#endif

#undef Q_GADGET
#define Q_GADGET \
public: \
    static const QMetaObject staticMetaObject; \
    void qt_check_for_QGADGET_macro(); \
    typedef void QtGadgetHelper; \
private: \
    Q_DECL_HIDDEN_STATIC_METACALL static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **); \
    QT_ANNOTATE_CLASS(qt_qgadget, "")

//for qnamespace.h because Q_MOC_RUN is defined
#if defined(Q_MOC_RUN)
#undef Q_OBJECT
#define Q_OBJECT QT_ANNOTATE_CLASS(qt_qobject, "")
#undef Q_ENUM_IMPL
#define Q_ENUM_IMPL(ENUM)
#endif

#undef Q_OBJECT_FAKE
#define Q_OBJECT_FAKE Q_OBJECT QT_ANNOTATE_CLASS(qt_fake, "")

#undef QT_MOC_COMPAT
#define QT_MOC_COMPAT  __attribute__((annotate("qt_moc_compat")))

//for qnamespace.h again
#ifndef Q_DECLARE_FLAGS
#define Q_DECLARE_FLAGS(Flags, Enum) typedef QFlags<Enum> Flags;
#endif

#endif
)-";

void MocPPCallbacks::InjectQObjectDefs(clang::SourceLocation Loc) {
  auto Buf =
      llvm::MemoryBuffer::getMemBuffer(Injected, "qobjectdefs-injected.moc");
  Loc = PP.getSourceManager().getFileLoc(Loc);
  PP.EnterSourceFile(CreateFileIDForMemBuffer(PP, std::move(Buf), Loc), nullptr,
                     Loc);
}

void MocPPCallbacks::EnterMainFile(clang::StringRef Name) {
  if (Name.endswith("global/qnamespace.h")) {
    // qnamsepace.h is a bit special because it contains all the Qt namespace
    // enums but all the Q_ENUMS are within a Q_MOC_RUN scope, which also do all
    // sort of things.

    clang::MacroInfo *MI = PP.AllocateMacroInfo({});
    MI->setIsBuiltinMacro();
    PP.appendDefMacroDirective(PP.getIdentifierInfo("Q_MOC_RUN"), MI);
    InjectQObjectDefs({});
  }
}

void MocPPCallbacks::MacroUndefined(const clang::Token &MacroNameTok,
                                    const clang::MacroDefinition &,
                                    const clang::MacroDirective *) {
  if (MacroNameTok.getIdentifierInfo()->getName() == "QT_NO_KEYWORDS") {
    InjectQObjectDefs(MacroNameTok.getLocation());
  }
}

void MocPPCallbacks::FileChanged(clang::SourceLocation Loc,
                                 FileChangeReason Reason,
                                 clang::SrcMgr::CharacteristicKind FileType,
                                 clang::FileID PrevFID) {
  clang::SourceManager &SM = PP.getSourceManager();
  IsInMainFile = (SM.getFileID(SM.getFileLoc(Loc)) == SM.getMainFileID());

  if (IsInMainFile && Reason == EnterFile) {
    EnterMainFile(SM.getFilename(Loc));
  }

  if (Reason != ExitFile)
    return;
  auto F = PP.getSourceManager().getFileEntryForID(PrevFID);
  if (!F)
    return;

  llvm::StringRef name = F->getName();
  if (name.endswith("qobjectdefs.h")) {
    InjectQObjectDefs(Loc);
  }
}

bool MocPPCallbacks::FileNotFound(llvm::StringRef FileName) {
  if (FileName.endswith(".moc") || FileName.endswith("_moc.cpp") ||
      FileName.startswith("moc_")) {
    if (!PP.GetSuppressIncludeNotFoundError()) {
      PP.SetSuppressIncludeNotFoundError(true);
      IncludeNotFoundSupressed = true;
    }
  } else {
    if (IncludeNotFoundSupressed) {
      PP.SetSuppressIncludeNotFoundError(false);
      IncludeNotFoundSupressed = false;
    } else {
      ShouldWarnHeaderNotFound = true;
    }
  }
  return false;
}

void MocPPCallbacks::InclusionDirective(
    clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
    clang::StringRef FileName, bool IsAngled,
    clang::CharSourceRange FilenameRange, clang::OptionalFileEntryRef File,
    clang::StringRef SearchPath, clang::StringRef RelativePath,
    const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType) {
  if (!File && ShouldWarnHeaderNotFound) {
    /* This happens when we are not running as a plugin
     * We want to transform the "include not found" error in a warning.
     */
    PP.getDiagnostics().Report(
        FilenameRange.getBegin(),
        PP.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                            "'%0' file not found"))
        << FileName << FilenameRange;
  }
  ShouldWarnHeaderNotFound = false;
}

void MocPPCallbacks::Defined(const clang::Token &MacroNameTok,
                             const clang::MacroDefinition &MD,
                             clang::SourceRange Range) {
  if (MacroNameTok.getIdentifierInfo()->getName() != "Q_MOC_RUN")
    return;
  auto F = PP.getSourceManager().getFileEntryForID(
      PP.getSourceManager().getFileID(MacroNameTok.getLocation()));
  if (!F)
    return;
  llvm::StringRef name = F->getName();
  if (name.endswith("qobjectdefs.h") || name.endswith("qglobal.h"))
    return;
  InQMOCRUN = true;
}

void MocPPCallbacks::Ifdef(clang::SourceLocation Loc,
                           const clang::Token &MacroNameTok,
                           const clang::MacroDefinition &MD) {
  Defined(MacroNameTok, {}, {});
}

void MocPPCallbacks::Ifndef(clang::SourceLocation Loc,
                            const clang::Token &MacroNameTok,
                            const clang::MacroDefinition &MD) {
  Defined(MacroNameTok, {}, {});
}

void MocPPCallbacks::Endif(clang::SourceLocation Loc,
                           clang::SourceLocation IfLoc) {
  InQMOCRUN = false;
}

void MocPPCallbacks::MacroDefined(const clang::Token &MacroNameTok,
                                  const clang::MacroDirective *) {
  if (!InQMOCRUN)
    return;
  PossibleTags.insert(MacroNameTok.getIdentifierInfo()->getName().str());
}

void MocPPCallbacks::MacroExpands(const clang::Token &MacroNameTok,
                                  const clang::MacroDefinition &,
                                  clang::SourceRange Range,
                                  const clang::MacroArgs *) {
  if (InQMOCRUN)
    return;
  if (PossibleTags.count(MacroNameTok.getIdentifierInfo()->getName().str())) {
    clang::SourceLocation FileLoc =
        PP.getSourceManager().getFileLoc(MacroNameTok.getLocation());
    Tags.insert({FileLoc, MacroNameTok.getIdentifierInfo()->getName().str()});
  }
}
