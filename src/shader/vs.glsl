layout (location = 0) in vec3 VS_IN_Position;
layout (location = 1) in vec2 VS_IN_Texcoord;
layout (location = 2) in vec3 VS_IN_Normal;
layout (location = 3) in vec3 VS_IN_Tangent;
layout (location = 4) in vec3 VS_IN_Bitangent;

const int MAX_BONES = 128;

layout (std140) uniform u_GlobalUBO
{ 
    mat4 view;
    mat4 projection;
    mat4 inv_view;
    mat4 inv_projection;
    mat4 inv_view_projection;
    vec4 view_pos;
};

layout (std140) uniform u_ObjectUBO
{ 
    mat4 model;
};


out vec3 PS_IN_FragPos;
out vec3 PS_IN_Normal;

void main()
{
    vec4 world_pos = model * vec4(VS_IN_Position, 1.0f);
    PS_IN_FragPos = world_pos.xyz;

	mat3 model_mat = mat3(model);

	PS_IN_Normal = normalize(model_mat * VS_IN_Normal);

	gl_Position = projection * view * world_pos;
}
