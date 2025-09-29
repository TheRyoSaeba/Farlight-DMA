#pragma once
#include <unordered_map>
#include <string>

enum class BoneID : int {
    PELVIS = 1,
    SPINE_01 = 2,
    SPINE_02 = 3,
    SPINE_03 = 4,
    NECK_01 = 5,
    UPPERARM_L = 6,
    LOWERARM_L = 7,
    HAND_L = 8,
    UPPERARM_R = 9,
    LOWERARM_R = 10,
    HAND_R = 11,
    THIGH_L = 12,
    CALF_L = 13,
    FOOT_L = 14,
    THIGH_R = 15,
    CALF_R = 16,
    FOOT_R = 17
};

static const std::unordered_map<std::string, BoneID> BONE_NAME_MAP = {
    {"pelvis", BoneID::PELVIS},
    {"spine_01", BoneID::SPINE_01},
    {"spine_02", BoneID::SPINE_02},
    {"spine_03", BoneID::SPINE_03},
    {"neck_01", BoneID::NECK_01},
    {"upperarm_l", BoneID::UPPERARM_L},
    {"lowerarm_l", BoneID::LOWERARM_L},
    {"hand_l", BoneID::HAND_L},
    {"upperarm_r", BoneID::UPPERARM_R},
    {"lowerarm_r", BoneID::LOWERARM_R},
    {"hand_r", BoneID::HAND_R},
    {"thigh_l", BoneID::THIGH_L},
    {"calf_l", BoneID::CALF_L},
    {"foot_l", BoneID::FOOT_L},
    {"thigh_r", BoneID::THIGH_R},
    {"calf_r", BoneID::CALF_R},
    {"foot_r", BoneID::FOOT_R}
};

//






