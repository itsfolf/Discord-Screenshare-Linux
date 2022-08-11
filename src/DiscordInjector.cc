#include <cstdint>
#include <link.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>
#include <gelf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

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
  static GMainLoop *loop = NULL;
  webrtc_demo::DesktopCapture *capturer;
  std::atomic_bool useComposer = ATOMIC_VAR_INIT(false);

  void DiscordInjector::Inject(pid_t pid)
  {
    GlobalInjectorThread()->PostTask([this, pid]()
                                     { _Inject(pid); });
  }

  static gboolean stop(gpointer user_data)
  {
    g_main_loop_quit(loop);

    return FALSE;
  }

  static void on_message(FridaScript *script, const gchar *message, GBytes *data,
                         gpointer user_data)
  {
    JsonParser *parser;
    JsonObject *root;
    const gchar *type;

    parser = json_parser_new();
    json_parser_load_from_data(parser, message, -1, NULL);
    root = json_node_get_object(json_parser_get_root(parser));

    type = json_object_get_string_member(root, "type");
    if (strcmp(type, "log") == 0)
    {
      const gchar *log_message;

      log_message = json_object_get_string_member(root, "payload");
      RTC_LOG(LS_INFO) << log_message;
    }
    else
    {
      JsonObject *cmd = json_object_get_object_member(root, "payload");
      const gchar *op = json_object_get_string_member(cmd, "op");
      if (strcmp(op, "start_capturer") == 0)
      {
        if (capturer)
          capturer->StopCapture();
        capturer = webrtc_demo::DesktopCapture::Create(&useComposer);
        const gchar *callbackAddrString =
            json_object_get_string_member(cmd, "callbackAddr");
        intptr_t callbackAddr = strtoll(callbackAddrString, NULL, 16);
        RTC_LOG(LS_INFO) << "callbackAddr: " << callbackAddr;
        webrtc::DesktopCapturer::Callback *f =
            (webrtc::DesktopCapturer::Callback *)callbackAddr;
        capturer->StartCapture(f);
        useComposer = false;
      }
      else if (strcmp(op, "capture_frame") == 0)
      {
        capturer->CaptureFrame();
      }
      else if (strcmp(op, "start_composer") == 0)
      {
        RTC_LOG(LS_INFO) << "Starting composer";
        useComposer = true;
      }
      else
      {
        RTC_LOG(LS_INFO) << "on_message: " << message;
      }
    }

    g_object_unref(parser);
  }

  static void on_detached(FridaSession *session, FridaSessionDetachReason reason,
                          FridaCrash *crash, gpointer user_data)
  {
    gchar *reason_str;

    reason_str = g_enum_to_string(FRIDA_TYPE_SESSION_DETACH_REASON, reason);
    g_print("on_detached: reason=%s crash=%p\n", reason_str, crash);
    g_free(reason_str);

    g_idle_add(stop, NULL);
  }

  int retrieve_symbols(struct dl_phdr_info *info, size_t info_size,
                       void *symbol_names_vector)
  {
    map<string, vector<string>> *symbol_names =
        reinterpret_cast<map<string, vector<string>> *>(symbol_names_vector);

    for (size_t header_index = 0; header_index < info->dlpi_phnum;
         header_index++)
    {
      
    }

    return 0;
  }

  intptr_t translate_elf_to_active_addr(intptr_t addr)
  {
    return addr; // TODO
  }

  void DiscordInjector::_Inject(pid_t pid)
  {
    string path = "/home/folf/.config/discordcanary/0.0.136/modules/"
                  "discord_voice/discord_voice.node";
    auto fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
    {
      RTC_LOG(LS_ERROR) << "Failed to open " << path;
      return;
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
      RTC_LOG(LS_ERROR) << "Invalid ELF on " << path;
      return;
    }

    int header_count = ehdr->e_phnum;
    Elf64_Addr preferred_address;
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
              ? translate_elf_to_active_addr(sym.st_value)
              : 0;
      details.size = sym.st_size;
      details.type = GELF_ST_TYPE(sym.st_info);
      details.bind = GELF_ST_BIND(sym.st_info);
      details.section_header_index = sym.st_shndx;
      RTC_LOG(LS_INFO) << details.name << " " << details.type << " " << details.address;
    }
    

    /*FridaDeviceManager *manager;
    GError *error = NULL;
    FridaDeviceList *devices;
    gint num_devices, i;
    FridaDevice *local_device;
    FridaSession *session;

    frida_init();

    loop = g_main_loop_new(NULL, TRUE);

    manager = frida_device_manager_new();
    RTC_LOG(LS_VERBOSE) << "Searching for devices";
    devices = frida_device_manager_enumerate_devices_sync(manager, NULL, &error);
    g_assert(error == NULL);

    local_device = NULL;
    num_devices = frida_device_list_size(devices);
    for (i = 0; i != num_devices; i++)
    {
        FridaDevice *device = frida_device_list_get(devices, i);

        RTC_LOG(LS_VERBOSE) << "Found device: " << frida_device_get_name(device);

        if (frida_device_get_dtype(device) == FRIDA_DEVICE_TYPE_LOCAL)
            local_device = g_object_ref(device);

        g_object_unref(device);
    }
    g_assert(local_device != NULL);
    RTC_LOG(LS_INFO) << "Selected device: " <<
    frida_device_get_name(local_device); frida_unref(devices); devices = NULL;

    RTC_LOG(LS_INFO) << "Attatching to process: " << std::to_string(pid);
    session = frida_device_attach_sync(local_device, pid, NULL, NULL, &error);
    if (error == NULL)
    {
        FridaScript *script;
        FridaScriptOptions *options;

        g_signal_connect(session, "detached", G_CALLBACK(on_detached), NULL);
        if (frida_session_is_detached(session))
            goto session_detached_prematurely;

        RTC_LOG(LS_INFO) << "Attached.";

        options = frida_script_options_new();
        frida_script_options_set_name(options, "example");
        frida_script_options_set_runtime(options, FRIDA_SCRIPT_RUNTIME_QJS);

        script = frida_session_create_script_sync(session,
                                                  "const symbolsDiscord =
    Module.enumerateSymbolsSync('discord_voice.node');" "const symbolsThis =
    Module.enumerateSymbolsSync('discord-pawgers.node');" "const funcsDiscord =
    symbolsDiscord.filter(x => x.type == 'function');" "const funcsThis =
    symbolsThis.filter(x => x.type == 'function');"
                                                  ""
                                                  "const x11CapturerStartName =
    '_ZN6webrtc17ScreenCapturerX115StartEPNS_15DesktopCapturer8CallbackE';" "const
    x11CapturerCaptureFrameName =
    '_ZN6webrtc17ScreenCapturerX1112CaptureFrameEv';" "const
    cursorComposerStartName =
    '_ZN6webrtc24DesktopAndCursorComposer5StartEPNS_15DesktopCapturer8CallbackE';"
                                                  "const x11CapturerStartDisc =
    funcsDiscord.find(x => x.name == x11CapturerStartName);" "const
    x11CapturerCaptureFrameDisc = funcsDiscord.find(x => x.name ==
    x11CapturerCaptureFrameName);" "const cursorComposerStartDisc =
    funcsDiscord.find(x => x.name == cursorComposerStartName);" "const
    x11CapturerStart = " "      new NativeFunction(funcsThis.find(x => x.name ==
    x11CapturerStartName).address, 'void', ['pointer', 'pointer']);"
                                                  ""
                                                  "Interceptor.replace(x11CapturerStartDisc.address,
    new NativeCallback((target, callback) => {" "  send({op: 'start_capturer',
    callbackAddr: callback});"
                                                  "}, 'void', ['pointer',
    'pointer']));"
                                                  ""
                                                  "Interceptor.attach(cursorComposerStartDisc.address,
    {" "  onEnter(args) {" "    send({op: 'start_composer'}); " "  }"
                                                  "});"
                                                  ""
                                                  "Interceptor.replace(x11CapturerCaptureFrameDisc.address,
    new NativeCallback((target, opts) => {" "  send({op: 'capture_frame'});"
                                                  "}, 'void', ['pointer',
    'pointer']));", options, NULL, &error); if (error != NULL)
        {
            RTC_LOG(LS_ERROR) << "Failed to create script (skill issue lol): " <<
    error->message; g_error_free(error); error = NULL; goto
    session_detached_prematurely;
        }

        g_clear_object(&options);

        g_signal_connect(script, "message", G_CALLBACK(on_message), NULL);

        frida_script_load_sync(script, NULL, &error);
        g_assert(error == NULL);

        RTC_LOG(LS_VERBOSE) << "Script loaded";

        if (!g_main_loop_is_running(loop))
            g_main_loop_run(loop);

        // RTC_LOG(LS_VERBOSE) << "Stopped";

        // frida_script_unload_sync(script, NULL, NULL);
        // frida_unref(script);
        // RTC_LOG(LS_VERBOSE) << "Unloaded";

        // frida_session_detach_sync(session, NULL, NULL);
    session_detached_prematurely:
        // frida_unref(session);
        RTC_LOG(LS_VERBOSE) << "Detached";
    }
    else
    {
        RTC_LOG(LS_ERROR) << "Failed to attach: " << error->message;
        g_error_free(error);
    }*/
  }
}; // namespace LinuxFix