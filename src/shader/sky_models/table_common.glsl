#ifndef TABLE_COMMON_GLSL
#define TABLE_COMMON_GLSL

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform vec3 u_Direction;
uniform int TABLE_SIZE;

uniform sampler2D s_Table;

// ------------------------------------------------------------------
// CONSTANTS --------------------------------------------------------
// ------------------------------------------------------------------

#define THETA_TABLE 0
#define GAMMA_TABLE 1

// XYZ/RGB for sRGB primaries
const vec3 kXYZToR = vec3( 3.2404542, -1.5371385, -0.4985314);
const vec3 kXYZToG = vec3(-0.9692660,  1.8760108,  0.0415560);
const vec3 kXYZToB = vec3( 0.0556434, -0.2040259,  1.0572252);
const vec3 kRGBToX = vec3(0.4124564,  0.3575761,  0.1804375);
const vec3 kRGBToY = vec3(0.2126729,  0.7151522,  0.0721750);
const vec3 kRGBToZ = vec3(0.0193339,  0.1191920,  0.9503041);

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 table_value(int i, int comp)
{
    return texelFetch(s_Table, ivec2(i, comp), 0).rgb;
}

// ------------------------------------------------------------------

float map_gamma(float g)
{
    return sqrt(0.5 * (1.0 - g));
}

// ------------------------------------------------------------------

vec3 table_lerp(float s, int n, int comp)
{
    s = clamp(s, 0.0, 1.0 - 1e-6);
    
    s *= n - 1;

    int si0 = int(s);
    int si1 = (si0 + 1);
    float sf = s - si0;

    return table_value(si0, comp) * (1 - sf) + table_value(si1, comp) * sf;
}

// ------------------------------------------------------------------

vec3 xyY_to_XYZ(vec3 c)
{
    return vec3(c.x, c.y, 1.0 - c.x - c.y) * c.z / c.y;
}

// ------------------------------------------------------------------

vec3 xyY_to_RGB(vec3 xyY)
{
    vec3 XYZ = xyY_to_XYZ(xyY);
    return vec3(dot(kXYZToR, XYZ),
                dot(kXYZToG, XYZ),
                dot(kXYZToB, XYZ));
}

// ------------------------------------------------------------------

vec3 XYZ_to_RGB(vec3 XYZ)
{
    return vec3(dot(kXYZToR, XYZ), dot(kXYZToG, XYZ), dot(kXYZToB, XYZ));
}

// ------------------------------------------------------------------

#endif