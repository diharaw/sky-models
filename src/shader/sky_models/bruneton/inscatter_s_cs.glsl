/*
 * Proland: a procedural landscape rendering library.
 * Copyright (c) 2008-2011 INRIA
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Proland is distributed under a dual-license scheme.
 * You can obtain a specific license from Inria: proland-licensing@inria.fr.
 */

/**
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Author: Eric Bruneton
 * Modified and ported to Unity by Justin Hawkins 2014
 */
 
 // copies deltaS into S (line 5 in algorithm 4.1)

#include <precompute_common.glsl>

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image3D i_DeltaJWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_DeltaERead; 
uniform sampler3D s_DeltaSRRead;
uniform sampler3D s_DeltaSMRead;

uniform int u_Layer;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------


void Inscatter(float r, float mu, float muS, float nu, out vec3 raymie) 
{ 
	float dphi = M_PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES); 
	float dtheta = M_PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);

    r = clamp(r, Rg, Rt); 
    mu = clamp(mu, -1.0, 1.0); 
    muS = clamp(muS, -1.0, 1.0); 
    float var = sqrt(1.0 - mu * mu) * sqrt(1.0 - muS * muS); 
    nu = clamp(nu, muS * mu - var, muS * mu + var); 
 
    float cthetamin = -sqrt(1.0 - (Rg / r) * (Rg / r)); 
 
    vec3 v = vec3(sqrt(1.0 - mu * mu), 0.0, mu); 
    float sx = v.x == 0.0 ? 0.0 : (nu - muS * mu) / v.x; 
    vec3 s = vec3(sx, sqrt(max(0.0, 1.0 - sx * sx - muS * muS)), muS); 
 
    raymie = vec3(0,0,0); 
 
    // integral over 4.PI around x with two nested loops over w directions (theta,phi) -- Eq (7) 
    for (int itheta = 0; itheta < INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++itheta) 
    { 
        float theta = (float(itheta) + 0.5) * dtheta; 
        float ctheta = cos(theta); 
 
        float greflectance = 0.0; 
        float dground = 0.0; 
        vec3 gtransp = vec3(0,0,0);
         
        if (ctheta < cthetamin) 
        { 	// if ground visible in direction w 
            // compute transparency gtransp between x and ground 
            greflectance = AVERAGE_GROUND_REFLECTANCE / M_PI; 
            dground = -r * ctheta - sqrt(r * r * (ctheta * ctheta - 1.0) + Rg * Rg); 
            gtransp = Transmittance(Rg, -(r * ctheta + dground) / Rg, dground); 
        } 
 
        for (int iphi = 0; iphi < 2 * INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++iphi) 
        { 
            float phi = (float(iphi) + 0.5) * dphi; 
            float dw = dtheta * dphi * sin(theta); 
            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), ctheta); 
 
            float nu1 = dot(s, w); 
            float nu2 = dot(v, w); 
            float pr2 = PhaseFunctionR(nu2); 
            float pm2 = PhaseFunctionM(nu2); 
 
            // compute irradiance received at ground in direction w (if ground visible) =deltaE 
            vec3 gnormal = (vec3(0.0, 0.0, r) + dground * w) / Rg; 
            vec3 girradiance = Irradiance(s_DeltaERead, Rg, dot(gnormal, s)); 
 
            vec3 raymie1; // light arriving at x from direction w 
 
            // first term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE 
            raymie1 = greflectance * girradiance * gtransp; 
 
            // second term = inscattered light, =deltaS 
            if (first == 1) 
            { 
                // first iteration is special because Rayleigh and Mie were stored separately, 
                // without the phase functions factors; they must be reintroduced here 
                float pr1 = PhaseFunctionR(nu1); 
                float pm1 = PhaseFunctionM(nu1); 
                vec3 ray1 = Texture4D(s_DeltaSRRead, r, w.z, muS, nu1).rgb; 
                vec3 mie1 = Texture4D(s_DeltaSMRead, r, w.z, muS, nu1).rgb; 
                raymie1 += ray1 * pr1 + mie1 * pm1; 
            } 
            else 
            { 
                raymie1 += Texture4D(s_DeltaSRRead, r, w.z, muS, nu1).rgb; 
            } 
 
            // light coming from direction w and scattered in direction v 
            // = light arriving at x from direction w (raymie1) * SUM(scattering coefficient * phaseFunction) 
            // see Eq (7) 
            raymie += raymie1 * (betaR.xyz * exp(-(r - Rg) / HR) * pr2 + betaMSca.xyz * exp(-(r - Rg) / HM) * pm2) * dw; 
        } 
    }
 
    // output raymie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1) 
} 

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 raymie; 
    vec4 dhdH;
    float mu, muS, nu, r;  
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetLayer(u_Layer, r, dhdH); 
    GetMuMuSNu(coords, r, dhdH, mu, muS, nu); 
    
    Inscatter(r, mu, muS, nu, raymie); 

    imageStore(i_DeltaJWrite, ivec3(gl_GlobalInvocationID.xy, u_Layer), vec4(raymie, 0.0));
}

// ------------------------------------------------------------------