// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/input/pointer.h"

#include <linux/input.h>

#include "ozone/wayland/input/cursor.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/window.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ui/events/event.h"

namespace ozonewayland {

WaylandPointer::WaylandPointer()
  : cursor_(NULL),
    dispatcher_(NULL),
    pointer_position_(0, 0),
    input_pointer_(NULL) {
}

WaylandPointer::~WaylandPointer() {
  delete cursor_;
  if (input_pointer_)
    wl_pointer_destroy(input_pointer_);
}

void WaylandPointer::OnSeatCapabilities(wl_seat *seat, uint32_t caps) {
  static const struct wl_pointer_listener kInputPointerListener = {
    WaylandPointer::OnPointerEnter,
    WaylandPointer::OnPointerLeave,
    WaylandPointer::OnMotionNotify,
    WaylandPointer::OnButtonNotify,
    WaylandPointer::OnAxisNotify,
  };

  if (!cursor_)
    cursor_ = new WaylandCursor();

  dispatcher_ = WaylandDisplay::GetInstance();
  seat_ = static_cast<WaylandSeat*>(wl_seat_get_user_data(seat));

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !cursor_->GetInputPointer()) {
    input_pointer_ = wl_seat_get_pointer(seat);
      cursor_->SetInputPointer(input_pointer_);
    wl_pointer_set_user_data(input_pointer_, this);
    wl_pointer_add_listener(input_pointer_, &kInputPointerListener, this);
    device_id_ = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(input_pointer_));
  }
}

void WaylandPointer::OnMotionNotify(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t time,
                                    wl_fixed_t sx_w,
                                    wl_fixed_t sy_w) {
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  WaylandSeat* seat = device->seat_;
  float sx = wl_fixed_to_double(sx_w);
  float sy = wl_fixed_to_double(sy_w);

  device->pointer_position_.SetPoint(sx, sy);
  if (seat->GetGrabWindowHandle() &&
        seat->GetGrabWindowHandle() != seat->GetFocusWindowHandle()) {
      return;
  }

  device->dispatcher_->MotionNotify(sx, sy, device->device_id_);
}

void WaylandPointer::OnButtonNotify(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t serial,
                                    uint32_t time,
                                    uint32_t button,
                                    uint32_t state) {
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  WaylandDisplay::GetInstance()->SetSerial(serial);
  WaylandSeat* seat = device->seat_;
  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(seat->GetFocusWindowHandle());

  if (seat->GetFocusWindowHandle() && seat->GetGrabButton() == 0 &&
        state == WL_POINTER_BUTTON_STATE_PRESSED)
    seat->SetGrabWindowHandle(seat->GetFocusWindowHandle(), button);

  if (seat->GetGrabWindowHandle()) {
    ui::EventType type = ui::ET_MOUSE_PRESSED;
    if (state == WL_POINTER_BUTTON_STATE_RELEASED)
      type = ui::ET_MOUSE_RELEASED;

    // TODO(vignatti): simultaneous clicks fail
    ui::EventFlags flags = ui::EF_NONE;
    if (button == BTN_LEFT)
      flags = ui::EF_LEFT_MOUSE_BUTTON;
    else if (button == BTN_RIGHT)
      flags = ui::EF_RIGHT_MOUSE_BUTTON;
    else if (button == BTN_MIDDLE)
      flags = ui::EF_MIDDLE_MOUSE_BUTTON;

    device->dispatcher_->ButtonNotify(seat->GetFocusWindowHandle(),
                                      type,
                                      flags,
                                      device->pointer_position_.x(),
                                      device->pointer_position_.y(),
                                      device->device_id_);
  }

  if (seat->GetGrabWindowHandle() && seat->GetGrabButton() == button &&
        state == WL_POINTER_BUTTON_STATE_RELEASED)
    seat->SetGrabWindowHandle(0, 0);
}

void WaylandPointer::OnAxisNotify(void* data,
                                  wl_pointer* input_pointer,
                                  uint32_t time,
                                  uint32_t axis,
                                  int32_t value) {
  int x_offset = 0, y_offset = 0;
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  const int delta = ui::MouseWheelEvent::kWheelDelta;

  switch (axis) {
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
      x_offset = value > 0 ? -delta : delta;
      break;
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
      y_offset = value > 0 ? -delta : delta;
      break;
    default:
      break;
  }

  device->dispatcher_->AxisNotify(device->pointer_position_.x(),
                                  device->pointer_position_.y(),
                                  x_offset,
                                  y_offset,
                                  device->device_id_);
}

void WaylandPointer::OnPointerEnter(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t serial,
                                    wl_surface* surface,
                                    wl_fixed_t sx_w,
                                    wl_fixed_t sy_w) {
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  WaylandSeat* seat = device->seat_;
  std::string seat_name = seat->GetName();

  if (!surface) {
    seat->SetFocusWindowHandle(0);
    return;
  }

  WaylandWindow* window =
      static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  if (!window || !window->ShellSurface()->CanAcceptSeatEvents(seat_name.c_str()))
      return;
  unsigned handle = window->Handle();
  float sx = wl_fixed_to_double(sx_w);
  float sy = wl_fixed_to_double(sy_w);

  WaylandDisplay::GetInstance()->SetSerial(serial);
  device->pointer_position_.SetPoint(sx, sy);
  seat->SetFocusWindowHandle(handle);
  device->dispatcher_->PointerEnter(handle,
                                    device->pointer_position_.x(),
                                    device->pointer_position_.y());
}

void WaylandPointer::OnPointerLeave(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t serial,
                                    wl_surface* surface) {
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  WaylandDisplay::GetInstance()->SetSerial(serial);

  WaylandSeat* seat = device->seat_;
  device->dispatcher_->PointerLeave(seat->GetFocusWindowHandle(),
                                    device->pointer_position_.x(),
                                    device->pointer_position_.y());
  seat->SetFocusWindowHandle(0);
}

}  // namespace ozonewayland
