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
                { SKEL_Head,        xorstr("Head") },
                { SKEL_Neck_1,      xorstr("Neck") },
                { SKEL_Spine3,      xorstr("Chest") },
                { SKEL_Spine2,      xorstr("Upper Spine") },
                { SKEL_Spine1,      xorstr("Mid Spine") },
                { SKEL_Spine0,      xorstr("Lower Spine") },
                { SKEL_Spine_Root,  xorstr("Spine Root") },
                { SKEL_Pelvis,      xorstr("Pelvis") },
                { SKEL_L_Clavicle,  xorstr("L Clavicle") },
                { SKEL_L_UpperArm,  xorstr("L UpperArm") },
                { SKEL_L_Forearm,   xorstr("L Forearm") },
                { SKEL_L_Hand,      xorstr("L Hand") },
                { SKEL_R_Clavicle,  xorstr("R Clavicle") },
                { SKEL_R_UpperArm,  xorstr("R UpperArm") },
                { SKEL_R_Forearm,   xorstr("R Forearm") },
                { SKEL_R_Hand,      xorstr("R Hand") },
                { SKEL_L_Thigh,     xorstr("L Thigh") },
                { SKEL_L_Calf,      xorstr("L Calf") },
                { SKEL_L_Foot,      xorstr("L Foot") },
                { SKEL_L_Toe0,      xorstr("L Toe") },
                { SKEL_R_Thigh,     xorstr("R Thigh") },
                { SKEL_R_Calf,      xorstr("R Calf") },
                { SKEL_R_Foot,      xorstr("R Foot") },
                { SKEL_R_Toe0,      xorstr("R Toe") }
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
