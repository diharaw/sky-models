#include "hosek_wilkie_sky_model.h"

#include <macros.h>
#include <logger.h>
#include <utility.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "hosek_data_rgb.inl"

// -----------------------------------------------------------------------------------------------------------------------------------


double evaluate_spline(const double* spline, size_t stride, double value)
{
    return
    1 *  pow(1 - value, 5) *                 spline[0 * stride] +
    5 *  pow(1 - value, 4) * pow(value, 1) * spline[1 * stride] +
    10 * pow(1 - value, 3) * pow(value, 2) * spline[2 * stride] +
    10 * pow(1 - value, 2) * pow(value, 3) * spline[3 * stride] +
    5 *  pow(1 - value, 1) * pow(value, 4) * spline[4 * stride] +
    1 *                      pow(value, 5) * spline[5 * stride];
}

// -----------------------------------------------------------------------------------------------------------------------------------

double evaluate(const double * dataset, size_t stride, float turbidity, float albedo, float sunTheta)
{
    // splines are functions of elevation^1/3
    double elevationK = pow(std::max<float>(0.f, 1.f - sunTheta / (M_PI / 2.f)), 1.f / 3.0f);
    
    // table has values for turbidity 1..10
    int turbidity0 = glm::clamp(static_cast<int>(turbidity), 1, 10);
    int turbidity1 = std::min(turbidity0 + 1, 10);
    float turbidityK = glm::clamp(turbidity - turbidity0, 0.f, 1.f);
    
    const double * datasetA0 = dataset;
    const double * datasetA1 = dataset + stride * 6 * 10;
    
    double a0t0 = evaluate_spline(datasetA0 + stride * 6 * (turbidity0 - 1), stride, elevationK);
    double a1t0 = evaluate_spline(datasetA1 + stride * 6 * (turbidity0 - 1), stride, elevationK);
    double a0t1 = evaluate_spline(datasetA0 + stride * 6 * (turbidity1 - 1), stride, elevationK);
    double a1t1 = evaluate_spline(datasetA1 + stride * 6 * (turbidity1 - 1), stride, elevationK);
    
    return a0t0 * (1 - albedo) * (1 - turbidityK) + a1t0 * albedo * (1 - turbidityK) + a0t1 * (1 - albedo) * turbidityK + a1t1 * albedo * turbidityK;
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 hosek_wilkie(float cos_theta, float gamma, float cos_gamma, glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D, glm::vec3 E, glm::vec3 F, glm::vec3 G, glm::vec3 H, glm::vec3 I)
{
    glm::vec3 chi = (1.f + cos_gamma * cos_gamma) / pow(1.f + H * H - 2.f * cos_gamma * H, glm::vec3(1.5f));
    return (1.f + A * exp(B / (cos_theta + 0.01f))) * (C + D * exp(E * gamma) + F * (cos_gamma * cos_gamma) + G * chi + I * (float) sqrt(std::max(0.f, cos_theta)));
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
    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void HosekWilkieSkyModel::update()
{
	const float sunTheta = std::acos(glm::clamp(m_direction.y, 0.f, 1.f));

    for (int i = 0; i < 3; ++i)
    {
        A[i] = evaluate(datasetsRGB[i] + 0, 9, m_turbidity, m_albedo, sunTheta);
        B[i] = evaluate(datasetsRGB[i] + 1, 9, m_turbidity, m_albedo, sunTheta);
        C[i] = evaluate(datasetsRGB[i] + 2, 9, m_turbidity, m_albedo, sunTheta);
        D[i] = evaluate(datasetsRGB[i] + 3, 9, m_turbidity, m_albedo, sunTheta);
        E[i] = evaluate(datasetsRGB[i] + 4, 9, m_turbidity, m_albedo, sunTheta);
        F[i] = evaluate(datasetsRGB[i] + 5, 9, m_turbidity, m_albedo, sunTheta);
        G[i] = evaluate(datasetsRGB[i] + 6, 9, m_turbidity, m_albedo, sunTheta);
        
        // Swapped in the dataset
        H[i] = evaluate(datasetsRGB[i] + 8, 9, m_turbidity, m_albedo, sunTheta);
        I[i] = evaluate(datasetsRGB[i] + 7, 9, m_turbidity, m_albedo, sunTheta);
        
        Z[i] = evaluate(datasetsRGBRad[i], 1, m_turbidity, m_albedo, sunTheta);
    }
    
    if (m_normalized_sun_y)
    {
        glm::vec3 S = hosek_wilkie(std::cos(sunTheta), 0, 1.f, A, B, C, D, E, F, G, H, I) * Z;
        Z /= glm::dot(S, glm::vec3(0.2126, 0.7152, 0.0722));
        Z *= m_normalized_sun_y;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void HosekWilkieSkyModel::set_render_uniforms(dw::Program* program)
{
	program->set_uniform("u_Direction", m_direction);
	program->set_uniform("A", A);
	program->set_uniform("B", B);
	program->set_uniform("C", C);
	program->set_uniform("D", D);
	program->set_uniform("E", E);
	program->set_uniform("F", F);
	program->set_uniform("G", G);
	program->set_uniform("H", H);
	program->set_uniform("I", I);
	program->set_uniform("Z", Z);
}

// -----------------------------------------------------------------------------------------------------------------------------------