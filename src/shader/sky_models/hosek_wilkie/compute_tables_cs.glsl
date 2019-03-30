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
uniform vec3 u_CoeffsXYZ[9];

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
    float t = dt * float(gl_GlobalInvocationID.x); // epsilon to avoid divide by 0, which can lead to NaN when m_perez_[1] = 0
    float cosTheta = t;
    int idx = int(gl_GlobalInvocationID.x);

    float cosGamma = unmap_gamma(t);
    float gamma = acos(cosGamma);

    float rayM = cosGamma * cosGamma;

    for (int j = 0; j < 3; j++)
    {
        theta_value[j] = -u_CoeffsXYZ[0][j] * exp(u_CoeffsXYZ[1][j] / (cosTheta + 0.01));

        float expM   = exp(u_CoeffsXYZ[4][j] * gamma);
        float mieM   = (1.0 + rayM) / pow((1.0 + u_CoeffsXYZ[8][j] * u_CoeffsXYZ[8][j] - 2.0 * u_CoeffsXYZ[8][j] * cosGamma), 1.5);

        gamma_value[j] = u_CoeffsXYZ[3][j] * expM + u_CoeffsXYZ[5][j] * rayM + u_CoeffsXYZ[6][j] * mieM;
    }

    imageStore(i_Table, ivec2(idx, 0), vec4(theta_value, 0));
    imageStore(i_Table, ivec2(idx, 1), vec4(gamma_value, 0));
}

// ------------------------------------------------------------------