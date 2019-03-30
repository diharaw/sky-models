#include <../table_common.glsl>

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform vec3 u_CoeffsXYZ[9];
uniform vec3 u_RadXYZ;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 hosek_wilkie_sky_rgb(vec3 v)
{
    float cosTheta = v.y;
    float cosGamma = dot(u_Direction, v);

    if (cosTheta < 0.0)
        cosTheta = 0.0;

    float t = cosTheta;
    float g = map_gamma(cosGamma);

    vec3 F = table_lerp(t, TABLE_SIZE, THETA_TABLE);
    vec3 G = table_lerp(g, TABLE_SIZE, GAMMA_TABLE);

    const float zenith = pow(cosTheta, 2.0);
    vec3 H = vec3(u_CoeffsXYZ[7].x * zenith + (u_CoeffsXYZ[2].x - 1.0),
                  u_CoeffsXYZ[7].y * zenith + (u_CoeffsXYZ[2].y - 1.0),
                  u_CoeffsXYZ[7].z * zenith + (u_CoeffsXYZ[2].z - 1.0));

    // (1 - F(theta)) * (1 + G(phi) + H(theta))
    vec3 XYZ = (vec3(1.0) - F) * (vec3(1.0) + G + H);

    XYZ *= u_RadXYZ;

    return XYZ_to_RGB(XYZ) * 5e-5;
}

// ------------------------------------------------------------------