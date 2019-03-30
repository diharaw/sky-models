#include "hosek_wilkie_sky_model.h"
#include "hosek_data_XYZ.h"

#include <macros.h>
#include <logger.h>
#include <utility.h>

#define _USE_MATH_DEFINES
#include <math.h>

static const float kHalfPI = M_PI / 2.0f;

// -----------------------------------------------------------------------------------------------------------------------------------

float eval_quintic(const float w[6], const float data[6])
{
    return    w[0] * data[0]
            + w[1] * data[1]
            + w[2] * data[2]
            + w[3] * data[3]
            + w[4] * data[4]
            + w[5] * data[5];
}

// -----------------------------------------------------------------------------------------------------------------------------------

void eval_quintic(const float w[6], const float data[6][9], float coeffs[9])
{
    for (int i = 0; i < 9; i++)
    {
        coeffs[i] =   w[0] * data[0][i]
                    + w[1] * data[1][i]
                    + w[2] * data[2][i]
                    + w[3] * data[3][i]
                    + w[4] * data[4][i]
                    + w[5] * data[5][i];
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void find_quintic_weights(float s, float w[6])
{
    float s1 = s;
    float s2 = s1 * s1;
    float s3 = s1 * s2;
    float s4 = s2 * s2;
    float s5 = s2 * s3;

    float is1 = 1.0f - s1;
    float is2 = is1 * is1;
    float is3 = is1 * is2;
    float is4 = is2 * is2;
    float is5 = is2 * is3;

    w[0] = is5;
    w[1] = is4 * s1 *  5.0f;
    w[2] = is3 * s2 * 10.0f;
    w[3] = is2 * s3 * 10.0f;
    w[4] = is1 * s4 *  5.0f;
    w[5] =       s5;
}

// -----------------------------------------------------------------------------------------------------------------------------------

float find_hosek_coeffs(const float   dataset9[2][10][6][9],    // albedo x 2, turbidity x 10, quintics x 6, weights x 9
					    const float   datasetR[2][10][6],       // albedo x 2, turbidity x 10, quintics x 6
					    float         turbidity,
					    float         albedo,
					    float         solarElevation,
					    float         coeffs[9])
{
    int tbi = int(floorf(turbidity));

    if (tbi < 1)
        tbi = 1;
    else if (tbi > 9)
        tbi = 9;

    float tbf = turbidity - tbi;

    const float s = powf(solarElevation / kHalfPI, (1.0f / 3.0f));

    float quinticWeights[6];
    find_quintic_weights(s, quinticWeights);

    float ic[4][9];

    eval_quintic(quinticWeights, dataset9[0][tbi - 1], ic[0]);
    eval_quintic(quinticWeights, dataset9[1][tbi - 1], ic[1]);
    eval_quintic(quinticWeights, dataset9[0][tbi    ], ic[2]);
    eval_quintic(quinticWeights, dataset9[1][tbi    ], ic[3]);

    float ir[4] =
    {
        eval_quintic(quinticWeights, datasetR[0][tbi - 1]),
        eval_quintic(quinticWeights, datasetR[1][tbi - 1]),
        eval_quintic(quinticWeights, datasetR[0][tbi    ]),
        eval_quintic(quinticWeights, datasetR[1][tbi    ]),
    };

    float cw[4] =
    {
        (1.0f - albedo) * (1.0f - tbf),
        albedo          * (1.0f - tbf),
        (1.0f - albedo) * tbf,
        albedo          * tbf,
    };

	for (int i = 0; i < 9; i++)
	{
		coeffs[i] = cw[0] * ic[0][i]
                  + cw[1] * ic[1][i]
                  + cw[2] * ic[2][i]
                  + cw[3] * ic[3][i];
	}

    return cw[0] * ir[0] + cw[1] * ir[1] + cw[2] * ir[2] + cw[3] * ir[3];
}

// -----------------------------------------------------------------------------------------------------------------------------------

float eval_hosek_coeffs(const float coeffs[9], float cosTheta, float gamma, float cosGamma)
{
    // Current coeffs ordering is AB I CDEF HG
    //                            01 2 3456 78
    const float expM   = expf(coeffs[4] * gamma);   // D g
    const float rayM   = cosGamma * cosGamma;       // Rayleigh scattering
    const float mieM   = (1.0f + rayM) / powf((1.0f + coeffs[8] * coeffs[8] - 2.0f * coeffs[8] * cosGamma), 1.5f);  // G
    const float zenith = sqrtf(cosTheta);           // vertical zenith gradient

    return (1.0f
                 + coeffs[0] * expf(coeffs[1] / (cosTheta + 0.01f))     // A, B
           )
         * (1.0f
                 + coeffs[3] * expM     // C
                 + coeffs[5] * rayM     // E
                 + coeffs[6] * mieM     // F
                 + coeffs[7] * zenith   // H
                 + (coeffs[2] - 1.0f)   // I
           );
}

// -----------------------------------------------------------------------------------------------------------------------------------

HosekWilkieSkyModel::HosekWilkieSkyModel()
{

}

// -----------------------------------------------------------------------------------------------------------------------------------

HosekWilkieSkyModel::~HosekWilkieSkyModel()
{

}

// -----------------------------------------------------------------------------------------------------------------------------------

bool HosekWilkieSkyModel::initialize()
{
	m_table = new dw::Texture2D(TABLE_SIZE, 2, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	m_table->set_min_filter(GL_LINEAR);
	m_table->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    if (!dw::utility::create_compute_program("shader/sky_models/hosek_wilkie/compute_tables_cs.glsl", &m_compute_tables_cs, &m_compute_tables_program))
	{
		DW_LOG_ERROR("Failed to load shaders");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void HosekWilkieSkyModel::update()
{
	float solarElevation = m_direction.y > 0.0f ? asinf(m_direction.y) : 0.0f;    // altitude rather than zenith, so sin rather than cos

    glm::vec3 albedo = rgb_to_XYZ(m_rgb_albedo);

    // Note that the hosek coefficients change with time of day, vs. Preetham where the 'upper' coefficients stay the same,
    // and only the scaler mPerezInvDen, consisting of time-dependent normalisation and zenith luminnce factors, changes.
    m_rad_XYZ[0] = find_hosek_coeffs(kHosekCoeffsX, kHosekRadX, m_turbidity, albedo.x, solarElevation, m_coeffs_XYZ[0]);
    m_rad_XYZ[1] = find_hosek_coeffs(kHosekCoeffsY, kHosekRadY, m_turbidity, albedo.y, solarElevation, m_coeffs_XYZ[1]);
    m_rad_XYZ[2] = find_hosek_coeffs(kHosekCoeffsZ, kHosekRadZ, m_turbidity, albedo.z, solarElevation, m_coeffs_XYZ[2]);

    m_rad_XYZ *= 683; // convert to luminance in lumens

    if (m_direction.y < 0.0f)   // sun below horizon?
    {
        float s = glm::clamp(1.0f + m_direction.y * 50.0f, 0.0f, 1.0f);   // goes from 1 to 0 as the sun sets
		float is = 1.0f - s;

        // Emulate Preetham's zenith darkening
        float darken = zenith_luminance(acosf(m_direction.y), m_turbidity) / zenith_luminance(kHalfPI, m_turbidity);

        // Take C/E/F which control sun term to zero
        for (int j = 0; j < 3; j++)
        {
            m_coeffs_XYZ[j][3] *= s;
            m_coeffs_XYZ[j][5] *= s;
            m_coeffs_XYZ[j][6] *= s;

            // Take horizon term H to zero, as it's an orange glow at this point
            m_coeffs_XYZ[j][7] *= s;

            // Take I term back to 1
            m_coeffs_XYZ[j][2] *= s;
            m_coeffs_XYZ[j][2] += is;
        }

        m_rad_XYZ *= darken;
    }

    if (m_overcast != 0.0f)      // Handle overcast term
    {
        float is = m_overcast;
        float s = 1.0f - m_overcast;     // goes to 0 as we go to overcast

        // Hosek isn't self-normalising, unlike Preetham/CIE, which divides by PreethamLower().
        // Thus when we lerp to the CIE overcast model, we get some non-linearities.
        // We deal with this by using ratios of normalisation terms to balance.
        // Another difference is that Hosek is relative to the average radiance,
        // whereas CIE is the zenith radiance, so rather than taking the zenith
        // as normalising as in CIE, we average over the zenith and two horizon
        // points.
        float cosGammaZ = m_direction.y;
        float gammaZ    = acosf(cosGammaZ);
        float cosGammaH = m_direction.z;
        float gammaHP   = acosf(+m_direction.z);
        float gammaHN   = M_PI - gammaHP;

        float sc0 = eval_hosek_coeffs(m_coeffs_XYZ[1], 1.0f, gammaZ, cosGammaZ) * 2.0f
                  + eval_hosek_coeffs(m_coeffs_XYZ[1], 0.0f, gammaHP, +cosGammaH)
                  + eval_hosek_coeffs(m_coeffs_XYZ[1], 0.0f, gammaHN, -cosGammaH);

        for (int j = 0; j < 3; j++)
        {
            // sun flare -> 0 strength/base chroma
            // Take C/E/F which control sun term to zero
            m_coeffs_XYZ[j][3] *= s;
            m_coeffs_XYZ[j][5] *= s;
            m_coeffs_XYZ[j][6] *= s;

            // Take H back to 0
            m_coeffs_XYZ[j][7] *= s;

            // Take I term back to 1
            m_coeffs_XYZ[j][2] *= s;
            m_coeffs_XYZ[j][2] += is;

            // Take A/B to  CIE cloudy sky model: 4, -0.7
            m_coeffs_XYZ[j][0] = glm::mix(m_coeffs_XYZ[j][0],  4.0f, is);
            m_coeffs_XYZ[j][1] = glm::mix(m_coeffs_XYZ[j][1], -0.7f, is);
        }

        float sc1 = eval_hosek_coeffs(m_coeffs_XYZ[1], 1.0f, gammaZ, cosGammaZ) * 2.0f
                  + eval_hosek_coeffs(m_coeffs_XYZ[1], 0.0f, gammaHP, +cosGammaH)
                  + eval_hosek_coeffs(m_coeffs_XYZ[1], 0.0f, gammaHN, -cosGammaH);

        float rescale = sc0 / sc1;
        m_rad_XYZ *= rescale;

        // move back to white point
        m_rad_XYZ.x = glm::mix(m_rad_XYZ.x, m_rad_XYZ.y, is);
        m_rad_XYZ.z = glm::mix(m_rad_XYZ.z, m_rad_XYZ.y, is);
    }

	// -----------------------------------------------------------------------------
    // Compute Tables with through Compute Shader
    // -----------------------------------------------------------------------------

    m_compute_tables_program->use();
    
	m_compute_tables_program->set_uniform("TABLE_SIZE", TABLE_SIZE);

	for (int i = 0; i < 9; i++)
	{
		m_coeffs_XYZ_GPU[i][0] = m_coeffs_XYZ[0][i];
		m_coeffs_XYZ_GPU[i][1] = m_coeffs_XYZ[1][i];
		m_coeffs_XYZ_GPU[i][2] = m_coeffs_XYZ[2][i];
	}

    m_compute_tables_program->set_uniform("u_CoeffsXYZ[0]", 9, m_coeffs_XYZ_GPU);

    m_table->bind_image(0, 0, 0, GL_READ_WRITE, m_table->internal_format());

	GL_CHECK_ERROR(glDispatchCompute(TABLE_SIZE / NUM_THREADS, 1, 1));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void HosekWilkieSkyModel::set_render_uniforms(dw::Program* program)
{
	if (program->set_uniform("s_Table", 3))
		m_table->bind(3);

	program->set_uniform("TABLE_SIZE", TABLE_SIZE);
	program->set_uniform("u_Direction", m_direction);
	program->set_uniform("u_CoeffsXYZ[0]", 9, m_coeffs_XYZ_GPU);
	program->set_uniform("u_RadXYZ", m_rad_XYZ);
}

// -----------------------------------------------------------------------------------------------------------------------------------