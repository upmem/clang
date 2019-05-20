//===--- DPURTE.h - DPU RTE ToolChain Implementations -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "DPURTE.h"
#include "CommonArgs.h"
#include "InputInfo.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "llvm/Option/ArgList.h"

using namespace llvm::opt;

namespace clang {
namespace driver {
namespace toolchains {
void DPURTE::AddClangSystemIncludeArgs(
    const llvm::opt::ArgList &DriverArgs,
    llvm::opt::ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  CC1Args.push_back("-nostdsysteminc");
  addSystemInclude(DriverArgs, CC1Args, StringRef(PathToStdlibIncludes));
  addSystemInclude(DriverArgs, CC1Args, StringRef(PathToSyslibIncludes));
}

char *DPURTE::GetUpmemSdkPath(const char *Path) const {
  char *UpmemHome = getenv("UPMEM_HOME");
  if (UpmemHome == nullptr) {
    // Assume that the toolchain is installed at system root
    return strdup(Path);
  } else {
    char *result;
    asprintf(&result, "%s%s", UpmemHome, Path);
    return result;
  }
}

Tool *DPURTE::buildLinker() const {
  return new tools::dpu::Linker(*this, PathToLinkScript, PathToRtLibDirectory,
                                PathToRtLibBc);
}

void DPURTE::addClangTargetOptions(
    const llvm::opt::ArgList &DriverArgs, llvm::opt::ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadKind) const {
  Generic_ELF::addClangTargetOptions(DriverArgs, CC1Args, DeviceOffloadKind);
  Arg *A = DriverArgs.getLastArg(options::OPT_O_Group);
  if (!A || !A->getOption().matches(options::OPT_O0)) {
    // In -O0 we need to keep some unused section (from the linker point of
    // view) that will be used for debug purpose
    CC1Args.push_back("-ffunction-sections");
  }
  CC1Args.push_back("-fdata-sections");
}
} // namespace toolchains

namespace tools {
namespace dpu {
void Linker::ConstructJob(Compilation &C, const JobAction &JA,
                          const InputInfo &Output, const InputInfoList &Inputs,
                          const llvm::opt::ArgList &TCArgs,
                          const char *LinkingOutput) const {
  std::string Linker = getToolChain().GetProgramPath(getShortName());
  // Put additional linker options
  ArgStringList CmdArgs;
  CmdArgs.push_back("--discard-locals");

  AddLinkerInputs(getToolChain(), Inputs, TCArgs, CmdArgs, JA);
  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  bool HasArgScript = false;
  for (unsigned int EachArg = 0; EachArg < CmdArgs.size(); EachArg++) {
    if (CmdArgs[EachArg][0] == '-' &&
        (!strncmp("-T", CmdArgs[EachArg], 2) ||
         !strncmp("--script", CmdArgs[EachArg], 8))) {
      HasArgScript = true;
      break;
    }
  }
  if (!HasArgScript) {
    CmdArgs.push_back("-T");
    CmdArgs.push_back(LinkScript);
  }

  CmdArgs.push_back("-gc-sections");
  // Must force common allocation, so that symbols with SHN_COMMON (aka .common)
  // have space allocated in WRAM. Otherwise, the linker places symbols at
  // the very beginning of memory with no allocation.
  CmdArgs.push_back("--define-common");
  if (!TCArgs.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    CmdArgs.push_back("-L");
    CmdArgs.push_back(RtLibraryPath);
    if (TCArgs.hasArg(options::OPT_flto) ||
        TCArgs.hasArg(options::OPT_flto_EQ)) {
      CmdArgs.push_back("-lrtmin");
      // Need to inject the RTE BC library into the whole chain.
      CmdArgs.push_back(RtBcLibrary);
    } else {
      CmdArgs.push_back("-lrt");
    }
  }

  /* Pass -L options to the linker */
  TCArgs.AddAllArgs(CmdArgs, options::OPT_L);

  C.addCommand(llvm::make_unique<Command>(
      JA, *this, TCArgs.MakeArgString(Linker), CmdArgs, Inputs));
}
} // namespace dpu
} // namespace tools
} // namespace driver
} // namespace clang
