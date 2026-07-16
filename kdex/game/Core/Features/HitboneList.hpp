#pragma once
#include "../SDK/Structs/GameClasses.hpp"
#include "../../Security/xorstr.hpp"
#include <string>
#include <vector>

namespace Core {
    namespace Features {

        struct HitboneEntry {
            unsigned int id;
            const char* name;
        };

        inline const std::vector<HitboneEntry>& GetHitboneTable() {
            static const std::vector<HitboneEntry> table = {
                { SKEL_Head,        "Head" },
                { SKEL_Neck_1,      "Neck" },
                { SKEL_Spine3,      "Chest" },
                { SKEL_Spine2,      "Upper Spine" },
                { SKEL_Spine1,      "Mid Spine" },
                { SKEL_Spine0,      "Lower Spine" },
                { SKEL_Spine_Root,  "Spine Root" },
                { SKEL_Pelvis,      "Pelvis" },
                { SKEL_L_Clavicle,  "L Clavicle" },
                { SKEL_L_UpperArm,  "L UpperArm" },
                { SKEL_L_Forearm,   "L Forearm" },
                { SKEL_L_Hand,      "L Hand" },
                { SKEL_R_Clavicle,  "R Clavicle" },
                { SKEL_R_UpperArm,  "R UpperArm" },
                { SKEL_R_Forearm,   "R Forearm" },
                { SKEL_R_Hand,      "R Hand" },
                { SKEL_L_Thigh,     "L Thigh" },
                { SKEL_L_Calf,      "L Calf" },
                { SKEL_L_Foot,      "L Foot" },
                { SKEL_L_Toe0,      "L Toe" },
                { SKEL_R_Thigh,     "R Thigh" },
                { SKEL_R_Calf,      "R Calf" },
                { SKEL_R_Foot,      "R Foot" },
                { SKEL_R_Toe0,      "R Toe" }
            };
            return table;
        }

        inline int GetHitboneCount() {
            return static_cast<int>(GetHitboneTable().size());
        }

        inline unsigned int GetHitboneId(int index) {
            const auto& t = GetHitboneTable();
            if (index < 0 || index >= static_cast<int>(t.size()))
                return SKEL_Head;
            return t[index].id;
        }

        inline std::vector<std::string> BuildHitboneNameList() {
            return {
                std::string(xorstr("Head")),
                std::string(xorstr("Neck")),
                std::string(xorstr("Chest")),
                std::string(xorstr("Upper Spine")),
                std::string(xorstr("Mid Spine")),
                std::string(xorstr("Lower Spine")),
                std::string(xorstr("Spine Root")),
                std::string(xorstr("Pelvis")),
                std::string(xorstr("L Clavicle")),
                std::string(xorstr("L UpperArm")),
                std::string(xorstr("L Forearm")),
                std::string(xorstr("L Hand")),
                std::string(xorstr("R Clavicle")),
                std::string(xorstr("R UpperArm")),
                std::string(xorstr("R Forearm")),
                std::string(xorstr("R Hand")),
                std::string(xorstr("L Thigh")),
                std::string(xorstr("L Calf")),
                std::string(xorstr("L Foot")),
                std::string(xorstr("L Toe")),
                std::string(xorstr("R Thigh")),
                std::string(xorstr("R Calf")),
                std::string(xorstr("R Foot")),
                std::string(xorstr("R Toe"))
            };
        }
    }
}
