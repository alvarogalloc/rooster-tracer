module;
#include <stb_image.h>
#include <stb_image_write.h>

export module stb;
export {
    using ::stbi_image_free;
    using ::stbi_load;
    using ::stbi_loadf;
    using ::stbi_set_flip_vertically_on_load;
    using ::stbi_write_png;
}
