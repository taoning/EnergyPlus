// EnergyPlus, Copyright (c) 1996-2023, The Board of Trustees of the University of Illinois,
// The Regents of the University of California, through Lawrence Berkeley National Laboratory
// (subject to receipt of any required approvals from the U.S. Dept. of Energy), Oak Ridge
// National Laboratory, managed by UT-Battelle, Alliance for Sustainable Energy, LLC, and other
// contributors. All rights reserved.
//
// NOTICE: This Software was developed under funding from the U.S. Department of Energy and the
// U.S. Government consequently retains certain rights. As such, the U.S. Government has been
// granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable,
// worldwide license in the Software to reproduce, distribute copies to the public, prepare
// derivative works, and perform publicly and display publicly, and to permit others to do so.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice, this list of
//     conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National Laboratory,
//     the University of Illinois, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without specific prior
//     written permission.
//
// (4) Use of EnergyPlus(TM) Name. If Licensee (i) distributes the software in stand-alone form
//     without changes from the version obtained under this License, or (ii) Licensee makes a
//     reference solely to the software portion of its product, Licensee must refer to the
//     software as "EnergyPlus version X" software, where "X" is the version number Licensee
//     obtained under this License and may not use a different name for the software. Except as
//     specifically required in this Section (4), Licensee shall not use in a company name, a
//     product name, in advertising, publicity, or other promotional activities any name, trade
//     name, trademark, logo, or other designation of "EnergyPlus", "E+", "e+" or confusingly
//     similar designation, without the U.S. Department of Energy's prior written consent.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// C++ Headers
#include <memory>
#include <vector>

// EnergyPlus Headers
#include <EnergyPlus/Data/EnergyPlusData.hh>
#include <EnergyPlus/EnergyPlus.hh>
#include <EnergyPlus/GroundTemperatureModeling/BaseGroundTemperatureModel.hh>
#include <EnergyPlus/GroundTemperatureModeling/FiniteDifferenceGroundTemperatureModel.hh>
#include <EnergyPlus/GroundTemperatureModeling/GroundTemperatureModelManager.hh>
#include <EnergyPlus/GroundTemperatureModeling/KusudaAchenbachGroundTemperatureModel.hh>
#include <EnergyPlus/GroundTemperatureModeling/SiteBuildingSurfaceGroundTemperatures.hh>
#include <EnergyPlus/GroundTemperatureModeling/SiteDeepGroundTemperatures.hh>
#include <EnergyPlus/GroundTemperatureModeling/SiteFCFactorMethodGroundTemperatures.hh>
#include <EnergyPlus/GroundTemperatureModeling/SiteShallowGroundTemperatures.hh>
#include <EnergyPlus/GroundTemperatureModeling/XingGroundTemperatureModel.hh>
#include <EnergyPlus/UtilityRoutines.hh>

namespace EnergyPlus {

namespace GroundTemperatureManager {

    constexpr std::array<std::string_view, static_cast<int>(GroundTempObjType::Num)> groundTempModelNamesUC = {
        "SITE:GROUNDTEMPERATURE:UNDISTURBED:KUSUDAACHENBACH",
        "SITE:GROUNDTEMPERATURE:UNDISTURBED:FINITEDIFFERENCE",
        "SITE:GROUNDTEMPERATURE:BUILDINGSURFACE",
        "SITE:GROUNDTEMPERATURE:SHALLOW",
        "SITE:GROUNDTEMPERATURE:DEEP",
        "SITE:GROUNDTEMPERATURE:FCFACTORMETHOD",
        "SITE:GROUNDTEMPERATURE:UNDISTURBED:XING"};

    constexpr std::array<std::string_view, static_cast<int>(GroundTempObjType::Num)> groundTempModelNames = {
        "Site:GroundTemperature:Undisturbed:KusudaAchenbach",
        "Site:GroundTemperature:Undisturbed:FiniteDifference",
        "Site:GroundTemperature:BuildingSurface",
        "Site:GroundTemperature:Shallow",
        "Site:GroundTemperature:Deep",
        "Site:GroundTemperature:FCfactorMethod",
        "Site:GroundTemperature:Undisturbed:Xing"};

    std::shared_ptr<BaseGroundTempsModel>
    GetGroundTempModelAndInit(EnergyPlusData &state, std::string_view const objectType_str, std::string const &objectName)
    {
        // SUBROUTINE INFORMATION:
        //       AUTHOR         Matt Mitchell
        //       DATE WRITTEN   Summer 2015

        // PURPOSE OF THIS SUBROUTINE:
        // Called by objects requiring ground temperature models. Determines type and calls appropriate factory method.

        // Set object type
        GroundTempObjType objectType = static_cast<GroundTempObjType>(getEnumValue(groundTempModelNamesUC, Util::makeUPPER(objectType_str)));

        assert(objectType != GroundTempObjType::Invalid);

        int numGTMs = state.dataGrndTempModelMgr->groundTempModels.size();

        // Check if this instance of this model has already been retrieved
        for (int i = 0; i < numGTMs; ++i) {
            auto currentModel = state.dataGrndTempModelMgr->groundTempModels[i]; // (AUTO_OK_UNIQUE_PTR)
            // Check if the type and name match
            if (objectType == currentModel->objectType && objectName == currentModel->objectName) {
                return state.dataGrndTempModelMgr->groundTempModels[i];
            }
        }

        // If not found, create new instance of the model
        if (objectType == GroundTempObjType::KusudaGroundTemp) {
            return KusudaGroundTempsModel::KusudaGTMFactory(state, objectName);
        } else if (objectType == GroundTempObjType::FiniteDiffGroundTemp) {
            return FiniteDiffGroundTempsModel::FiniteDiffGTMFactory(state, objectName);
        } else if (objectType == GroundTempObjType::SiteBuildingSurfaceGroundTemp) {
            return SiteBuildingSurfaceGroundTemps::BuildingSurfaceGTMFactory(state, objectName);
        } else if (objectType == GroundTempObjType::SiteShallowGroundTemp) {
            return SiteShallowGroundTemps::ShallowGTMFactory(state, objectName);
        } else if (objectType == GroundTempObjType::SiteDeepGroundTemp) {
            return SiteDeepGroundTemps::DeepGTMFactory(state, objectName);
        } else if (objectType == GroundTempObjType::SiteFCFactorMethodGroundTemp) {
            return SiteFCFactorMethodGroundTemps::FCFactorGTMFactory(state, objectName);
        } else if (objectType == GroundTempObjType::XingGroundTemp) {
            return XingGroundTempsModel::XingGTMFactory(state, objectName);
        } else {
            assert(false);
            return nullptr;
        }
    }

} // namespace GroundTemperatureManager

} // namespace EnergyPlus
