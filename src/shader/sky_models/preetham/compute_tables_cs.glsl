#define NUM_THREADS 8

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image2D i_Table;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform int TABLE_SIZE;
uniform float u_Perez_x[5];
uniform float u_Perez_y[5];
uniform float u_Perez_Y[5];

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float unmap_gamma(float g)
{
    return 1.0 - 2.0 * pow(g, 2.0);
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 theta_value = vec3(0.0);
    vec3 gamma_value = vec3(0.0);

    float dt = 1.0 / (TABLE_SIZE - 1);
    float t = dt * 1e-6 + dt * float(gl_GlobalInvocationID.x); // epsilon to avoid divide by 0, which can lead to NaN when m_perez_[1] = 0
    float cosTheta = t;
    int idx = int(gl_GlobalInvocationID.x);

    theta_value[0] =  -u_Perez_x[0] * exp(u_Perez_x[1] / cosTheta);
    theta_value[1] =  -u_Perez_y[0] * exp(u_Perez_y[1] / cosTheta);
    theta_value[2] =  -u_Perez_Y[0] * exp(u_Perez_Y[1] / cosTheta);

    imageStore(i_Table, ivec2(idx, 0), vec4(theta_value, 0));

    float cosGamma = unmap_gamma(t);
    float gamma = acos(cosGamma);

    gamma_value[0] =  u_Perez_x[2] * exp(u_Perez_x[3] * gamma) + u_Perez_x[4] * pow(cosGamma, 2.0);
    gamma_value[1] =  u_Perez_y[2] * exp(u_Perez_y[3] * gamma) + u_Perez_y[4] * pow(cosGamma, 2.0);
    gamma_value[2] =  u_Perez_Y[2] * exp(u_Perez_Y[3] * gamma) + u_Perez_Y[4] * pow(cosGamma, 2.0);

    imageStore(i_Table, ivec2(idx, 1), vec4(gamma_value, 0));
}

// ------------------------------------------------------------------