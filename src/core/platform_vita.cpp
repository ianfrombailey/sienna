#include "core/platform.hpp"

#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/io/stat.h>
#include <psp2/libime.h>

#include <cstdint>

namespace sienna::platform {

namespace {

const std::string kDataDir = "ux0:data/sienna/";
const std::string kResDir = "app0:res/";

SceWChar16 gTitle[SCE_IME_DIALOG_MAX_TITLE_LENGTH + 1];
SceWChar16 gInitial[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
SceWChar16 gInput[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
bool gKeyboardOpen = false;

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
