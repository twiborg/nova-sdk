#pragma once

// Public module information model.
//
// This struct holds the *collected* contents of a loaded module. It does not
// expose any raw Windows-specific structures (MODULEINFO, HMODULE, etc.).
// All fields are populated explicitly by the module enumerator.

#include "nova/loader/pe/pe_types.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace nova::loader {

// Architecture of a loaded module.
enum class ModuleArchitecture : int {
    Unknown = 0,
    X86 = 1,
    X64 = 2,
};

// Information about a single module loaded in the current process.
struct ModuleInfo {
    std::string name;                       // module base name, UTF-8 encoded (e.g. "NOVA.exe")
    std::filesystem::path full_path;        // full filesystem path, when available
    std::uintptr_t base_address = 0;        // module load base address
    std::uint64_t image_size = 0;           // size of the module image, in bytes
    ModuleArchitecture architecture = ModuleArchitecture::Unknown;
    PEMachine pe_machine = PEMachine::Unknown;  // raw PE machine type, when known
};

}  // namespace nova::loader
