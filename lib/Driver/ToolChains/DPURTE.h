//===--- DPURTE.h - DPU RTE ToolChain Implementations -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DPURTE_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DPURTE_H

#include "Gnu.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"
#include "llvm/Option/ArgList.h"
#include <stdlib.h>

namespace clang {
namespace driver {
namespace toolchains {
class LLVM_LIBRARY_VISIBILITY DPURTE : public Generic_ELF {
public:
  DPURTE(const Driver &D, const llvm::Triple &Triple,
         const llvm::opt::ArgList &Args)
      : Generic_ELF(D, Triple, Args) {
    PathToStdlibIncludes = GetUpmemSdkPath("/usr/share/upmem/include/stdlib");
    PathToSyslibIncludes = GetUpmemSdkPath("/usr/share/upmem/include/syslib");
    PathToLinkScript = GetUpmemSdkPath("/usr/share/upmem/include/link/dpu.lds");
    PathToRtLibDirectory =
        Args.hasArg(options::OPT_pg)
            ? GetUpmemSdkPath("/usr/share/upmem/include/built-in-profiling")
            : GetUpmemSdkPath("/usr/share/upmem/include/built-in");
    PathToRtLibBc =
        Args.hasArg(options::OPT_pg)
            ? GetUpmemSdkPath(
                  "/usr/share/upmem/include/built-in-profiling/rtlib.bc")
            : GetUpmemSdkPath("/usr/share/upmem/include/built-in/rtlib.bc");
    PathToStartFile = GetUpmemSdkPath(
        "/usr/share/upmem/include/built-in/dpu_sys_mram_loader.o");
  }

  ~DPURTE() override {
    free(PathToStdlibIncludes);
    free(PathToSyslibIncludes);
    free(PathToLinkScript);
    free(PathToRtLibDirectory);
    free(PathToRtLibBc);
    free(PathToStartFile);
  }

  SanitizerMask getSupportedSanitizers() const override {
    SanitizerMask Res = ToolChain::getSupportedSanitizers();
    // Safe stack not supported yet Res |= SanitizerKind::SafeStack;
    return Res;
  }

  bool IsIntegratedAssemblerDefault() const override { return true; }

  bool HasNativeLLVMSupport() const override { return true; }

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;

  void
  addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                        llvm::opt::ArgStringList &CC1Args,
                        Action::OffloadKind DeviceOffloadKind) const override;

protected:
  Tool *buildLinker() const override;

private:
  char *GetUpmemSdkPath(const char *Path);

  char *PathToSDK = NULL;
  char *PathToSyslibIncludes;
  char *PathToStdlibIncludes;
  char *PathToLinkScript;
  char *PathToRtLibDirectory;
  char *PathToRtLibBc;
  char *PathToStartFile;
};
} // end namespace toolchains
namespace tools {
namespace dpu {
class LLVM_LIBRARY_VISIBILITY Linker : public GnuTool {
public:
  Linker(const ToolChain &TC, const char *Script, const char *RtLibDir,
         const char *PathToRtLibBc, const char *PathToStartFile)
      : GnuTool("dpu::Linker", "ld.lld", TC) {
    LinkScript = Script;
    RtLibraryPath = RtLibDir;
    RtBcLibrary = PathToRtLibBc;
    StartFile = PathToStartFile;
  }

  bool isLinkJob() const override { return true; }

  bool hasIntegratedCPP() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;

private:
  const char *LinkScript;
  const char *RtLibraryPath;
  const char *RtBcLibrary;
  const char *StartFile;
};
} // end namespace dpu
} // end namespace tools
} // end namespace driver
} // end namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DPURTE_H
