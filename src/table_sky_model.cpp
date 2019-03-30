#include "table_sky_model.h"

#define _USE_MATH_DEFINES
#include <math.h>

const glm::vec3 kRGBToX(0.4124564f,  0.3575761f,  0.1804375f);
const glm::vec3 kRGBToY(0.2126729f,  0.7151522f,  0.0721750f);
const glm::vec3 kRGBToZ(0.0193339f,  0.1191920f,  0.9503041f);

// -----------------------------------------------------------------------------------------------------------------------------------

float TableSkyModel::zenith_luminance(float thetaS, float T)
{
    float chi = (4.0f / 9.0f - T / 120.0f) * (M_PI - 2.0f * thetaS);
    float Lz = (4.0453f * T - 4.9710f) * tanf(chi) - 0.2155f * T + 2.4192f;
    Lz *= 1000.0;   // conversion from kcd/m^2 to cd/m^2
    return Lz;
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 TableSkyModel::rgb_to_XYZ(const glm::vec3& rgb)
{
    return glm::vec3(glm::dot(kRGBToX, rgb), glm::dot(kRGBToY, rgb), glm::dot(kRGBToZ, rgb));
}

// -----------------------------------------------------------------------------------------------------------------------------------
