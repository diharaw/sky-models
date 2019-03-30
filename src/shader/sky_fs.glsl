#include <sky_models/bruneton/atmosphere.glsl>
#include <sky_models/preetham/atmosphere.glsl>
#include <sky_models/hosek_wilkie/atmosphere.glsl>

out vec4 PS_OUT_Color;

in vec3 PS_IN_TexCoord;

uniform vec3 u_Direction;
uniform vec3 camera_pos;
uniform int sky_model;

void main()
{
	vec3 dir = normalize(PS_IN_TexCoord);

	if (sky_model == 0)
	{
		float sun = step(cos(M_PI / 360.0), dot(dir, SUN_DIR));
					
		vec3 sunColor = vec3(sun,sun,sun) * SUN_INTENSITY;

		vec3 extinction;
		vec3 inscatter = SkyRadiance(camera_pos, dir, extinction);
		vec3 col = sunColor * extinction + inscatter;

		PS_OUT_Color = vec4(col, 1.0);
	}
	else if (sky_model == 1)
	{
		vec3 col = preetham_sky_rgb(dir, u_Direction);

		PS_OUT_Color = vec4(col, 1.0);
	}
	else
	{
		vec3 col = hosek_wilkie_sky_rgb(dir, u_Direction);

		PS_OUT_Color = vec4(col, 1.0);
	}
}
