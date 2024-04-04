/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 *
 */

#include "backends/graphics/metal/shader.h"
#include <Metal/Metal.hpp>

namespace Common {
DECLARE_SINGLETON(Metal::ShaderManager);
}

namespace Metal {

namespace {

#pragma mark - Builtin Shader Sources -

using NS::StringEncoding::UTF8StringEncoding;

const char* shaderSrc = R"(
	#include <metal_stdlib>
	#include <simd/simd.h>
	using namespace metal;

	struct Vertex
	{
		float4 position   [[attribute(0)]];
		float2 texCoord   [[attribute(1)]];
	};

	struct VertexOut
	{
		float4 position [[position]];
		float2 texCoord;
	};

	vertex VertexOut vertexFunction(Vertex in [[stage_in]])
	{
		VertexOut out;
		out.position = in.position;
		out.texCoord = in.texCoord;
		return out;
	}

	fragment float4 fragmentFunction(VertexOut in [[stage_in]],
		texture2d<float> colorTexture [[texture(0)]])
	{
		constexpr sampler colorSampler (mip_filter::linear, mag_filter::linear, min_filter::linear);
		// Sample the texture to obtain a color
		float4 color = colorTexture.sample(colorSampler, in.texCoord);

		// return the color of the texture
		return color;
	}

	fragment float4 clut8FragmentFunction(VertexOut in [[stage_in]],
		texture2d<float> colorTexture [[texture(0)]],
		texture2d<float> palette [[texture(1)]])
	{
		constexpr sampler colorSampler (address::clamp_to_edge, filter::linear);

		const float adjustFactor = 255.0 / 256.0 + 1.0 / (2.0 * 256.0);

		// Sample the texture to obtain a color
		float4 index = colorTexture.sample(colorSampler, in.texCoord);
		float4 color = palette.sample(colorSampler, float2(index.a * adjustFactor, 0.0f));

		// return the color of the texture
		return color;
	})";
} // End of anonymous namespace

ShaderManager::ShaderManager() : _metalDevice(nullptr) {
	NS::Error* error = nullptr;
	_shaderLibrary = _metalDevice->newLibrary(NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &error);
	if (!_shaderLibrary)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}
}

ShaderManager::~ShaderManager() {
}

void ShaderManager::notifyDestroy() {
	_shaderLibrary->release();
	_shaderLibrary = nullptr;
}

void ShaderManager::notifyCreate() {
	// Ensure everything is destroyed
	notifyDestroy();

	_defaultVertexShader = _shaderLibrary->newFunction(NS::String::string("vertexFunction", UTF8StringEncoding));
	_defaultFragmentShader = _shaderLibrary->newFunction(NS::String::string("fragmentFunction", UTF8StringEncoding));
	_lookUpFragmentShader = _shaderLibrary->newFunction(NS::String::string("clut8FragmentFunction", UTF8StringEncoding));
}

MTL::Function *ShaderManager::query(ShaderUsage shader) const {
	switch (shader) {
	case kDefault:
		return _defaultFragmentShader;
		break;
	case kCLUT8LookUp:
		return _lookUpFragmentShader;
		break;
	default:
		return nullptr;
	}
}

} // End of namespace Metal
