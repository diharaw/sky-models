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

layout (binding = 0, rgba32f) uniform image2D i_DeltaEWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler3D s_DeltaSRRead; 
uniform sampler3D s_DeltaSMRead;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    float r, muS;
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetIrradianceRMuS(coords, r, muS); 
    
    vec3 s = vec3(sqrt(max(1.0 - muS * muS, 0.0)), 0.0, muS); 
 
    vec3 result = vec3(0,0,0); 
    // integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15) 
    
    float dphi = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES); 
	float dtheta = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES); 

    for (int iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi) { 

        float phi = (float(iphi) + 0.5) * dphi; 

        for (int itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta) { 

            float theta = (float(itheta) + 0.5) * dtheta; 

            float dw = dtheta * dphi * sin(theta); 

            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)); 

            float nu = dot(s, w); 

            if (first == 1) 
            { 
                // first iteration is special because Rayleigh and Mie were stored separately, 
                // without the phase functions factors; they must be reintroduced here 
                float pr1 = PhaseFunctionR(nu); 
                float pm1 = PhaseFunctionM(nu); 
                vec3 ray1 = Texture4D(s_DeltaSRRead, r, w.z, muS, nu).rgb; 
                vec3 mie1 = Texture4D(s_DeltaSMRead, r, w.z, muS, nu).rgb; 
                result += (ray1 * pr1 + mie1 * pm1) * w.z * dw; 
            } 
            else { 
                result += Texture4D(s_DeltaSRRead, r, w.z, muS, nu).rgb * w.z * dw; 

            } 
        } 
    } 
 
    imageStore(i_DeltaEWrite, ivec2(gl_GlobalInvocationID.xy), vec4(result, 1.0)); 
}

// ------------------------------------------------------------------