#pragma once

#include <pugixml.hpp>
#include "xcml_type_fwd.hpp"

namespace xcml {

void to_xml(pugi::xml_node& xml, node_ptr const& node);
void to_xml(pugi::xml_node& xml, xcml_program_node_ptr const& node);

}  // namespace xcml
