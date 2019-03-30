#pragma once

#include "sky_model.h"
#include <memory>

class BrunetonSkyModel : public SkyModel
{
private:
	//Dont change these
	const int NUM_THREADS = 8;
	const int READ = 0;
	const int WRITE = 1;
	const float SCALE = 1000.0f;

	//Will save the tables as 8 bit png files so they can be
	//viewed in photoshop. Used for debugging.
	const bool WRITE_DEBUG_TEX = false;

	//You can change these
	//The radius of the planet (Rg), radius of the atmosphere (Rt)
	const float Rg = 6360.0f;
	const float Rt = 6420.0f;
	const float RL = 6421.0f;

	//Dimensions of the tables
	const int TRANSMITTANCE_W = 256;
	const int TRANSMITTANCE_H = 64;

	const int IRRADIANCE_W = 64;
	const int IRRADIANCE_H = 16;

	const int INSCATTER_R = 32;
	const int INSCATTER_MU = 128;
	const int INSCATTER_MU_S = 32;
	const int INSCATTER_NU = 8;

	//Physical settings, Mie and Rayliegh values
	const float AVERAGE_GROUND_REFLECTANCE = 0.1f;
	const glm::vec4 BETA_R = glm::vec4(5.8e-3f, 1.35e-2f, 3.31e-2f, 0.0f);
	const glm::vec4 BETA_MSca = glm::vec4(4e-3f, 4e-3f, 4e-3f, 0.0f);
	const glm::vec4 BETA_MEx = glm::vec4(4.44e-3f, 4.44e-3f, 4.44e-3f, 0.0f);

	//Asymmetry factor for the mie phase function
	//A higher number meands more light is scattered in the forward direction
	const float MIE_G = 0.8f;

	//Half heights for the atmosphere air density (HR) and particle density (HM)
	//This is the height in km that half the particles are found below
	const float HR = 8.0f;
	const float HM = 1.2f;

	glm::vec3 m_beta_r = glm::vec3(0.0058f, 0.0135f, 0.0331f);
    float m_mie_g = 0.75f;
    float m_sun_intensity = 100.0f;

	dw::Texture2D* m_transmittance_t;
	dw::Texture2D* m_delta_et;
	dw::Texture3D* m_delta_srt;
	dw::Texture3D* m_delta_smt;
	dw::Texture3D* m_delta_jt;
	dw::Texture2D* m_irradiance_t[2];
	dw::Texture3D* m_inscatter_t[2];

	dw::Shader* m_copy_inscatter_1_cs;
	dw::Shader* m_copy_inscatter_n_cs;
	dw::Shader* m_copy_irradiance_cs;
	dw::Shader* m_inscatter_1_cs;
	dw::Shader* m_inscatter_n_cs;
	dw::Shader* m_inscatter_s_cs;
	dw::Shader* m_irradiance_1_cs;
	dw::Shader* m_irradiance_n_cs;
	dw::Shader* m_transmittance_cs;

	dw::Program* m_copy_inscatter_1_program;
	dw::Program* m_copy_inscatter_n_program;
	dw::Program* m_copy_irradiance_program;
	dw::Program* m_inscatter_1_program;
	dw::Program* m_inscatter_n_program;
	dw::Program* m_inscatter_s_program;
	dw::Program* m_irradiance_1_program;
	dw::Program* m_irradiance_n_program;
	dw::Program* m_transmittance_program;

public:
	BrunetonSkyModel();
	~BrunetonSkyModel();

	bool initialize() override;
	void update() override;
	void set_render_uniforms(dw::Program* program) override;
	
private:
	void set_uniforms(dw::Program* program);
	bool load_cached_textures();
	void write_textures();
	void precompute();
	dw::Texture2D* new_texture_2d(int width, int height);
	dw::Texture3D* new_texture_3d(int width, int height, int depth);
	void swap(dw::Texture2D** arr);
	void swap(dw::Texture3D** arr);
};