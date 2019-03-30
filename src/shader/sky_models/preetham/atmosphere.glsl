#include <../table_common.glsl>

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform vec3 u_PerezInvDen;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 preetham_sky_rgb(vec3 v)
{
    float cosTheta = v.y;
    float cosGamma = dot(u_Direction, v);

    if (cosTheta < 0.0)
        cosTheta = 0.0;

    float t = cosTheta;
    float g = map_gamma(cosGamma);

    vec3 F = table_lerp(t, TABLE_SIZE, THETA_TABLE);
    vec3 G = table_lerp(g, TABLE_SIZE, GAMMA_TABLE);

    vec3 xyY = (vec3(1.0) - F) * (vec3(1.0) + G);

    xyY *= u_PerezInvDen;

    return xyY_to_RGB(xyY) * 5e-5;
}

// ------------------------------------------------------------------