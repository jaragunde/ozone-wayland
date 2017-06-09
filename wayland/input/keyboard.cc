// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/input/keyboard.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell_surface.h"
#include <string>

namespace ozonewayland {

WaylandKeyboard::WaylandKeyboard() : input_keyboard_(NULL),
    dispatcher_(NULL) {
}

WaylandKeyboard::~WaylandKeyboard() {
  if (input_keyboard_)
    wl_keyboard_destroy(input_keyboard_);
}

void WaylandKeyboard::OnSeatCapabilities(wl_seat *seat, uint32_t caps) {
  static const struct wl_keyboard_listener kInputKeyboardListener = {
    WaylandKeyboard::OnKeyboardKeymap,
    WaylandKeyboard::OnKeyboardEnter,
    WaylandKeyboard::OnKeyboardLeave,
    WaylandKeyboard::OnKeyNotify,
    WaylandKeyboard::OnKeyModifiers,
  };

  dispatcher_ =
      WaylandDisplay::GetInstance();
  seat_ = static_cast<WaylandSeat*>(wl_seat_get_user_data(seat));

  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !input_keyboard_) {
    input_keyboard_ = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(input_keyboard_, &kInputKeyboardListener,
        this);
    device_id_ = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(input_keyboard_));
  }
}

void WaylandKeyboard::OnKeyNotify(void* data,
                                  wl_keyboard* input_keyboard,
                                  uint32_t serial,
                                  uint32_t time,
                                  uint32_t key,
                                  uint32_t state) {
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  WaylandSeat* seat = device->seat_;
  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(seat->GetKeyboardFocusWindowHandle());
  std::string seat_name = seat->GetName();
  if (!window) {
    LOG(ERROR) << "WaylandKeyboard::OnKeyNotify no focused window ";
    return;
  }
  if (!window->ShellSurface()->CanAcceptSeatEvents(seat_name.c_str())) {
    return;
  }
  ui::EventType type = ui::ET_KEY_PRESSED;
  WaylandDisplay::GetInstance()->SetSerial(serial);
  if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    type = ui::ET_KEY_RELEASED;
  const uint32_t device_id = wl_proxy_get_id(
      reinterpret_cast<wl_proxy*>(input_keyboard));
  device->dispatcher_->KeyNotify(type, key, device_id);
}

void WaylandKeyboard::OnKeyboardKeymap(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t format,
                                       int fd,
                                       uint32_t size) {
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);

  if (!data) {
    close(fd);
    return;
  }

  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    close(fd);
    return;
  }

  base::SharedMemoryHandle section =  base::FileDescriptor(fd, true);
  device->dispatcher_->InitializeXKB(section, size);
}

void WaylandKeyboard::OnKeyboardEnter(void* data,
                                      wl_keyboard* input_keyboard,
                                      uint32_t serial,
                                      wl_surface* surface,
                                      wl_array* keys) {
  WaylandDisplay::GetInstance()->SetSerial(serial);
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  WaylandSeat* seat = device->seat_;
  WaylandWindow* window =
    static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  unsigned handle = window->Handle();
  seat->SetKeyboardFocusWindowHandle(handle);
  device->dispatcher_->KeyboardEnter(handle);
}

void WaylandKeyboard::OnKeyboardLeave(void* data,
                                      wl_keyboard* input_keyboard,
                                      uint32_t serial,
                                      wl_surface* surface) {
  WaylandDisplay::GetInstance()->SetSerial(serial);
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  WaylandSeat* seat = device->seat_;
  WaylandWindow* window =
    static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  unsigned handle = window->Handle();
  seat->SetKeyboardFocusWindowHandle(0);
  device->dispatcher_->KeyboardLeave(handle);
}

void WaylandKeyboard::OnKeyModifiers(void *data,
                                     wl_keyboard *keyboard,
                                     uint32_t serial,
                                     uint32_t mods_depressed,
                                     uint32_t mods_latched,
                                     uint32_t mods_locked,
                                     uint32_t group) {
}

}  // namespace ozonewayland
