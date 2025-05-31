#pragma once 
 
#include <map> 
#include <string> 
#include <tuple> 
#include <vector> 
 
namespace ImGui { 
 
using UnitDim = std::tuple<int, int, int, int, int>; 
 
struct UnitDef { 
    double factor; 
    std::string label; 
}; 
 
struct Units { 
    UnitDim dimension = { 0, 0, 0, 0, 0 }; 
    double current_factor = 1.f; 
}; 

void clearUnits();
void registerUnit(UnitDim dim, UnitDef def); 
 
double findFactor(UnitDim dim, const char* label); 
 
} // namespace ImGui 
