#pragma once

#include "color.h"

struct Material {
  Color diffuse;
  float albedo;
  float specularAlbedo;
  float specularCoefficient;
};
