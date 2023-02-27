//
//  shaders.metal
//  scummvm
//
//  Created by Lars Sundström on 2023-02-26.
//

#include <metal_stdlib>
using namespace metal;

typedef struct
{
	// Positions in pixel space. A value of 100 indicates 100 pixels from the origin/center.
	vector_float2 position;

	// 2D texture coordinate
	vector_float2 textureCoordinate;
} AAPLVertex;


struct RasterizerData
{
	// The [[position]] attribute qualifier of this member indicates this value is
	// the clip space position of the vertex when this structure is returned from
	// the vertex shader
	float4 position [[position]];

	// Since this member does not have a special attribute qualifier, the rasterizer
	// will interpolate its value with values of other vertices making up the triangle
	// and pass that interpolated value to the fragment shader for each fragment in
	// that triangle.
	float2 textureCoordinate;

};
