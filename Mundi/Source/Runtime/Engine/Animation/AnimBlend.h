#pragma once

struct FAnimBlend
{
	static void Blend(
		const FPoseContext& Start, 
		const FPoseContext& End, 
		float Alpha, 
		FPoseContext& Out);
};