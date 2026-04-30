export module obj_parser;
import std;
import scene;
export namespace cg {
namespace parsers {
void parse_obj_file(std::istringstream &ss, scene::object_collection &objects);
}

} // namespace cg
