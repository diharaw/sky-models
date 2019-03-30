#include "bruneton_sky_model.h"
#include <macros.h>
#include <utility.h>
#include <logger.h>
#include <stdio.h>

// -----------------------------------------------------------------------------------------------------------------------------------

BrunetonSkyModel::BrunetonSkyModel()
{
	m_transmittance_t = nullptr;
    m_irradiance_t[0] = nullptr;
    m_irradiance_t[1] = nullptr;
    m_inscatter_t[0] = nullptr;
    m_inscatter_t[1] = nullptr;
    m_delta_et = nullptr;
    m_delta_srt = nullptr;
    m_delta_smt = nullptr;
    m_delta_jt = nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------

BrunetonSkyModel::~BrunetonSkyModel()
{
	DW_SAFE_DELETE(m_copy_inscatter_1_program);
	DW_SAFE_DELETE(m_copy_inscatter_n_program);
	DW_SAFE_DELETE(m_copy_irradiance_program);
	DW_SAFE_DELETE(m_inscatter_1_program);
	DW_SAFE_DELETE(m_inscatter_n_program);
	DW_SAFE_DELETE(m_inscatter_s_program);
	DW_SAFE_DELETE(m_irradiance_1_program);
	DW_SAFE_DELETE(m_irradiance_n_program);
	DW_SAFE_DELETE(m_transmittance_program);

	DW_SAFE_DELETE(m_copy_inscatter_1_cs);
	DW_SAFE_DELETE(m_copy_inscatter_n_cs);
	DW_SAFE_DELETE(m_copy_irradiance_cs);
	DW_SAFE_DELETE(m_inscatter_1_cs);
	DW_SAFE_DELETE(m_inscatter_n_cs);
	DW_SAFE_DELETE(m_inscatter_s_cs);
	DW_SAFE_DELETE(m_irradiance_1_cs);
	DW_SAFE_DELETE(m_irradiance_n_cs);
	DW_SAFE_DELETE(m_transmittance_cs);

	DW_SAFE_DELETE(m_transmittance_t);
    DW_SAFE_DELETE(m_delta_et);
    DW_SAFE_DELETE(m_delta_srt);
    DW_SAFE_DELETE(m_delta_smt);
    DW_SAFE_DELETE(m_delta_jt);
    DW_SAFE_DELETE(m_irradiance_t[0]);
    DW_SAFE_DELETE(m_irradiance_t[0]);
    DW_SAFE_DELETE(m_inscatter_t[0]);
    DW_SAFE_DELETE(m_inscatter_t[1]);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool BrunetonSkyModel::initialize()
{
	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/copy_inscatter_1_cs.glsl", &m_copy_inscatter_1_cs, &m_copy_inscatter_1_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/copy_inscatter_n_cs.glsl", &m_copy_inscatter_n_cs, &m_copy_inscatter_n_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/copy_irradiance_cs.glsl", &m_copy_irradiance_cs, &m_copy_irradiance_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/inscatter_1_cs.glsl", &m_inscatter_1_cs, &m_inscatter_1_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/inscatter_n_cs.glsl", &m_inscatter_n_cs, &m_inscatter_n_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/inscatter_s_cs.glsl", &m_inscatter_s_cs, &m_inscatter_s_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/irradiance_1_cs.glsl", &m_irradiance_1_cs, &m_irradiance_1_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/irradiance_n_cs.glsl", &m_irradiance_n_cs, &m_irradiance_n_program))
		DW_LOG_ERROR("Failed to load shaders");

	if (!dw::utility::create_compute_program("shader/sky_models/bruneton/transmittance_cs.glsl", &m_transmittance_cs, &m_transmittance_program))
		DW_LOG_ERROR("Failed to load shaders");

	m_transmittance_t = new_texture_2d(TRANSMITTANCE_W, TRANSMITTANCE_H);

    m_irradiance_t[0] = new_texture_2d(IRRADIANCE_W, IRRADIANCE_H);
    m_irradiance_t[1] = new_texture_2d(IRRADIANCE_W, IRRADIANCE_H);

    m_inscatter_t[0] = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);
    m_inscatter_t[1] = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);

    m_delta_et = new_texture_2d(IRRADIANCE_W, IRRADIANCE_H);
    m_delta_srt = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);
    m_delta_smt = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);
    m_delta_jt = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);

    if (!load_cached_textures())
        precompute();

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::update()
{

}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::set_render_uniforms(dw::Program* program)
{
	program->set_uniform("betaR", m_beta_r / SCALE);
    program->set_uniform("mieG", m_mie_g);
	program->set_uniform("SUN_INTENSITY", m_sun_intensity);
	program->set_uniform("EARTH_POS", glm::vec3(0.0f, 6360010.0f, 0.0f));
	program->set_uniform("SUN_DIR", m_direction * 1.0f);

	if (program->set_uniform("s_Transmittance", 0))
		m_transmittance_t->bind(0);

	if (program->set_uniform("s_Irradiance", 1))
		m_irradiance_t[READ]->bind(1);

	if (program->set_uniform("s_Inscatter", 2))
		m_inscatter_t[READ]->bind(2);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::set_uniforms(dw::Program* program)
{
	program->set_uniform("Rg", Rg);
	program->set_uniform("Rt", Rt);
	program->set_uniform("RL", RL);
	program->set_uniform("TRANSMITTANCE_W", TRANSMITTANCE_W);
	program->set_uniform("TRANSMITTANCE_H", TRANSMITTANCE_H);
	program->set_uniform("SKY_W", IRRADIANCE_W);
	program->set_uniform("SKY_H", IRRADIANCE_H);
	program->set_uniform("RES_R", INSCATTER_R);
	program->set_uniform("RES_MU", INSCATTER_MU);
	program->set_uniform("RES_MU_S", INSCATTER_MU_S);
	program->set_uniform("RES_NU", INSCATTER_NU);
	program->set_uniform("AVERAGE_GROUND_REFLECTANCE", AVERAGE_GROUND_REFLECTANCE);
	program->set_uniform("HR", HR);
	program->set_uniform("HM", HM);
	program->set_uniform("betaR", BETA_R);
	program->set_uniform("betaMSca", BETA_MSca);
	program->set_uniform("betaMEx", BETA_MEx);
	program->set_uniform("mieG", glm::clamp(MIE_G, 0.0f, 0.99f));
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool BrunetonSkyModel::load_cached_textures()
{
	FILE* transmittance = fopen("transmittance.raw", "r");

	if (transmittance)
	{
		size_t n = sizeof(float) * TRANSMITTANCE_W * TRANSMITTANCE_H * 4;

		void* data = malloc(n);
		fread(data, n, 1, transmittance);

		m_transmittance_t->set_data(0, 0, data);

		fclose(transmittance);
		free(data);
	}
	else
		return false;

    FILE* irradiance = fopen("irradiance.raw", "r");

    if (irradiance)
    {
        size_t n = sizeof(float) * IRRADIANCE_W * IRRADIANCE_H * 4;

        void* data = malloc(n);
		fread(data, n, 1, irradiance);

        m_irradiance_t[READ]->set_data(0, 0, data);

        fclose(irradiance);
        free(data);
    }
	else
		return false;

    FILE* inscatter = fopen("inscatter.raw", "r");

    if (inscatter)
    {
        size_t n = sizeof(float) * INSCATTER_MU_S * INSCATTER_NU * INSCATTER_MU * INSCATTER_R * 4;

        void* data = malloc(n);
		fread(data, n, 1, inscatter);

        m_inscatter_t[READ]->set_data(0, data);

        fclose(inscatter);
        free(data);
    }
	else
		return false;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::write_textures()
{
	{
		FILE* transmittance = fopen("transmittance.raw", "wb");
	
		size_t n = sizeof(float) * TRANSMITTANCE_W * TRANSMITTANCE_H * 4;
		void* data = malloc(n);

		m_transmittance_t->data(0, 0, data);

		fwrite(data, n, 1, transmittance);

		fclose(transmittance);
		free(data);
	}

	{
		FILE* irradiance = fopen("irradiance.raw", "wb");
	
		size_t n = sizeof(float) * IRRADIANCE_W * IRRADIANCE_H * 4;
		void* data = malloc(n);

		m_irradiance_t[READ]->data(0, 0, data);

		fwrite(data, n, 1, irradiance);

		fclose(irradiance);
		free(data);
	}

	{
		FILE* inscatter = fopen("inscatter.raw", "wb");
	
		size_t n = sizeof(float) * INSCATTER_MU_S * INSCATTER_NU * INSCATTER_MU * INSCATTER_R * 4;
		void* data = malloc(n);

		m_inscatter_t[READ]->data(0, data);

		fwrite(data, n, 1, inscatter);

		fclose(inscatter);
		free(data);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::precompute()
{
	// -----------------------------------------------------------------------------
    // 1. Compute Transmittance Texture T
    // -----------------------------------------------------------------------------

    m_transmittance_program->use();
    set_uniforms(m_transmittance_program);

    m_transmittance_t->bind_image(0, 0, 0, GL_READ_WRITE, m_transmittance_t->internal_format());

    GL_CHECK_ERROR(glDispatchCompute(TRANSMITTANCE_W/NUM_THREADS, TRANSMITTANCE_H/NUM_THREADS, 1));
	GL_CHECK_ERROR(glFinish());

    // -----------------------------------------------------------------------------
    // 2. Compute Irradiance Texture deltaE
    // -----------------------------------------------------------------------------

    m_irradiance_1_program->use();
    set_uniforms(m_irradiance_1_program);

	m_delta_et->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_et->internal_format());

	if (m_irradiance_1_program->set_uniform("s_TransmittanceRead", 0))
		m_transmittance_t->bind(0);

    GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W/NUM_THREADS, IRRADIANCE_H/NUM_THREADS, 1));
	GL_CHECK_ERROR(glFinish());

    // -----------------------------------------------------------------------------
    // 3. Compute Single Scattering Texture
    // -----------------------------------------------------------------------------

    m_inscatter_1_program->use();
    set_uniforms(m_inscatter_1_program);

	m_delta_srt->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_srt->internal_format());
    m_delta_smt->bind_image(1, 0, 0, GL_READ_WRITE, m_delta_smt->internal_format());

	if (m_inscatter_1_program->set_uniform("s_TransmittanceRead", 0))
		m_transmittance_t->bind(0);

    for (int i = 0; i < INSCATTER_R; i++) 
    {
	    m_inscatter_1_program->set_uniform("u_Layer", i);
		GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S*INSCATTER_NU)/NUM_THREADS, INSCATTER_MU/NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());
	}

    // -----------------------------------------------------------------------------
    // 4. Copy deltaE into Irradiance Texture E 
    // -----------------------------------------------------------------------------

    m_copy_irradiance_program->use();
    set_uniforms(m_copy_irradiance_program);

    m_copy_irradiance_program->set_uniform("u_K", 0.0f);

    m_irradiance_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_irradiance_t[WRITE]->internal_format());

	if (m_copy_irradiance_program->set_uniform("s_DeltaERead", 0))
		m_delta_et->bind(0);

	if (m_copy_irradiance_program->set_uniform("s_IrradianceRead", 1))
		m_irradiance_t[READ]->bind(1);

    GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W/NUM_THREADS, IRRADIANCE_H/NUM_THREADS, 1));
	GL_CHECK_ERROR(glFinish());

    for (int order = 2; order < 4; order++)
    {
        // -----------------------------------------------------------------------------
        // 5. Copy deltaS into Inscatter Texture S 
        // -----------------------------------------------------------------------------

        m_copy_inscatter_1_program->use();
        set_uniforms(m_copy_inscatter_1_program);

        m_inscatter_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_inscatter_t[WRITE]->internal_format());

		if (m_copy_inscatter_1_program->set_uniform("s_DeltaSRRead", 0))
			m_delta_srt->bind(0);

		if (m_copy_inscatter_1_program->set_uniform("s_DeltaSMRead", 1))
			m_delta_smt->bind(1);

        for (int i = 0; i < INSCATTER_R; i++) 
        {
            m_copy_inscatter_1_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S*INSCATTER_NU)/NUM_THREADS, INSCATTER_MU/NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        swap(m_inscatter_t);

        // -----------------------------------------------------------------------------
        // 6. Compute deltaJ
        // -----------------------------------------------------------------------------

        m_inscatter_s_program->use();
        set_uniforms(m_inscatter_s_program);

        m_inscatter_s_program->set_uniform("first", (order == 2) ? 1 : 0);

		m_delta_jt->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_jt->internal_format());

		if (m_inscatter_s_program->set_uniform("s_TransmittanceRead", 0))
			m_transmittance_t->bind(0);

		if (m_inscatter_s_program->set_uniform("s_DeltaERead", 1))
			m_delta_et->bind(1);

		if (m_inscatter_s_program->set_uniform("s_DeltaSRRead", 2))
			m_delta_srt->bind(2);

		if (m_inscatter_s_program->set_uniform("s_DeltaSMRead", 3))
			m_delta_smt->bind(3);
        
        for (int i = 0; i < INSCATTER_R; i++) 
        {
            m_inscatter_s_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S*INSCATTER_NU)/NUM_THREADS, INSCATTER_MU/NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        // -----------------------------------------------------------------------------
        // 7. Compute deltaE
        // -----------------------------------------------------------------------------

        m_irradiance_n_program->use();
        set_uniforms(m_irradiance_n_program);

        m_irradiance_n_program->set_uniform("first", (order == 2) ? 1 : 0);

		m_delta_et->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_et->internal_format());

		if (m_irradiance_n_program->set_uniform("s_DeltaSRRead", 0))
			m_delta_srt->bind(0);

		if (m_irradiance_n_program->set_uniform("s_DeltaSMRead", 1))
			m_delta_smt->bind(1);

        GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W/NUM_THREADS, IRRADIANCE_H/NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());

        // -----------------------------------------------------------------------------
        // 8. Compute deltaS
        // -----------------------------------------------------------------------------

        m_inscatter_n_program->use();
        set_uniforms(m_inscatter_n_program);

        m_inscatter_n_program->set_uniform("first", (order == 2) ? 1 : 0);

		m_delta_srt->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_srt->internal_format());

		if (m_inscatter_n_program->set_uniform("s_TransmittanceRead", 0))
			m_transmittance_t->bind(0);

		if (m_inscatter_n_program->set_uniform("s_DeltaJRead", 1))
			m_delta_jt->bind(1);

        for (int i = 0; i < INSCATTER_R; i++) 
        {
            m_inscatter_n_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S*INSCATTER_NU)/NUM_THREADS, INSCATTER_MU/NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        // -----------------------------------------------------------------------------
        // 9. Adds deltaE into Irradiance Texture E
        // -----------------------------------------------------------------------------

        m_copy_irradiance_program->use();
        set_uniforms(m_copy_irradiance_program);

        m_copy_irradiance_program->set_uniform("u_K", 1.0f);

		m_irradiance_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_irradiance_t[WRITE]->internal_format());

		if (m_copy_irradiance_program->set_uniform("s_DeltaERead", 0))
			m_delta_et->bind(0);

		if (m_copy_irradiance_program->set_uniform("s_IrradianceRead", 1))
			m_irradiance_t[READ]->bind(1);

        GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W/NUM_THREADS, IRRADIANCE_H/NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());

        swap(m_irradiance_t);

        // -----------------------------------------------------------------------------
        // 10. Adds deltaS into Inscatter Texture S
        // -----------------------------------------------------------------------------

        m_copy_inscatter_n_program->use();
        set_uniforms(m_copy_inscatter_n_program);

		m_inscatter_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_inscatter_t[WRITE]->internal_format());

		if (m_copy_inscatter_n_program->set_uniform("s_InscatterRead", 0))
			m_inscatter_t[READ]->bind(0);

		if (m_copy_inscatter_n_program->set_uniform("s_DeltaSRead", 1))
			m_delta_srt->bind(1);

        for (int i = 0; i < INSCATTER_R; i++) 
        {
            m_copy_inscatter_n_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S*INSCATTER_NU)/NUM_THREADS, INSCATTER_MU/NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        swap(m_inscatter_t);
    }

    // -----------------------------------------------------------------------------
    // 11. Save to disk
    // -----------------------------------------------------------------------------

	write_textures();
}

// -----------------------------------------------------------------------------------------------------------------------------------

dw::Texture2D* BrunetonSkyModel::new_texture_2d(int width, int height)
{
	dw::Texture2D* texture = new dw::Texture2D(width, height, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    texture->set_min_filter(GL_LINEAR);
	texture->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	return texture;
}

// -----------------------------------------------------------------------------------------------------------------------------------

dw::Texture3D* BrunetonSkyModel::new_texture_3d(int width, int height, int depth)
{
	dw::Texture3D* texture = new dw::Texture3D(width, height, depth, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    texture->set_min_filter(GL_LINEAR);
    texture->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    return texture;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::swap(dw::Texture2D** arr)
{
	dw::Texture2D* tmp = arr[READ];
    arr[READ] = arr[WRITE];
    arr[WRITE] = tmp;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::swap(dw::Texture3D** arr)
{
	dw::Texture3D* tmp = arr[READ];
    arr[READ] = arr[WRITE];
    arr[WRITE] = tmp;
}

// -----------------------------------------------------------------------------------------------------------------------------------