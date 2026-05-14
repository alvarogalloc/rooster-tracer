import std;
import raytracer;
import scene_parser;
int main(int argc, char** argv)
{
  if (argc != 3)
  {
    std::println(std::cerr,
                 "please set the scene to render and the output img\n usage: "
                 "rooster-tracer scene.rscn output.png");
    return -1;
  }
  const auto scene_path{argv[1]};
  const auto img_path{argv[2]};
  try
  {
    const auto parsed_scene = cg::parse_scene(scene_path);
    if (const auto validation_error = cg::validate_scene_for_render(parsed_scene))
    {
      throw std::runtime_error{
          std::format("scene validation failed: {}", *validation_error)};
    }
    cg::render_to_png(parsed_scene, img_path);
  }
  catch (const std::exception& ex)
  {
    std::println(
        std::cerr,
        "exception caught (rooster-tracer)!, exiting...\n[reason] -> \"{}\"",
        ex.what());
    return -1;
  }
  std::println("{} rendered correctly!!!", img_path);
}
