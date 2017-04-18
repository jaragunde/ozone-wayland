// Copyright 2017 GENIVI Alliance. All rights reserved.
// Copyright 2017 Igalia, S.L. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

namespace ui {

class OzoneWaylandSeat {
 public:
  OzoneWaylandSeat(const std::string name,
		   std::vector<uint32_t> device_ids);

  ~OzoneWaylandSeat();

  bool ContainsDevice(uint32_t device_id);

 private:
  const std::string name_;
  std::vector<uint32_t> device_ids_;
};

}  // namespace ui
