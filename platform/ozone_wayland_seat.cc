// Copyright 2017 GENIVI Alliance. All rights reserved.
// Copyright 2017 Igalia, S.L. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/platform/ozone_wayland_seat.h"

#include "base/logging.h"

namespace ui {

OzoneWaylandSeat::OzoneWaylandSeat(const std::string name,
                                   std::vector<uint32_t> device_ids)
    : name_(name),
      device_ids_(device_ids) {
}

OzoneWaylandSeat::~OzoneWaylandSeat() {
}

bool OzoneWaylandSeat::ContainsDevice(uint32_t device_id) {
  std::vector<uint32_t>::const_iterator i;
  for(i=device_ids_.begin(); i!=device_ids_.end(); ++i) {
    LOG(ERROR) << "ContainsDevice device_id? Tis is " << *i;
    if (*i == device_id)
      return true;
  }
  return false;
}
}  // namespace ui
