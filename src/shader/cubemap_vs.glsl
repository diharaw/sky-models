// Using a fullscreen triangle for post-processing.
// https://www.saschawillems.de/?page_id=2122
// https://michaldrobot.com/2014/04/01/gcn-execution-patterns-in-full-screen-passes/

layout (std140) uniform u_GlobalUBO
{ 
    mat4 view;
    mat4 projection;
    mat4 inv_view;
    mat4 inv_projection;
    mat4 inv_view_projection;
    vec4 view_pos;
};


out vec3 PS_IN_TexCoord;

// void main()
// {
//     vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
// 	gl_Position = vec4(pos * 2.0f + -1.0f, 1.0f, 1.0f);

//     vec4 clip_pos = vec4(gl_Position.xy, -1.0, 1.0);
//     vec4 view_pos  = inv_projection * clip_pos;
   
//     vec3 dir = vec3(inv_view * vec4(view_pos.x, view_pos.y, -1.0, 0.0));
//     dir = normalize(dir);

//     PS_IN_TexCoord = dir;
// }

void main(void)
{
     const vec3 vertices[4] = vec3[4](vec3(-1.0f, -1.0f, 1.0f),
                                      vec3( 1.0f, -1.0f, 1.0f),
                                      vec3(-1.0f,  1.0f, 1.0f),
                                      vec3( 1.0f,  1.0f, 1.0f));


    vec4 clip_pos = vec4(vertices[gl_VertexID].xy, -1.0, 1.0);
    vec4 view_pos  = inv_view_projection * clip_pos;
   
    vec3 dir = vec3(view_pos);//vec3(inv_view * vec4(view_pos.x, view_pos.y, -1.0, 0.0));
    dir = normalize(dir);

    PS_IN_TexCoord = dir;

    gl_Position = vec4(vertices[gl_VertexID], 1.0f);
}