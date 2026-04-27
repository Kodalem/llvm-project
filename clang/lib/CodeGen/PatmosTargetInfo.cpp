//===--- PatmosTargetInfo.cpp - Patmos target specific CodeGenInfo -------===//
//
// This file defines Patmos-specific ABI/TargetCodeGenInfo glue for clang
//
//===----------------------------------------------------------------------===//

#include "TargetInfo.h"
#include "ABIInfoImpl.h"
#include "CodeGenModule.h"
#include "CodeGenTypes.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Attr.h"
#include "clang/CodeGen/CGFunctionInfo.h"

using namespace clang;
using namespace CodeGen;

namespace {
class PatmosABIInfo : public DefaultABIInfo {
public:
  PatmosABIInfo(CodeGen::CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}
};

class PatmosTargetCodeGenInfo : public TargetCodeGenInfo {
public:
  PatmosTargetCodeGenInfo(CodeGenTypes &CGT)
    : TargetCodeGenInfo(std::make_unique<PatmosABIInfo>(CGT)) {}

  void setTargetAttributes(const Decl *D, llvm::GlobalValue *GV,
                           CodeGen::CodeGenModule &CGM) const override {
    const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(D);
    if (!FD || !GV)
      return;
    llvm::Function *Fn = dyn_cast<llvm::Function>(GV);
    if (!Fn)
      return;

    auto &Opts = CGM.getCodeGenOpts();
    bool is_cet_fun = Opts.PatmosEnableCet &&
        std::any_of(Opts.PatmosCetFuncs.begin(), Opts.PatmosCetFuncs.end(),
            [&](const std::string &fn_name){ return fn_name == FD->getNameAsString();});

    if (FD->hasAttr<SinglePathAttr>() || is_cet_fun) {
      Fn->addFnAttr("sp-root");
      Fn->addFnAttr(llvm::Attribute::NoInline);
    }
  }
};
} // end anonymous namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createPatmosTargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<PatmosTargetCodeGenInfo>(CGM.getTypes());
}

