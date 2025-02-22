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

// Google Test Headers
#include <gtest/gtest.h>

// EnergyPlus Headers
#include "Fixtures/EnergyPlusFixture.hh"
#include <EnergyPlus/Data/EnergyPlusData.hh>
#include <EnergyPlus/OutputProcessor.hh>
#include <EnergyPlus/PollutionModule.hh>

using namespace EnergyPlus;
using namespace EnergyPlus::Pollution;

TEST_F(EnergyPlusFixture, PollutionModule_TestOutputVariables)
{
    // This unit test checks pollution module output variables

    std::string const idf_objects = delimited_string({
        "    Output:EnvironmentalImpactFactors,",
        "      Monthly;                 !- Reporting Frequency",
        "",
        "    EnvironmentalImpactFactors,",
        "      0.3,                     !- District Heating Water Efficiency",
        "      3.0,                     !- District Cooling COP {W/W}",
        "      0.25,                    !- Steam Conversion Efficiency",
        "      80.7272,                 !- Total Carbon Equivalent Emission Factor From N2O {kg/kg}",
        "      6.2727,                  !- Total Carbon Equivalent Emission Factor From CH4 {kg/kg}",
        "      0.2727;                  !- Total Carbon Equivalent Emission Factor From CO2 {kg/kg}",
        "",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement D, July 1998",
        "    ! Chapter 1 External Combustion Sources",
        "    ! Section 1.4 Natural Gas Combustion",
        "    !",
        "",
        "    FuelFactors,",
        "      NaturalGas,              !- Existing Fuel Resource Name",
        "      1.0,                     !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      50.23439,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      3.51641E-02,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      9.62826E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      4.18620E-02,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      9.20964E-04,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      2.51172E-04,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      3.18151E-03,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      2.38613E-03,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      7.95378E-04,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      2.30241E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      1.08841E-07,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      2.09310E-07,             !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement B, October 1996",
        "    ! Chapter 3 Stationary Internal Combustion Sources",
        "    ! Section 3.3 Gasoline And Diesel Industrial Engines",
        "    !",
        "",
        "    FuelFactors,",
        "      Diesel,                  !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      70.50731,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      4.08426E-01,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      0,                       !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      1.89596,                 !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      0,                       !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      1.24678E-01,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      0,                       !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      1.33276E-01,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      0,                       !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      1.50473E-01,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      0,                       !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      0,                       !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement B, October 1996",
        "    ! Chapter 3 Stationary Internal Combustion Sources",
        "    ! Section 3.3 Gasoline And Diesel Industrial Engines",
        "    !",
        "",
        "    FuelFactors,",
        "      Gasoline,                !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      66.20808,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      2.69561E+01,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      0,                       !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      7.00774E-01,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      0,                       !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      3.61135E-02,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      0,                       !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      4.29923E-02,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      0,                       !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      9.02837E-01,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      0,                       !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      0,                       !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    !NOTE:  EnergyPlus does not include LPG as a fuel type, use Propane as the resource name.",
        "    !",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement B, October 1996",
        "    ! Chapter 1 External Combustion Sources",
        "    ! Section 1.5 Liquefied Petroleum Gas Combustion",
        "    !",
        "",
        "    FuelFactors,",
        "      Propane,                 !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      62.70851,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      9.20894E-03,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      8.77042E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      6.57782E-02,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      3.94669E-03,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      3.94669E-04,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      2.19261E-03,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      1.64445E-03,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      5.48151E-04,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      1.75408E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      0,                       !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      0,                       !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement E, September 1998",
        "    ! Chapter 1 External Combustion Sources",
        "    ! Section 1.3 Fuel Oil Combustion",
        "    !",
        "",
        "    FuelFactors,",
        "      FuelOilNo1,                !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      66.02330,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      1.53543E-02,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      6.63304E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      6.14170E-02,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      3.37794E-04,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      4.36061E-01,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      6.14170E-03,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      3.31652E-03,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      2.54881E-03,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      1.04409E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      3.47006E-06,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      4.63699E-06,             !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement E, September 1998",
        "    ! Chapter 1 External Combustion Sources",
        "    ! Section 1.3 Fuel Oil Combustion",
        "    !",
        "",
        "    FuelFactors,",
        "      FuelOilNo2,                !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      68.47998,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      1.53543E-02,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      6.63304E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      7.37004E-02,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      3.37794E-04,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      4.82124E-01,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      6.14170E-03,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      3.31652E-03,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      2.54881E-03,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      1.04409E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      3.47006E-06,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      4.63699E-06,             !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    !NOTE:  EnergyPlus does not include FuelOil#4 as a fuel, use OtherFuel1 as the resource name",
        "    !",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement E, September 1998",
        "    ! Chapter 1 External Combustion Sources",
        "    ! Section 1.3 Fuel Oil Combustion",
        "    !",
        "",
        "    FuelFactors,",
        "      OtherFuel1,               !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      76.77128,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      1.53543E-02,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      6.63304E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      6.14170E-02,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      3.37794E-04,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      4.60628E-01,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      2.14960E-02,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      1.58763E-02,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      5.89603E-03,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      1.04409E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      3.47006E-06,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      4.63699E-06,             !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    ! USEPA Compilation of Air Pollutant Emission Factors, AP-42, Fifth Edition",
        "    ! Volume I: Stationary Point and Area Sources",
        "    ! Supplement E, September 1998",
        "    ! Chapter 1 External Combustion Sources",
        "    ! Section 1.1 Bituminous And Subbituminous Coal Combustion",
        "    !",
        "",
        "    FuelFactors,",
        "      Coal,                    !- Existing Fuel Resource Name",
        "      1,                       !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      91.11052,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      8.26774E-03,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      6.61419E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      1.98426E-01,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      4.96065E-04,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      6.28348E-01,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      1.65355E-01,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      3.80316E-02,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      9.92129E-03,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      9.92129E-04,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      6.94490E-06,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      1.37245E-06,             !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
        "",
        "    ! United States 1999 national average electricity emissions factors based on eGRID, 1605, AirData",
        "    ! United States Water Emission Fuel Factors are the combined thermoelectric and hydroelectric weighted",
        "    ! averages from:",
        "    !   Torcellini, Paul; Long, Nicholas; Judkoff, Ron; Consumptive Water Use for U.S. Power Production;",
        "    !      NREL Report No. TP-550-33905.  Golden, CO; 2003; http://www.nrel.gov/docs/fy04osti/33905.pdf;",
        "    !    or",
        "    !   Torcellini, Paul; Long, Nicholas; Judkoff, Ron; Consumptive Water Use for U.S. Power Production;",
        "    !      ASHRAE Transactions 2003, Vol 110, Part 1.  Atlanta, GA; January 2004;",
        "",
        "    FuelFactors,",
        "      Electricity,             !- Existing Fuel Resource Name",
        "      2.253,                   !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      168.33317,               !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      4.20616E-02,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      1.39858E-03,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      4.10753E-01,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      2.41916E-03,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      8.65731E-01,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      2.95827E-02,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      1.80450E-02,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      1.15377E-02,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      1.10837E-03,             !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      3.72332E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      3.36414E-06,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      0,                       !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      2.10074,                 !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
    });
    ASSERT_TRUE(process_idf(idf_objects));

    state->dataPollution->GetInputFlagPollution = true;
    Pollution::SetupPollutionMeterReporting(*state);

    // Test get output variables for Total Sky Cover and Opaque Sky Cover
    for (int i = 0; i < (int)state->dataPollution->pollFuelFactorList.size(); ++i) {
        PollFuel pollFuel = state->dataPollution->pollFuelFactorList[i];
        std::string_view fuelName = Constant::eFuelNames[(int)pollFuel2fuel[(int)pollFuel]];

        EXPECT_EQ(format("Site:Environmental Impact {} Source Energy", fuelName), state->dataOutputProcessor->RVariableTypes(i * 17 + 1).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} CO2 Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 2).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} CO Emissions Mass", fuelName), state->dataOutputProcessor->RVariableTypes(i * 17 + 3).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} CH4 Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 4).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} NOx Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 5).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} N2O Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 6).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} SO2 Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 7).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} PM Emissions Mass", fuelName), state->dataOutputProcessor->RVariableTypes(i * 17 + 8).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} PM10 Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 9).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} PM2.5 Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 10).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} NH3 Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 11).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} NMVOC Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 12).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} Hg Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 13).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} Pb Emissions Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 14).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} Water Consumption Volume", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 15).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} Nuclear High Level Waste Mass", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 16).VarName);
        EXPECT_EQ(format("Site:Environmental Impact {} Nuclear Low Level Waste Volume", fuelName),
                  state->dataOutputProcessor->RVariableTypes(i * 17 + 17).VarName);
    }

    // Variables specific to the Electricity fuel type
    EXPECT_EQ("Site:Environmental Impact Purchased Electricity Source Energy",
              state->dataOutputProcessor->RVariableTypes((int)state->dataPollution->pollFuelFactorList.size() * 17 + 1).VarName);
    EXPECT_EQ("Site:Environmental Impact Surplus Sold Electricity Source",
              state->dataOutputProcessor->RVariableTypes((int)state->dataPollution->pollFuelFactorList.size() * 17 + 2).VarName);

    // Variables always setup for total carbon equivalent
    EXPECT_EQ("Site:Environmental Impact Total N2O Emissions Carbon Equivalent Mass",
              state->dataOutputProcessor->RVariableTypes((int)state->dataPollution->pollFuelFactorList.size() * 17 + 3).VarName);
    EXPECT_EQ("Site:Environmental Impact Total CH4 Emissions Carbon Equivalent Mass",
              state->dataOutputProcessor->RVariableTypes((int)state->dataPollution->pollFuelFactorList.size() * 17 + 4).VarName);
    EXPECT_EQ("Site:Environmental Impact Total CO2 Emissions Carbon Equivalent Mass",
              state->dataOutputProcessor->RVariableTypes((int)state->dataPollution->pollFuelFactorList.size() * 17 + 5).VarName);
}

TEST_F(EnergyPlusFixture, PollutionModule_TestEnvironmentalImpactFactors)
{

    std::string const idf_objects = delimited_string({
        "    EnvironmentalImpactFactors,",
        "      0.3,                     !- District Heating Efficiency",
        "      3.0,                     !- District Cooling COP {W/W}",
        "      0.3,                     !- Steam Conversion Efficiency",
        "      80.7272,                 !- Total Carbon Equivalent Emission Factor From N2O {kg/kg}",
        "      6.2727,                  !- Total Carbon Equivalent Emission Factor From CH4 {kg/kg}",
        "      0.2727;                  !- Total Carbon Equivalent Emission Factor From CO2 {kg/kg}",
        "",
        "    FuelFactors,",
        "      NaturalGas,              !- Existing Fuel Resource Name",
        "      1.0,                     !- Source Energy Factor {J/J}",
        "      ,                        !- Source Energy Schedule Name",
        "      50.23439,                !- CO2 Emission Factor {g/MJ}",
        "      ,                        !- CO2 Emission Factor Schedule Name",
        "      3.51641E-02,             !- CO Emission Factor {g/MJ}",
        "      ,                        !- CO Emission Factor Schedule Name",
        "      9.62826E-04,             !- CH4 Emission Factor {g/MJ}",
        "      ,                        !- CH4 Emission Factor Schedule Name",
        "      4.18620E-02,             !- NOx Emission Factor {g/MJ}",
        "      ,                        !- NOx Emission Factor Schedule Name",
        "      9.20964E-04,             !- N2O Emission Factor {g/MJ}",
        "      ,                        !- N2O Emission Factor Schedule Name",
        "      2.51172E-04,             !- SO2 Emission Factor {g/MJ}",
        "      ,                        !- SO2 Emission Factor Schedule Name",
        "      3.18151E-03,             !- PM Emission Factor {g/MJ}",
        "      ,                        !- PM Emission Factor Schedule Name",
        "      2.38613E-03,             !- PM10 Emission Factor {g/MJ}",
        "      ,                        !- PM10 Emission Factor Schedule Name",
        "      7.95378E-04,             !- PM2.5 Emission Factor {g/MJ}",
        "      ,                        !- PM2.5 Emission Factor Schedule Name",
        "      0,                       !- NH3 Emission Factor {g/MJ}",
        "      ,                        !- NH3 Emission Factor Schedule Name",
        "      2.30241E-03,             !- NMVOC Emission Factor {g/MJ}",
        "      ,                        !- NMVOC Emission Factor Schedule Name",
        "      1.08841E-07,             !- Hg Emission Factor {g/MJ}",
        "      ,                        !- Hg Emission Factor Schedule Name",
        "      2.09310E-07,             !- Pb Emission Factor {g/MJ}",
        "      ,                        !- Pb Emission Factor Schedule Name",
        "      0,                       !- Water Emission Factor {L/MJ}",
        "      ,                        !- Water Emission Factor Schedule Name",
        "      0,                       !- Nuclear High Level Emission Factor {g/MJ}",
        "      ,                        !- Nuclear High Level Emission Factor Schedule Name",
        "      0;                       !- Nuclear Low Level Emission Factor {m3/MJ}",
    });
    ASSERT_TRUE(process_idf(idf_objects));

    Real64 ExpectedOutput(0.3);
    Real64 AllowedTolerance(0.001);

    Pollution::GetPollutionFactorInput(*state);

    // The get routine should rest the steam conversion efficiency to the default value of 0.25.
    // Previously because of a typo, it would reset it to the input value of zero (or even a negative number).
    ASSERT_NEAR(state->dataPollution->SteamConvEffic, ExpectedOutput, AllowedTolerance);
}
