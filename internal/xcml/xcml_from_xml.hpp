#pragma once

#include <pugixml.hpp>
#include "xcml_type_fwd.hpp"

namespace xcml {

void from_xml(pugi::xml_node const& xml, xcml::xcml_program_node_ptr& node);
void make_from_xml(pugi::xml_node const& xml, xcml::expr_ptr& node);
void make_from_xml(pugi::xml_node const& xml, xcml::compound_stmt_ptr& node);

}  // namespace xcml
