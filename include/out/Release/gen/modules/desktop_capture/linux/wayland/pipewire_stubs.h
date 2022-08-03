// This is generated file. Do not modify directly.

#ifndef GEN_MODULES_DESKTOP_CAPTURE_LINUX_WAYLAND_PIPEWIRE_STUBS_H_
#define GEN_MODULES_DESKTOP_CAPTURE_LINUX_WAYLAND_PIPEWIRE_STUBS_H_

#include <map>
#include <string>
#include <vector>

namespace modules_desktop_capture_linux_wayland {
// Individual module initializer functions.
bool IsPipewireInitialized();
void InitializePipewire(void* module);
void UninitializePipewire();

bool IsDrmInitialized();
void InitializeDrm(void* module);
void UninitializeDrm();

// Enum and typedef for umbrella initializer.
enum StubModules {
  kModulePipewire = 0,
  kModuleDrm,
  kNumStubModules
};

typedef std::map<StubModules, std::vector<std::string>> StubPathMap;

// Umbrella initializer for all the modules in this stub file.
bool InitializeStubs(const StubPathMap& path_map);
}  // namespace modules_desktop_capture_linux_wayland

#endif  // GEN_MODULES_DESKTOP_CAPTURE_LINUX_WAYLAND_PIPEWIRE_STUBS_H_
