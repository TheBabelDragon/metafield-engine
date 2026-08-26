#pragma once

#include "engine/ingest/observation.hpp"
#include <string_view>

namespace mf {

FieldObservation parse_csi_line(std::string_view line);

} // namespace mf
