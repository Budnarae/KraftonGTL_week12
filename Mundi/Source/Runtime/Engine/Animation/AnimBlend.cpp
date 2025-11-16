#include "pch.h"
#include "AnimBlend.h"

void FAnimBlend::Blend(const FPoseContext& Start, const FPoseContext& End, float Alpha, FPoseContext& Out)
{
	const int32 BoneCnt = std::min(Start.EvaluatedPoses.Num(), End.EvaluatedPoses.Num());
	Out.EvaluatedPoses.SetNum(BoneCnt);

	const float ClampedWeight = std::clamp(Alpha, 0.0f, 1.0f);

	for (int i = 0; i < BoneCnt; i++)
	{
		Out.EvaluatedPoses[i] = FTransform::Lerp(Start.EvaluatedPoses[i], End.EvaluatedPoses[i], Alpha);
	}
}