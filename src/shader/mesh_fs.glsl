
out vec4 PS_OUT_Color;

in vec3 PS_IN_FragPos;
in vec3 PS_IN_Normal;

uniform vec3 direction;

void main()
{
	vec3 diffuse = vec3(0.5);
	vec3 color = dot(PS_IN_Normal, -direction) * diffuse;

	PS_OUT_Color = vec4(color, 1.0);
}
