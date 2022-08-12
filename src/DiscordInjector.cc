#include <cstdint>
#include <link.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <gelf.h>

#include "DiscordInjector.h"
#include "desktop_capture.h"
#include "rtc_base/logging.h"
#include "rtc_base/thread.h"

using namespace std;

rtc::Thread *GlobalInjectorThread()
{
  static auto result = []
  {
    auto thread = rtc::Thread::Create();
    thread->SetName("LinuxFix-Injector", nullptr);
    thread->Start();
    return thread;
  }();
  return result.get();
}

namespace LinuxFix
{
  void DiscordInjector::Inject(pid_t pid)
  {
    GlobalInjectorThread()->PostTask([this, pid]()
                                     { _Inject(pid); });
  }

  int FindDiscordModule(void *voice_module)
  {
    DiscordVoiceModule *module = (DiscordVoiceModule *)voice_module;

    ElfW(Addr) base_address;
    int n;
    char path[PATH_MAX], buf[PATH_MAX];
    auto fd = fopen("/proc/self/maps", "r");

    while (fgets(buf, PATH_MAX, fd))
    {
      RTC_LOG(LS_VERBOSE) << buf;
      n = sscanf(buf,
                 "%lx-%*lx "
                 "%*4c "
                 "%*x %*s %*d "
                 "%[^\n]",
                 &base_address,
                 path);
      if (n == 3)
        continue;
      assert(n == 4);
      if (strstr(path, "discord_voice.node"))
      {
        module->base_address = base_address;
        module->path = path;
        RTC_LOG(LS_VERBOSE) << "Found discord_voice.node, base_address=" << base_address;
        return 1;
      }
    }
    return 0;
  }

  bool DiscordInjector::FindFunctionPointers(DiscordVoiceModule *voice_module, map<string, intptr_t> *pointer_map)
  {
    auto fd = open(voice_module->path, O_RDONLY);
    if (fd < 0)
    {
      RTC_LOG(LS_ERROR) << "Failed to open " << voice_module->path;
      return 0;
    }

    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    char *file_data = (char *)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    auto elf_data = elf_memory(file_data, file_size);
    GElf_Ehdr ehdr_storage;
    GElf_Ehdr *ehdr = gelf_getehdr(elf_data, &ehdr_storage);
    auto type = ehdr->e_type;

    if (type != ET_EXEC && type != ET_DYN)
    {
      RTC_LOG(LS_ERROR) << "Invalid ELF on " << voice_module->path;
      return 0;
    }

    int header_count = ehdr->e_phnum;
    ElfW(Addr) preferred_address;
    for (int i = 0; i < header_count; i++)
    {
      GElf_Phdr phdr;
      gelf_getphdr(elf_data, i, &phdr);
      if (phdr.p_type == PT_LOAD && phdr.p_offset == 0)
      {
        preferred_address = phdr.p_vaddr;
        break;
      }
    }
    RTC_LOG(LS_VERBOSE) << "Preferred addr: " << preferred_address;

    GElf_Shdr shdr;
    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf_data, scn)) != NULL)
    {
      gelf_getshdr(scn, &shdr);
      if (shdr.sh_type == SHT_SYMTAB)
      {
        break;
      }
    }

    GElf_Word symbol_count = shdr.sh_size / shdr.sh_entsize;
    Elf_Data *data = elf_getdata(scn, NULL);

    GElf_Word symbol_index;
    for (symbol_index = 0; symbol_index != symbol_count; symbol_index++)
    {
      GElf_Sym sym;
      ElfSymbolDetails details;

      gelf_getsym(data, symbol_index, &sym);

      details.name = elf_strptr(elf_data, shdr.sh_link, sym.st_name);
      if (details.name == NULL)
        continue;
      details.address =
          (sym.st_value != 0)
              ? (voice_module->base_address + (sym.st_value - voice_module->preferred_address))
              : 0;
      details.size = sym.st_size;
      details.type = GELF_ST_TYPE(sym.st_info);
      details.bind = GELF_ST_BIND(sym.st_info);
      details.section_header_index = sym.st_shndx;
      if (pointer_map->count(details.name))
      {
        if (pointer_map->at(details.name) != 0)
        {
          RTC_LOG(LS_ERROR) << "Function " << details.name << " has different addresses (duplicate)";
          return false;
        }
        pointer_map->at(details.name) = details.address;
      }
    }

    for (auto it = pointer_map->begin(); it != pointer_map->end(); ++it)
    {
      if (it->second == 0)
      {
        RTC_LOG(LS_ERROR) << "Function " << it->first << " not found";
        return false;
      }
    }
    return 1;
  }

  void DiscordInjector::_Inject(pid_t pid)
  {
    DiscordVoiceModule voice_module;
    int i = FindDiscordModule(&voice_module);
    if (!i || voice_module.path == NULL)
    {
      RTC_LOG(LS_ERROR) << "Could not find discord_voice.node";
      return;
    }

    map<string, intptr_t> function_map;
    function_map.insert(pair<string, intptr_t>("_ZN6webrtc21DesktopCaptureOptions13CreateDefaultEv", 0));
    function_map.insert(pair<string, intptr_t>("_ZN6webrtc15DesktopCapturer20CreateScreenCapturerERKNS_21DesktopCaptureOptionsE", 0));

    if (!FindFunctionPointers(&voice_module, &function_map))
    {
      RTC_LOG(LS_ERROR) << "Could not find function pointers";
      return;
    }
    // TODO Hooking
  }

}; // namespace LinuxFix