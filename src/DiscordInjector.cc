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

#include "rtc_base/logging.h"
#include "rtc_base/thread.h"
#include "Hooker.h"
#include "DiscordInjector.h"

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
    GlobalInjectorThread()->PostTask(
        [this, pid]()
        {
          try
          {
            _Inject(pid);
          }
          catch (const std::exception &e)
          {
            RTC_LOG(LS_ERROR) << "Failed to inject: " << e.what();
          }
        });
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
      n = sscanf(buf,
                 "%lx-%*lx "
                 "%*4c "
                 "%*x %*s %*d "
                 "%[^\n]",
                 &base_address,
                 path);
      if (n != 2)
        continue;
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

  int DiscordInjector::FindFunctionPointers(DiscordVoiceModule *voice_module)
  {
    map<string, uint64_t> *pointer_map = &voice_module->pointer_map;
    vector<string> *requiredFunctions = Hooker::GetRequiredFunctions();

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
    for (int i = 0; i < header_count; i++)
    {
      GElf_Phdr phdr;
      gelf_getphdr(elf_data, i, &phdr);
      if (phdr.p_type == PT_LOAD && phdr.p_offset == 0)
      {
        voice_module->preferred_address = phdr.p_vaddr;
        break;
      }
    }
    RTC_LOG(LS_VERBOSE) << "Preferred addr: " << voice_module->preferred_address;

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
      if (std::find(requiredFunctions->begin(), requiredFunctions->end(), details.name) != requiredFunctions->end())
      {
        if (pointer_map->find(details.name) != pointer_map->end())
        {
          RTC_LOG(LS_ERROR) << "Function " << details.name << " is not unique";
          return -1;
        }
        RTC_LOG(LS_VERBOSE) << "Found " << details.name << " at " << details.address;
        pointer_map->insert(make_pair(details.name, details.address));
      }
    }

    for (auto it = requiredFunctions->begin(); it != requiredFunctions->end(); ++it)
    {
      if (pointer_map->find(*it) == pointer_map->end())
      {
        RTC_LOG(LS_ERROR) << "Function " << *it << " not found";
        return -1;
      }
    }
    return 0;
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

    if (FindFunctionPointers(&voice_module) != 0)
    {
      RTC_LOG(LS_ERROR) << "Could not find function pointers";
      return;
    }

    Hooker::Install(&voice_module);
  }

}; // namespace LinuxFix