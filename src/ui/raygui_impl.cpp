// Third-party raygui implementation unit — keep warnings soft here.
#include "gui_text_box.hpp"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

namespace requestman {

void set_gui_text_box_cursor(int index) noexcept {
  if (index < 0) {
    index = 0;
  }
  textBoxCursorIndex = index;
}

int gui_text_box_cursor() noexcept { return textBoxCursorIndex; }

} // namespace requestman
