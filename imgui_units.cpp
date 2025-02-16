#include "imgui_units.h" 
 
namespace ImGui { 
 
static std::map<UnitDim, std::vector<UnitDef>> units; 
 
void registerUnit(UnitDim dim, UnitDef def) { 
    units[dim].push_back(def); 
} 
 
static bool equals_case_insensitive(const char* s1, const char* s2, int length) { 
    for (int i = 0; i < length; ++i) { 
        if (std::tolower(s1[i]) != std::tolower(s2[i])) { 
            return false; 
        } 
    } 
    return true; 
} 
 
double findFactor(UnitDim dim, const char* label) { 
    while (*label == ' ') { 
        label++; 
    } 
    const int num_chars = (int)strlen(label); 
    for (const UnitDef& def : units[dim]) { 
        if (def.label.size() == num_chars && 
            equals_case_insensitive(def.label.c_str(), label, num_chars)) { 
            return def.factor; 
        } 
    } 
    return 0; 
} 
 
} // namespace ImGui 
