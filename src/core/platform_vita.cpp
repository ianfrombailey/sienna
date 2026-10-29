#include "core/platform.hpp"

#include <psp2/appmgr.h>
#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/libime.h>
#include <psp2/power.h>

#include <cstdint>
#include <cstring>

// SceAppMgr's application event queue, which the SDK headers leave out.
extern "C" {
struct AppMgrEvent {
  int event;
  SceUID appId;
  char param[56];
};
int sceAppMgrReceiveEventNum(int* num);
int sceAppMgrReceiveEvent(AppMgrEvent* event);
}

namespace sienna::platform {

namespace {

const std::string kDataDir = "ux0:data/sienna/";
const std::string kResDir = "app0:res/";

SceWChar16 gTitle[SCE_IME_DIALOG_MAX_TITLE_LENGTH + 1];
SceWChar16 gInitial[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
SceWChar16 gInput[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
bool gKeyboardOpen = false;

// event ids from SceAppMgr. the names are missing from older headers.
constexpr int kEventActivate = 0x10000001;
constexpr int kEventDeactivate = 0x10000002;
constexpr int kEventResume = 0x10000003;
constexpr int kEventExit = 0x20000001;

// the app attribute turns off system auto-sleep, so SIENNA sleeps itself when idle.
constexpr unsigned kIdleSuspendSeconds = 10 * 60;

volatile bool gQuit = false;
volatile bool gForeground = true;
volatile bool gScreenOn = true;
volatile bool gBusy = false;
volatile unsigned gLastActive = 0;
SceUInt64 gNextPoll = 0;

unsigned nowSeconds() {
  return static_cast<unsigned>(sceKernelGetProcessTimeWide() / 1000000);
}

// runs on its own thread because the main loop slows down while SIENNA is hidden.
int powerThread(SceSize, void*) {
  for (;;) {
    if (gBusy) {
      sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
    } else if (nowSeconds() - gLastActive > kIdleSuspendSeconds) {
      gLastActive = nowSeconds();
      scePowerRequestSuspend();
    }
    sceKernelDelayThreadCB(10 * 1000 * 1000);
  }
  return 0;
}

void onEvent(int event) {
  // ignoring the exit event leaves the shell waiting on us forever.
  if (event == kEventExit) {
    gQuit = true;
  } else if (event == kEventDeactivate) {
    gForeground = false;
  } else if (event == kEventActivate || event == kEventResume) {
    gForeground = true;
    gScreenOn = true;
  }
}

void pollEvents() {
  int n = 0;
  if (sceAppMgrReceiveEventNum(&n) == 0) {
    while (n-- > 0) {
      AppMgrEvent ev;
      if (sceAppMgrReceiveEvent(&ev) < 0) {
        break;
      }
      onEvent(ev.event);
    }
  }
  SceAppMgrAppState state;
  std::memset(&state, 0, sizeof(state));
  if (_sceAppMgrGetAppState(&state, sizeof(state), 0x03570011) < 0) {
    return;
  }
  while (state.systemEventNum-- > 0) {
    SceAppMgrSystemEvent ev;
    if (sceAppMgrReceiveSystemEvent(&ev) < 0) {
      break;
    }
    onEvent(ev.systemEvent);
  }
}

// converts to UTF-16, truncating to max code units (plus a terminator).
void toUtf16(const std::string& in, SceWChar16* out, std::size_t max) {
  std::size_t o = 0;
  for (std::size_t i = 0; i < in.size() && o < max;) {
    const unsigned char c = static_cast<unsigned char>(in[i]);
    std::uint32_t cp;
    std::size_t len;
    if (c < 0x80) {
      cp = c;
      len = 1;
    } else if ((c >> 5) == 0x6) {
      cp = c & 0x1F;
      len = 2;
    } else if ((c >> 4) == 0xE) {
      cp = c & 0x0F;
      len = 3;
    } else {
      cp = c & 0x07;
      len = 4;
    }
    if (i + len > in.size()) {
      break;
    }
    for (std::size_t k = 1; k < len; ++k) {
      cp = (cp << 6) | (static_cast<unsigned char>(in[i + k]) & 0x3F);
    }
    i += len;
    if (cp >= 0x10000) {
      if (o + 2 > max) {
        break;
      }
      cp -= 0x10000;
      out[o++] = static_cast<SceWChar16>(0xD800 + (cp >> 10));
      out[o++] = static_cast<SceWChar16>(0xDC00 + (cp & 0x3FF));
    } else {
      out[o++] = static_cast<SceWChar16>(cp);
    }
  }
  out[o] = 0;
}

std::string fromUtf16(const SceWChar16* in) {
  std::string out;
  for (std::size_t i = 0; in[i] != 0; ++i) {
    std::uint32_t cp = in[i];
    if (cp >= 0xD800 && cp < 0xDC00 && in[i + 1] >= 0xDC00 && in[i + 1] < 0xE000) {
      cp = 0x10000 + ((cp - 0xD800) << 10) + (in[i + 1] - 0xDC00);
      ++i;
    }
    if (cp < 0x80) {
      out += static_cast<char>(cp);
    } else if (cp < 0x800) {
      out += static_cast<char>(0xC0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }
  return out;
}

}  // namespace

void init() {
  sceIoMkdir("ux0:data/sienna", 0777);
  sceIoMkdir("ux0:data/sienna/music", 0777);

  // the BGM port lets audio keep going while SIENNA is in the background.
  sceAppMgrAcquireBgmPort();
  gLastActive = nowSeconds();
  const SceUID thread = sceKernelCreateThread("sienna_power", powerThread, 0x10000100, 0x4000, 0, 0, nullptr);
  if (thread >= 0) {
    sceKernelStartThread(thread, 0, nullptr);
  }
}

void shutdown() {
  sceAppMgrReleaseBgmPort();
}

std::vector<std::string> defaultMusicFolders() {
  return {"ux0:music", kDataDir + "music"};
}

void tick(bool busy, bool input) {
  gBusy = busy;
  if (busy || input) {
    gLastActive = nowSeconds();
  }
  // events are IPC to the shell, so ten polls a second is plenty.
  const SceUInt64 now = sceKernelGetProcessTimeWide();
  if (now >= gNextPoll) {
    gNextPoll = now + 100 * 1000;
    pollEvents();
  }
}

bool quitRequested() {
  return gQuit;
}

bool visible() {
  return gForeground && gScreenOn;
}

void setScreenOn(bool on) {
  gScreenOn = on;
  if (on) {
    scePowerRequestDisplayOn();
  } else {
    scePowerRequestDisplayOff();
  }
}

bool screenOn() {
  return gScreenOn;
}

const std::string& dataDir() {
  return kDataDir;
}

const std::string& resDir() {
  return kResDir;
}

// SDL's renderer calls sceCommonDialogUpdate every frame, so the dialog draws on its own.
bool openKeyboard(const std::string& title, const std::string& initial, bool multiline) {
  if (gKeyboardOpen) {
    return true;
  }
  toUtf16(title, gTitle, SCE_IME_DIALOG_MAX_TITLE_LENGTH);
  toUtf16(initial, gInitial, SCE_IME_DIALOG_MAX_TEXT_LENGTH);
  gInput[0] = 0;

  SceImeDialogParam param;
  sceImeDialogParamInit(&param);
  param.supportedLanguages = 0;
  param.languagesForced = SCE_FALSE;
  param.type = SCE_IME_TYPE_DEFAULT;
  param.option = multiline ? SCE_IME_OPTION_MULTILINE : 0;
  param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_DEFAULT;
  param.title = gTitle;
  param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
  param.initialText = gInitial;
  param.inputTextBuffer = gInput;

  gKeyboardOpen = sceImeDialogInit(&param) >= 0;
  return gKeyboardOpen;
}

KeyboardStatus pollKeyboard(std::string& result) {
  if (!gKeyboardOpen) {
    return KeyboardStatus::Cancelled;
  }
  if (sceImeDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
    return KeyboardStatus::Running;
  }
  SceImeDialogResult res{};
  sceImeDialogGetResult(&res);
  sceImeDialogTerm();
  gKeyboardOpen = false;
  // in multiline mode enter types a newline, so closing the keyboard is how text is confirmed.
  if (res.button != SCE_IME_DIALOG_BUTTON_ENTER && res.button != SCE_IME_DIALOG_BUTTON_CLOSE) {
    return KeyboardStatus::Cancelled;
  }
  result = fromUtf16(gInput);
  return KeyboardStatus::Accepted;
}

}  // namespace sienna::platform
