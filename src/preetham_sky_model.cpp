#include "preetham_sky_model.h"

#include <macros.h>
#include <logger.h>
#include <utility.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/compatibility.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

// -----------------------------------------------------------------------------------------------------------------------------------

//
// The Perez function is as follows:
//
//    P(t, g) =  (1 + A e ^ (B / cos(t))) (1 + C e ^ (D g)  + E cos(g)  ^ 2)
//               --------------------------------------------------------
//               (1 + A e ^ B)            (1 + C e ^ (D ts) + E cos(ts) ^ 2)
//
// A: sky
// B: sky tightness
// C: sun
// D: sun tightness, higher = tighter
// E: rosy hue around sun

inline float perez_upper(const float* lambdas, float cosTheta, float gamma, float cosGamma)
{
    return  (1.0f + lambdas[0] * expf(lambdas[1] / (cosTheta + 1e-6f)))
          * (1.0f + lambdas[2] * expf(lambdas[3] * gamma) + lambdas[4] * pow(cosGamma, 2.0f));
}

// -----------------------------------------------------------------------------------------------------------------------------------

inline float perez_lower(const float* lambdas, float cosThetaS, float thetaS)
{
    return  (1.0f + lambdas[0] * expf(lambdas[1]))
          * (1.0f + lambdas[2] * expf(lambdas[3] * thetaS) + lambdas[4] * pow(cosThetaS, 2.0f));
}

// -----------------------------------------------------------------------------------------------------------------------------------

PreethamSkyModel::PreethamSkyModel()
{

}

// -----------------------------------------------------------------------------------------------------------------------------------

PreethamSkyModel::~PreethamSkyModel()
{
    DW_SAFE_DELETE(m_table);
    DW_SAFE_DELETE(m_compute_tables_cs);
	DW_SAFE_DELETE(m_compute_tables_program);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool PreethamSkyModel::initialize()
{
#ifdef SIM_CLAMP
	int size = TABLE_SIZE + NUM_THREADS;
#else
	int size = TABLE_SIZE;
#endif

	m_table = new dw::Texture2D(size, 2, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	m_table->set_min_filter(GL_LINEAR);
	m_table->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    if (!dw::utility::create_compute_program("shader/sky_models/preetham/compute_tables_cs.glsl", &m_compute_tables_cs, &m_compute_tables_program))
	{
		DW_LOG_ERROR("Failed to load shaders");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void PreethamSkyModel::update()
{
    float T = m_turbidity;

    m_perez_Y[0] =  0.17872f * T - 1.46303f;
    m_perez_Y[1] = -0.35540f * T + 0.42749f;
    m_perez_Y[2] = -0.02266f * T + 5.32505f;
    m_perez_Y[3] =  0.12064f * T - 2.57705f;
    m_perez_Y[4] = -0.06696f * T + 0.37027f;

    m_perez_x[0] = -0.01925f * T - 0.25922f;
    m_perez_x[1] = -0.06651f * T + 0.00081f;
    m_perez_x[2] = -0.00041f * T + 0.21247f;
    m_perez_x[3] = -0.06409f * T - 0.89887f;
    m_perez_x[4] = -0.00325f * T + 0.04517f;

    m_perez_y[0] = -0.01669f * T - 0.26078f;
    m_perez_y[1] = -0.09495f * T + 0.00921f;
    m_perez_y[2] = -0.00792f * T + 0.21023f;
    m_perez_y[3] = -0.04405f * T - 1.65369f;
    m_perez_y[4] = -0.01092f * T + 0.05291f;

    float cosTheta = m_direction.y;
    float theta  = acosf(cosTheta);    // angle from zenith rather than horizon
    float theta2 = pow(theta, 2.0f);
	float theta3 = theta2 * theta;
	float T2 = pow(T, 2.0f);

    // m_zenith stored as xyY
    m_zenith.z = zenith_luminance(theta, T);

    m_zenith.x =
        ( 0.00165f * theta3 - 0.00374f * theta2 + 0.00208f * theta + 0.00000f) * T2 +
        (-0.02902f * theta3 + 0.06377f * theta2 - 0.03202f * theta + 0.00394f) * T  +
        ( 0.11693f * theta3 - 0.21196f * theta2 + 0.06052f * theta + 0.25885f);

    m_zenith.y =
        ( 0.00275f * theta3 - 0.00610f * theta2 + 0.00316f * theta + 0.00000f) * T2 +
        (-0.04214f * theta3 + 0.08970f * theta2 - 0.04153f * theta + 0.00515f) * T  +
        ( 0.15346f * theta3 - 0.26756f * theta2 + 0.06669f * theta + 0.26688f);

    // Adjustments (extensions)

    if (cosTheta < 0.0f)    // Handle sun going below the horizon
    {
        float s = glm::clamp(1.0f + cosTheta * 50.0f, 0.0f, 1.0f);   // goes from 1 to 0 as the sun sets

        // Take C/E which control sun term to zero
        m_perez_x[2] *= s;
        m_perez_y[2] *= s;
        m_perez_Y[2] *= s;
        m_perez_x[4] *= s;
        m_perez_y[4] *= s;
        m_perez_Y[4] *= s;
    }

    if (m_overcast != 0.0f)      // Handle m_overcast term
    {
        float invOvercast = 1.0f - m_overcast;

        // lerp back towards unity
        m_perez_x[0] *= invOvercast;  // main sky chroma -> base
        m_perez_y[0] *= invOvercast;

        // sun flare -> 0 strength/base chroma
        m_perez_x[2] *= invOvercast;
        m_perez_y[2] *= invOvercast;
        m_perez_Y[2] *= invOvercast;
        m_perez_x[4] *= invOvercast;
        m_perez_y[4] *= invOvercast;
        m_perez_Y[4] *= invOvercast;

        // lerp towards a fit of the CIE cloudy sky model: 4, -0.7
        m_perez_Y[0] = glm::lerp(m_perez_Y[0],  4.0f, m_overcast);
        m_perez_Y[1] = glm::lerp(m_perez_Y[1], -0.7f, m_overcast);

        // lerp base colour towards white point
        m_zenith.x = m_zenith.x * invOvercast + 0.333f * m_overcast;
        m_zenith.y = m_zenith.y * invOvercast + 0.333f * m_overcast;
    }

    if (m_horiz_crush != 0.0f)
    {
        // The Preetham sky model has a "muddy" horizon, which can be objectionable in
        // typical game views. We allow artistic control over it.
        m_perez_Y[1] *= m_horiz_crush;
        m_perez_x[1] *= m_horiz_crush;
        m_perez_y[1] *= m_horiz_crush;
    }

    // initialize sun-constant parts of the Perez functions

    glm::vec3 perezLower    // denominator terms are dependent on sun angle only
    (
        perez_lower(m_perez_x, cosTheta, theta),
        perez_lower(m_perez_y, cosTheta, theta),
        perez_lower(m_perez_Y, cosTheta, theta)
    );

	m_perez_inv_den = m_zenith / perezLower;

    // -----------------------------------------------------------------------------
    // Compute Tables with through Compute Shader
    // -----------------------------------------------------------------------------

    m_compute_tables_program->use();
    
	m_compute_tables_program->set_uniform("TABLE_SIZE", TABLE_SIZE);
    m_compute_tables_program->set_uniform("u_Perez_x[0]", 5, m_perez_x);
	m_compute_tables_program->set_uniform("u_Perez_y[0]", 5, m_perez_y);
	m_compute_tables_program->set_uniform("u_Perez_Y[0]", 5, m_perez_Y);

    m_table->bind_image(0, 0, 0, GL_READ_WRITE, m_table->internal_format());

	GL_CHECK_ERROR(glDispatchCompute(TABLE_SIZE / NUM_THREADS, 1, 1));

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void PreethamSkyModel::set_render_uniforms(dw::Program* program)
{
	if (program->set_uniform("s_Table", 3))
		m_table->bind(3);

	program->set_uniform("TABLE_SIZE", TABLE_SIZE);
	program->set_uniform("u_Direction", m_direction);
	program->set_uniform("u_PerezInvDen", m_perez_inv_den);
}

// -----------------------------------------------------------------------------------------------------------------------------------