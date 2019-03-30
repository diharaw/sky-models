
out vec4 PS_OUT_Color;

in vec2 PS_IN_TexCoord;

uniform sampler2D s_Texture;
uniform float exposure;

vec3 reinhard(vec3 L)
{
	L = L * exposure;
	return L / (1 + L);
}

void main()
{
	vec3 color = reinhard(texture(s_Texture, PS_IN_TexCoord).rgb);
	PS_OUT_Color = vec4(pow(color, vec3(1.0/2.2)), 1.0);
}
