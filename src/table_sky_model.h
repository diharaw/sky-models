#pragma once

#include "sky_model.h"

class TableSkyModel : public SkyModel
{
public:
	TableSkyModel() {}
	~TableSkyModel() {}

	inline float turbidity() { return m_turbidity; }
	inline void set_turbidity(float t) { m_turbidity = t; }

	inline float overcast() { return m_overcast; }
	inline void set_overcast(float o) { m_overcast = o; }

	inline float horiz_crush() { return m_horiz_crush; }
	inline void set_horiz_crush(float h) { m_horiz_crush = h; }

protected:
	glm::vec3 rgb_to_XYZ(const glm::vec3& rgb);
	float zenith_luminance(float thetaS, float T);

protected:
	static const int TABLE_SIZE = 64;
    static const int NUM_THREADS = 8;

	dw::Texture2D* m_table;
	dw::Shader* m_compute_tables_cs;
	dw::Program* m_compute_tables_program;

    float m_turbidity = 2.5f; 
    float m_overcast = 0.0f;
    float m_horiz_crush = 0.0f;
};