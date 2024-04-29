//
//  kernel.metal
//  ScummVM-iOS
//
//  Created by Lars Sundström on 2024-04-29.
//

#include <metal_stdlib>
using namespace metal;

kernel void compute(
					texture2d<float,access::write> dst [[texture(0)]],
					texture2d<float, access::read> src [[texture(1)]],
					uint2 gid [[thread_position_in_grid]])
{


	float4 flipColor = src.read(uint2(gid.x, -gid.y));

	dst.write(flipColor, gid);
}
