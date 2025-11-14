#include "pch.h"
#include "AnimDataModel.h"
#include "FbxLoader.h"

IMPLEMENT_CLASS(UAnimDataModel)

UAnimDataModel::~UAnimDataModel()
{
}

const FBoneAnimationTrack* UAnimDataModel::FindTrackByBone(const FName& BoneName)
{
    for (auto& BoneTrack : BoneAnimationTracks)
    {
        if (BoneTrack.Name == BoneName)
        {
            return &BoneTrack;
        }
    }
    return nullptr;
}
