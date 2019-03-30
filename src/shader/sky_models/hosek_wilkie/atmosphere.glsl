// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform vec3 A, B, C, D, E, F, G, H, I, Z;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 hosek_wilkie(float cos_theta, float gamma, float cos_gamma)
{
	vec3 chi = (1 + cos_gamma * cos_gamma) / pow(1 + H * H - 2 * cos_gamma * H, vec3(1.5));
    return (1 + A * exp(B / (cos_theta + 0.01))) * (C + D * exp(E * gamma) + F * (cos_gamma * cos_gamma) + G * chi + I * sqrt(cos_theta));
}

// ------------------------------------------------------------------

vec3 hosek_wilkie_sky_rgb(vec3 v, vec3 sun_dir)
{
    float cos_theta = clamp(v.y, 0, 1);
	float cos_gamma = clamp(dot(v, sun_dir), 0, 1);
	float gamma_ = acos(cos_gamma);

	vec3 R = Z * hosek_wilkie(cos_theta, gamma_, cos_gamma);
    return R;
}

// ------------------------------------------------------------------