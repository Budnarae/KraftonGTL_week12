#pragma once
#include "VertexData.h"
#include "AnimationSequence.h"

struct FAnimNode_Base {
	virtual void Update(const FAnimationUpdateContext& Context) = 0;
	virtual void Evaluate(FPoseContext& Output) = 0;
	virtual ~FAnimNode_Base() = default;
};
struct FAnimNode_Sequence : FAnimNode_Base
{
    UAnimationSequence* Sequence = nullptr;
    float CurrentTime = 0.0f;
    bool bLooping = true;

    void SetSequence(UAnimationSequence* InSeq, bool bInLoop = true)
    {
        Sequence = InSeq;
        bLooping = bInLoop;
        CurrentTime = 0.0f;
    }

    virtual void Update(const FAnimationUpdateContext& Context) override
    {
        if (!Sequence)
            return;

        CurrentTime += Context.DeltaTime;

        float PlayLength = Sequence->GetDataModel()->GetPlayLength();
        if (PlayLength <= 0.0f)
            return;

        if (CurrentTime >= PlayLength)
        {
            if (bLooping)
            {
                CurrentTime = fmod(CurrentTime, PlayLength);
            }
            else
            {
                CurrentTime = PlayLength;
            }
        }
    }

    virtual void Evaluate(FPoseContext& Output) override
    {
        if (!Sequence || !Output.Skeleton)
            return;

        const FSkeleton Skeleton = *Output.Skeleton;
        const int32 NumBones = Skeleton.Bones.Num();

        Output.EvaluatedPoses.Empty();
        Output.EvaluatedPoses.SetNum(NumBones);

        for (int32 BoneIndex = 0; BoneIndex < NumBones; BoneIndex++)
        {
            const FBone& Bone = Skeleton.Bones[BoneIndex];
            const FName BoneName(Bone.Name);
            Output.EvaluatedPoses[BoneIndex] = GetBonePose(BoneName, CurrentTime);
        }
    }
};