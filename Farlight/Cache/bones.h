#pragma once
#include <unordered_map>
#include <string>
#include <array>
#include <Utils/Utils.h>

struct SkeletonBones {
    Vector2 pelvis;
    Vector2 spine01;
    Vector2 spine02;
    Vector2 spine03;
    Vector2 neck01;
    Vector2 head;
    Vector2 upperArmL;
    Vector2 lowerArmL;
    Vector2 handL;
    Vector2 upperArmR;
    Vector2 lowerArmR;
    Vector2 handR;
    Vector2 thighL;
    Vector2 calfL;
    Vector2 footL;
    Vector2 thighR;
    Vector2 calfR;
    Vector2 footR;
};

enum class BoneID : int {
    Root = 0,
    Pelvis = 2,
    Spine01 = 3,
    Spine02 = 4,
    Spine03 = 5,
    Neck01 = 6,
    Head = 7,
    UpperArmL = 9,
    LowerArmL = 10,
    HandL = 11,
    UpperArmR = 31,
    LowerArmR = 32,
    HandR = 33,
    ThighL = 61,
    CalfL = 62,
    FootL = 63,
    ThighR = 67,
    CalfR = 68,
    FootR = 69
};

// Bones needed for skeleton ESP rendering
static constexpr std::array<BoneID, 18> SKELETON_BONES = {
    BoneID::Pelvis,
    BoneID::Spine01,
    BoneID::Spine02,
    BoneID::Spine03,
    BoneID::Neck01,
    BoneID::Head,
    BoneID::UpperArmL,
    BoneID::LowerArmL,
    BoneID::HandL,
    BoneID::UpperArmR,
    BoneID::LowerArmR,
    BoneID::HandR,
    BoneID::ThighL,
    BoneID::CalfL,
    BoneID::FootL,
    BoneID::ThighR,
    BoneID::CalfR,
    BoneID::FootR
};

static const std::unordered_map<std::string, BoneID> BONE_NAME_MAP = {
    {"root", BoneID::Root},
    {"pelvis", BoneID::Pelvis},
    {"spine_01", BoneID::Spine01},
    {"spine_02", BoneID::Spine02},
    {"spine_03", BoneID::Spine03},
    {"neck_01", BoneID::Neck01},
    {"head", BoneID::Head},
    {"upperarm_l", BoneID::UpperArmL},
    {"lowerarm_l", BoneID::LowerArmL},
    {"hand_l", BoneID::HandL},
    {"upperarm_r", BoneID::UpperArmR},
    {"lowerarm_r", BoneID::LowerArmR},
    {"hand_r", BoneID::HandR},
    {"thigh_l", BoneID::ThighL},
    {"calf_l", BoneID::CalfL},
    {"foot_l", BoneID::FootL},
    {"thigh_r", BoneID::ThighR},
    {"calf_r", BoneID::CalfR},
    {"foot_r", BoneID::FootR}
};

// Populate skeleton struct from bone transforms
template<typename TransformFunc>
inline void PopulateSkeletonBones(SkeletonBones& skeleton, const std::unordered_map<int, FTransform>& boneTransforms, TransformFunc doMatrix) {
    skeleton.pelvis = doMatrix(boneTransforms.at(static_cast<int>(BoneID::Pelvis)));
    skeleton.spine01 = doMatrix(boneTransforms.at(static_cast<int>(BoneID::Spine01)));
    skeleton.spine02 = doMatrix(boneTransforms.at(static_cast<int>(BoneID::Spine02)));
    skeleton.spine03 = doMatrix(boneTransforms.at(static_cast<int>(BoneID::Spine03)));
    skeleton.neck01 = doMatrix(boneTransforms.at(static_cast<int>(BoneID::Neck01)));
    skeleton.head = doMatrix(boneTransforms.at(static_cast<int>(BoneID::Head)));
    skeleton.upperArmL = doMatrix(boneTransforms.at(static_cast<int>(BoneID::UpperArmL)));
    skeleton.lowerArmL = doMatrix(boneTransforms.at(static_cast<int>(BoneID::LowerArmL)));
    skeleton.handL = doMatrix(boneTransforms.at(static_cast<int>(BoneID::HandL)));
    skeleton.upperArmR = doMatrix(boneTransforms.at(static_cast<int>(BoneID::UpperArmR)));
    skeleton.lowerArmR = doMatrix(boneTransforms.at(static_cast<int>(BoneID::LowerArmR)));
    skeleton.handR = doMatrix(boneTransforms.at(static_cast<int>(BoneID::HandR)));
    skeleton.thighL = doMatrix(boneTransforms.at(static_cast<int>(BoneID::ThighL)));
    skeleton.calfL = doMatrix(boneTransforms.at(static_cast<int>(BoneID::CalfL)));
    skeleton.footL = doMatrix(boneTransforms.at(static_cast<int>(BoneID::FootL)));
    skeleton.thighR = doMatrix(boneTransforms.at(static_cast<int>(BoneID::ThighR)));
    skeleton.calfR = doMatrix(boneTransforms.at(static_cast<int>(BoneID::CalfR)));
    skeleton.footR = doMatrix(boneTransforms.at(static_cast<int>(BoneID::FootR)));
}

// Draw skeleton using ImGui (must be included in rendering context)
template<typename DrawLineFunc>
inline void DrawSkeletonLines(const SkeletonBones& s, DrawLineFunc drawLine) {
    // Head to neck to spine chain (spine03 is upper, spine01 is lower)
    drawLine(s.head, s.neck01);
    drawLine(s.neck01, s.spine03);
    drawLine(s.spine03, s.spine02);
    drawLine(s.spine02, s.spine01);
    drawLine(s.spine01, s.pelvis);

    // Arms from spine03 (upper spine/shoulders)
    drawLine(s.spine03, s.upperArmL);
    drawLine(s.spine03, s.upperArmR);

    // Left arm
    drawLine(s.upperArmL, s.lowerArmL);
    drawLine(s.lowerArmL, s.handL);

    // Right arm
    drawLine(s.upperArmR, s.lowerArmR);
    drawLine(s.lowerArmR, s.handR);

    // Left leg
    drawLine(s.pelvis, s.thighL);
    drawLine(s.thighL, s.calfL);
    drawLine(s.calfL, s.footL);

    // Right leg
    drawLine(s.pelvis, s.thighR);
    drawLine(s.thighR, s.calfR);
    drawLine(s.calfR, s.footR);
}

//






