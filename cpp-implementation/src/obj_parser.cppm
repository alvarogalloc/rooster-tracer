export module obj_parser;
import std;
import scene;
import object3d;
export namespace cg {
namespace parsers {
void parse_obj_file(std::istringstream &ss, scene &s);
}

} // namespace cg
