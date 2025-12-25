#include "common/game_object.h"
#include "common/renderer.h"
#include "resource/shader.h"
#include "service/service_locator.h"
#include "utils/config.h"
#include "utils/macros.h"
#include "utils/path_handler.h"
#include "utils/profiler.h"
#include "interaction/input.h"
#include "interaction/camera.h"
#include "common/engine.h"
#include "entity/plane.h"
#include "resource/particles.h"

#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>

class Weapons{


};