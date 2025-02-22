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
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>

// ObjexxFCL Headers
#include <ObjexxFCL/Array.functions.hh>
#include <ObjexxFCL/Fmath.hh>
#include <ObjexxFCL/string.functions.hh>

// EnergyPlus Headers
#include "re2/re2.h"
#include <EnergyPlus/Data/EnergyPlusData.hh>
#include <EnergyPlus/DataEnvironment.hh>
#include <EnergyPlus/DataGlobalConstants.hh>
#include <EnergyPlus/DataHVACGlobals.hh>
#include <EnergyPlus/DataIPShortCuts.hh>
#include <EnergyPlus/DataOutputs.hh>
#include <EnergyPlus/DataStringGlobals.hh>
#include <EnergyPlus/DataSystemVariables.hh>
#include <EnergyPlus/General.hh>
#include <EnergyPlus/GlobalNames.hh>
#include <EnergyPlus/InputProcessing/InputProcessor.hh>
#include <EnergyPlus/OutputProcessor.hh>
#include <EnergyPlus/OutputReportPredefined.hh>
#include <EnergyPlus/ResultsFramework.hh>
#include <EnergyPlus/SQLiteProcedures.hh>
#include <EnergyPlus/ScheduleManager.hh>
#include <EnergyPlus/SortAndStringUtilities.hh>
#include <EnergyPlus/UtilityRoutines.hh>

#include <fmt/ostream.h>
#include <milo/dtoa.h>
#include <milo/itoa.h>

namespace EnergyPlus {

namespace OutputProcessor {

    // MODULE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   December 1998
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS MODULE:
    // This module contains the major Output Processor routines.
    // In addition, in this file are several routines which can be called
    // without using the OutputProcessor Module

    // METHODOLOGY EMPLOYED:
    // Lots of pointers and other fancy data stuff.

    // Routines tagged on the end of this module:
    //  AddToOutputVariableList
    //  AssignReportNumber
    //  GenOutputVariablesAuditReport
    //  GetCurrentMeterValue
    //  GetInstantMeterValue
    //  GetInternalVariableValue
    //  GetInternalVariableValueExternalInterface
    //  GetMeteredVariables
    //  GetMeterIndex
    //  GetMeterResourceType
    //  GetNumMeteredVariables
    //  GetVariableKeyCountandType
    //  GetVariableKeys
    //  InitPollutionMeterReporting
    //  ProduceRDDMDD
    //  ReportingThisVariable
    //  SetInitialMeterReportingAndOutputNames
    //  SetupOutputVariable
    //  UpdateDataandReport
    //  UpdateMeterReporting

    // Functions

    inline void ReallocateRVar(EnergyPlusData &state)
    {
        state.dataOutputProcessor->RVariableTypes.redimension(state.dataOutputProcessor->MaxRVariable += RVarAllocInc);
    }

    inline void ReallocateIVar(EnergyPlusData &state)
    {
        state.dataOutputProcessor->IVariableTypes.redimension(state.dataOutputProcessor->MaxIVariable += IVarAllocInc);
    }

    int DetermineMinuteForReporting(EnergyPlusData &state)
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2012
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // When reporting peaks, minutes are used but not necessarily easily calculated.

        Real64 constexpr FracToMin(60.0);
        return ((state.dataGlobal->CurrentTime + state.dataHVACGlobal->SysTimeElapsed) - int(state.dataGlobal->CurrentTime)) * FracToMin;
    }

    void InitializeOutput(EnergyPlusData &state)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine initializes the OutputProcessor data structures.

        auto &op = state.dataOutputProcessor;

        op->RVariableTypes.allocate(RVarAllocInc);
        op->MaxRVariable = RVarAllocInc;

        op->IVariableTypes.allocate(IVarAllocInc);
        op->MaxIVariable = IVarAllocInc;

        // First index is the frequency designation (-1 = each call, etc)
        // Second index is the variable type (1=Average, 2=Sum)
        // Note, Meters always report like Average (with min/max, etc) for hourly and above
        // FreqNotice( 1, -1 ) = " !Each Call";
        // FreqNotice( 1, 0 ) = " !TimeStep";
        // FreqNotice( 1, 1 ) = " !Hourly";
        // FreqNotice( 1, 2 ) = " !Daily [Value,Min,Hour,Minute,Max,Hour,Minute]";
        // FreqNotice( 1, 3 ) = " !Monthly [Value,Min,Day,Hour,Minute,Max,Day,Hour,Minute]";
        // FreqNotice( 1, 4 ) = " !RunPeriod [Value,Min,Month,Day,Hour,Minute,Max,Month,Day,Hour,Minute]";
        // FreqNotice( 2, -1 ) = " !Each Call";
        // FreqNotice( 2, 0 ) = " !TimeStep";
        // FreqNotice( 2, 1 ) = " !Hourly";
        // FreqNotice( 2, 2 ) = " !Daily [Value,Min,Hour,Minute,Max,Hour,Minute]";
        // FreqNotice( 2, 3 ) = " !Monthly [Value,Min,Day,Hour,Minute,Max,Day,Hour,Minute]";
        // FreqNotice( 2, 4 ) = " !RunPeriod [Value,Min,Month,Day,Hour,Minute,Max,Month,Day,Hour,Minute]";

        op->ReportList.allocate(500);
        op->NumReportList = 500;
        op->ReportList = 0;
        op->NumExtraVars = 0;

        // Initialize end use category names - the indices must match up with endUseNames in OutputReportTabular
        op->EndUseCategory.allocate(static_cast<int>(Constant::EndUse::Num));
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Heating) + 1).Name = "Heating";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Cooling) + 1).Name = "Cooling";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::InteriorLights) + 1).Name = "InteriorLights";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::ExteriorLights) + 1).Name = "ExteriorLights";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::InteriorEquipment) + 1).Name = "InteriorEquipment";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::ExteriorEquipment) + 1).Name = "ExteriorEquipment";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Fans) + 1).Name = "Fans";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Pumps) + 1).Name = "Pumps";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::HeatRejection) + 1).Name = "HeatRejection";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Humidification) + 1).Name = "Humidifier";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::HeatRecovery) + 1).Name = "HeatRecovery";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::WaterSystem) + 1).Name = "WaterSystems";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Refrigeration) + 1).Name = "Refrigeration";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Cogeneration) + 1).Name = "Cogeneration";

        // Initialize display names for output table - this could go away if end use key names are changed to match
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Heating) + 1).DisplayName = "Heating";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Cooling) + 1).DisplayName = "Cooling";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::InteriorLights) + 1).DisplayName = "Interior Lighting";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::ExteriorLights) + 1).DisplayName = "Exterior Lighting";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::InteriorEquipment) + 1).DisplayName = "Interior Equipment";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::ExteriorEquipment) + 1).DisplayName = "Exterior Equipment";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Fans) + 1).DisplayName = "Fans";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Pumps) + 1).DisplayName = "Pumps";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::HeatRejection) + 1).DisplayName = "Heat Rejection";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Humidification) + 1).DisplayName = "Humidification";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::HeatRecovery) + 1).DisplayName = "Heat Recovery";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::WaterSystem) + 1).DisplayName = "Water Systems";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Refrigeration) + 1).DisplayName = "Refrigeration";
        op->EndUseCategory(static_cast<int>(Constant::EndUse::Cogeneration) + 1).DisplayName = "Generators";

        op->OutputInitialized = true;

        op->TimeStepZoneSec = double(state.dataGlobal->MinutesPerTimeStep) * 60.0;

        state.files.mtd.ensure_open(state, "InitializeMeters", state.files.outputControl.mtd);
    }

    void SetupTimePointers(EnergyPlusData &state,
                           OutputProcessor::SOVTimeStepType const TimeStepTypeKey, // Which timestep is being set up, 'Zone'=1, 'HVAC'=2
                           Real64 &TimeStep                                        // The timestep variable.  Used to get the address
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine sets up the derived type for the output processor that
        // contains pointers to the TimeStep values used in the simulation.

        // METHODOLOGY EMPLOYED:
        // Indicate that the TimeStep passed in is a target for the pointer
        // attributes in the derived types.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        // ValidateTimeStepType will throw a Fatal if not valid
        TimeStepType timeStepType = ValidateTimeStepType(state, TimeStepTypeKey);

        TimeSteps tPtr;
        tPtr.TimeStep = &TimeStep;
        if (!state.dataOutputProcessor->TimeValue.insert(std::make_pair(timeStepType, tPtr)).second) {
            // The element was already present... shouldn't happen
            ShowFatalError(state, format("SetupTimePointers was already called for {}", sovTimeStepTypeStrings[(int)TimeStepTypeKey]));
        }
    }

    void CheckReportVariable(EnergyPlusData &state, std::string_view const KeyedValue, std::string_view const VarName)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine will get the report variable information from input and
        // determine if this variable (KeyedValue and VariableName) should be reported
        // and, if so, what frequency to report.

        // This routine is called when SetupOutputVariable is called with no "optional"
        // Reporting Frequency.  It is expected that SetupOutputVariable would only be
        // called once for each keyed variable to be triggered for output (from the input
        // requests).  The optional report frequency would only be used for debugging
        // purposes.  Therefore, this routine will collect all occasions where this
        // passed variablename would be reported from the requested input.  It builds
        // a list of these requests (ReportList) so that the calling routine can propagate
        // the requests into the correct data structure.

        // METHODOLOGY EMPLOYED:
        // This instance being requested will always have a key associated with it.  Matching
        // instances (from input) may or may not have keys, but only one instance of a reporting
        // frequency per variable is allowed.  ReportList will be populated with ReqRepVars indices
        // of those extra things from input that satisfy this condition.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:

        // Make sure that input has been read
        GetReportVariableInput(state);

        auto &op = state.dataOutputProcessor;

        // Zero out the array / counter we use to determine if there are duplicates
        op->NumExtraVars = 0;
        op->ReportList = 0;

        for (int i = 1; i <= op->NumOfReqVariables; ++i) {
            auto &reqRepVar = op->ReqRepVars(i);

            if (!Util::SameString(reqRepVar.VarName, VarName)) {
                continue;
            }

            if (!reqRepVar.Key.empty() && !(reqRepVar.is_simple_string && Util::SameString(reqRepVar.Key, KeyedValue)) &&
                !(!reqRepVar.is_simple_string && RE2::FullMatch(std::string{KeyedValue}, *(reqRepVar.case_insensitive_pattern)))) {
                continue;
            }

            // A match. Make sure doesn't duplicate
            reqRepVar.Used = true;
            bool Dup = false;
            // op->ReportList is allocated to a large value, so we can't use a std::find_if on it
            for (int Loop1 = 1; Loop1 <= op->NumExtraVars; ++Loop1) {
                if (op->ReqRepVars(op->ReportList(Loop1)).frequency == reqRepVar.frequency &&
                    op->ReqRepVars(op->ReportList(Loop1)).SchedPtr == reqRepVar.SchedPtr) {
                    Dup = true;
                    break;
                }
            }

            if (!Dup) {
                ++op->NumExtraVars;
                if (op->NumExtraVars == op->NumReportList) {
                    op->ReportList.redimension(op->NumReportList += 100, 0);
                }
                op->ReportList(op->NumExtraVars) = i;
            }
        }
    }

    static std::string frequencyNotice([[maybe_unused]] StoreType storeType, ReportingFrequency reportingInterval)
    {
        switch (reportingInterval) {
        case ReportingFrequency::EachCall:
            return " !Each Call";
            break;
        case ReportingFrequency::TimeStep:
            return " !TimeStep";
            break;
        case ReportingFrequency::Hourly:
            return " !Hourly";
            break;
        case ReportingFrequency::Daily:
            return " !Daily [Value,Min,Hour,Minute,Max,Hour,Minute]";
            break;
        case ReportingFrequency::Monthly:
            return " !Monthly [Value,Min,Day,Hour,Minute,Max,Day,Hour,Minute]";
            break;
        case ReportingFrequency::Yearly:
            return " !Annual [Value,Min,Month,Day,Hour,Minute,Max,Month,Day,Hour,Minute]";
            break;
        case ReportingFrequency::Simulation:
            return " !RunPeriod [Value,Min,Month,Day,Hour,Minute,Max,Month,Day,Hour,Minute]";
            break;
        default:
            return " !Hourly";
        }
    }

    std::string reportingFrequency(ReportingFrequency reportingInterval)
    {
        switch (reportingInterval) {
        case ReportingFrequency::EachCall:
            return "Each Call";
            break;
        case ReportingFrequency::TimeStep:
            return "TimeStep";
            break;
        case ReportingFrequency::Hourly:
            return "Hourly";
            break;
        case ReportingFrequency::Daily:
            return "Daily";
            break;
        case ReportingFrequency::Monthly:
            return "Monthly";
            break;
        case ReportingFrequency::Yearly:
            return "Annual";
            break;
        case ReportingFrequency::Simulation:
            return "RunPeriod";
            break;
        default:
            return "Hourly";
        }
    }

    ReportingFrequency determineFrequency(EnergyPlusData &state, const std::string_view FreqString)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       December 2017; Jason DeGraw
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine looks at the passed in report frequency string and
        // determines the reporting frequency.

        // METHODOLOGY EMPLOYED:
        // na

        // REFERENCES:
        //       \field Reporting Frequency
        //       \type choice
        //       \key Detailed
        //       \note Detailed lists every instance (i.e. HVAC variable timesteps)
        //       \key Timestep
        //       \note Timestep refers to the zone Timestep/Number of Timesteps in hour value
        //       \note RunPeriod, Environment, and Annual are the same
        //       \key Hourly
        //       \key Daily
        //       \key Monthly
        //       \key RunPeriod
        //       \key Environment
        //       \key Annual
        //       \default Hourly
        //       \note RunPeriod and Environment are synonymous

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // SUBROUTINE PARAMETER DEFINITIONS:
        static std::vector<std::string> const PossibleFreqs({"DETA", "TIME", "HOUR", "DAIL", "MONT", "RUNP", "ENVI", "ANNU"});
        //=(/'detail','Timestep','Hourly','Daily','Monthly','RunPeriod','Environment','Annual'/)
        static std::vector<std::string> const ExactFreqStrings(
            {"Detailed", "Timestep", "Hourly", "Daily", "Monthly", "RunPeriod", "Environment", "Annual"});
        static std::vector<std::string> const ExactFreqStringsUpper(
            {"DETAILED", "TIMESTEP", "HOURLY", "DAILY", "MONTHLY", "RUNPERIOD", "ENVIRONMENT", "ANNUAL"});
        // Vector of the result, was { -1, 0, 1, 2, 3, 4, 4, 4 } before the addition of Yearly;
        static std::vector<ReportingFrequency> const FreqValues({ReportingFrequency::EachCall,
                                                                 ReportingFrequency::TimeStep,
                                                                 ReportingFrequency::Hourly,
                                                                 ReportingFrequency::Daily,
                                                                 ReportingFrequency::Monthly,
                                                                 ReportingFrequency::Simulation,
                                                                 ReportingFrequency::Simulation,
                                                                 ReportingFrequency::Yearly});
        // note: runperiod and environment are synonomous

        // INTERFACE BLOCK SPECIFICATIONS:
        // na

        // DERIVED TYPE DEFINITIONS:
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:

        ReportingFrequency ReportFreq(ReportingFrequency::Hourly); // Default
        // TODO: I think it's supposed to be upper case already, but tests aren't doing that at least...
        const std::string FreqStringUpper = Util::makeUPPER(FreqString);
        std::string::size_type const LenString = min(len(FreqString), static_cast<std::string::size_type>(4u));

        if (LenString < 4u) {
            return ReportFreq;
        }

        std::string const FreqStringTrim(FreqStringUpper.substr(0, LenString));
        for (unsigned Loop = 0; Loop < FreqValues.size(); ++Loop) {
            if (FreqStringTrim == PossibleFreqs[Loop]) {
                if (FreqStringUpper != ExactFreqStringsUpper[Loop]) {
                    ShowWarningError(state, format("DetermineFrequency: Entered frequency=\"{}\" is not an exact match to key strings.", FreqString));
                    ShowContinueError(state, format("Frequency={} will be used.", ExactFreqStrings[Loop]));
                }
                ReportFreq = std::max(FreqValues[Loop], state.dataOutputProcessor->minimumReportFrequency);
                break;
            }
        }
        return ReportFreq;
    }

    void GetReportVariableInput(EnergyPlusData &state)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       December 2017; Jason DeGraw
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine gets the requested report variables from
        // the input file.
        // Report Variable,
        //        \memo each Report Variable command picks variables to be put onto the standard output file (.eso)
        //        \memo some variables may not be reported for every simulation
        //   A1 , \field Key_Value
        //        \note use '*' (without quotes) to apply this variable to all keys
        //   A2 , \field Variable_Name
        //   A3 , \field Reporting_Frequency
        //        \type choice
        //        \key detailed
        //        \key timestep
        //        \key hourly
        //        \key daily
        //        \key monthly
        //        \key runperiod
        //   A4 ; \field Schedule_Name
        //        \type object-list
        //        \object-list ScheduleNames

        // Using/Aliasing
        using ScheduleManager::GetScheduleIndex;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop;
        int NumAlpha;
        int NumNumbers;
        int IOStat;
        bool ErrorsFound(false); // If errors detected in input
        std::string cCurrentModuleObject;
        Array1D_string cAlphaArgs(4);
        Array1D_string cAlphaFieldNames(4);
        Array1D_bool lAlphaFieldBlanks(4);
        Array1D<Real64> rNumericArgs(1);
        Array1D_string cNumericFieldNames(1);
        Array1D_bool lNumericFieldBlanks(1);
        auto &op = state.dataOutputProcessor;

        // Bail out if the input has already been read in
        if (!op->GetOutputInputFlag) {
            return;
        }
        op->GetOutputInputFlag = false;

        // First check environment variable to see of possible override for minimum reporting frequency
        if (!state.dataSysVars->MinReportFrequency.empty()) {
            // Formats
            static constexpr std::string_view Format_800("! <Minimum Reporting Frequency (overriding input value)>, Value, Input Value\n");
            static constexpr std::string_view Format_801(" Minimum Reporting Frequency, {},{}\n");
            op->minimumReportFrequency = determineFrequency(state, state.dataSysVars->MinReportFrequency);
            print(state.files.eio, Format_800);
            print(
                state.files.eio, Format_801, frequencyNotice(StoreType::Averaged, op->minimumReportFrequency), state.dataSysVars->MinReportFrequency);
        }

        cCurrentModuleObject = "Output:Variable";
        op->NumOfReqVariables = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);
        op->ReqRepVars.allocate(op->NumOfReqVariables);

        for (Loop = 1; Loop <= op->NumOfReqVariables; ++Loop) {

            state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                     cCurrentModuleObject,
                                                                     Loop,
                                                                     cAlphaArgs,
                                                                     NumAlpha,
                                                                     rNumericArgs,
                                                                     NumNumbers,
                                                                     IOStat,
                                                                     lNumericFieldBlanks,
                                                                     lAlphaFieldBlanks,
                                                                     cAlphaFieldNames,
                                                                     cNumericFieldNames);

            // Check for duplicates?
            auto &reqRepVar = op->ReqRepVars(Loop);
            reqRepVar.Key = cAlphaArgs(1);
            if (reqRepVar.Key == "*") {
                reqRepVar.Key = std::string();
            }

            bool is_simple_string = !DataOutputs::isKeyRegexLike(reqRepVar.Key);
            reqRepVar.is_simple_string = is_simple_string;
            if (!is_simple_string) {
                reqRepVar.case_insensitive_pattern = std::make_shared<RE2>("(?i)" + reqRepVar.Key);
            }

            std::string::size_type const lbpos = index(cAlphaArgs(2), '['); // Remove Units designation if user put it in
            if (lbpos != std::string::npos) {
                cAlphaArgs(2).erase(lbpos);
                // right trim
                cAlphaArgs(2) = cAlphaArgs(2).substr(0, std::min(cAlphaArgs(2).find_last_not_of(" \f\n\r\t\v") + 1, cAlphaArgs(2).size()));
            }
            reqRepVar.VarName = cAlphaArgs(2);

            reqRepVar.frequency = determineFrequency(state, cAlphaArgs(3));

            // Schedule information
            reqRepVar.SchedName = cAlphaArgs(4);
            if (not_blank(reqRepVar.SchedName)) {
                reqRepVar.SchedPtr = GetScheduleIndex(state, reqRepVar.SchedName);
                if (reqRepVar.SchedPtr == 0) {
                    ShowSevereError(state,
                                    format("GetReportVariableInput: {}=\"{}:{}\" invalid {}=\"{}\" - not found.",
                                           cCurrentModuleObject,
                                           cAlphaArgs(1),
                                           reqRepVar.VarName,
                                           cAlphaFieldNames(4),
                                           reqRepVar.SchedName));
                    ErrorsFound = true;
                }
            } else {
                reqRepVar.SchedPtr = 0;
            }

            reqRepVar.Used = false;
        }

        if (ErrorsFound) {
            ShowFatalError(state, format("GetReportVariableInput:{}: errors in input.", cCurrentModuleObject));
        }
    }

    void ProduceMinMaxString(std::string &String,                // Current value
                             int const DateValue,                // Date of min/max
                             ReportingFrequency const ReportFreq // Reporting Frequency
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine produces the appropriate min/max string depending
        // on the reporting frequency.

        // METHODOLOGY EMPLOYED:
        // Prior to calling this routine, the basic value string will be
        // produced, but DecodeMonDayHrMin will not have been called.

        // SUBROUTINE PARAMETER DEFINITIONS:
        static constexpr std::string_view DayFormat("{},{:2},{:2}");
        static constexpr std::string_view MonthFormat("{},{:2},{:2},{:2}");
        static constexpr std::string_view EnvrnFormat("{},{:2},{:2},{:2},{:2}");

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Mon;
        int Day;
        int Hour;
        int Minute;
        General::DecodeMonDayHrMin(DateValue, Mon, Day, Hour, Minute);

        switch (ReportFreq) {
        case ReportingFrequency::Daily:
            String = format(DayFormat, strip(String), Hour, Minute);
            return;
        case ReportingFrequency::Monthly:
            String = format(MonthFormat, strip(String), Day, Hour, Minute);
            return;
        case ReportingFrequency::Yearly:
        case ReportingFrequency::Simulation:
            String = format(EnvrnFormat, strip(String), Mon, Day, Hour, Minute);
            return;
        default: // Each, TimeStep, Hourly dont have this
            String = std::string();
            return;
        }
    }

    TimeStepType ValidateTimeStepType(EnergyPlusData &state,
                                      OutputProcessor::SOVTimeStepType const TimeStepTypeKey) // Index type (Zone, HVAC) for variables
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function validates the requested "index" type and returns
        // the proper value for use inside the OutputProcessor.

        // METHODOLOGY EMPLOYED:
        // Look it up in a list of valid index types.
        switch (TimeStepTypeKey) {
        case OutputProcessor::SOVTimeStepType::Zone:
            return TimeStepType::Zone;
        case OutputProcessor::SOVTimeStepType::HVAC:
        case OutputProcessor::SOVTimeStepType::System:
        case OutputProcessor::SOVTimeStepType::Plant:
            return TimeStepType::System;
        case OutputProcessor::SOVTimeStepType::Invalid:
        case OutputProcessor::SOVTimeStepType::Num:
            ShowFatalError(state, "Bad SOVTimeStepType passed to ValidateTimeStepType");
        }
        return TimeStepType::System; // compiler doesn't understand that ShowFatalError aborts
    }

    std::string StandardTimeStepTypeKey(TimeStepType const timeStepType)
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function gives the standard string for the index type
        // given.

        // METHODOLOGY EMPLOYED:
        // Look it up in a list of valid index types.

        if (timeStepType == TimeStepType::Zone) {
            return "Zone";
        } else if (timeStepType == TimeStepType::System) {
            return "HVAC";
        } else {
            return "UNKW";
        }
    }

    StoreType validateVariableType(EnergyPlusData &state, OutputProcessor::SOVStoreType const VariableTypeKey)
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   December 1998
        //       MODIFIED       December 2017; Jason DeGraw
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function validates the VariableTypeKey passed to the SetupVariable
        // routine and assigns it the value used in the OutputProcessor.

        // FUNCTION LOCAL VARIABLE DECLARATIONS:
        switch (VariableTypeKey) {
        case OutputProcessor::SOVStoreType::State:
        case OutputProcessor::SOVStoreType::Average:
            return StoreType::Averaged;
        case OutputProcessor::SOVStoreType::NonState:
        case OutputProcessor::SOVStoreType::Summed:
            return StoreType::Summed;
        case OutputProcessor::SOVStoreType::Invalid:
        case OutputProcessor::SOVStoreType::Num:
            ShowFatalError(state, "Bad SOVStoreType passed to validateVariableType");
        }
        return StoreType::Summed; // compiler doesn't understand that ShowFatalError aborts
    }

    std::string standardVariableTypeKey(StoreType const VariableType)
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Linda K. Lawrie
        //       DATE WRITTEN   July 1999
        //       MODIFIED       December 2017; Jason DeGraw
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function gives the standard string for the variable type
        // given.

        // METHODOLOGY EMPLOYED:
        // From variable type value, produce proper string.

        // TODO: Use a constexpr std::array<std::string_view, ... for this
        switch (VariableType) {
        case StoreType::Averaged:
            return "Average";
        case StoreType::Summed:
            return "Sum";
        default:
            return "Unknown";
        }
    }

    // *****************************************************************************
    // The following routines implement Energy Meters in EnergyPlus.
    // *****************************************************************************

    void GetCustomMeterInput(EnergyPlusData &state, bool &ErrorsFound)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2006
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This routine will help implement "custom"/user defined meters.  However, it must be called after all
        // the other meters are set up and all report variables are established.

        // METHODOLOGY EMPLOYED:
        // na

        // REFERENCES:
        // Processes the objects:
        // Meter:Custom,
        //    \extensible:2 - repeat last two fields, remembering to remove ; from "inner" fields.
        //    \memo Used to allow users to combine specific variables and/or meters into
        //    \memo "custom" meter configurations.
        //    A1,  \field Name
        //         \required-field
        //         \reference CustomMeterNames
        //    A2,  \field Fuel Type
        //         \type choice
        //         \key Electricity
        //         \key NaturalGas
        //         \key PropaneGas
        //         \key FuelOilNo1
        //         \key FuelOilNo2
        //         \key Coal
        //         \key Diesel
        //         \key Gasoline
        //         \key Water
        //         \key Generic
        //         \key OtherFuel1
        //         \key OtherFuel2
        //    A3,  \field Key Name 1
        //         \required-field
        //         \begin-extensible
        //    A4,  \field Report Variable or Meter Name 1
        //         \required-field
        // <etc>
        // AND
        // Meter:CustomDecrement,
        //    \extensible:2 - repeat last two fields, remembering to remove ; from "inner" fields.
        //    \memo Used to allow users to combine specific variables and/or meters into
        //    \memo "custom" meter configurations.
        //    A1,  \field Name
        //         \required-field
        //         \reference CustomMeterNames
        //    A2,  \field Fuel Type
        //         \type choice
        //         \key Electricity
        //         \key NaturalGas
        //         \key PropaneGas
        //         \key FuelOilNo1
        //         \key FuelOilNo2
        //         \key Coal
        //         \key Diesel
        //         \key Gasoline
        //         \key Water
        //         \key Generic
        //         \key OtherFuel1
        //         \key OtherFuel2
        //    A3,  \field Source Meter Name
        //         \required-field
        //    A4,  \field Key Name 1
        //         \required-field
        //         \begin-extensible
        //    A5,  \field Report Variable or Meter Name 1
        //         \required-field
        // <etc>

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        auto &op = state.dataOutputProcessor;
        int NumAlpha;
        int NumNumbers;
        int Loop;
        int IOStat;
        int NumCustomMeters;
        int NumCustomDecMeters;
        int fldIndex;
        bool KeyIsStar;
        Array1D_string NamesOfKeys;                                    // Specific key name
        Array1D_int IndexesForKeyVar;                                  // Array index
        OutputProcessor::Unit UnitsVar(OutputProcessor::Unit::None);   // Units enumeration
        OutputProcessor::Unit MeterUnits(OutputProcessor::Unit::None); // Units enumeration
        int KeyCount;
        VariableType TypeVar;
        OutputProcessor::StoreType AvgSumVar;
        OutputProcessor::TimeStepType StepTypeVar;
        int iKey;
        int iKey1;
        bool MeterCreated;
        Array1D_int VarsOnCustomMeter;
        int MaxVarsOnCustomMeter;
        int NumVarsOnCustomMeter;
        Array1D_int VarsOnSourceMeter;
        int MaxVarsOnSourceMeter;
        int NumVarsOnSourceMeter;
        int iOnMeter;
        int WhichMeter;
        bool errFlag;
        bool BigErrorsFound;
        bool testa;
        bool testb;
        bool Tagged; // variable is appropriate to put on meter
        std::string::size_type lbrackPos;

        BigErrorsFound = false;
        auto &cCurrentModuleObject = state.dataIPShortCut->cCurrentModuleObject;

        cCurrentModuleObject = "Meter:Custom";
        NumCustomMeters = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);

        // make list of names for all Meter:Custom since they cannot refer to other Meter:Custom's
        std::unordered_set<std::string> namesOfMeterCustom;
        namesOfMeterCustom.reserve(NumCustomMeters);

        for (Loop = 1; Loop <= NumCustomMeters; ++Loop) {
            state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                     cCurrentModuleObject,
                                                                     Loop,
                                                                     state.dataIPShortCut->cAlphaArgs,
                                                                     NumAlpha,
                                                                     state.dataIPShortCut->rNumericArgs,
                                                                     NumNumbers,
                                                                     IOStat,
                                                                     state.dataIPShortCut->lNumericFieldBlanks,
                                                                     state.dataIPShortCut->lAlphaFieldBlanks,
                                                                     state.dataIPShortCut->cAlphaFieldNames,
                                                                     state.dataIPShortCut->cNumericFieldNames);
            namesOfMeterCustom.emplace(Util::makeUPPER(state.dataIPShortCut->cAlphaArgs(1)));
        }

        for (Loop = 1; Loop <= NumCustomMeters; ++Loop) {
            state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                     cCurrentModuleObject,
                                                                     Loop,
                                                                     state.dataIPShortCut->cAlphaArgs,
                                                                     NumAlpha,
                                                                     state.dataIPShortCut->rNumericArgs,
                                                                     NumNumbers,
                                                                     IOStat,
                                                                     state.dataIPShortCut->lNumericFieldBlanks,
                                                                     state.dataIPShortCut->lAlphaFieldBlanks,
                                                                     state.dataIPShortCut->cAlphaFieldNames,
                                                                     state.dataIPShortCut->cNumericFieldNames);
            lbrackPos = index(state.dataIPShortCut->cAlphaArgs(1), '[');
            if (lbrackPos != std::string::npos) state.dataIPShortCut->cAlphaArgs(1).erase(lbrackPos);
            MeterCreated = false;
            if (GlobalNames::VerifyUniqueInterObjectName(state,
                                                         op->UniqueMeterNames,
                                                         state.dataIPShortCut->cAlphaArgs(1),
                                                         cCurrentModuleObject,
                                                         state.dataIPShortCut->cAlphaFieldNames(1),
                                                         ErrorsFound)) {
                continue;
            }
            if (allocated(VarsOnCustomMeter)) VarsOnCustomMeter.deallocate();
            VarsOnCustomMeter.allocate(1000);
            VarsOnCustomMeter = 0;
            MaxVarsOnCustomMeter = 1000;
            NumVarsOnCustomMeter = 0;
            // check if any fields reference another Meter:Custom
            int found = 0;
            for (fldIndex = 4; fldIndex <= NumAlpha; fldIndex += 2) {
                if (namesOfMeterCustom.find(Util::makeUPPER(state.dataIPShortCut->cAlphaArgs(fldIndex))) != namesOfMeterCustom.end()) {
                    found = fldIndex;
                    break;
                }
            }
            if (found != 0) {
                ShowWarningError(state,
                                 format("{}=\"{}\", contains a reference to another {} in field: {}=\"{}\".",
                                        cCurrentModuleObject,
                                        state.dataIPShortCut->cAlphaArgs(1),
                                        cCurrentModuleObject,
                                        state.dataIPShortCut->cAlphaFieldNames(found),
                                        state.dataIPShortCut->cAlphaArgs(found)));
                continue;
            }

            for (fldIndex = 3; fldIndex <= NumAlpha; fldIndex += 2) {
                if (state.dataIPShortCut->cAlphaArgs(fldIndex) == "*" || state.dataIPShortCut->lAlphaFieldBlanks(fldIndex)) {
                    KeyIsStar = true;
                    state.dataIPShortCut->cAlphaArgs(fldIndex) = "*";
                } else {
                    KeyIsStar = false;
                }
                if (state.dataIPShortCut->lAlphaFieldBlanks(fldIndex + 1)) {
                    ShowSevereError(state,
                                    format("{}=\"{}\", blank {}.",
                                           cCurrentModuleObject,
                                           state.dataIPShortCut->cAlphaArgs(1),
                                           state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1)));
                    ShowContinueError(state, "...cannot create custom meter.");
                    BigErrorsFound = true;
                    continue;
                }
                if (BigErrorsFound) continue;
                // Don't build/check things out if there were errors anywhere.  Use "GetVariableKeys" to map to actual variables...
                lbrackPos = index(state.dataIPShortCut->cAlphaArgs(fldIndex + 1), '[');
                if (lbrackPos != std::string::npos) state.dataIPShortCut->cAlphaArgs(fldIndex + 1).erase(lbrackPos);
                Tagged = false;
                GetVariableKeyCountandType(
                    state, state.dataIPShortCut->cAlphaArgs(fldIndex + 1), KeyCount, TypeVar, AvgSumVar, StepTypeVar, UnitsVar);
                if (TypeVar == VariableType::NotFound) {
                    ShowWarningError(state,
                                     format("{}=\"{}\", invalid {}=\"{}\".",
                                            cCurrentModuleObject,
                                            state.dataIPShortCut->cAlphaArgs(1),
                                            state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                            state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                    ShowContinueError(state, "...will not be shown with the Meter results.");
                    continue;
                }
                if (!MeterCreated) {
                    MeterUnits = UnitsVar; // meter units are same as first variable on custom meter
                    AddMeter(state, state.dataIPShortCut->cAlphaArgs(1), UnitsVar, std::string(), std::string(), std::string(), std::string());
                    op->EnergyMeters(op->NumEnergyMeters).TypeOfMeter = MtrType::Custom;
                    // Can't use resource type in AddMeter cause it will confuse it with other meters.  So, now:
                    GetStandardMeterResourceType(
                        state, op->EnergyMeters(op->NumEnergyMeters).ResourceType, Util::makeUPPER(state.dataIPShortCut->cAlphaArgs(2)), errFlag);
                    if (errFlag) {
                        ShowContinueError(state, format("..on {}=\"{}\".", cCurrentModuleObject, state.dataIPShortCut->cAlphaArgs(1)));
                        BigErrorsFound = true;
                    }
                    DetermineMeterIPUnits(state,
                                          op->EnergyMeters(op->NumEnergyMeters).RT_forIPUnits,
                                          op->EnergyMeters(op->NumEnergyMeters).ResourceType,
                                          UnitsVar,
                                          errFlag);
                    if (errFlag) {
                        ShowContinueError(state, format("..on {}=\"{}\".", cCurrentModuleObject, state.dataIPShortCut->cAlphaArgs(1)));
                        ShowContinueError(state, "..requests for IP units from this meter will be ignored.");
                    }
                    //        EnergyMeters(NumEnergyMeters)%RT_forIPUnits=DetermineMeterIPUnits(EnergyMeters(NumEnergyMeters)%ResourceType,UnitsVar)
                    MeterCreated = true;
                }
                if (UnitsVar != MeterUnits) {
                    ShowWarningError(state,
                                     format("{}=\"{}\", differing units in {}=\"{}\".",
                                            cCurrentModuleObject,
                                            state.dataIPShortCut->cAlphaArgs(1),
                                            state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                            state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                    ShowContinueError(state,
                                      format("...will not be shown with the Meter results; units for meter={}, units for this variable={}.",
                                             unitEnumToString(MeterUnits),
                                             unitEnumToString(UnitsVar)));
                    continue;
                }
                if ((TypeVar == VariableType::Real || TypeVar == VariableType::Integer) && AvgSumVar == StoreType::Summed) {
                    Tagged = true;
                    NamesOfKeys.allocate(KeyCount);
                    IndexesForKeyVar.allocate(KeyCount);
                    GetVariableKeys(state, state.dataIPShortCut->cAlphaArgs(fldIndex + 1), TypeVar, NamesOfKeys, IndexesForKeyVar);
                    iOnMeter = 0;
                    if (KeyIsStar) {
                        for (iKey = 1; iKey <= KeyCount; ++iKey) {
                            ++NumVarsOnCustomMeter;
                            if (NumVarsOnCustomMeter > MaxVarsOnCustomMeter) {
                                VarsOnCustomMeter.redimension(MaxVarsOnCustomMeter += 100, 0);
                            }
                            VarsOnCustomMeter(NumVarsOnCustomMeter) = IndexesForKeyVar(iKey);
                            iOnMeter = 1;
                        }
                        if (iOnMeter == 0) {
                            ShowSevereError(state,
                                            format("{}=\"{}\", invalid (all keys) {}=\"{}\".",
                                                   cCurrentModuleObject,
                                                   state.dataIPShortCut->cAlphaArgs(1),
                                                   state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                                   state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                            ErrorsFound = true;
                        }
                    } else { // Key is not "*"
                        for (iKey = 1; iKey <= KeyCount; ++iKey) {
                            if (NamesOfKeys(iKey) != state.dataIPShortCut->cAlphaArgs(fldIndex)) continue;
                            ++NumVarsOnCustomMeter;
                            if (NumVarsOnCustomMeter > MaxVarsOnCustomMeter) {
                                VarsOnCustomMeter.redimension(MaxVarsOnCustomMeter += 100, 0);
                            }
                            VarsOnCustomMeter(NumVarsOnCustomMeter) = IndexesForKeyVar(iKey);
                            iOnMeter = 1;
                        }
                        if (iOnMeter == 0) {
                            ShowSevereError(state,
                                            format("{}=\"{}\", invalid {}:{}",
                                                   cCurrentModuleObject,
                                                   state.dataIPShortCut->cAlphaArgs(1),
                                                   state.dataIPShortCut->cAlphaArgs(fldIndex),
                                                   state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                            ErrorsFound = true;
                        }
                    }
                    NamesOfKeys.deallocate();
                    IndexesForKeyVar.deallocate();
                }
                if (TypeVar == VariableType::Meter && AvgSumVar == StoreType::Summed) {
                    Tagged = true;
                    NamesOfKeys.allocate(KeyCount);
                    IndexesForKeyVar.allocate(KeyCount);
                    GetVariableKeys(state, state.dataIPShortCut->cAlphaArgs(fldIndex + 1), TypeVar, NamesOfKeys, IndexesForKeyVar);
                    WhichMeter = IndexesForKeyVar(1);
                    NamesOfKeys.deallocate();
                    IndexesForKeyVar.deallocate();
                    // for meters there will only be one key...  but it has variables associated...
                    for (iOnMeter = 1; iOnMeter <= op->NumVarMeterArrays; ++iOnMeter) {
                        if (!any_eq(op->VarMeterArrays(iOnMeter).OnMeters, WhichMeter)) continue;
                        ++NumVarsOnCustomMeter;
                        if (NumVarsOnCustomMeter > MaxVarsOnCustomMeter) {
                            VarsOnCustomMeter.redimension(MaxVarsOnCustomMeter += 100, 0);
                        }
                        VarsOnCustomMeter(NumVarsOnCustomMeter) = op->VarMeterArrays(iOnMeter).RepVariable;
                    }
                }
                if (!Tagged) { // couldn't find place for this item on a meter
                    if (AvgSumVar != StoreType::Summed) {
                        ShowWarningError(state,
                                         format("{}=\"{}\", variable not summed variable {}=\"{}\".",
                                                cCurrentModuleObject,
                                                state.dataIPShortCut->cAlphaArgs(1),
                                                state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                                state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                        ShowContinueError(state,
                                          format("...will not be shown with the Meter results; units for meter={}, units for this variable={}.",
                                                 unitEnumToString(MeterUnits),
                                                 unitEnumToString(UnitsVar)));
                    }
                }
            }
            // Check for duplicates
            for (iKey = 1; iKey <= NumVarsOnCustomMeter; ++iKey) {
                if (VarsOnCustomMeter(iKey) == 0) continue;
                for (iKey1 = iKey + 1; iKey1 <= NumVarsOnCustomMeter; ++iKey1) {
                    if (iKey == iKey1) continue;
                    if (VarsOnCustomMeter(iKey) != VarsOnCustomMeter(iKey1)) continue;
                    ShowWarningError(state,
                                     format("{}=\"{}\", duplicate name=\"{}\".",
                                            cCurrentModuleObject,
                                            state.dataIPShortCut->cAlphaArgs(1),
                                            op->RVariableTypes(VarsOnCustomMeter(iKey1)).VarName));
                    ShowContinueError(state, "...only one value with this name will be shown with the Meter results.");
                    VarsOnCustomMeter(iKey1) = 0;
                }
            }
            for (iKey = 1; iKey <= NumVarsOnCustomMeter; ++iKey) {
                if (VarsOnCustomMeter(iKey) == 0) continue;
                auto &tmpVar = op->RVariableTypes(VarsOnCustomMeter(iKey)).VarPtr;
                AttachCustomMeters(state, VarsOnCustomMeter(iKey), tmpVar.MeterArrayPtr, op->NumEnergyMeters);
            }
            if (NumVarsOnCustomMeter == 0) {
                ShowWarningError(state, format("{}=\"{}\", no items assigned ", cCurrentModuleObject, state.dataIPShortCut->cAlphaArgs(1)));
                ShowContinueError(
                    state, "...will not be shown with the Meter results. This may be caused by a Meter:Custom be assigned to another Meter:Custom.");
            }
        }

        cCurrentModuleObject = "Meter:CustomDecrement";
        NumCustomDecMeters = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);

        for (Loop = 1; Loop <= NumCustomDecMeters; ++Loop) {
            state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                     cCurrentModuleObject,
                                                                     Loop,
                                                                     state.dataIPShortCut->cAlphaArgs,
                                                                     NumAlpha,
                                                                     state.dataIPShortCut->rNumericArgs,
                                                                     NumNumbers,
                                                                     IOStat,
                                                                     state.dataIPShortCut->lNumericFieldBlanks,
                                                                     state.dataIPShortCut->lAlphaFieldBlanks,
                                                                     state.dataIPShortCut->cAlphaFieldNames,
                                                                     state.dataIPShortCut->cNumericFieldNames);
            lbrackPos = index(state.dataIPShortCut->cAlphaArgs(1), '[');
            if (lbrackPos != std::string::npos) state.dataIPShortCut->cAlphaArgs(1).erase(lbrackPos);
            MeterCreated = false;
            if (GlobalNames::VerifyUniqueInterObjectName(state,
                                                         op->UniqueMeterNames,
                                                         state.dataIPShortCut->cAlphaArgs(1),
                                                         cCurrentModuleObject,
                                                         state.dataIPShortCut->cAlphaFieldNames(1),
                                                         ErrorsFound)) {
                continue;
            }
            if (allocated(VarsOnCustomMeter)) VarsOnCustomMeter.deallocate();
            VarsOnCustomMeter.allocate(1000);
            VarsOnCustomMeter = 0;
            MaxVarsOnCustomMeter = 1000;
            NumVarsOnCustomMeter = 0;

            lbrackPos = index(state.dataIPShortCut->cAlphaArgs(3), '[');
            if (lbrackPos != std::string::npos) state.dataIPShortCut->cAlphaArgs(1).erase(lbrackPos);
            WhichMeter = Util::FindItem(state.dataIPShortCut->cAlphaArgs(3), op->EnergyMeters);
            if (WhichMeter == 0) {
                ShowSevereError(state,
                                format("{}=\"{}\", invalid {}=\"{}\".",
                                       cCurrentModuleObject,
                                       state.dataIPShortCut->cAlphaArgs(1),
                                       state.dataIPShortCut->cAlphaFieldNames(3),
                                       state.dataIPShortCut->cAlphaArgs(3)));
                ErrorsFound = true;
                continue;
            }
            //  Set up array of Vars that are on the source meter (for later validation).
            if (allocated(VarsOnSourceMeter)) VarsOnSourceMeter.deallocate();
            VarsOnSourceMeter.allocate(1000);
            VarsOnSourceMeter = 0;
            MaxVarsOnSourceMeter = 1000;
            NumVarsOnSourceMeter = 0;
            for (iKey = 1; iKey <= op->NumVarMeterArrays; ++iKey) {
                if (op->VarMeterArrays(iKey).NumOnMeters == 0 && op->VarMeterArrays(iKey).NumOnCustomMeters == 0) continue;
                //  On a meter
                if (any_eq(op->VarMeterArrays(iKey).OnMeters, WhichMeter)) {
                    ++NumVarsOnSourceMeter;
                    if (NumVarsOnSourceMeter > MaxVarsOnSourceMeter) {
                        VarsOnSourceMeter.redimension(MaxVarsOnSourceMeter += 100, 0);
                    }
                    VarsOnSourceMeter(NumVarsOnSourceMeter) = op->VarMeterArrays(iKey).RepVariable;
                    continue;
                }
                if (op->VarMeterArrays(iKey).NumOnCustomMeters == 0) continue;
                if (any_eq(op->VarMeterArrays(iKey).OnCustomMeters, WhichMeter)) {
                    ++NumVarsOnSourceMeter;
                    if (NumVarsOnSourceMeter > MaxVarsOnSourceMeter) {
                        VarsOnSourceMeter.redimension(MaxVarsOnSourceMeter += 100, 0);
                    }
                    VarsOnSourceMeter(NumVarsOnSourceMeter) = op->VarMeterArrays(iKey).RepVariable;
                    continue;
                }
            }

            for (fldIndex = 4; fldIndex <= NumAlpha; fldIndex += 2) {
                if (state.dataIPShortCut->cAlphaArgs(fldIndex) == "*" || state.dataIPShortCut->lAlphaFieldBlanks(fldIndex)) {
                    KeyIsStar = true;
                    state.dataIPShortCut->cAlphaArgs(fldIndex) = "*";
                } else {
                    KeyIsStar = false;
                }
                if (state.dataIPShortCut->lAlphaFieldBlanks(fldIndex + 1)) {
                    ShowSevereError(state,
                                    format("{}=\"{}\", blank {}.",
                                           cCurrentModuleObject,
                                           state.dataIPShortCut->cAlphaArgs(1),
                                           state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1)));
                    ShowContinueError(state, "...cannot create custom meter.");
                    BigErrorsFound = true;
                    continue;
                }
                if (BigErrorsFound) continue;
                Tagged = false;
                lbrackPos = index(state.dataIPShortCut->cAlphaArgs(fldIndex + 1), '[');
                if (lbrackPos != std::string::npos) state.dataIPShortCut->cAlphaArgs(fldIndex + 1).erase(lbrackPos);
                // Don't build/check things out if there were errors anywhere.  Use "GetVariableKeys" to map to actual variables...
                GetVariableKeyCountandType(
                    state, state.dataIPShortCut->cAlphaArgs(fldIndex + 1), KeyCount, TypeVar, AvgSumVar, StepTypeVar, UnitsVar);
                if (TypeVar == VariableType::NotFound) {
                    ShowWarningError(state,
                                     format("{}=\"{}\", invalid {}=\"{}\".",
                                            cCurrentModuleObject,
                                            state.dataIPShortCut->cAlphaArgs(1),
                                            state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                            state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                    ShowContinueError(state, "...will not be shown with the Meter results.");
                    continue;
                }
                if (!MeterCreated) {
                    MeterUnits = UnitsVar;
                    AddMeter(state, state.dataIPShortCut->cAlphaArgs(1), UnitsVar, std::string(), std::string(), std::string(), std::string());
                    op->EnergyMeters(op->NumEnergyMeters).TypeOfMeter = MtrType::CustomDec;
                    op->EnergyMeters(op->NumEnergyMeters).SourceMeter = WhichMeter;

                    // Can't use resource type in AddMeter cause it will confuse it with other meters.  So, now:
                    GetStandardMeterResourceType(
                        state, op->EnergyMeters(op->NumEnergyMeters).ResourceType, Util::makeUPPER(state.dataIPShortCut->cAlphaArgs(2)), errFlag);
                    if (errFlag) {
                        ShowContinueError(state, format("..on {}=\"{}\".", cCurrentModuleObject, state.dataIPShortCut->cAlphaArgs(1)));
                        BigErrorsFound = true;
                    }
                    DetermineMeterIPUnits(state,
                                          op->EnergyMeters(op->NumEnergyMeters).RT_forIPUnits,
                                          op->EnergyMeters(op->NumEnergyMeters).ResourceType,
                                          UnitsVar,
                                          errFlag);
                    if (errFlag) {
                        ShowContinueError(state, format("..on {}=\"{}\".", cCurrentModuleObject, state.dataIPShortCut->cAlphaArgs(1)));
                        ShowContinueError(state, "..requests for IP units from this meter will be ignored.");
                    }
                    //        EnergyMeters(NumEnergyMeters)%RT_forIPUnits=DetermineMeterIPUnits(EnergyMeters(NumEnergyMeters)%ResourceType,UnitsVar)
                    MeterCreated = true;
                }
                if (UnitsVar != MeterUnits) {
                    ShowWarningError(state,
                                     format("{}=\"{}\", differing units in {}=\"{}\".",
                                            cCurrentModuleObject,
                                            state.dataIPShortCut->cAlphaArgs(1),
                                            state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                            state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                    ShowContinueError(state,
                                      format("...will not be shown with the Meter results; units for meter={}, units for this variable={}.",
                                             unitEnumToString(MeterUnits),
                                             unitEnumToString(UnitsVar)));
                    continue;
                }
                if ((TypeVar == VariableType::Real || TypeVar == VariableType::Integer) && AvgSumVar == StoreType::Summed) {
                    Tagged = true;
                    NamesOfKeys.allocate(KeyCount);
                    IndexesForKeyVar.allocate(KeyCount);
                    GetVariableKeys(state, state.dataIPShortCut->cAlphaArgs(fldIndex + 1), TypeVar, NamesOfKeys, IndexesForKeyVar);
                    iOnMeter = 0;
                    if (KeyIsStar) {
                        for (iKey = 1; iKey <= KeyCount; ++iKey) {
                            ++NumVarsOnCustomMeter;
                            if (NumVarsOnCustomMeter > MaxVarsOnCustomMeter) {
                                VarsOnCustomMeter.redimension(MaxVarsOnCustomMeter += 100, 0);
                            }
                            VarsOnCustomMeter(NumVarsOnCustomMeter) = IndexesForKeyVar(iKey);
                            iOnMeter = 1;
                        }
                        if (iOnMeter == 0) {
                            ShowSevereError(state,
                                            format("{}=\"{}\", invalid (all keys) {}=\"{}\".",
                                                   cCurrentModuleObject,
                                                   state.dataIPShortCut->cAlphaArgs(1),
                                                   state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                                   state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                            ErrorsFound = true;
                        }
                    } else {
                        for (iKey = 1; iKey <= KeyCount; ++iKey) {
                            if (NamesOfKeys(iKey) != state.dataIPShortCut->cAlphaArgs(fldIndex)) continue;
                            ++NumVarsOnCustomMeter;
                            if (NumVarsOnCustomMeter > MaxVarsOnCustomMeter) {
                                VarsOnCustomMeter.redimension(MaxVarsOnCustomMeter += 100, 0);
                            }
                            VarsOnCustomMeter(NumVarsOnCustomMeter) = IndexesForKeyVar(iKey);
                            iOnMeter = 1;
                        }
                        if (iOnMeter == 0) {
                            ShowSevereError(state,
                                            format("{}=\"{}\", invalid {}:{}",
                                                   cCurrentModuleObject,
                                                   state.dataIPShortCut->cAlphaArgs(1),
                                                   state.dataIPShortCut->cAlphaArgs(fldIndex),
                                                   state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                            ErrorsFound = true;
                        }
                    }
                    NamesOfKeys.deallocate();
                    IndexesForKeyVar.deallocate();
                }
                if (TypeVar == VariableType::Meter && AvgSumVar == StoreType::Summed) {
                    Tagged = true;
                    NamesOfKeys.allocate(KeyCount);
                    IndexesForKeyVar.allocate(KeyCount);
                    GetVariableKeys(state, state.dataIPShortCut->cAlphaArgs(fldIndex + 1), TypeVar, NamesOfKeys, IndexesForKeyVar);
                    WhichMeter = IndexesForKeyVar(1);
                    NamesOfKeys.deallocate();
                    IndexesForKeyVar.deallocate();
                    // for meters there will only be one key...  but it has variables associated...
                    for (iOnMeter = 1; iOnMeter <= op->NumVarMeterArrays; ++iOnMeter) {
                        testa = any_eq(op->VarMeterArrays(iOnMeter).OnMeters, WhichMeter);
                        testb = false;
                        if (op->VarMeterArrays(iOnMeter).NumOnCustomMeters > 0) {
                            testb = any_eq(op->VarMeterArrays(iOnMeter).OnCustomMeters, WhichMeter);
                        }
                        if (!(testa || testb)) continue;
                        ++NumVarsOnCustomMeter;
                        if (NumVarsOnCustomMeter > MaxVarsOnCustomMeter) {
                            VarsOnCustomMeter.redimension(MaxVarsOnCustomMeter += 100, 0);
                        }
                        VarsOnCustomMeter(NumVarsOnCustomMeter) = op->VarMeterArrays(iOnMeter).RepVariable;
                    }
                }
                if (!Tagged) { // couldn't find place for this item on a meter
                    if (AvgSumVar != StoreType::Summed) {
                        ShowWarningError(state,
                                         format("{}=\"{}\", variable not summed variable {}=\"{}\".",
                                                cCurrentModuleObject,
                                                state.dataIPShortCut->cAlphaArgs(1),
                                                state.dataIPShortCut->cAlphaFieldNames(fldIndex + 1),
                                                state.dataIPShortCut->cAlphaArgs(fldIndex + 1)));
                        ShowContinueError(state,
                                          format("...will not be shown with the Meter results; units for meter={}, units for this variable={}.",
                                                 unitEnumToString(MeterUnits),
                                                 unitEnumToString(UnitsVar)));
                    }
                }
            }
            // Check for duplicates
            for (iKey = 1; iKey <= NumVarsOnCustomMeter; ++iKey) {
                if (VarsOnCustomMeter(iKey) == 0) continue;
                for (iKey1 = iKey + 1; iKey1 <= NumVarsOnCustomMeter; ++iKey1) {
                    if (iKey == iKey1) continue;
                    if (VarsOnCustomMeter(iKey) != VarsOnCustomMeter(iKey1)) continue;
                    ShowWarningError(state,
                                     format("{}=\"{}\", duplicate name=\"{}\".",
                                            cCurrentModuleObject,
                                            state.dataIPShortCut->cAlphaArgs(1),
                                            op->RVariableTypes(VarsOnCustomMeter(iKey1)).VarName));
                    ShowContinueError(state, "...only one value with this name will be shown with the Meter results.");
                    VarsOnCustomMeter(iKey1) = 0;
                }
            }
            for (iKey = 1; iKey <= NumVarsOnCustomMeter; ++iKey) {
                if (VarsOnCustomMeter(iKey) == 0) continue;
                auto &tmpVar = op->RVariableTypes(VarsOnCustomMeter(iKey)).VarPtr;
                AttachCustomMeters(state, VarsOnCustomMeter(iKey), tmpVar.MeterArrayPtr, op->NumEnergyMeters);
            }

            errFlag = false;
            for (iKey = 1; iKey <= NumVarsOnCustomMeter; ++iKey) {
                for (iKey1 = 1; iKey1 <= NumVarsOnSourceMeter; ++iKey1) {
                    if (any_eq(VarsOnSourceMeter, VarsOnCustomMeter(iKey))) break;
                    if (!errFlag) {
                        ShowSevereError(state,
                                        format("{}=\"{}\", invalid specification to {}=\"{}\".",
                                               cCurrentModuleObject,
                                               state.dataIPShortCut->cAlphaArgs(1),
                                               state.dataIPShortCut->cAlphaFieldNames(3),
                                               state.dataIPShortCut->cAlphaArgs(3)));
                        errFlag = true;
                    }
                    ShowContinueError(state, format("..Variable={}", op->RVariableTypes(VarsOnCustomMeter(iKey)).VarName));
                    ErrorsFound = true;
                    break;
                }
            }
            if (NumVarsOnCustomMeter == 0) {
                ShowWarningError(state, format("{}=\"{}\", no items assigned ", cCurrentModuleObject, state.dataIPShortCut->cAlphaArgs(1)));
                ShowContinueError(state, "...will not be shown with the Meter results");
            }

            VarsOnCustomMeter.deallocate();
            VarsOnSourceMeter.deallocate();
        }

        if (BigErrorsFound) ErrorsFound = true;
    }

    void GetStandardMeterResourceType(EnergyPlusData &state,
                                      std::string &OutResourceType,
                                      std::string const &UserInputResourceType, // Passed uppercase
                                      bool &ErrorsFound)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   April 2006
        //       MODIFIED       Dareum Nam, April 2023, revised the function by using enumaration value
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This routine compares the user input resource type with valid ones and returns
        // the standard resource type.

        ErrorsFound = false;
        std::string const meterType = Util::makeUPPER(UserInputResourceType);

        int eMeterResource = getEnumValue(Constant::eResourceNamesUC, meterType);

        if (eMeterResource == static_cast<int>(Constant::eResource::Invalid)) {
            ShowSevereError(state, format("GetStandardMeterResourceType: Illegal OutResourceType (for Meters) Entered={}", UserInputResourceType));
            ErrorsFound = true;
            return;
        }

        OutResourceType = Constant::eResourceNames[eMeterResource];
    }

    void AddMeter(EnergyPlusData &state,
                  std::string const &Name,              // Name for the meter
                  OutputProcessor::Unit const MtrUnits, // Units for the meter
                  std::string const &ResourceType,      // ResourceType for the meter
                  std::string const &EndUse,            // EndUse for the meter
                  std::string const &EndUseSub,         // EndUse subcategory for the meter
                  std::string const &Group              // Group for the meter
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine adds a meter to the current definition set of meters.  If the maximum has
        // already been reached, a reallocation procedure begins.  This action needs to be done at the
        // start of the simulation, primarily before any output is stored.

        // Make sure this isn't already in the list of meter names
        auto &op = state.dataOutputProcessor;
        int Found;

        if (op->NumEnergyMeters > 0) {
            Found = Util::FindItemInList(Name, op->EnergyMeters);
        } else {
            Found = 0;
        }

        if (Found == 0) {
            op->EnergyMeters.redimension(++op->NumEnergyMeters);
            op->EnergyMeters(op->NumEnergyMeters).Name = Name;
            op->EnergyMeters(op->NumEnergyMeters).ResourceType = ResourceType;
            op->EnergyMeters(op->NumEnergyMeters).EndUse = EndUse;
            op->EnergyMeters(op->NumEnergyMeters).EndUseSub = EndUseSub;
            op->EnergyMeters(op->NumEnergyMeters).Group = Group;
            op->EnergyMeters(op->NumEnergyMeters).Units = MtrUnits;
            op->EnergyMeters(op->NumEnergyMeters).TSValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).CurTSValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).RptTS = false;
            op->EnergyMeters(op->NumEnergyMeters).RptTSFO = false;
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).TSRptNum);
            op->EnergyMeters(op->NumEnergyMeters).TSRptNumChr = fmt::to_string(op->EnergyMeters(op->NumEnergyMeters).TSRptNum);
            op->EnergyMeters(op->NumEnergyMeters).HRValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).RptHR = false;
            op->EnergyMeters(op->NumEnergyMeters).RptHRFO = false;
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).HRRptNum);
            op->EnergyMeters(op->NumEnergyMeters).HRRptNumChr = fmt::to_string(op->EnergyMeters(op->NumEnergyMeters).HRRptNum);
            op->EnergyMeters(op->NumEnergyMeters).DYValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).DYMaxVal = MaxSetValue;
            op->EnergyMeters(op->NumEnergyMeters).DYMaxValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).DYMinVal = MinSetValue;
            op->EnergyMeters(op->NumEnergyMeters).DYMinValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).RptDY = false;
            op->EnergyMeters(op->NumEnergyMeters).RptDYFO = false;
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).DYRptNum);
            op->EnergyMeters(op->NumEnergyMeters).DYRptNumChr = fmt::to_string(op->EnergyMeters(op->NumEnergyMeters).DYRptNum);
            op->EnergyMeters(op->NumEnergyMeters).MNValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).MNMaxVal = MaxSetValue;
            op->EnergyMeters(op->NumEnergyMeters).MNMaxValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).MNMinVal = MinSetValue;
            op->EnergyMeters(op->NumEnergyMeters).MNMinValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).RptMN = false;
            op->EnergyMeters(op->NumEnergyMeters).RptMNFO = false;
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).MNRptNum);
            op->EnergyMeters(op->NumEnergyMeters).MNRptNumChr = fmt::to_string(op->EnergyMeters(op->NumEnergyMeters).MNRptNum);
            op->EnergyMeters(op->NumEnergyMeters).YRValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).YRMaxVal = MaxSetValue;
            op->EnergyMeters(op->NumEnergyMeters).YRMaxValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).YRMinVal = MinSetValue;
            op->EnergyMeters(op->NumEnergyMeters).YRMinValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).RptYR = false;
            op->EnergyMeters(op->NumEnergyMeters).RptYRFO = false;
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).YRRptNum);
            op->EnergyMeters(op->NumEnergyMeters).YRRptNumChr = fmt::to_string(op->EnergyMeters(op->NumEnergyMeters).YRRptNum);
            op->EnergyMeters(op->NumEnergyMeters).SMValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).SMMaxVal = MaxSetValue;
            op->EnergyMeters(op->NumEnergyMeters).SMMaxValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).SMMinVal = MinSetValue;
            op->EnergyMeters(op->NumEnergyMeters).SMMinValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).RptSM = false;
            op->EnergyMeters(op->NumEnergyMeters).RptSMFO = false;
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).SMRptNum);
            op->EnergyMeters(op->NumEnergyMeters).SMRptNumChr = fmt::to_string(op->EnergyMeters(op->NumEnergyMeters).SMRptNum);
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).TSAccRptNum);
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).HRAccRptNum);
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).DYAccRptNum);
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).MNAccRptNum);
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).YRAccRptNum);
            AssignReportNumber(state, op->EnergyMeters(op->NumEnergyMeters).SMAccRptNum);
            op->EnergyMeters(op->NumEnergyMeters).FinYrSMValue = 0.0;
            op->EnergyMeters(op->NumEnergyMeters).FinYrSMMaxVal = MaxSetValue;
            op->EnergyMeters(op->NumEnergyMeters).FinYrSMMaxValDate = 0;
            op->EnergyMeters(op->NumEnergyMeters).FinYrSMMinVal = MinSetValue;
            op->EnergyMeters(op->NumEnergyMeters).FinYrSMMinValDate = 0;
        } else {
            ShowFatalError(state, format("Requested to Add Meter which was already present={}", Name));
        }
        if (!ResourceType.empty()) {
            bool errFlag;
            DetermineMeterIPUnits(state, op->EnergyMeters(op->NumEnergyMeters).RT_forIPUnits, ResourceType, MtrUnits, errFlag);
            if (errFlag) {
                ShowContinueError(state, format("..on Meter=\"{}\".", Name));
                ShowContinueError(state, "..requests for IP units from this meter will be ignored.");
            }
        }
    }

    void AttachMeters(EnergyPlusData &state,
                      OutputProcessor::Unit const MtrUnits, // Units for this meter
                      std::string &ResourceType,            // Electricity, Gas, etc.
                      std::string &EndUse,                  // End-use category (Lights, Heating, etc.)
                      std::string &EndUseSub,               // End-use subcategory (user-defined, e.g., General Lights, Task Lights, etc.)
                      std::string &Group,                   // Group key (Facility, Zone, Building, etc.)
                      std::string const &ZoneName,          // Zone key only applicable for Building group
                      std::string const &SpaceType,         // Space Type key only applicable for Building group
                      int const RepVarNum,                  // Number of this report variable
                      int &MeterArrayPtr,                   // Output set of Pointers to Meters
                      bool &ErrorsFound                     // True if errors in this call
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine determines which meters this variable will be on (if any),
        // sets up the meter pointer arrays, and returns a index value to this array which
        // is stored with the variable.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        auto &op = state.dataOutputProcessor;

        ValidateNStandardizeMeterTitles(state, MtrUnits, ResourceType, EndUse, EndUseSub, Group, ErrorsFound, ZoneName, SpaceType);

        op->VarMeterArrays.redimension(++op->NumVarMeterArrays);
        MeterArrayPtr = op->NumVarMeterArrays;
        op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters = 0;
        op->VarMeterArrays(op->NumVarMeterArrays).RepVariable = RepVarNum;
        op->VarMeterArrays(op->NumVarMeterArrays).OnMeters = 0;
        int Found = Util::FindItem(ResourceType + ":Facility", op->EnergyMeters);
        if (Found != 0) {
            ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
            op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
        }
        if (!Group.empty()) {
            Found = Util::FindItem(ResourceType + ':' + Group, op->EnergyMeters);
            if (Found != 0) {
                ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
            }
            if (Util::SameString(Group, "Building")) { // Match to Zone and Space Type
                if (!ZoneName.empty()) {
                    Found = Util::FindItem(ResourceType + ":Zone:" + ZoneName, op->EnergyMeters);
                    if (Found != 0) {
                        ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                        op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
                    }
                }
                if (!SpaceType.empty()) {
                    Found = Util::FindItem(ResourceType + ":SpaceType:" + SpaceType, op->EnergyMeters);
                    if (Found != 0) {
                        ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                        op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
                    }
                }
            }
        }

        //!! Following if EndUse is by ResourceType
        if (!EndUse.empty()) {
            Found = Util::FindItem(EndUse + ':' + ResourceType, op->EnergyMeters);
            if (Found != 0) {
                ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
            }
            if (Util::SameString(Group, "Building")) { // Match to Zone
                if (!ZoneName.empty()) {
                    Found = Util::FindItem(EndUse + ':' + ResourceType + ":Zone:" + ZoneName, op->EnergyMeters);
                    if (Found != 0) {
                        ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                        op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
                    }
                }
                if (!SpaceType.empty()) {
                    Found = Util::FindItem(EndUse + ':' + ResourceType + ":SpaceType:" + SpaceType, op->EnergyMeters);
                    if (Found != 0) {
                        ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                        op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;

                        addEndUseSpaceType(state, EndUse, SpaceType);
                    }
                }
            }

            // End use subcategory
            if (!EndUseSub.empty()) {
                Found = Util::FindItem(EndUseSub + ':' + EndUse + ':' + ResourceType, op->EnergyMeters);
                if (Found != 0) {
                    ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                    op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;

                    addEndUseSubcategory(state, EndUse, EndUseSub);
                }
                if (Util::SameString(Group, "Building")) { // Match to Zone
                    if (!ZoneName.empty()) {
                        Found = Util::FindItem(EndUseSub + ':' + EndUse + ':' + ResourceType + ":Zone:" + ZoneName, op->EnergyMeters);
                        if (Found != 0) {
                            ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                            op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
                        }
                    }
                    if (!SpaceType.empty()) {
                        Found = Util::FindItem(EndUseSub + ':' + EndUse + ':' + ResourceType + ":SpaceType:" + SpaceType, op->EnergyMeters);
                        if (Found != 0) {
                            ++op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters;
                            op->VarMeterArrays(op->NumVarMeterArrays).OnMeters(op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters) = Found;
                        }
                    }
                }
            }
        }
    }

    void AttachCustomMeters(EnergyPlusData &state,
                            int const RepVarNum, // Number of this report variable
                            int &MeterArrayPtr,  // Input/Output set of Pointers to Meters
                            int const MeterIndex // Which meter this is
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2006
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine determines which meters this variable will be on (if any),
        // sets up the meter pointer arrays, and returns a index value to this array which
        // is stored with the variable.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        auto &op = state.dataOutputProcessor;

        if (MeterArrayPtr == 0) {
            op->VarMeterArrays.redimension(++op->NumVarMeterArrays);
            MeterArrayPtr = op->NumVarMeterArrays;
            op->VarMeterArrays(op->NumVarMeterArrays).NumOnMeters = 0;
            op->VarMeterArrays(op->NumVarMeterArrays).RepVariable = RepVarNum;
            op->VarMeterArrays(op->NumVarMeterArrays).OnMeters = 0;
            op->VarMeterArrays(op->NumVarMeterArrays).OnCustomMeters.allocate(1);
            op->VarMeterArrays(op->NumVarMeterArrays).NumOnCustomMeters = 1;
        } else { // MeterArrayPtr set
            op->VarMeterArrays(MeterArrayPtr).OnCustomMeters.redimension(++op->VarMeterArrays(MeterArrayPtr).NumOnCustomMeters);
        }
        op->VarMeterArrays(MeterArrayPtr).OnCustomMeters(op->VarMeterArrays(MeterArrayPtr).NumOnCustomMeters) = MeterIndex;
    }

    void ValidateNStandardizeMeterTitles(EnergyPlusData &state,
                                         OutputProcessor::Unit const MtrUnits, // Units for the meter
                                         std::string &ResourceType,            // Electricity, Gas, etc.
                                         std::string &EndUse,                  // End Use Type (Lights, Heating, etc.)
                                         std::string &EndUseSub,               // End Use Sub Type (General Lights, Task Lights, etc.)
                                         std::string &Group,                   // Group key (Facility, Zone, Building, etc.)
                                         bool &ErrorsFound,                    // True if errors in this call
                                         const std::string &ZoneName,          // Zone Name when Group=Building
                                         const std::string &SpaceType          // Space Type when Group=Building
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine uses the keys for the Energy Meters given to the SetupOutputVariable routines
        // and makes sure they are "standard" as well as creating meters which need to be added as this
        // is the first use of that kind of meter designation.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Found; // For checking whether meter is already defined
        bool LocalErrorsFound = false;
        std::string MeterName;
        auto &op = state.dataOutputProcessor;

        // Basic ResourceType Meters
        GetStandardMeterResourceType(state, ResourceType, Util::makeUPPER(ResourceType), LocalErrorsFound);

        if (!LocalErrorsFound) {
            if (op->NumEnergyMeters > 0) {
                Found = Util::FindItem(ResourceType + ":Facility", op->EnergyMeters);
            } else {
                Found = 0;
            }
            if (Found == 0) AddMeter(state, ResourceType + ":Facility", MtrUnits, ResourceType, "", "", "");
        }

        //!  Group Meters
        {
            std::string const groupMeter = uppercased(Group);

            if (groupMeter.empty()) {

            } else if (groupMeter == "BUILDING") {
                Group = "Building";

            } else if (groupMeter == "HVAC" || groupMeter == "SYSTEM") {
                Group = "HVAC";

            } else if (groupMeter == "PLANT") {
                Group = "Plant";

            } else {
                ShowSevereError(state, format("Illegal Group (for Meters) Entered={}", Group));
                LocalErrorsFound = true;
            }
        }

        if (!LocalErrorsFound && !Group.empty()) {
            Found = Util::FindItem(ResourceType + ':' + Group, op->EnergyMeters);
            if (Found == 0) AddMeter(state, ResourceType + ':' + Group, MtrUnits, ResourceType, "", "", Group);
            if (Group == "Building") {
                if (!ZoneName.empty()) {
                    Found = Util::FindItem(ResourceType + ":Zone:" + ZoneName, op->EnergyMeters);
                    if (Found == 0) {
                        AddMeter(state, ResourceType + ":Zone:" + ZoneName, MtrUnits, ResourceType, "", "", "Zone");
                    }
                }
                if (!SpaceType.empty()) {
                    Found = Util::FindItem(ResourceType + ":SpaceType:" + SpaceType, op->EnergyMeters);
                    if (Found == 0) {
                        AddMeter(state, ResourceType + ":SpaceType:" + SpaceType, MtrUnits, ResourceType, "", "", "SpaceType");
                    }
                }
            }
        }

        //!!! EndUse Meters
        {
            std::string const endUseMeter = uppercased(EndUse);

            if (endUseMeter.empty()) {

            } else if (endUseMeter == "INTERIOR LIGHTS" || endUseMeter == "INTERIORLIGHTS") {
                EndUse = "InteriorLights";

            } else if (endUseMeter == "EXTERIOR LIGHTS" || endUseMeter == "EXTERIORLIGHTS") {
                EndUse = "ExteriorLights";

            } else if (endUseMeter == "HEATING" || endUseMeter == "HTG") {
                EndUse = "Heating";

            } else if (endUseMeter == "HEATPRODUCED") {
                EndUse = "HeatProduced";

            } else if (endUseMeter == "COOLING" || endUseMeter == "CLG") {
                EndUse = "Cooling";

            } else if (endUseMeter == "DOMESTICHOTWATER" || endUseMeter == "DHW" || endUseMeter == "DOMESTIC HOT WATER") {
                EndUse = "WaterSystems";

            } else if (endUseMeter == "COGEN" || endUseMeter == "COGENERATION") {
                EndUse = "Cogeneration";

            } else if (endUseMeter == "INTERIOREQUIPMENT" || endUseMeter == "INTERIOR EQUIPMENT") {
                EndUse = "InteriorEquipment";

            } else if (endUseMeter == "EXTERIOREQUIPMENT" || endUseMeter == "EXTERIOR EQUIPMENT" || endUseMeter == "EXT EQ" ||
                       endUseMeter == "EXTERIOREQ") {
                EndUse = "ExteriorEquipment";

            } else if (endUseMeter == "EXTERIOR:WATEREQUIPMENT") {
                EndUse = "ExteriorEquipment";

            } else if (endUseMeter == "PURCHASEDHOTWATER" || endUseMeter == "DISTRICTHOTWATER" || endUseMeter == "PURCHASED HEATING") {
                EndUse = "DistrictHotWater";

            } else if (endUseMeter == "PURCHASEDCOLDWATER" || endUseMeter == "DISTRICTCHILLEDWATER" || endUseMeter == "PURCHASEDCHILLEDWATER" ||
                       endUseMeter == "PURCHASED COLD WATER" || endUseMeter == "PURCHASED COOLING") {
                EndUse = "DistrictChilledWater";

            } else if (endUseMeter == "FANS" || endUseMeter == "FAN") {
                EndUse = "Fans";

            } else if (endUseMeter == "HEATINGCOILS" || endUseMeter == "HEATINGCOIL" || endUseMeter == "HEATING COILS" ||
                       endUseMeter == "HEATING COIL") {
                EndUse = "HeatingCoils";

            } else if (endUseMeter == "COOLINGCOILS" || endUseMeter == "COOLINGCOIL" || endUseMeter == "COOLING COILS" ||
                       endUseMeter == "COOLING COIL") {
                EndUse = "CoolingCoils";

            } else if (endUseMeter == "PUMPS" || endUseMeter == "PUMP") {
                EndUse = "Pumps";

            } else if (endUseMeter == "FREECOOLING" || endUseMeter == "FREE COOLING") {
                EndUse = "Freecooling";

            } else if (endUseMeter == "LOOPTOLOOP") {
                EndUse = "LoopToLoop";

            } else if (endUseMeter == "CHILLERS" || endUseMeter == "CHILLER") {
                EndUse = "Chillers";

            } else if (endUseMeter == "BOILERS" || endUseMeter == "BOILER") {
                EndUse = "Boilers";

            } else if (endUseMeter == "BASEBOARD" || endUseMeter == "BASEBOARDS") {
                EndUse = "Baseboard";

            } else if (endUseMeter == "COOLINGPANEL" || endUseMeter == "COOLINGPANELS") {
                EndUse = "CoolingPanel";

            } else if (endUseMeter == "HEATREJECTION" || endUseMeter == "HEAT REJECTION") {
                EndUse = "HeatRejection";

            } else if (endUseMeter == "HUMIDIFIER" || endUseMeter == "HUMIDIFIERS") {
                EndUse = "Humidifier";

            } else if (endUseMeter == "HEATRECOVERY" || endUseMeter == "HEAT RECOVERY") {
                EndUse = "HeatRecovery";

            } else if (endUseMeter == "PHOTOVOLTAICS" || endUseMeter == "PV" || endUseMeter == "PHOTOVOLTAIC") {
                EndUse = "Photovoltaic";

            } else if (endUseMeter == "WINDTURBINES" || endUseMeter == "WT" || endUseMeter == "WINDTURBINE") {
                EndUse = "WindTurbine";

            } else if (endUseMeter == "ELECTRICSTORAGE") {
                EndUse = "ElectricStorage";

            } else if (endUseMeter == "POWERCONVERSION") {

                EndUse = "PowerConversion";

            } else if (endUseMeter == "HEAT RECOVERY FOR COOLING" || endUseMeter == "HEATRECOVERYFORCOOLING" ||
                       endUseMeter == "HEATRECOVERYCOOLING") {
                EndUse = "HeatRecoveryForCooling";

            } else if (endUseMeter == "HEAT RECOVERY FOR HEATING" || endUseMeter == "HEATRECOVERYFORHEATING" ||
                       endUseMeter == "HEATRECOVERYHEATING") {
                EndUse = "HeatRecoveryForHeating";

            } else if (endUseMeter == "ELECTRICITYEMISSIONS") {
                EndUse = "ElectricityEmissions";

            } else if (endUseMeter == "PURCHASEDELECTRICITYEMISSIONS") {
                EndUse = "PurchasedElectricityEmissions";

            } else if (endUseMeter == "SOLDELECTRICITYEMISSIONS") {
                EndUse = "SoldElectricityEmissions";

            } else if (endUseMeter == "NATURALGASEMISSIONS") {
                EndUse = "NaturalGasEmissions";

            } else if (endUseMeter == "FUELOILNO1EMISSIONS") {
                EndUse = "FuelOilNo1Emissions";

            } else if (endUseMeter == "FUELOILNO2EMISSIONS") {
                EndUse = "FuelOilNo2Emissions";

            } else if (endUseMeter == "COALEMISSIONS") {
                EndUse = "CoalEmissions";

            } else if (endUseMeter == "GASOLINEEMISSIONS") {
                EndUse = "GasolineEmissions";

            } else if (endUseMeter == "PROPANEEMISSIONS") {
                EndUse = "PropaneEmissions";

            } else if (endUseMeter == "DIESELEMISSIONS") {
                EndUse = "DieselEmissions";

            } else if (endUseMeter == "OTHERFUEL1EMISSIONS") {
                EndUse = "OtherFuel1Emissions";

            } else if (endUseMeter == "OTHERFUEL2EMISSIONS") {
                EndUse = "OtherFuel2Emissions";

            } else if (endUseMeter == "CARBONEQUIVALENTEMISSIONS") {
                EndUse = "CarbonEquivalentEmissions";

            } else if (endUseMeter == "REFRIGERATION") {
                EndUse = "Refrigeration";

            } else if (endUseMeter == "COLDSTORAGECHARGE") {
                EndUse = "ColdStorageCharge";

            } else if (endUseMeter == "COLDSTORAGEDISCHARGE") {
                EndUse = "ColdStorageDischarge";

            } else if (endUseMeter == "WATERSYSTEMS" || endUseMeter == "WATERSYSTEM" || endUseMeter == "Water System") {
                EndUse = "WaterSystems";

            } else if (endUseMeter == "RAINWATER") {
                EndUse = "Rainwater";

            } else if (endUseMeter == "CONDENSATE") {
                EndUse = "Condensate";

            } else if (endUseMeter == "WELLWATER") {
                EndUse = "Wellwater";

            } else if (endUseMeter == "MAINSWATER" || endUseMeter == "PURCHASEDWATER") {
                EndUse = "MainsWater";

            } else {
                ShowSevereError(state, format("Illegal EndUse (for Meters) Entered={}", EndUse));
                LocalErrorsFound = true;
            }
        }

        //!! Following if we do EndUse by ResourceType
        if (!LocalErrorsFound && !EndUse.empty()) {
            Found = Util::FindItem(EndUse + ':' + ResourceType, op->EnergyMeters);
            if (Found == 0) AddMeter(state, EndUse + ':' + ResourceType, MtrUnits, ResourceType, EndUse, "", "");

            if (Group == "Building") { // Match to Zone and Space
                if (!ZoneName.empty()) {
                    Found = Util::FindItem(EndUse + ':' + ResourceType + ":Zone:" + ZoneName, op->EnergyMeters);
                    if (Found == 0) {
                        AddMeter(state, EndUse + ':' + ResourceType + ":Zone:" + ZoneName, MtrUnits, ResourceType, EndUse, "", "Zone");
                    }
                }
                if (!SpaceType.empty()) {
                    Found = Util::FindItem(EndUse + ':' + ResourceType + ":SpaceType:" + SpaceType, op->EnergyMeters);
                    if (Found == 0) {
                        AddMeter(state, EndUse + ':' + ResourceType + ":SpaceType:" + SpaceType, MtrUnits, ResourceType, EndUse, "", "SpaceType");
                    }
                }
            }
        } else if (LocalErrorsFound) {
            ErrorsFound = true;
        }

        // End-Use Subcategories
        if (!LocalErrorsFound && !EndUseSub.empty()) {
            MeterName = EndUseSub + ':' + EndUse + ':' + ResourceType;
            Found = Util::FindItem(MeterName, op->EnergyMeters);
            if (Found == 0) AddMeter(state, MeterName, MtrUnits, ResourceType, EndUse, EndUseSub, "");

            if (Group == "Building") { // Match to Zone and Space
                if (!ZoneName.empty()) {
                    Found = Util::FindItem(EndUseSub + ':' + EndUse + ':' + ResourceType + ":Zone:" + ZoneName, op->EnergyMeters);
                    if (Found == 0) {
                        AddMeter(state,
                                 EndUseSub + ':' + EndUse + ':' + ResourceType + ":Zone:" + ZoneName,
                                 MtrUnits,
                                 ResourceType,
                                 EndUse,
                                 EndUseSub,
                                 "Zone");
                    }
                }
                if (!SpaceType.empty()) {
                    Found = Util::FindItem(EndUseSub + ':' + EndUse + ':' + ResourceType + ":SpaceType:" + SpaceType, op->EnergyMeters);
                    if (Found == 0) {
                        AddMeter(state,
                                 EndUseSub + ':' + EndUse + ':' + ResourceType + ":SpaceType:" + SpaceType,
                                 MtrUnits,
                                 ResourceType,
                                 EndUse,
                                 EndUseSub,
                                 "SpaceType");
                    }
                }
            }
        } else if (LocalErrorsFound) {
            ErrorsFound = true;
        }
    }

    void DetermineMeterIPUnits(EnergyPlusData &state,
                               OutputProcessor::RT_IPUnits &CodeForIPUnits, // Output Code for IP Units
                               std::string const &ResourceType,             // Resource Type
                               OutputProcessor::Unit const MtrUnits,        // Meter units
                               bool &ErrorsFound                            // true if errors found during subroutine
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2012
        //       MODIFIED       September 2012; made into subroutine
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // In order to set up tabular reports for IP units, need to search on same strings
        // that tabular reports does for IP conversion.

        // REFERENCES:
        // OutputReportTabular looks for:
        // CONSUMP - not used in meters
        // ELEC - Electricity (kWH)
        // GAS - Gas (therm)
        // COOL - Cooling (ton)
        // and we need to add WATER (for m3/gal, etc)

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:

        ErrorsFound = false;
        std::string UC_ResourceType = Util::makeUPPER(ResourceType);

        CodeForIPUnits = RT_IPUnits::OtherJ;
        if (has(UC_ResourceType, "ELEC")) {
            CodeForIPUnits = RT_IPUnits::Electricity;
        } else if (has(UC_ResourceType, "GAS")) {
            CodeForIPUnits = RT_IPUnits::Gas;
        } else if (has(UC_ResourceType, "COOL")) {
            CodeForIPUnits = RT_IPUnits::Cooling;
        }
        if (MtrUnits == OutputProcessor::Unit::m3 && has(UC_ResourceType, "WATER")) {
            CodeForIPUnits = RT_IPUnits::Water;
        } else if (MtrUnits == OutputProcessor::Unit::m3) {
            CodeForIPUnits = RT_IPUnits::OtherM3;
        }
        if (MtrUnits == OutputProcessor::Unit::kg) {
            CodeForIPUnits = RT_IPUnits::OtherKG;
        }
        if (MtrUnits == OutputProcessor::Unit::L) {
            CodeForIPUnits = RT_IPUnits::OtherL;
        }
        //  write(outputfiledebug,*) 'resourcetype=',TRIM(resourcetype)
        //  write(outputfiledebug,*) 'ipunits type=',CodeForIPUnits
        if (MtrUnits != OutputProcessor::Unit::kg && MtrUnits != OutputProcessor::Unit::J && MtrUnits != OutputProcessor::Unit::m3 &&
            MtrUnits != OutputProcessor::Unit::L) {
            ShowWarningError(state,
                             format("DetermineMeterIPUnits: Meter units not recognized for IP Units conversion=[{}].", unitEnumToString(MtrUnits)));
            ErrorsFound = true;
        }
    }

    void UpdateMeters(EnergyPlusData &state, int const TimeStamp) // Current TimeStamp (for max/min)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   April 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine updates the meters with the current time step value
        // for each meter.  Also, sets min/max values for hourly...run period reporting.

        // METHODOLOGY EMPLOYED:
        // Goes thru the number of meters, setting min/max as appropriate.  Uses timestamp
        // from calling program.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:

        if (state.dataGlobal->WarmupFlag) {
            return;
        }

        auto &op = state.dataOutputProcessor;

        if (!op->MeterValue.allocated()) {
            return;
        }

        for (int Meter = 1; Meter <= op->NumEnergyMeters; ++Meter) {
            if (op->EnergyMeters(Meter).TypeOfMeter != MtrType::CustomDec && op->EnergyMeters(Meter).TypeOfMeter != MtrType::CustomDiff) {
                op->EnergyMeters(Meter).TSValue += op->MeterValue(Meter);
            } else {
                op->EnergyMeters(Meter).TSValue = op->EnergyMeters(op->EnergyMeters(Meter).SourceMeter).TSValue - op->MeterValue(Meter);
            }
            op->EnergyMeters(Meter).HRValue += op->EnergyMeters(Meter).TSValue;
            op->EnergyMeters(Meter).DYValue += op->EnergyMeters(Meter).TSValue;
            op->EnergyMeters(Meter).MNValue += op->EnergyMeters(Meter).TSValue;
            op->EnergyMeters(Meter).YRValue += op->EnergyMeters(Meter).TSValue;
            op->EnergyMeters(Meter).SMValue += op->EnergyMeters(Meter).TSValue;
            // if (op->isFinalYear) op->EnergyMeters(Meter).FinYrSMValue += op->EnergyMeters(Meter).TSValue;
            op->EnergyMeters(Meter).FinYrSMValue += op->EnergyMeters(Meter).TSValue;
        }
        // Set Max
        for (int Meter = 1; Meter <= op->NumEnergyMeters; ++Meter) {
            // Todo - HRMinVal, HRMaxVal not used
            if (op->EnergyMeters(Meter).TSValue > op->EnergyMeters(Meter).DYMaxVal) {
                op->EnergyMeters(Meter).DYMaxVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).DYMaxValDate = TimeStamp;
            } else {
                continue; // Not max val of month or year, if not max of day so far
            }
            if (op->EnergyMeters(Meter).TSValue > op->EnergyMeters(Meter).MNMaxVal) {
                op->EnergyMeters(Meter).MNMaxVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).MNMaxValDate = TimeStamp;
            } else {
                continue;
            }
            if (op->EnergyMeters(Meter).TSValue > op->EnergyMeters(Meter).YRMaxVal) {
                op->EnergyMeters(Meter).YRMaxVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).YRMaxValDate = TimeStamp;
            }
            if (op->EnergyMeters(Meter).TSValue > op->EnergyMeters(Meter).SMMaxVal) {
                op->EnergyMeters(Meter).SMMaxVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).SMMaxValDate = TimeStamp;
            }
            // if (op->isFinalYear) {
            if (op->EnergyMeters(Meter).TSValue > op->EnergyMeters(Meter).FinYrSMMaxVal) {
                op->EnergyMeters(Meter).FinYrSMMaxVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).FinYrSMMaxValDate = TimeStamp;
            }
            //}
        }
        // Set Min
        for (int Meter = 1; Meter <= op->NumEnergyMeters; ++Meter) {
            if (op->EnergyMeters(Meter).TSValue < op->EnergyMeters(Meter).DYMinVal) {
                op->EnergyMeters(Meter).DYMinVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).DYMinValDate = TimeStamp;
            } else {
                continue;
            }
            if (op->EnergyMeters(Meter).TSValue < op->EnergyMeters(Meter).MNMinVal) {
                op->EnergyMeters(Meter).MNMinVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).MNMinValDate = TimeStamp;
            } else {
                continue;
            }
            if (op->EnergyMeters(Meter).TSValue < op->EnergyMeters(Meter).YRMinVal) {
                op->EnergyMeters(Meter).YRMinVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).YRMinValDate = TimeStamp;
            }
            if (op->EnergyMeters(Meter).TSValue < op->EnergyMeters(Meter).SMMinVal) {
                op->EnergyMeters(Meter).SMMinVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).SMMinValDate = TimeStamp;
            }
            // if (op->isFinalYear) {
            if (op->EnergyMeters(Meter).TSValue < op->EnergyMeters(Meter).FinYrSMMinVal) {
                op->EnergyMeters(Meter).FinYrSMMinVal = op->EnergyMeters(Meter).TSValue;
                op->EnergyMeters(Meter).FinYrSMMinValDate = TimeStamp;
            }
            //}
        }
        for (int Meter = 1; Meter <= op->NumEnergyMeters; ++Meter) {
            op->MeterValue(Meter) = 0.0; // Ready for next update
        }
    }

    void ReportTSMeters(EnergyPlusData &state,
                        Real64 const StartMinute, // Start Minute for TimeStep
                        Real64 const EndMinute,   // End Minute for TimeStep
                        bool &PrintESOTimeStamp,  // True if the ESO Time Stamp also needs to be printed
                        bool PrintTimeStampToSQL  // Print Time Stamp to SQL file
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine reports on the meters that have been requested for
        // reporting on each time step.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop; // Loop Control
        bool PrintTimeStamp;
        int CurDayType;
        auto &op = state.dataOutputProcessor;

        if (!state.dataResultsFramework->resultsFramework->TSMeters.rDataFrameEnabled()) {
            state.dataResultsFramework->resultsFramework->initializeMeters(op->EnergyMeters, ReportingFrequency::TimeStep);
        }

        PrintTimeStamp = true;
        for (Loop = 1; Loop <= op->NumEnergyMeters; ++Loop) {
            op->EnergyMeters(Loop).CurTSValue = op->EnergyMeters(Loop).TSValue;
            if (!op->EnergyMeters(Loop).RptTS && !op->EnergyMeters(Loop).RptAccTS) continue;
            if (PrintTimeStamp) {
                CurDayType = state.dataEnvrn->DayOfWeek;
                if (state.dataEnvrn->HolidayIndex > 0) {
                    CurDayType = state.dataEnvrn->HolidayIndex;
                }
                WriteTimeStampFormatData(state,
                                         state.files.mtr,
                                         ReportingFrequency::EachCall,
                                         op->TimeStepStampReportNbr,
                                         op->TimeStepStampReportChr,
                                         state.dataGlobal->DayOfSimChr,
                                         PrintTimeStamp && PrintTimeStampToSQL,
                                         state.dataEnvrn->Month,
                                         state.dataEnvrn->DayOfMonth,
                                         state.dataGlobal->HourOfDay,
                                         EndMinute,
                                         StartMinute,
                                         state.dataEnvrn->DSTIndicator,
                                         ScheduleManager::dayTypeNames[CurDayType]);
                if (state.dataResultsFramework->resultsFramework->TSMeters.rDataFrameEnabled()) {
                    state.dataResultsFramework->resultsFramework->TSMeters.newRow(
                        state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, EndMinute, state.dataGlobal->CalendarYear);
                }
                PrintTimeStamp = false;
                PrintTimeStampToSQL = false;
            }

            if (PrintESOTimeStamp && !op->EnergyMeters(Loop).RptTSFO && !op->EnergyMeters(Loop).RptAccTSFO) {
                CurDayType = state.dataEnvrn->DayOfWeek;
                if (state.dataEnvrn->HolidayIndex > 0) {
                    CurDayType = state.dataEnvrn->HolidayIndex;
                }
                WriteTimeStampFormatData(state,
                                         state.files.eso,
                                         ReportingFrequency::EachCall,
                                         op->TimeStepStampReportNbr,
                                         op->TimeStepStampReportChr,
                                         state.dataGlobal->DayOfSimChr,
                                         PrintTimeStamp && PrintESOTimeStamp && PrintTimeStampToSQL,
                                         state.dataEnvrn->Month,
                                         state.dataEnvrn->DayOfMonth,
                                         state.dataGlobal->HourOfDay,
                                         EndMinute,
                                         StartMinute,
                                         state.dataEnvrn->DSTIndicator,
                                         ScheduleManager::dayTypeNames[CurDayType]);
                PrintESOTimeStamp = false;
            }

            if (op->EnergyMeters(Loop).RptTS) {
                WriteReportMeterData(state,
                                     op->EnergyMeters(Loop).TSRptNum,
                                     op->EnergyMeters(Loop).TSRptNumChr,
                                     op->EnergyMeters(Loop).TSValue,
                                     ReportingFrequency::TimeStep,
                                     state.dataOutputProcessor->rDummy1TS,
                                     state.dataOutputProcessor->iDummy1TS,
                                     state.dataOutputProcessor->rDummy2TS,
                                     state.dataOutputProcessor->iDummy2TS,
                                     op->EnergyMeters(Loop).RptTSFO);
                state.dataResultsFramework->resultsFramework->TSMeters.pushVariableValue(op->EnergyMeters(Loop).TSRptNum,
                                                                                         op->EnergyMeters(Loop).TSValue);
            }

            if (op->EnergyMeters(Loop).RptAccTS) {
                WriteCumulativeReportMeterData(state,
                                               op->EnergyMeters(Loop).TSAccRptNum,
                                               fmt::to_string(op->EnergyMeters(Loop).TSAccRptNum),
                                               op->EnergyMeters(Loop).SMValue,
                                               op->EnergyMeters(Loop).RptAccTSFO);
                state.dataResultsFramework->resultsFramework->TSMeters.pushVariableValue(op->EnergyMeters(Loop).TSAccRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }
        }

        if (op->NumEnergyMeters > 0) {
            for (auto &e : op->EnergyMeters)
                e.TSValue = 0.0;
        }
    }

    void ReportHRMeters(EnergyPlusData &state, bool PrintTimeStampToSQL // Print Time Stamp to SQL file
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine reports on the meters that have been requested for
        // reporting on each hour.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop; // Loop Control
        bool PrintTimeStamp;
        int CurDayType;
        auto &op = state.dataOutputProcessor;

        if (!state.dataResultsFramework->resultsFramework->HRMeters.rDataFrameEnabled()) {
            state.dataResultsFramework->resultsFramework->initializeMeters(op->EnergyMeters, ReportingFrequency::Hourly);
        }

        PrintTimeStamp = true;
        for (Loop = 1; Loop <= op->NumEnergyMeters; ++Loop) {
            if (!op->EnergyMeters(Loop).RptHR && !op->EnergyMeters(Loop).RptAccHR) continue;
            if (PrintTimeStamp) {
                CurDayType = state.dataEnvrn->DayOfWeek;
                if (state.dataEnvrn->HolidayIndex > 0) {
                    CurDayType = state.dataEnvrn->HolidayIndex;
                }
                WriteTimeStampFormatData(state,
                                         state.files.mtr,
                                         ReportingFrequency::Hourly,
                                         op->TimeStepStampReportNbr,
                                         op->TimeStepStampReportChr,
                                         state.dataGlobal->DayOfSimChr,
                                         PrintTimeStamp && PrintTimeStampToSQL,
                                         state.dataEnvrn->Month,
                                         state.dataEnvrn->DayOfMonth,
                                         state.dataGlobal->HourOfDay,
                                         _,
                                         _,
                                         state.dataEnvrn->DSTIndicator,
                                         ScheduleManager::dayTypeNames[CurDayType]);
                if (state.dataResultsFramework->resultsFramework->HRMeters.rDataFrameEnabled()) {
                    state.dataResultsFramework->resultsFramework->HRMeters.newRow(
                        state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
                }
                PrintTimeStamp = false;
                PrintTimeStampToSQL = false;
            }

            if (op->EnergyMeters(Loop).RptHR) {
                WriteReportMeterData(state,
                                     op->EnergyMeters(Loop).HRRptNum,
                                     op->EnergyMeters(Loop).HRRptNumChr,
                                     op->EnergyMeters(Loop).HRValue,
                                     ReportingFrequency::Hourly,
                                     state.dataOutputProcessor->rDummy1,
                                     state.dataOutputProcessor->iDummy1,
                                     state.dataOutputProcessor->rDummy2,
                                     state.dataOutputProcessor->iDummy2,
                                     op->EnergyMeters(Loop).RptHRFO); // EnergyMeters(Loop)%HRMinVal, EnergyMeters(Loop)%HRMinValDate, & |
                                                                      // EnergyMeters(Loop)%HRMaxVal, EnergyMeters(Loop)%HRMaxValDate, &
                state.dataResultsFramework->resultsFramework->HRMeters.pushVariableValue(op->EnergyMeters(Loop).HRRptNum,
                                                                                         op->EnergyMeters(Loop).HRValue);
                op->EnergyMeters(Loop).HRValue = 0.0;
            }

            if (op->EnergyMeters(Loop).RptAccHR) {
                WriteCumulativeReportMeterData(state,
                                               op->EnergyMeters(Loop).HRAccRptNum,
                                               fmt::to_string(op->EnergyMeters(Loop).HRAccRptNum),
                                               op->EnergyMeters(Loop).SMValue,
                                               op->EnergyMeters(Loop).RptAccHRFO);
                state.dataResultsFramework->resultsFramework->HRMeters.pushVariableValue(op->EnergyMeters(Loop).HRAccRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }
        }
    }

    void ReportDYMeters(EnergyPlusData &state, bool PrintTimeStampToSQL // Print Time Stamp to SQL file
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine reports on the meters that have been requested for
        // reporting on each day.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop; // Loop Control
        bool PrintTimeStamp;
        int CurDayType;
        auto &op = state.dataOutputProcessor;

        if (!state.dataResultsFramework->resultsFramework->DYMeters.rVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeMeters(op->EnergyMeters, ReportingFrequency::Daily);
        }

        PrintTimeStamp = true;
        for (Loop = 1; Loop <= op->NumEnergyMeters; ++Loop) {
            if (!op->EnergyMeters(Loop).RptDY && !op->EnergyMeters(Loop).RptAccDY) continue;
            if (PrintTimeStamp) {
                CurDayType = state.dataEnvrn->DayOfWeek;
                if (state.dataEnvrn->HolidayIndex > 0) {
                    CurDayType = state.dataEnvrn->HolidayIndex;
                }
                WriteTimeStampFormatData(state,
                                         state.files.mtr,
                                         ReportingFrequency::Daily,
                                         op->DailyStampReportNbr,
                                         op->DailyStampReportChr,
                                         state.dataGlobal->DayOfSimChr,
                                         PrintTimeStamp && PrintTimeStampToSQL,
                                         state.dataEnvrn->Month,
                                         state.dataEnvrn->DayOfMonth,
                                         _,
                                         _,
                                         _,
                                         state.dataEnvrn->DSTIndicator,
                                         ScheduleManager::dayTypeNames[CurDayType]);
                if (state.dataResultsFramework->resultsFramework->DYMeters.rDataFrameEnabled()) {
                    state.dataResultsFramework->resultsFramework->DYMeters.newRow(
                        state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
                }
                PrintTimeStamp = false;
                PrintTimeStampToSQL = false;
            }

            if (op->EnergyMeters(Loop).RptDY) {
                WriteReportMeterData(state,
                                     op->EnergyMeters(Loop).DYRptNum,
                                     op->EnergyMeters(Loop).DYRptNumChr,
                                     op->EnergyMeters(Loop).DYValue,
                                     ReportingFrequency::Daily,
                                     op->EnergyMeters(Loop).DYMinVal,
                                     op->EnergyMeters(Loop).DYMinValDate,
                                     op->EnergyMeters(Loop).DYMaxVal,
                                     op->EnergyMeters(Loop).DYMaxValDate,
                                     op->EnergyMeters(Loop).RptDYFO);
                state.dataResultsFramework->resultsFramework->DYMeters.pushVariableValue(op->EnergyMeters(Loop).DYRptNum,
                                                                                         op->EnergyMeters(Loop).DYValue);
                op->EnergyMeters(Loop).DYValue = 0.0;
                op->EnergyMeters(Loop).DYMinVal = MinSetValue;
                op->EnergyMeters(Loop).DYMaxVal = MaxSetValue;
            }

            if (op->EnergyMeters(Loop).RptAccDY) {
                WriteCumulativeReportMeterData(state,
                                               op->EnergyMeters(Loop).DYAccRptNum,
                                               fmt::to_string(op->EnergyMeters(Loop).DYAccRptNum),
                                               op->EnergyMeters(Loop).SMValue,
                                               op->EnergyMeters(Loop).RptAccDYFO);
                state.dataResultsFramework->resultsFramework->DYMeters.pushVariableValue(op->EnergyMeters(Loop).DYAccRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }
        }
    }

    void ReportMNMeters(EnergyPlusData &state, bool PrintTimeStampToSQL // Print Time Stamp to SQL file
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine reports on the meters that have been requested for
        // reporting on each month.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop; // Loop Control
        bool PrintTimeStamp;
        auto &op = state.dataOutputProcessor;

        if (!state.dataResultsFramework->resultsFramework->MNMeters.rVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeMeters(op->EnergyMeters, ReportingFrequency::Monthly);
        }

        PrintTimeStamp = true;
        for (Loop = 1; Loop <= op->NumEnergyMeters; ++Loop) {
            if (!op->EnergyMeters(Loop).RptMN && !op->EnergyMeters(Loop).RptAccMN) continue;
            if (PrintTimeStamp) {
                WriteTimeStampFormatData(state,
                                         state.files.mtr,
                                         ReportingFrequency::Monthly,
                                         op->MonthlyStampReportNbr,
                                         op->MonthlyStampReportChr,
                                         state.dataGlobal->DayOfSimChr,
                                         PrintTimeStamp && PrintTimeStampToSQL,
                                         state.dataEnvrn->Month);
                if (state.dataResultsFramework->resultsFramework->MNMeters.rDataFrameEnabled()) {
                    state.dataResultsFramework->resultsFramework->MNMeters.newRow(
                        state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
                }
                PrintTimeStamp = false;
                PrintTimeStampToSQL = false;
            }

            if (op->EnergyMeters(Loop).RptMN) {
                WriteReportMeterData(state,
                                     op->EnergyMeters(Loop).MNRptNum,
                                     op->EnergyMeters(Loop).MNRptNumChr,
                                     op->EnergyMeters(Loop).MNValue,
                                     ReportingFrequency::Monthly,
                                     op->EnergyMeters(Loop).MNMinVal,
                                     op->EnergyMeters(Loop).MNMinValDate,
                                     op->EnergyMeters(Loop).MNMaxVal,
                                     op->EnergyMeters(Loop).MNMaxValDate,
                                     op->EnergyMeters(Loop).RptMNFO);
                state.dataResultsFramework->resultsFramework->MNMeters.pushVariableValue(op->EnergyMeters(Loop).MNRptNum,
                                                                                         op->EnergyMeters(Loop).MNValue);
                op->EnergyMeters(Loop).MNValue = 0.0;
                op->EnergyMeters(Loop).MNMinVal = MinSetValue;
                op->EnergyMeters(Loop).MNMaxVal = MaxSetValue;
            }

            if (op->EnergyMeters(Loop).RptAccMN) {
                WriteCumulativeReportMeterData(state,
                                               op->EnergyMeters(Loop).MNAccRptNum,
                                               fmt::to_string(op->EnergyMeters(Loop).MNAccRptNum),
                                               op->EnergyMeters(Loop).SMValue,
                                               op->EnergyMeters(Loop).RptAccMNFO);
                state.dataResultsFramework->resultsFramework->MNMeters.pushVariableValue(op->EnergyMeters(Loop).MNAccRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }
        }
    }

    void ReportYRMeters(EnergyPlusData &state, bool PrintTimeStampToSQL)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Jason DeGraw
        //       DATE WRITTEN   January 2018
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine reports on the meters that have been requested for
        // reporting on each year.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop; // Loop Control
        bool PrintTimeStamp;
        auto &op = state.dataOutputProcessor;

        if (!state.dataResultsFramework->resultsFramework->YRMeters.rVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeMeters(op->EnergyMeters, ReportingFrequency::Yearly);
        }

        PrintTimeStamp = true;
        for (Loop = 1; Loop <= op->NumEnergyMeters; ++Loop) {
            if (!op->EnergyMeters(Loop).RptYR && !op->EnergyMeters(Loop).RptAccYR) continue;
            if (PrintTimeStamp) {
                WriteYearlyTimeStamp(
                    state, state.files.mtr, op->YearlyStampReportChr, state.dataGlobal->CalendarYearChr, PrintTimeStamp && PrintTimeStampToSQL);
                if (state.dataResultsFramework->resultsFramework->YRMeters.rDataFrameEnabled()) {
                    state.dataResultsFramework->resultsFramework->YRMeters.newRow(
                        state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
                }
                PrintTimeStamp = false;
                PrintTimeStampToSQL = false;
            }

            if (op->EnergyMeters(Loop).RptYR) {
                WriteReportMeterData(state,
                                     op->EnergyMeters(Loop).YRRptNum,
                                     op->EnergyMeters(Loop).YRRptNumChr,
                                     op->EnergyMeters(Loop).YRValue,
                                     ReportingFrequency::Yearly,
                                     op->EnergyMeters(Loop).YRMinVal,
                                     op->EnergyMeters(Loop).YRMinValDate,
                                     op->EnergyMeters(Loop).YRMaxVal,
                                     op->EnergyMeters(Loop).YRMaxValDate,
                                     op->EnergyMeters(Loop).RptYRFO);
                state.dataResultsFramework->resultsFramework->YRMeters.pushVariableValue(op->EnergyMeters(Loop).YRRptNum,
                                                                                         op->EnergyMeters(Loop).YRValue);
                op->EnergyMeters(Loop).YRValue = 0.0;
                op->EnergyMeters(Loop).YRMinVal = MinSetValue;
                op->EnergyMeters(Loop).YRMaxVal = MaxSetValue;
            }

            if (op->EnergyMeters(Loop).RptAccYR) {
                WriteCumulativeReportMeterData(state,
                                               op->EnergyMeters(Loop).YRAccRptNum,
                                               fmt::to_string(op->EnergyMeters(Loop).YRAccRptNum),
                                               op->EnergyMeters(Loop).YRValue,
                                               op->EnergyMeters(Loop).RptAccYRFO);
                state.dataResultsFramework->resultsFramework->YRMeters.pushVariableValue(op->EnergyMeters(Loop).YRAccRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }
        }
    }

    void ReportSMMeters(EnergyPlusData &state, bool PrintTimeStampToSQL // Print Time Stamp to SQL file
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2001
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine reports on the meters that have been requested for
        // reporting on each environment/run period.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Loop; // Loop Control
        bool PrintTimeStamp;
        auto &op = state.dataOutputProcessor;

        if (!state.dataResultsFramework->resultsFramework->SMMeters.rVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeMeters(op->EnergyMeters, ReportingFrequency::Simulation);
        }

        PrintTimeStamp = true;
        for (Loop = 1; Loop <= op->NumEnergyMeters; ++Loop) {
            op->EnergyMeters(Loop).LastSMValue = op->EnergyMeters(Loop).SMValue;
            op->EnergyMeters(Loop).LastSMMinVal = op->EnergyMeters(Loop).SMMinVal;
            op->EnergyMeters(Loop).LastSMMinValDate = op->EnergyMeters(Loop).SMMinValDate;
            op->EnergyMeters(Loop).LastSMMaxVal = op->EnergyMeters(Loop).SMMaxVal;
            op->EnergyMeters(Loop).LastSMMaxValDate = op->EnergyMeters(Loop).SMMaxValDate;
            if (!op->EnergyMeters(Loop).RptSM && !op->EnergyMeters(Loop).RptAccSM) continue;
            if (PrintTimeStamp) {
                WriteTimeStampFormatData(state,
                                         state.files.mtr,
                                         ReportingFrequency::Simulation,
                                         op->RunPeriodStampReportNbr,
                                         op->RunPeriodStampReportChr,
                                         state.dataGlobal->DayOfSimChr,
                                         PrintTimeStamp && PrintTimeStampToSQL);
                if (state.dataResultsFramework->resultsFramework->SMMeters.rDataFrameEnabled()) {
                    state.dataResultsFramework->resultsFramework->SMMeters.newRow(
                        state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
                }
                PrintTimeStamp = false;
                PrintTimeStampToSQL = false;
            }

            if (op->EnergyMeters(Loop).RptSM) {
                WriteReportMeterData(state,
                                     op->EnergyMeters(Loop).SMRptNum,
                                     op->EnergyMeters(Loop).SMRptNumChr,
                                     op->EnergyMeters(Loop).SMValue,
                                     ReportingFrequency::Simulation,
                                     op->EnergyMeters(Loop).SMMinVal,
                                     op->EnergyMeters(Loop).SMMinValDate,
                                     op->EnergyMeters(Loop).SMMaxVal,
                                     op->EnergyMeters(Loop).SMMaxValDate,
                                     op->EnergyMeters(Loop).RptSMFO);
                state.dataResultsFramework->resultsFramework->SMMeters.pushVariableValue(op->EnergyMeters(Loop).SMRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }

            if (op->EnergyMeters(Loop).RptAccSM) {
                WriteCumulativeReportMeterData(state,
                                               op->EnergyMeters(Loop).SMAccRptNum,
                                               fmt::to_string(op->EnergyMeters(Loop).SMAccRptNum),
                                               op->EnergyMeters(Loop).SMValue,
                                               op->EnergyMeters(Loop).RptAccSMFO);
                state.dataResultsFramework->resultsFramework->SMMeters.pushVariableValue(op->EnergyMeters(Loop).SMAccRptNum,
                                                                                         op->EnergyMeters(Loop).SMValue);
            }
        }

        if (op->NumEnergyMeters > 0) {
            for (auto &e : op->EnergyMeters) {
                e.SMValue = 0.0;
                e.SMMinVal = MinSetValue;
                e.SMMaxVal = MaxSetValue;
            }
        }
    }

    void ReportForTabularReports(EnergyPlusData &state)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   August 2013
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine is called after all the simulation is done and before
        // tabular reports in order to reduce the number of calls to the predefined routine
        // for SM (Simulation period) meters, the value of the last calculation is stored
        // in the data structure.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        auto &op = state.dataOutputProcessor;

        for (auto &m : op->EnergyMeters) {
            OutputProcessor::RT_IPUnits const RT_forIPUnits(m.RT_forIPUnits);
            if (RT_forIPUnits == RT_IPUnits::Electricity) {
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMelecannual, m.Name, m.FinYrSMValue * Constant::convertJtoGJ);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMelecminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMelecminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMelecmaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMelecmaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else if (RT_forIPUnits == RT_IPUnits::Gas) {
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMgasannual, m.Name, m.FinYrSMValue * Constant::convertJtoGJ);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMgasminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMgasminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMgasmaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMgasmaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else if (RT_forIPUnits == RT_IPUnits::Cooling) {
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMcoolannual, m.Name, m.FinYrSMValue * Constant::convertJtoGJ);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMcoolminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMcoolminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMcoolmaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMcoolmaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else if (RT_forIPUnits == RT_IPUnits::Water) {
                OutputReportPredefined::PreDefTableEntry(state, state.dataOutRptPredefined->pdchEMwaterannual, m.Name, m.FinYrSMValue);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMwaterminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMwaterminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMwatermaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMwatermaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else if (RT_forIPUnits == RT_IPUnits::OtherKG) {
                OutputReportPredefined::PreDefTableEntry(state, state.dataOutRptPredefined->pdchEMotherKGannual, m.Name, m.FinYrSMValue);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherKGminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherKGminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherKGmaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherKGmaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else if (RT_forIPUnits == RT_IPUnits::OtherM3) {
                OutputReportPredefined::PreDefTableEntry(state, state.dataOutRptPredefined->pdchEMotherM3annual, m.Name, m.FinYrSMValue, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherM3minvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherM3minvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherM3maxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherM3maxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else if (RT_forIPUnits == RT_IPUnits::OtherL) {
                OutputReportPredefined::PreDefTableEntry(state, state.dataOutRptPredefined->pdchEMotherLannual, m.Name, m.FinYrSMValue, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherLminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherLminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherLmaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec, 3);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherLmaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            } else {
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherJannual, m.Name, m.FinYrSMValue * Constant::convertJtoGJ);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherJminvalue, m.Name, m.FinYrSMMinVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherJminvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMinValDate));
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherJmaxvalue, m.Name, m.FinYrSMMaxVal / state.dataGlobal->TimeStepZoneSec);
                OutputReportPredefined::PreDefTableEntry(
                    state, state.dataOutRptPredefined->pdchEMotherJmaxvaluetime, m.Name, DateToStringWithMonth(m.FinYrSMMaxValDate));
            }
        }
    }

    std::string DateToStringWithMonth(int const codedDate) // word containing encoded month, day, hour, minute
    {
        // SUBROUTINE INFORMATION:
        //       AUTHOR         Jason Glazer
        //       DATE WRITTEN   August 2003
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        //   Convert the coded date format into a usable
        //   string

        if (codedDate == 0) return "-";

        static constexpr std::string_view DateFmt("{:02}-{:3}-{:02}:{:02}");

        // ((month*100 + day)*100 + hour)*100 + minute
        int Month;  // month in integer format (1-12)
        int Day;    // day in integer format (1-31)
        int Hour;   // hour in integer format (1-24)
        int Minute; // minute in integer format (0:59)

        General::DecodeMonDayHrMin(codedDate, Month, Day, Hour, Minute);

        if (Month < 1 || Month > 12) return "-";
        if (Day < 1 || Day > 31) return "-";
        if (Hour < 1 || Hour > 24) return "-";
        if (Minute < 0 || Minute > 60) return "-";

        --Hour;
        if (Minute == 60) {
            ++Hour;
            Minute = 0;
        }

        std::string monthName;
        switch (Month) {
        case 1:
            monthName = "JAN";
            break;
        case 2:
            monthName = "FEB";
            break;
        case 3:
            monthName = "MAR";
            break;
        case 4:
            monthName = "APR";
            break;
        case 5:
            monthName = "MAY";
            break;
        case 6:
            monthName = "JUN";
            break;
        case 7:
            monthName = "JUL";
            break;
        case 8:
            monthName = "AUG";
            break;
        case 9:
            monthName = "SEP";
            break;
        case 10:
            monthName = "OCT";
            break;
        case 11:
            monthName = "NOV";
            break;
        case 12:
            monthName = "DEC";
            break;
        default:
            assert(false);
        }

        return format(DateFmt, Day, monthName, Hour, Minute);
    }

    void ReportMeterDetails(EnergyPlusData &state)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Linda Lawrie
        //       DATE WRITTEN   January 2006
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // Writes the meter details report.  This shows which variables are on
        // meters as well as the meter contents.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        auto &op = state.dataOutputProcessor;

        for (int VarMeter = 1; VarMeter <= op->NumVarMeterArrays; ++VarMeter) {

            const std::string mtrUnitString = unitEnumToStringBrackets(op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).units);

            std::string Multipliers;
            Real64 const ZoneMult = op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ZoneMult;
            Real64 const ZoneListMult = op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ZoneListMult;

            if (ZoneMult > 1 || ZoneListMult > 1) {
                Multipliers = format(" * {}  (Zone Multiplier = {}, Zone List Multiplier = {})", ZoneMult * ZoneListMult, ZoneMult, ZoneListMult);
            }

            print(state.files.mtd,
                  "\n Meters for {},{}{}{}\n",
                  op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ReportIDChr,
                  op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarName,
                  mtrUnitString,
                  Multipliers);

            for (int I = 1; I <= op->VarMeterArrays(VarMeter).NumOnMeters; ++I) {
                print(state.files.mtd, "  OnMeter={}{}\n", op->EnergyMeters(op->VarMeterArrays(VarMeter).OnMeters(I)).Name, mtrUnitString);
            }

            for (int I = 1; I <= op->VarMeterArrays(VarMeter).NumOnCustomMeters; ++I) {
                print(
                    state.files.mtd, "  OnCustomMeter={}{}\n", op->EnergyMeters(op->VarMeterArrays(VarMeter).OnCustomMeters(I)).Name, mtrUnitString);
            }
        }

        for (int Meter = 1; Meter <= op->NumEnergyMeters; ++Meter) {
            print(state.files.mtd, "\n For Meter={}{}", op->EnergyMeters(Meter).Name, unitEnumToStringBrackets(op->EnergyMeters(Meter).Units));
            if (!op->EnergyMeters(Meter).ResourceType.empty()) {
                print(state.files.mtd, ", ResourceType={}", op->EnergyMeters(Meter).ResourceType);
            }
            if (!op->EnergyMeters(Meter).EndUse.empty()) {
                print(state.files.mtd, ", EndUse={}", op->EnergyMeters(Meter).EndUse);
            }
            if (!op->EnergyMeters(Meter).Group.empty()) {
                print(state.files.mtd, ", Group={}", op->EnergyMeters(Meter).Group);
            }
            print(state.files.mtd, ", contents are:\n");

            bool CustDecWritten = false;

            for (int VarMeter = 1; VarMeter <= op->NumVarMeterArrays; ++VarMeter) {
                if (op->EnergyMeters(Meter).TypeOfMeter == MtrType::Normal) {
                    if (any_eq(op->VarMeterArrays(VarMeter).OnMeters, Meter)) {
                        for (int VarMeter1 = 1; VarMeter1 <= op->VarMeterArrays(VarMeter).NumOnMeters; ++VarMeter1) {
                            if (op->VarMeterArrays(VarMeter).OnMeters(VarMeter1) != Meter) continue;

                            std::string Multipliers;
                            Real64 const ZoneMult = op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ZoneMult;
                            Real64 const ZoneListMult = op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ZoneListMult;

                            if (ZoneMult > 1 || ZoneListMult > 1) {
                                Multipliers = format(
                                    " * {}  (Zone Multiplier = {}, Zone List Multiplier = {})", ZoneMult * ZoneListMult, ZoneMult, ZoneListMult);
                            }

                            print(state.files.mtd, "  {}{}\n", op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarName, Multipliers);
                        }
                    }
                }
                if (op->EnergyMeters(Meter).TypeOfMeter != MtrType::Normal) {
                    if (op->VarMeterArrays(VarMeter).NumOnCustomMeters > 0) {
                        if (any_eq(op->VarMeterArrays(VarMeter).OnCustomMeters, Meter)) {
                            if (!CustDecWritten && op->EnergyMeters(Meter).TypeOfMeter == MtrType::CustomDec) {
                                print(state.files.mtd,
                                      " Values for this meter will be Source Meter={}; but will be decremented by:\n",
                                      op->EnergyMeters(op->EnergyMeters(Meter).SourceMeter).Name);
                                CustDecWritten = true;
                            }
                            for (int VarMeter1 = 1; VarMeter1 <= op->VarMeterArrays(VarMeter).NumOnCustomMeters; ++VarMeter1) {
                                if (op->VarMeterArrays(VarMeter).OnCustomMeters(VarMeter1) != Meter) continue;

                                std::string Multipliers;
                                Real64 const ZoneMult = op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ZoneMult;
                                Real64 const ZoneListMult = op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarPtr.ZoneListMult;

                                if (ZoneMult > 1 || ZoneListMult > 1) {
                                    Multipliers = format(
                                        " * {}  (Zone Multiplier = {}, Zone List Multiplier = {})", ZoneMult * ZoneListMult, ZoneMult, ZoneListMult);
                                }

                                print(state.files.mtd, "  {}{}\n", op->RVariableTypes(op->VarMeterArrays(VarMeter).RepVariable).VarName, Multipliers);
                            }
                        }
                    }
                }
            }
        }
    }

    // *****************************************************************************
    // End of routines for Energy Meters implementation in EnergyPlus.
    // *****************************************************************************

    void addEndUseSubcategory(EnergyPlusData &state, std::string const &EndUseName, std::string const &EndUseSubName)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Peter Graham Ellis
        //       DATE WRITTEN   February 2006
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine manages the list of subcategories for each end-use category.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int EndUseSubNum;
        int NumSubs;
        auto &op = state.dataOutputProcessor;

        bool Found = false;
        for (size_t EndUseNum = 1; EndUseNum <= static_cast<size_t>(Constant::EndUse::Num); ++EndUseNum) {
            if (Util::SameString(op->EndUseCategory(EndUseNum).Name, EndUseName)) {

                for (EndUseSubNum = 1; EndUseSubNum <= op->EndUseCategory(EndUseNum).NumSubcategories; ++EndUseSubNum) {
                    if (Util::SameString(op->EndUseCategory(EndUseNum).SubcategoryName(EndUseSubNum), EndUseSubName)) {
                        // Subcategory already exists, no further action required
                        Found = true;
                        break;
                    }
                }

                if (!Found) {
                    // Add the subcategory by reallocating the array
                    NumSubs = op->EndUseCategory(EndUseNum).NumSubcategories;
                    op->EndUseCategory(EndUseNum).SubcategoryName.redimension(NumSubs + 1);

                    op->EndUseCategory(EndUseNum).NumSubcategories = NumSubs + 1;
                    op->EndUseCategory(EndUseNum).SubcategoryName(NumSubs + 1) = EndUseSubName;

                    if (op->EndUseCategory(EndUseNum).NumSubcategories > op->MaxNumSubcategories) {
                        op->MaxNumSubcategories = op->EndUseCategory(EndUseNum).NumSubcategories;
                    }

                    Found = true;
                }
                break;
            }
        }

        if (!Found) {
            ShowSevereError(state, format("Nonexistent end use passed to AddEndUseSubcategory={}", EndUseName));
        }
    }
    void addEndUseSpaceType(EnergyPlusData &state, std::string const &EndUseName, std::string const &EndUseSpaceTypeName)
    {

        auto &op = state.dataOutputProcessor;

        bool Found = false;
        for (size_t EndUseNum = 1; EndUseNum <= static_cast<size_t>(Constant::EndUse::Num); ++EndUseNum) {
            if (Util::SameString(op->EndUseCategory(EndUseNum).Name, EndUseName)) {

                for (int endUseSpTypeNum = 1; endUseSpTypeNum <= op->EndUseCategory(EndUseNum).numSpaceTypes; ++endUseSpTypeNum) {
                    if (Util::SameString(op->EndUseCategory(EndUseNum).spaceTypeName(endUseSpTypeNum), EndUseSpaceTypeName)) {
                        // Space type already exists, no further action required
                        Found = true;
                        break;
                    }
                }

                if (!Found) {
                    // Add the space type by reallocating the array
                    int numSpTypes = op->EndUseCategory(EndUseNum).numSpaceTypes;
                    op->EndUseCategory(EndUseNum).spaceTypeName.redimension(numSpTypes + 1);

                    op->EndUseCategory(EndUseNum).numSpaceTypes = numSpTypes + 1;
                    op->EndUseCategory(EndUseNum).spaceTypeName(numSpTypes + 1) = EndUseSpaceTypeName;

                    if (op->EndUseCategory(EndUseNum).numSpaceTypes > op->maxNumEndUseSpaceTypes) {
                        op->maxNumEndUseSpaceTypes = op->EndUseCategory(EndUseNum).numSpaceTypes;
                    }

                    Found = true;
                }
                break;
            }
        }

        if (!Found) {
            ShowSevereError(state, format("Nonexistent end use passed to addEndUseSpaceType={}", EndUseName));
        }
    }

    void WriteTimeStampFormatData(
        EnergyPlusData &state,
        InputOutputFile &outputFile,
        ReportingFrequency const reportingInterval, // See Module Parameter Definitions for ReportEach, ReportTimeStep, ReportHourly, etc.
        int const reportID,                         // The ID of the time stamp
        std::string const &reportIDString,          // The ID of the time stamp
        std::string const &DayOfSimChr,             // the number of days simulated so far
        bool writeToSQL,
        ObjexxFCL::Optional_int_const Month,           // the month of the reporting interval
        ObjexxFCL::Optional_int_const DayOfMonth,      // The day of the reporting interval
        ObjexxFCL::Optional_int_const Hour,            // The hour of the reporting interval
        ObjexxFCL::Optional<Real64 const> EndMinute,   // The last minute in the reporting interval
        ObjexxFCL::Optional<Real64 const> StartMinute, // The starting minute of the reporting interval
        ObjexxFCL::Optional_int_const DST,             // A flag indicating whether daylight savings time is observed
        ObjexxFCL::Optional_string_const DayType       // The day tied for the data (e.g., Monday)
    )
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   July 2008
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function reports the timestamp data for the output processor
        // Much of the code in this function was embedded in earlier versions of EnergyPlus
        // and was moved to this location to simplify maintenance and to allow for data output
        // to the SQL database

        assert(reportIDString.length() + DayOfSimChr.length() + (DayType.present() ? DayType().length() : 0u) + 26 <
               N_WriteTimeStampFormatData); // Check will fit in stamp size

        if (!outputFile.good()) return;

        switch (reportingInterval) {
        case ReportingFrequency::EachCall:
        case ReportingFrequency::TimeStep:
            print<FormatSyntax::FMT>(outputFile,
                                     "{},{},{:2d},{:2d},{:2d},{:2d},{:5.2f},{:5.2f},{}\n",
                                     reportIDString.c_str(),
                                     DayOfSimChr.c_str(),
                                     Month(),
                                     DayOfMonth(),
                                     DST(),
                                     Hour(),
                                     StartMinute(),
                                     EndMinute(),
                                     DayType().c_str());
            if (writeToSQL && state.dataSQLiteProcedures->sqlite) {
                state.dataSQLiteProcedures->sqlite->createSQLiteTimeIndexRecord(static_cast<int>(reportingInterval),
                                                                                reportID,
                                                                                state.dataGlobal->DayOfSim,
                                                                                state.dataEnvrn->CurEnvirNum,
                                                                                state.dataGlobal->CalendarYear,
                                                                                state.dataEnvrn->CurrentYearIsLeapYear,
                                                                                Month,
                                                                                DayOfMonth,
                                                                                Hour,
                                                                                EndMinute,
                                                                                StartMinute,
                                                                                DST,
                                                                                DayType,
                                                                                state.dataGlobal->WarmupFlag);
            }
            break;
        case ReportingFrequency::Hourly:
            print<FormatSyntax::FMT>(outputFile,
                                     "{},{},{:2d},{:2d},{:2d},{:2d},{:5.2f},{:5.2f},{}\n",
                                     reportIDString.c_str(),
                                     DayOfSimChr.c_str(),
                                     Month(),
                                     DayOfMonth(),
                                     DST(),
                                     Hour(),
                                     0.0,
                                     60.0,
                                     DayType().c_str());
            if (writeToSQL && state.dataSQLiteProcedures->sqlite) {
                state.dataSQLiteProcedures->sqlite->createSQLiteTimeIndexRecord(static_cast<int>(reportingInterval),
                                                                                reportID,
                                                                                state.dataGlobal->DayOfSim,
                                                                                state.dataEnvrn->CurEnvirNum,
                                                                                state.dataGlobal->CalendarYear,
                                                                                state.dataEnvrn->CurrentYearIsLeapYear,
                                                                                Month,
                                                                                DayOfMonth,
                                                                                Hour,
                                                                                _,
                                                                                _,
                                                                                DST,
                                                                                DayType,
                                                                                state.dataGlobal->WarmupFlag);
            }
            break;
        case ReportingFrequency::Daily:
            print<FormatSyntax::FMT>(outputFile,
                                     "{},{},{:2d},{:2d},{:2d},{}\n",
                                     reportIDString.c_str(),
                                     DayOfSimChr.c_str(),
                                     Month(),
                                     DayOfMonth(),
                                     DST(),
                                     DayType().c_str());
            if (writeToSQL && state.dataSQLiteProcedures->sqlite) {
                state.dataSQLiteProcedures->sqlite->createSQLiteTimeIndexRecord(static_cast<int>(reportingInterval),
                                                                                reportID,
                                                                                state.dataGlobal->DayOfSim,
                                                                                state.dataEnvrn->CurEnvirNum,
                                                                                state.dataGlobal->CalendarYear,
                                                                                state.dataEnvrn->CurrentYearIsLeapYear,
                                                                                Month,
                                                                                DayOfMonth,
                                                                                _,
                                                                                _,
                                                                                _,
                                                                                DST,
                                                                                DayType,
                                                                                state.dataGlobal->WarmupFlag);
            }
            break;
        case ReportingFrequency::Monthly:
            print<FormatSyntax::FMT>(outputFile, "{},{},{:2d}\n", reportIDString.c_str(), DayOfSimChr.c_str(), Month());
            if (writeToSQL && state.dataSQLiteProcedures->sqlite) {
                state.dataSQLiteProcedures->sqlite->createSQLiteTimeIndexRecord(static_cast<int>(reportingInterval),
                                                                                reportID,
                                                                                state.dataGlobal->DayOfSim,
                                                                                state.dataEnvrn->CurEnvirNum,
                                                                                state.dataGlobal->CalendarYear,
                                                                                state.dataEnvrn->CurrentYearIsLeapYear,
                                                                                Month);
            }
            break;
        case ReportingFrequency::Simulation:
            print<FormatSyntax::FMT>(outputFile, "{},{}\n", reportIDString.c_str(), DayOfSimChr.c_str());
            if (writeToSQL && state.dataSQLiteProcedures->sqlite) {
                state.dataSQLiteProcedures->sqlite->createSQLiteTimeIndexRecord(static_cast<int>(reportingInterval),
                                                                                reportID,
                                                                                state.dataGlobal->DayOfSim,
                                                                                state.dataEnvrn->CurEnvirNum,
                                                                                state.dataGlobal->CalendarYear,
                                                                                state.dataEnvrn->CurrentYearIsLeapYear);
            }
            break;
        default:
            if (state.dataSQLiteProcedures->sqlite) {
                state.dataSQLiteProcedures->sqlite->sqliteWriteMessage(format<FormatSyntax::FMT>(
                    "Illegal reportingInterval passed to WriteTimeStampFormatData: {}", static_cast<int>(reportingInterval)));
            }
            break;
        }
    }

    void WriteYearlyTimeStamp(EnergyPlusData &state,
                              InputOutputFile &outputFile,
                              std::string const &reportIDString, // The ID of the time stamp
                              std::string const &yearOfSimChr,   // the year of the simulation
                              bool writeToSQL)
    {
        print(outputFile, "{},{}\n", reportIDString, yearOfSimChr);
        if (writeToSQL && state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createYearlyTimeIndexRecord(state.dataGlobal->CalendarYear, state.dataEnvrn->CurEnvirNum);
        }
    }

    void WriteReportVariableDictionaryItem(EnergyPlusData &state,
                                           ReportingFrequency const reportingInterval, // The reporting interval (e.g., hourly, daily)
                                           StoreType const storeType,
                                           int const reportID,                       // The reporting ID for the data
                                           [[maybe_unused]] int const indexGroupKey, // The reporting group (e.g., Zone, Plant Loop, etc.)
                                           std::string const &indexGroup,            // The reporting group (e.g., Zone, Plant Loop, etc.)
                                           std::string const &reportIDChr,           // The reporting ID for the data
                                           std::string_view const keyedValue,        // The key name for the data
                                           std::string_view const variableName,      // The variable's actual name
                                           TimeStepType const timeStepType,
                                           OutputProcessor::Unit const unitsForVar, // The variables units
                                           ObjexxFCL::Optional_string_const customUnitName,
                                           std::string_view const ScheduleName)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   August 2008
        //       MODIFIED       April 2011; Linda Lawrie
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes the ESO data dictionary information to the output files
        // and the SQL database

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        std::string FreqString;
        auto &op = state.dataOutputProcessor;

        FreqString = frequencyNotice(storeType, reportingInterval);

        if (!ScheduleName.empty()) {
            FreqString = fmt::format("{},{}", FreqString, ScheduleName);
        }

        std::string UnitsString;
        if (unitsForVar == OutputProcessor::Unit::customEMS && present(customUnitName)) {
            UnitsString = customUnitName;
        } else {
            UnitsString = unitEnumToString(unitsForVar);
        }

        const auto write = [&](InputOutputFile &file, const int interval) {
            if (file.good()) {
                print(file, "{},{},{},{} [{}]{}\n", reportIDChr, interval, keyedValue, variableName, UnitsString, FreqString);
            }
        };
        switch (reportingInterval) {
        case ReportingFrequency::EachCall:
        case ReportingFrequency::TimeStep:
            write(state.files.eso, 1);
            break;
        case ReportingFrequency::Hourly:
            op->TrackingHourlyVariables = true;
            write(state.files.eso, 1);
            break;
        case ReportingFrequency::Daily:
            op->TrackingDailyVariables = true;
            write(state.files.eso, 7);
            break;
        case ReportingFrequency::Monthly:
            op->TrackingMonthlyVariables = true;
            write(state.files.eso, 9);
            break;
        case ReportingFrequency::Simulation:
            op->TrackingRunPeriodVariables = true;
            write(state.files.eso, 11);
            break;
        case ReportingFrequency::Yearly:
            op->TrackingYearlyVariables = true;
            write(state.files.eso, 11);
            break;
        default:
            assert(false);
        }

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDictionaryRecord(reportID,
                                                                                   static_cast<int>(storeType),
                                                                                   indexGroup,
                                                                                   keyedValue,
                                                                                   variableName,
                                                                                   static_cast<int>(timeStepType),
                                                                                   UnitsString,
                                                                                   static_cast<int>(reportingInterval),
                                                                                   false,
                                                                                   ScheduleName);
        }

        state.dataResultsFramework->resultsFramework->addReportVariable(keyedValue, variableName, UnitsString, reportingInterval);

        // add to ResultsFramework for output variable list, need to check RVI/MVI later
    }

    void WriteMeterDictionaryItem(EnergyPlusData &state,
                                  ReportingFrequency const reportingInterval, // The reporting interval (e.g., hourly, daily)
                                  StoreType const storeType,
                                  int const reportID,                       // The reporting ID in for the variable
                                  [[maybe_unused]] int const indexGroupKey, // The reporting group for the variable
                                  std::string const &indexGroup,            // The reporting group for the variable
                                  std::string const &reportIDChr,           // The reporting ID in for the variable
                                  std::string const &meterName,             // The variable's meter name
                                  OutputProcessor::Unit const unit,         // The variables units
                                  bool const cumulativeMeterFlag,           // A flag indicating cumulative data
                                  bool const meterFileOnlyFlag              // A flag indicating whether the data is to be written to standard output
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   August 2008
        //       MODIFIED       April 2011; Linda Lawrie
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // The subroutine writes meter data dictionary information to the output files
        // and the SQL database. Much of the code here was embedded in other subroutines
        // and was moved here for the purposes of ease of maintenance and to allow easy
        // data reporting to the SQL database

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        std::string UnitsString = unitEnumToString(unit);

        std::string const FreqString(frequencyNotice(storeType, reportingInterval));

        const auto print_meter = [&](EnergyPlusData &state, const int frequency) {
            const auto out = [&](InputOutputFile &of) {
                if (of.good()) {
                    if (cumulativeMeterFlag) {
                        static constexpr std::string_view fmt = "{},{},Cumulative {} [{}]{}\n";
                        size_t const lenString = index(FreqString, '[');
                        print(of, fmt, reportIDChr, 1, meterName, UnitsString, FreqString.substr(0, lenString));
                    } else {
                        static constexpr std::string_view fmt = "{},{},{} [{}]{}\n";
                        print(of, fmt, reportIDChr, frequency, meterName, UnitsString, FreqString);
                    }
                }
            };

            out(state.files.mtr);
            if (!meterFileOnlyFlag) {
                out(state.files.eso);
            }
        };

        switch (reportingInterval) {
        case ReportingFrequency::EachCall:
        case ReportingFrequency::TimeStep:
        case ReportingFrequency::Hourly: // -1, 0, 1
            print_meter(state, 1);
            break;
        case ReportingFrequency::Daily: //  2
            print_meter(state, 7);
            break;
        case ReportingFrequency::Monthly: //  3
            print_meter(state, 9);
            break;
        case ReportingFrequency::Yearly:     //  5
        case ReportingFrequency::Simulation: //  4
            print_meter(state, 11);
            break;
        default:
            assert(false);
        }

        static constexpr std::string_view keyedValueStringCum("Cumulative ");
        static constexpr std::string_view keyedValueStringNon;
        std::string_view const keyedValueString(cumulativeMeterFlag ? keyedValueStringCum : keyedValueStringNon);

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDictionaryRecord(reportID,
                                                                                   static_cast<int>(storeType),
                                                                                   indexGroup,
                                                                                   keyedValueString,
                                                                                   meterName,
                                                                                   1,
                                                                                   UnitsString,
                                                                                   static_cast<int>(reportingInterval),
                                                                                   true);
        }

        state.dataResultsFramework->resultsFramework->addReportMeter(meterName, UnitsString, reportingInterval);
        // add to ResultsFramework for output variable list, need to check RVI/MVI later
    }

    void WriteRealVariableOutput(EnergyPlusData &state,
                                 RealVariables &realVar,             // Real variable to write out
                                 ReportingFrequency const reportType // The report type or interval (e.g., hourly)
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   August 2008
        //       MODIFIED       April 2011; Linda Lawrie, December 2017; Jason DeGraw
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes real report variable data to the output file and
        // SQL database. Much of the code here was an included in earlier versions
        // of the UpdateDataandReport subroutine. The code was moved to facilitate
        // easier maintenance and writing of data to the SQL database.

        if (realVar.Report && realVar.frequency == reportType && realVar.Stored) {
            if (realVar.NumStored > 0.0) {
                WriteReportRealData(state,
                                    realVar.ReportID,
                                    realVar.ReportIDChr,
                                    realVar.StoreValue,
                                    realVar.storeType,
                                    realVar.NumStored,
                                    realVar.frequency,
                                    realVar.MinValue,
                                    realVar.minValueDate,
                                    realVar.MaxValue,
                                    realVar.maxValueDate);
                ++state.dataGlobal->StdOutputRecordCount;
            }

            realVar.StoreValue = 0.0;
            realVar.NumStored = 0.0;
            realVar.MinValue = MinSetValue;
            realVar.MaxValue = MaxSetValue;
            realVar.Stored = false;
        }
    }

    void WriteReportRealData(EnergyPlusData &state,
                             int const reportID,
                             std::string const &creportID,
                             Real64 const repValue,
                             StoreType const storeType,
                             Real64 const numOfItemsStored,
                             ReportingFrequency const reportingInterval,
                             Real64 const minValue,
                             int const minValueDate,
                             Real64 const MaxValue,
                             int const maxValueDate)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   July 2008
        //       MODIFIED       April 2011; Linda Lawrie
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes the average real data to the output files and
        // SQL database. It supports the WriteRealVariableOutput subroutine.
        // Much of the code here was an included in earlier versions
        // of the UpdateDataandReport subroutine. The code was moved to facilitate
        // easier maintenance and writing of data to the SQL database.

        std::string NumberOut;   // Character for producing "number out"
        Real64 repVal(repValue); // The variable's value

        if (storeType == StoreType::Averaged) {
            repVal /= numOfItemsStored;
        }
        if (repVal == 0.0) {
            NumberOut = "0.0";
        } else {
            dtoa(repVal, state.dataOutputProcessor->s_WriteReportRealData);
            NumberOut = std::string(state.dataOutputProcessor->s_WriteReportRealData);
        }

        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            //// The others (<= hourly) are handled inline with the code
            // add to daily TS data store
            if (reportingInterval == ReportingFrequency::Daily) {
                state.dataResultsFramework->resultsFramework->RIDailyTSData.pushVariableValue(reportID, repVal);
            }
            // add to monthly TS data store
            if (reportingInterval == ReportingFrequency::Monthly) {
                state.dataResultsFramework->resultsFramework->RIMonthlyTSData.pushVariableValue(reportID, repVal);
            }
            // add to run period TS data store
            if (reportingInterval == ReportingFrequency::Simulation) {
                state.dataResultsFramework->resultsFramework->RIRunPeriodTSData.pushVariableValue(reportID, repVal);
            }
            // add to annual TS data store
            if (reportingInterval == ReportingFrequency::Yearly) {
                state.dataResultsFramework->resultsFramework->RIYearlyTSData.pushVariableValue(reportID, repVal);
            }
        }

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDataRecord(
                reportID, repVal, static_cast<int>(reportingInterval), minValue, minValueDate, MaxValue, maxValueDate);
        }

        if ((reportingInterval == ReportingFrequency::EachCall) || (reportingInterval == ReportingFrequency::TimeStep) ||
            (reportingInterval == ReportingFrequency::Hourly)) { // -1, 0, 1
            if (state.files.eso.good()) {
                print(state.files.eso, "{},{}\n", creportID, NumberOut);
            }

        } else { // if ( ( reportingInterval == ReportingFrequency::Daily ) || ( reportingInterval == ReportingFrequency::Monthly ) || (
                 // reportingInterval == ReportingFrequency::Simulation ) ) { //  2, 3, 4, 5
            std::string MaxOut; // Character for Max out string
            std::string MinOut; // Character for Min out string

            if (MaxValue == 0.0) {
                MaxOut = "0.0";
            } else {
                dtoa(MaxValue, state.dataOutputProcessor->s_WriteReportRealData);
                MaxOut = std::string(state.dataOutputProcessor->s_WriteReportRealData);
            }

            if (minValue == 0.0) {
                MinOut = "0.0";
            } else {
                dtoa(minValue, state.dataOutputProcessor->s_WriteReportRealData);
                MinOut = std::string(state.dataOutputProcessor->s_WriteReportRealData);
            }

            // Append the min and max strings with date information
            ProduceMinMaxString(MinOut, minValueDate, reportingInterval);
            ProduceMinMaxString(MaxOut, maxValueDate, reportingInterval);

            if (state.files.eso.good()) {
                print(state.files.eso, "{},{},{},{}\n", creportID, NumberOut, MinOut, MaxOut);
            }
        }
    }

    void WriteCumulativeReportMeterData(EnergyPlusData &state,
                                        int const reportID,           // The variable's report ID
                                        std::string const &creportID, // variable ID in characters
                                        Real64 const repValue,        // The variable's value
                                        bool const meterOnlyFlag      // A flag that indicates if the data should be written to standard output
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   July 2008
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes the cumulative meter data to the output files and
        // SQL database.

        std::string NumberOut; // Character for producing "number out"

        if (repValue == 0.0) {
            NumberOut = "0.0";
        } else {
            dtoa(repValue, state.dataOutputProcessor->s_WriteCumulativeReportMeterData);
            NumberOut = std::string(state.dataOutputProcessor->s_WriteCumulativeReportMeterData);
        }

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDataRecord(reportID, repValue);
        }

        if (state.files.mtr.good()) print(state.files.mtr, "{},{}\n", creportID, NumberOut);
        ++state.dataGlobal->StdMeterRecordCount;

        if (!meterOnlyFlag) {
            if (state.files.eso.good()) print(state.files.eso, "{},{}\n", creportID, NumberOut);
            ++state.dataGlobal->StdOutputRecordCount;
        }
    }

    void WriteReportMeterData(EnergyPlusData &state,
                              int const reportID,                         // The variable's report ID
                              std::string const &creportID,               // variable ID in characters
                              Real64 const repValue,                      // The variable's value
                              ReportingFrequency const reportingInterval, // The variable's reporting interval (e.g., hourly)
                              Real64 const minValue,                      // The variable's minimum value during the reporting interval
                              int const minValueDate,                     // The date the minimum value occurred
                              Real64 const MaxValue,                      // The variable's maximum value during the reporting interval
                              int const maxValueDate,                     // The date of the maximum value
                              bool const meterOnlyFlag                    // Indicates whether the data is for the meter file only
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   July 2008
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes for the non-cumulative meter data to the output files and
        // SQL database.

        std::string NumberOut; // Character for producing "number out"

        if (repValue == 0.0) {
            NumberOut = "0.0";
        } else {
            dtoa(repValue, state.dataOutputProcessor->s_WriteReportMeterData);
            NumberOut = std::string(state.dataOutputProcessor->s_WriteReportMeterData);
        }

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDataRecord(reportID,
                                                                             repValue,
                                                                             static_cast<int>(reportingInterval),
                                                                             minValue,
                                                                             minValueDate,
                                                                             MaxValue,
                                                                             maxValueDate,
                                                                             state.dataGlobal->MinutesPerTimeStep);
        }

        if ((reportingInterval == ReportingFrequency::EachCall) || (reportingInterval == ReportingFrequency::TimeStep) ||
            (reportingInterval == ReportingFrequency::Hourly)) { // -1, 0, 1
            if (state.files.mtr.good()) {
                print(state.files.mtr, "{},{}\n", creportID, NumberOut);
            }
            ++state.dataGlobal->StdMeterRecordCount;
            if (state.files.eso.good() && !meterOnlyFlag) {
                print(state.files.eso, "{},{}\n", creportID, NumberOut);
                ++state.dataGlobal->StdOutputRecordCount;
            }
        } else { // if ( ( reportingInterval == ReportDaily ) || ( reportingInterval == ReportMonthly ) || ( reportingInterval == ReportSim ) ) {
                 // // 2, 3, 4
            std::string MaxOut; // Character for Max out string
            std::string MinOut; // Character for Min out string

            if (MaxValue == 0.0) {
                MaxOut = "0.0";
            } else {
                dtoa(MaxValue, state.dataOutputProcessor->s_WriteReportMeterData);
                MaxOut = std::string(state.dataOutputProcessor->s_WriteReportMeterData);
            }

            if (minValue == 0.0) {
                MinOut = "0.0";
            } else {
                dtoa(minValue, state.dataOutputProcessor->s_WriteReportMeterData);
                MinOut = std::string(state.dataOutputProcessor->s_WriteReportMeterData);
            }

            // Append the min and max strings with date information
            ProduceMinMaxString(MinOut, minValueDate, reportingInterval);
            ProduceMinMaxString(MaxOut, maxValueDate, reportingInterval);

            if (state.files.mtr.good()) {
                print(state.files.mtr, "{},{},{},{}\n", creportID, NumberOut, MinOut, MaxOut);
            }

            ++state.dataGlobal->StdMeterRecordCount;
            if (state.files.eso.good() && !meterOnlyFlag) {
                print(state.files.eso, "{},{},{},{}\n", creportID, NumberOut, MinOut, MaxOut);
                ++state.dataGlobal->StdOutputRecordCount;
            }
        }
    }

    void WriteNumericData(EnergyPlusData &state,
                          int const reportID,           // The variable's reporting ID
                          std::string const &creportID, // variable ID in characters
                          Real64 const repValue         // The variable's value
    )
    {
        // SUBROUTINE INFORMATION:
        //       AUTHOR         Mark Adams
        //       DATE WRITTEN   May 2016
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE:
        // This subroutine writes real data to the output files and
        // SQL database.
        // This is a refactor of WriteRealData.
        //
        // Much of the code here was an included in earlier versions
        // of the UpdateDataandReport subroutine. The code was moved to facilitate
        // easier maintenance and writing of data to the SQL database.

        if (state.dataSysVars->UpdateDataDuringWarmupExternalInterface && !state.dataSysVars->ReportDuringWarmup) return;

        dtoa(repValue, state.dataOutputProcessor->s_WriteNumericData);

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDataRecord(reportID, repValue);
        }

        if (state.files.eso.good()) {
            print<FormatSyntax::FMT>(state.files.eso, "{},{}\n", creportID, state.dataOutputProcessor->s_WriteNumericData);
        }
    }

    void WriteNumericData(EnergyPlusData &state,
                          int const reportID,           // The variable's reporting ID
                          std::string const &creportID, // variable ID in characters
                          int32_t const repValue        // The variable's value
    )
    {
        // SUBROUTINE INFORMATION:
        //       AUTHOR         Mark Adams
        //       DATE WRITTEN   May 2016
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE:
        // This subroutine writes real data to the output files and
        // SQL database.
        // This is a refactor of WriteIntegerData.
        //
        // Much of the code here was an included in earlier versions
        // of the UpdateDataandReport subroutine. The code was moved to facilitate
        // easier maintenance and writing of data to the SQL database.

        //        i32toa(repValue, state.dataOutputProcessor->s_WriteNumericData);

        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDataRecord(reportID, repValue);
        }

        if (state.files.eso.good()) {
            print<FormatSyntax::FMT>(state.files.eso, "{},{}\n", creportID, fmt::format_int(repValue).c_str());
        }
    }

    void WriteIntegerVariableOutput(EnergyPlusData &state,
                                    IntegerVariables &intVar,           // Integer variable to write out
                                    ReportingFrequency const reportType // The report type (i.e., the reporting interval)
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   August 2008
        //       MODIFIED       April 2011; Linda Lawrie, December 2017; Jason DeGraw
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes integer report variable data to the output file and
        // SQL database. Much of the code here was an included in earlier versions
        // of the UpdateDataandReport subroutine. The code was moved to facilitate
        // easier maintenance and writing of data to the SQL database.

        if (state.dataSysVars->UpdateDataDuringWarmupExternalInterface && !state.dataSysVars->ReportDuringWarmup) return;

        if (intVar.Report && intVar.frequency == reportType && intVar.Stored) {
            if (intVar.NumStored > 0.0) {
                WriteReportIntegerData(state,
                                       intVar.ReportID,
                                       intVar.ReportIDChr,
                                       intVar.StoreValue,
                                       intVar.storeType,
                                       intVar.NumStored,
                                       intVar.frequency,
                                       intVar.MinValue,
                                       intVar.minValueDate,
                                       intVar.MaxValue,
                                       intVar.maxValueDate);
                ++state.dataGlobal->StdOutputRecordCount;
            }

            intVar.StoreValue = 0.0;
            intVar.NumStored = 0.0;
            intVar.MinValue = IMinSetValue;
            intVar.MaxValue = IMaxSetValue;
            intVar.Stored = false;
        }
    }

    void WriteReportIntegerData(EnergyPlusData &state,
                                int const reportID,                         // The variable's reporting ID
                                std::string const &reportIDString,          // The variable's reporting ID (character)
                                Real64 const repValue,                      // The variable's value
                                StoreType const storeType,                  // Type of item (averaged or summed)
                                Real64 const numOfItemsStored,              // The number of items (hours or timesteps) of data stored
                                ReportingFrequency const reportingInterval, // The reporting interval (e.g., monthly)
                                int const minValue,                         // The variable's minimum value during the reporting interval
                                int const minValueDate,                     // The date the minimum value occurred
                                int const MaxValue,                         // The variable's maximum value during the reporting interval
                                int const maxValueDate                      // The date the maximum value occurred
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   July 2008
        //       MODIFIED       April 2011; Linda Lawrie
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine writes averaged integer data to the output files and
        // SQL database. It supports the WriteIntegerVariableOutput subroutine.
        // Much of the code here was an included in earlier versions
        // of the UpdateDataandReport subroutine. The code was moved to facilitate
        // easier maintenance and writing of data to the SQL database.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        std::string NumberOut; // Character for producing "number out"
        std::string MaxOut;    // Character for Max out string
        std::string MinOut;    // Character for Min out string
        Real64 rmaxValue;
        Real64 rminValue;
        Real64 repVal; // The variable's value

        repVal = repValue;
        if (storeType == StoreType::Averaged) {
            repVal /= numOfItemsStored;
        }
        if (repValue == 0.0) {
            NumberOut = "0.0";
        } else {
            NumberOut = format("{:f}", repVal);
        }

        // Append the min and max strings with date information
        MinOut = fmt::to_string(minValue);
        MaxOut = fmt::to_string(MaxValue);
        ProduceMinMaxString(MinOut, minValueDate, reportingInterval);
        ProduceMinMaxString(MaxOut, maxValueDate, reportingInterval);

        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            // add to daily TS data store
            if (reportingInterval == ReportingFrequency::Daily) {
                state.dataResultsFramework->resultsFramework->RIDailyTSData.pushVariableValue(reportID, repVal);
            }
            // add to monthly TS data store
            if (reportingInterval == ReportingFrequency::Monthly) {
                state.dataResultsFramework->resultsFramework->RIMonthlyTSData.pushVariableValue(reportID, repVal);
            }
            // add to run period TS data store
            if (reportingInterval == ReportingFrequency::Simulation) {
                state.dataResultsFramework->resultsFramework->RIRunPeriodTSData.pushVariableValue(reportID, repVal);
            }
            // add to annual TS data store
            if (reportingInterval == ReportingFrequency::Yearly) {
                state.dataResultsFramework->resultsFramework->RIYearlyTSData.pushVariableValue(reportID, repVal);
            }
        }

        rminValue = minValue;
        rmaxValue = MaxValue;
        if (state.dataSQLiteProcedures->sqlite) {
            state.dataSQLiteProcedures->sqlite->createSQLiteReportDataRecord(
                reportID, repVal, static_cast<int>(reportingInterval), rminValue, minValueDate, rmaxValue, maxValueDate);
        }

        if ((reportingInterval == ReportingFrequency::EachCall) || (reportingInterval == ReportingFrequency::TimeStep) ||
            (reportingInterval == ReportingFrequency::Hourly)) { // -1, 0, 1
            if (state.files.eso.good()) {
                print(state.files.eso, "{},{}\n", reportIDString, NumberOut);
            }
        } else { // if ( ( reportingInterval == ReportDaily ) || ( reportingInterval == ReportMonthly ) || ( reportingInterval == ReportSim ) ) {
                 // // 2, 3, 4
            if (state.files.eso.good()) {
                print(state.files.eso, "{},{},{},{}\n", reportIDString, NumberOut, MinOut, MaxOut);
            }
        }
    }

    int DetermineIndexGroupKeyFromMeterName(EnergyPlusData &state, std::string const &meterName) // the meter name
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   May 2009
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function attemps to guess determine how a meter variable should be
        // grouped.  It does this by parsing the meter name and then assigns a
        // indexGroupKey based on the name

        // Facility indices are in the 100s
        if (has(meterName, "Electricity:Facility")) {
            state.dataOutputProcessor->indexGroupKey = 100;
        } else if (has(meterName, "NaturalGas:Facility")) {
            state.dataOutputProcessor->indexGroupKey = 101;
        } else if (has(meterName, "DistricHeatingWater:Facility")) {
            state.dataOutputProcessor->indexGroupKey = 102;
        } else if (has(meterName, "DistricCooling:Facility")) {
            state.dataOutputProcessor->indexGroupKey = 103;
        } else if (has(meterName, "ElectricityNet:Facility")) {
            state.dataOutputProcessor->indexGroupKey = 104;

            // Building indices are in the 200s
        } else if (has(meterName, "Electricity:Building")) {
            state.dataOutputProcessor->indexGroupKey = 201;
        } else if (has(meterName, "NaturalGas:Building")) {
            state.dataOutputProcessor->indexGroupKey = 202;

            // HVAC indices are in the 300s
        } else if (has(meterName, "Electricity:HVAC")) {
            state.dataOutputProcessor->indexGroupKey = 301;

            // InteriorLights:Electricity:Zone indices are in the 500s
        } else if (has(meterName, "InteriorLights:Electricity:Zone")) {
            state.dataOutputProcessor->indexGroupKey = 501;

            // InteriorLights:Electricity indices are in the 400s
        } else if (has(meterName, "InteriorLights:Electricity")) {
            state.dataOutputProcessor->indexGroupKey = 401;

            // Unknown items have negative indices
        } else {
            state.dataOutputProcessor->indexGroupKey = -11;
        }

        return state.dataOutputProcessor->indexGroupKey;
    }

    std::string DetermineIndexGroupFromMeterGroup(MeterType const &meter) // the meter
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         Greg Stark
        //       DATE WRITTEN   May 2009
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // This function attemps to determine how a meter variable should be
        // grouped.  It does this by parsing the meter group

        // Return value
        std::string indexGroup;

        if (len(meter.Group) > 0) {
            indexGroup = meter.Group;
        } else {
            indexGroup = "Facility";
        }

        if (len(meter.ResourceType) > 0) {
            indexGroup += ":" + meter.ResourceType;
        }

        if (len(meter.EndUse) > 0) {
            indexGroup += ":" + meter.EndUse;
        }

        if (len(meter.EndUseSub) > 0) {
            indexGroup += ":" + meter.EndUseSub;
        }

        return indexGroup;
    }

    void SetInternalVariableValue(EnergyPlusData &state,
                                  OutputProcessor::VariableType const varType, // 1=integer, 2=real, 3=meter
                                  int const keyVarIndex,                       // Array index
                                  Real64 const SetRealVal,                     // real value to set, if type is real or meter
                                  int const SetIntVal                          // integer value to set if type is integer
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         B. Griffith
        //       DATE WRITTEN   August 2012
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This is a simple set routine for output pointers
        // It is intended for special use to reinitializations those pointers used for EMS sensors

        // METHODOLOGY EMPLOYED:
        // given a variable type and variable index,
        // assign the pointers the values passed in.

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        auto &op = state.dataOutputProcessor;

        if (varType == VariableType::Integer) {
            *op->IVariableTypes(keyVarIndex).VarPtr.Which = SetIntVal;
        } else if (varType == VariableType::Real) {
            *op->RVariableTypes(keyVarIndex).VarPtr.Which = SetRealVal;
        } else if (varType == VariableType::Meter) {
            op->EnergyMeters(keyVarIndex).CurTSValue = SetRealVal;
        }
    }

    // returns the string corresponding to the OutputProcessor::Unit enum in brackets
    std::string unitEnumToStringBrackets(EnergyPlus::OutputProcessor::Unit const unitIn)
    {
        // J.Glazer - August/September 2017
        return " [" + unitEnumToString(unitIn) + "]";
    }

    // returns the unit string for a DDVariableTypes item and custom string when customEMS is used
    std::string unitStringFromDDitem(EnergyPlusData &state, int const ddItemPtr // index provided for DDVariableTypes
    )
    {
        // J.Glazer - August/September 2017
        OutputProcessor::Unit ddUnit = state.dataOutputProcessor->DDVariableTypes(ddItemPtr).units;
        if (ddUnit != OutputProcessor::Unit::customEMS) {
            return unitEnumToStringBrackets(ddUnit);
        } else {
            return " [" + state.dataOutputProcessor->DDVariableTypes(ddItemPtr).unitNameCustomEMS + "]";
        }
    }

    // returns the string corresponding to the OutputProcessor::Unit enum
    std::string unitEnumToString(EnergyPlus::OutputProcessor::Unit const unitIn)
    {
        // J.Glazer - August/September 2017
        // TODO: Use a constexpr array of string views
        switch (unitIn) {
        case OutputProcessor::Unit::J:
            return "J";
        case OutputProcessor::Unit::W:
            return "W";
        case OutputProcessor::Unit::C:
            return "C";
        case OutputProcessor::Unit::None:
            return "";
        case OutputProcessor::Unit::kg:
            return "kg";
        case OutputProcessor::Unit::W_m2:
            return "W/m2";
        case OutputProcessor::Unit::m3:
            return "m3";
        case OutputProcessor::Unit::hr:
            return "hr";
        case OutputProcessor::Unit::kg_s:
            return "kg/s";
        case OutputProcessor::Unit::deg:
            return "deg";
        case OutputProcessor::Unit::m3_s:
            return "m3/s";
        case OutputProcessor::Unit::W_m2K:
            return "W/m2-K";
        case OutputProcessor::Unit::kgWater_kgDryAir:
            return "kgWater/kgDryAir";
        case OutputProcessor::Unit::Perc:
            return "%";
        case OutputProcessor::Unit::m_s:
            return "m/s";
        case OutputProcessor::Unit::lux:
            return "lux";
        case OutputProcessor::Unit::kgWater_s:
            return "kgWater/s";
        case OutputProcessor::Unit::rad:
            return "rad";
        case OutputProcessor::Unit::Pa:
            return "Pa";
        case OutputProcessor::Unit::J_kg:
            return "J/kg";
        case OutputProcessor::Unit::m:
            return "m";
        case OutputProcessor::Unit::lum_W:
            return "lum/W";
        case OutputProcessor::Unit::kg_m3:
            return "kg/m3";
        case OutputProcessor::Unit::L:
            return "L";
        case OutputProcessor::Unit::ach:
            return "ach";
        case OutputProcessor::Unit::m2:
            return "m2";
        case OutputProcessor::Unit::deltaC:
            return "deltaC";
        case OutputProcessor::Unit::J_kgK:
            return "J/kg-K";
        case OutputProcessor::Unit::W_W:
            return "W/W";
        case OutputProcessor::Unit::clo:
            return "clo";
        case OutputProcessor::Unit::W_mK:
            return "W/m-K";
        case OutputProcessor::Unit::W_K:
            return "W/K";
        case OutputProcessor::Unit::K_W:
            return "K/W";
        case OutputProcessor::Unit::ppm:
            return "ppm";
        case OutputProcessor::Unit::kg_kg:
            return "kg/kg";
        case OutputProcessor::Unit::s:
            return "s";
        case OutputProcessor::Unit::cd_m2:
            return "cd/m2";
        case OutputProcessor::Unit::kmol_s:
            return "kmol/s";
        case OutputProcessor::Unit::K_m:
            return "K/m";
        case OutputProcessor::Unit::min:
            return "min";
        case OutputProcessor::Unit::J_kgWater:
            return "J/kgWater";
        case OutputProcessor::Unit::rev_min:
            return "rev/min";
        case OutputProcessor::Unit::kg_m2s:
            return "kg/m2-s";
        case OutputProcessor::Unit::J_m2:
            return "J/m2";
        case OutputProcessor::Unit::A:
            return "A";
        case OutputProcessor::Unit::V:
            return "V";
        case OutputProcessor::Unit::W_m2C:
            return "W/m2-C";
        case OutputProcessor::Unit::Ah:
            return "Ah";
        case OutputProcessor::Unit::Btu_h_W:
            return "Btu/h-W";
        default:
            return "unknown";
        }
    }

    // returns the OutputProcessor::Unit enum value when a string containing the units is provided without brackets
    OutputProcessor::Unit unitStringToEnum(std::string const &unitIn)
    {
        // J.Glazer - August/September 2017
        std::string unitUpper = Util::makeUPPER(unitIn);
        if (unitUpper == "J") {
            return OutputProcessor::Unit::J;
        } else if (unitUpper == "DELTAC") {
            return OutputProcessor::Unit::deltaC;
        } else if (unitUpper.empty()) {
            return OutputProcessor::Unit::None;
        } else if (unitUpper == "W") {
            return OutputProcessor::Unit::W;
        } else if (unitUpper == "C") {
            return OutputProcessor::Unit::C;
        } else if (unitUpper == "KG/S") {
            return OutputProcessor::Unit::kg_s;
        } else if (unitUpper == "KGWATER/KGDRYAIR") {
            return OutputProcessor::Unit::kgWater_kgDryAir;
        } else if (unitUpper == "PPM") {
            return OutputProcessor::Unit::ppm;
        } else if (unitUpper == "PA") {
            return OutputProcessor::Unit::Pa;
        } else if (unitUpper == "M3/S") {
            return OutputProcessor::Unit::m3_s;
        } else if (unitUpper == "MIN") {
            return OutputProcessor::Unit::min;
        } else if (unitUpper == "M3") {
            return OutputProcessor::Unit::m3;
        } else if (unitUpper == "KG") {
            return OutputProcessor::Unit::kg;
        } else if (unitUpper == "ACH") {
            return OutputProcessor::Unit::ach;
        } else if (unitUpper == "W/W") {
            return OutputProcessor::Unit::W_W;
        } else if (unitUpper == "LUX") {
            return OutputProcessor::Unit::lux;
        } else if (unitUpper == "LUM/W") {
            return OutputProcessor::Unit::lum_W;
        } else if (unitUpper == "HR") {
            return OutputProcessor::Unit::hr;
        } else if (unitUpper == "CD/M2") {
            return OutputProcessor::Unit::cd_m2;
        } else if (unitUpper == "J/KGWATER") {
            return OutputProcessor::Unit::J_kgWater;
        } else if (unitUpper == "M/S") {
            return OutputProcessor::Unit::m_s;
        } else if (unitUpper == "W/M2") {
            return OutputProcessor::Unit::W_m2;
        } else if (unitUpper == "M") {
            return OutputProcessor::Unit::m;
        } else if (unitUpper == "AH") {
            return OutputProcessor::Unit::Ah;
        } else if (unitUpper == "A") {
            return OutputProcessor::Unit::A;
        } else if (unitUpper == "V") {
            return OutputProcessor::Unit::V;
        } else if (unitUpper == "KMOL/S") {
            return OutputProcessor::Unit::kmol_s;
        } else if (unitUpper == "REV/MIN") {
            return OutputProcessor::Unit::rev_min;
        } else if (unitUpper == "W/M2-K") {
            return OutputProcessor::Unit::W_m2K;
        } else if (unitUpper == "J/KG") {
            return OutputProcessor::Unit::J_kg;
        } else if (unitUpper == "KG/KG") {
            return OutputProcessor::Unit::kg_kg;
        } else if (unitUpper == "%") {
            return OutputProcessor::Unit::Perc;
        } else if (unitUpper == "DEG") {
            return OutputProcessor::Unit::deg;
        } else if (unitUpper == "S") {
            return OutputProcessor::Unit::s;
        } else if (unitUpper == "KG/M3") {
            return OutputProcessor::Unit::kg_m3;
        } else if (unitUpper == "KG/M2-S") {
            return OutputProcessor::Unit::kg_m2s;
        } else if (unitUpper == "J/KG-K") {
            return OutputProcessor::Unit::J_kgK;
        } else if (unitUpper == "L") {
            return OutputProcessor::Unit::L;
        } else if (unitUpper == "K/M") {
            return OutputProcessor::Unit::K_m;
        } else if (unitUpper == "M2") {
            return OutputProcessor::Unit::m2;
        } else if (unitUpper == "W/M2-C") {
            return OutputProcessor::Unit::W_m2C;
        } else if (unitUpper == "RAD") {
            return OutputProcessor::Unit::rad;
        } else if (unitUpper == "J/M2") {
            return OutputProcessor::Unit::J_m2;
        } else if (unitUpper == "CLO") {
            return OutputProcessor::Unit::clo;
        } else if (unitUpper == "W/M-K") {
            return OutputProcessor::Unit::W_mK;
        } else if (unitUpper == "W/K") {
            return OutputProcessor::Unit::W_K;
        } else if (unitUpper == "K/W") {
            return OutputProcessor::Unit::K_W;
        } else if (unitUpper == "KGWATER/S") {
            return OutputProcessor::Unit::kgWater_s;
        } else {
            return OutputProcessor::Unit::unknown;
        }
    }

} // namespace OutputProcessor

void SetupOutputVariable(EnergyPlusData &state,
                         std::string_view const VariableName,              // String Name of variable (with units)
                         OutputProcessor::Unit VariableUnit,               // Actual units corresponding to the actual variable
                         Real64 &ActualVariable,                           // Actual Variable, used to set up pointer
                         OutputProcessor::SOVTimeStepType TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                         OutputProcessor::SOVStoreType VariableTypeKey,    // State, Average=1, NonState, Sum=2
                         std::string_view const KeyedValue                 // Associated Key for this variable
)
{
    SetupOutputVariable(state, VariableName, VariableUnit, ActualVariable, TimeStepTypeKey, VariableTypeKey, KeyedValue, "");
}

void SetupOutputVariable(EnergyPlusData &state,
                         std::string_view const VariableName,              // String Name of variable (with units)
                         OutputProcessor::Unit VariableUnit,               // Actual units corresponding to the actual variable
                         Real64 &ActualVariable,                           // Actual Variable, used to set up pointer
                         OutputProcessor::SOVTimeStepType TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                         OutputProcessor::SOVStoreType VariableTypeKey,    // State, Average=1, NonState, Sum=2
                         std::string_view const KeyedValue,                // Associated Key for this variable
                         ReportFreqSOV ReportFreq,                         // Internal use -- causes reporting at this freqency
                         eResourceSOV ResourceTypeKey,                     // Meter Resource Type (Electricity, Gas, etc)
                         EndUseCatSOV EndUseKey,                           // Meter End Use Key (Lights, Heating, Cooling, etc)
                         std::string_view const EndUseSubKey,              // Meter End Use Sub Key (General Lights, Task Lights, etc)
                         GroupSOV GroupKey,                                // Meter Super Group Key (Building, System, Plant)
                         std::string_view const ZoneKey,                   // Meter Zone Key (zone name)
                         int const ZoneMult,                               // Zone Multiplier, defaults to 1
                         int const ZoneListMult,                           // Zone List Multiplier, defaults to 1
                         int const indexGroupKey,                          // Group identifier for SQL output
                         std::string_view const customUnitName,            // the custom name for the units from EMS definition of units
                         std::string_view const SpaceType                  // Space type (applicable for Building group only)
)
{
    std::string locReportFreq = "";
    std::string locResourceTypeKey = "";
    std::string locEndUseKey = "";
    std::string locGroupKey = "";

    if (ReportFreq == ReportFreqSOV::Num) {
        // Fatal error warning message
        ShowFatalError(state, "Invalid Resource Type.");
    } else if (ReportFreq == ReportFreqSOV::EachCall) // This is valid
    {
        locReportFreq = "DETAILED";
    } else {
        locReportFreq = ReporFreqSOVNames[static_cast<int>(ReportFreq)];
    }

    if (ResourceTypeKey == eResourceSOV::Invalid || ResourceTypeKey == eResourceSOV::Num) {
        // Fatal error warning message
        ShowFatalError(state, "Invalid Resource Type.");
    } else {
        locResourceTypeKey = eResourceSOVNames[static_cast<int>(ResourceTypeKey)];
    }

    if (EndUseKey == EndUseCatSOV::Invalid || EndUseKey == EndUseCatSOV::Num) {
        // Fatal error warning message
        ShowFatalError(state, "Invalid End Use Category.");
    } else {
        locEndUseKey = endUseCatSOVNames[static_cast<int>(EndUseKey)];
    }

    if (GroupKey == GroupSOV::Invalid || GroupKey == GroupSOV::Num) {
        ShowFatalError(state, "Invalid Group Type.");
    } else {
        locGroupKey = GroupSOVNames[static_cast<int>(GroupKey)];
    }

    SetupOutputVariable(state,
                        VariableName,
                        VariableUnit,
                        ActualVariable,
                        TimeStepTypeKey,
                        VariableTypeKey,
                        KeyedValue,
                        locReportFreq,
                        locResourceTypeKey,
                        locEndUseKey,
                        EndUseSubKey,
                        locGroupKey,
                        ZoneKey,
                        ZoneMult,
                        ZoneListMult,
                        indexGroupKey,
                        customUnitName,
                        SpaceType);
}

// TODO: Probably move these to a different location

void SetupOutputVariable(EnergyPlusData &state,
                         std::string_view const VariableName,                    // String Name of variable (with units)
                         OutputProcessor::Unit const VariableUnit,               // Actual units corresponding to the actual variable
                         Real64 &ActualVariable,                                 // Actual Variable, used to set up pointer
                         OutputProcessor::SOVTimeStepType const TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                         OutputProcessor::SOVStoreType const VariableTypeKey,    // State, Average=1, NonState, Sum=2
                         std::string_view const KeyedValue,                      // Associated Key for this variable
                         std::string_view const ReportFreq,                      // Internal use -- causes reporting at this frequency
                         std::string_view const ResourceTypeKey,                 // Meter Resource Type (Electricity, Gas, etc)
                         std::string_view const EndUseKey,                       // Meter End Use Key (Lights, Heating, Cooling, etc)
                         std::string_view const EndUseSubKey,                    // Meter End Use Sub Key (General Lights, Task Lights, etc)
                         std::string_view const GroupKey,                        // Meter Super Group Key (Building, System, Plant)
                         std::string_view const ZoneKey,                         // Meter Zone Key (zone name)
                         int const ZoneMult,                                     // Zone Multiplier, defaults to 1
                         int const ZoneListMult,                                 // Zone List Multiplier, defaults to 1
                         int const indexGroupKey,                                // Group identifier for SQL output
                         std::string_view const customUnitName,                  // the custom name for the units from EMS definition of units
                         std::string_view const SpaceType                        // Space type (applicable for Building group only)
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   December 1998
    //       MODIFIED       January 2001; Implement Meters
    //                      August 2008; Implement SQL output
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine sets up the variable data structure that will be used
    // to track values of the output variables of EnergyPlus.

    // METHODOLOGY EMPLOYED:
    // Pointers (as pointers), pointers (as indices), and lots of other KEWL data stuff.

    using namespace OutputProcessor;
    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int CV;
    TimeStepType TimeStepType; // 1=TimeStepZone, 2=TimeStepSys
    StoreType VariableType;    // 1=Average, 2=Sum, 3=Min/Max
    int Loop;
    ReportingFrequency RepFreq(ReportingFrequency::Hourly);
    std::string ResourceType; // Will hold value of ResourceTypeKey
    std::string EndUse;       // Will hold value of EndUseKey
    std::string EndUseSub;    // Will hold value of EndUseSubKey
    std::string Group;        // Will hold value of GroupKey
    std::string zoneName;     // Will hold value of ZoneKey
    std::string spaceType;    // Will hold value of SpaceType
    auto &op = state.dataOutputProcessor;

    if (!op->OutputInitialized) InitializeOutput(state);

    // Variable name without units
    const std::string_view VarName = VariableName;

    // Determine whether to Report or not
    CheckReportVariable(state, KeyedValue, VarName);

    if (op->NumExtraVars == 0) {
        op->NumExtraVars = 1;
        op->ReportList = -1;
    }

    // If ReportFreq present, overrides input
    if (!ReportFreq.empty()) {
        RepFreq = determineFrequency(state, ReportFreq);
        op->NumExtraVars = 1;
        op->ReportList = 0;
    }

    // DataOutputs::OutputVariablesForSimulation is case-insensitive
    bool const ThisOneOnTheList = DataOutputs::FindItemInVariableList(state, KeyedValue, VarName);
    bool OnMeter = false; // True if this variable is on a meter

    for (Loop = 1; Loop <= op->NumExtraVars; ++Loop) {

        if (Loop == 1) ++op->NumOfRVariable_Setup;

        if (Loop == 1) {
            OnMeter = false;
            if (!ResourceTypeKey.empty()) {
                ResourceType = ResourceTypeKey;
                OnMeter = true;
            } else {
                ResourceType = "";
            }
            if (!EndUseKey.empty()) {
                EndUse = EndUseKey;
                OnMeter = true;
            } else {
                EndUse = "";
            }
            if (!EndUseSubKey.empty()) {
                EndUseSub = EndUseSubKey;
                OnMeter = true;
            } else {
                EndUseSub = "";
                if (!EndUseKey.empty()) {
                    if (std::find(endUseCategoryNames.begin(), endUseCategoryNames.end(), Util::makeUPPER(std::string{EndUseKey})) !=
                        endUseCategoryNames.end()) {
                        EndUseSub = "General";
                    }
                }
            }
            if (!GroupKey.empty()) {
                Group = GroupKey;
                OnMeter = true;
            } else {
                Group = "";
            }
            if (!ZoneKey.empty()) {
                zoneName = ZoneKey;
                OnMeter = true;
            } else {
                zoneName = "";
            }
            if (!SpaceType.empty()) {
                spaceType = SpaceType;
                OnMeter = true;
            } else {
                spaceType = "";
            }
        }

        TimeStepType = ValidateTimeStepType(state, TimeStepTypeKey);
        VariableType = validateVariableType(state, VariableTypeKey);

        AddToOutputVariableList(state, VarName, TimeStepType, VariableType, VariableType::Real, VariableUnit, customUnitName);
        ++op->NumTotalRVariable;

        if (!OnMeter && !ThisOneOnTheList) continue;

        ++op->NumOfRVariable;
        if (Loop == 1 && VariableType == StoreType::Summed) {
            ++op->NumOfRVariable_Sum;
            if (!ResourceTypeKey.empty()) {
                ++op->NumOfRVariable_Meter;
            }
        }
        if (op->NumOfRVariable > op->MaxRVariable) {
            ReallocateRVar(state);
        }
        CV = op->NumOfRVariable;
        auto &thisRvar = op->RVariableTypes(CV);
        thisRvar.timeStepType = TimeStepType;
        thisRvar.storeType = VariableType;
        thisRvar.VarName = fmt::format("{}:{}", KeyedValue, VarName);
        thisRvar.VarNameOnly = VarName;
        thisRvar.VarNameOnlyUC = Util::makeUPPER(VarName);
        thisRvar.VarNameUC = Util::makeUPPER(thisRvar.VarName);
        thisRvar.KeyNameOnlyUC = Util::makeUPPER(KeyedValue);
        thisRvar.units = VariableUnit;
        if (VariableUnit == OutputProcessor::Unit::customEMS) {
            thisRvar.unitNameCustomEMS = customUnitName;
        }
        AssignReportNumber(state, op->CurrentReportNumber);
        std::string const IDOut = fmt::to_string(op->CurrentReportNumber);
        thisRvar.ReportID = op->CurrentReportNumber;
        auto &thisVarPtr = thisRvar.VarPtr;
        thisVarPtr.Value = 0.0;
        thisVarPtr.TSValue = 0.0;
        thisVarPtr.StoreValue = 0.0;
        thisVarPtr.NumStored = 0.0;
        thisVarPtr.MaxValue = MaxSetValue;
        thisVarPtr.maxValueDate = 0;
        thisVarPtr.MinValue = MinSetValue;
        thisVarPtr.minValueDate = 0;
        thisVarPtr.Which = &ActualVariable;
        thisVarPtr.ReportID = op->CurrentReportNumber;
        thisVarPtr.ReportIDChr = IDOut.substr(0, 15);
        thisVarPtr.storeType = VariableType;
        thisVarPtr.Stored = false;
        thisVarPtr.Report = false;
        thisVarPtr.frequency = ReportingFrequency::Hourly;
        thisVarPtr.SchedPtr = 0;
        thisVarPtr.MeterArrayPtr = 0;
        thisVarPtr.ZoneMult = ZoneMult;
        thisVarPtr.ZoneListMult = ZoneListMult;

        if (Loop == 1) {
            if (OnMeter) {
                if (VariableType == StoreType::Averaged) {
                    ShowSevereError(state, "Meters can only be \"Summed\" variables");
                    ShowContinueError(state, fmt::format("..reference variable={}:{}", KeyedValue, VariableName));
                } else {
                    Unit mtrUnits = op->RVariableTypes(CV).units;
                    bool ErrorsFound = false;
                    AttachMeters(
                        state, mtrUnits, ResourceType, EndUse, EndUseSub, Group, zoneName, spaceType, CV, thisVarPtr.MeterArrayPtr, ErrorsFound);
                    if (ErrorsFound) {
                        ShowContinueError(state, fmt::format("Invalid Meter spec for variable={}:{}", KeyedValue, VariableName));
                        op->ErrorsLogged = true;
                    }
                }
            }
        }

        if (op->ReportList(Loop) == -1) continue;

        thisVarPtr.Report = true;

        if (op->ReportList(Loop) == 0) {
            thisVarPtr.frequency = RepFreq;
            thisVarPtr.SchedPtr = 0;
        } else {
            thisVarPtr.frequency = op->ReqRepVars(op->ReportList(Loop)).frequency;
            thisVarPtr.SchedPtr = op->ReqRepVars(op->ReportList(Loop)).SchedPtr;
        }

        if (thisVarPtr.Report) {
            if (thisVarPtr.SchedPtr != 0) {
                WriteReportVariableDictionaryItem(state,
                                                  thisVarPtr.frequency,
                                                  thisVarPtr.storeType,
                                                  thisVarPtr.ReportID,
                                                  indexGroupKey,
                                                  std::string(sovTimeStepTypeStrings[(int)TimeStepTypeKey]),
                                                  thisVarPtr.ReportIDChr,
                                                  KeyedValue,
                                                  VarName,
                                                  thisRvar.timeStepType,
                                                  thisRvar.units,
                                                  thisRvar.unitNameCustomEMS,
                                                  op->ReqRepVars(op->ReportList(Loop)).SchedName);
            } else {
                WriteReportVariableDictionaryItem(state,
                                                  thisVarPtr.frequency,
                                                  thisVarPtr.storeType,
                                                  thisVarPtr.ReportID,
                                                  indexGroupKey,
                                                  std::string(sovTimeStepTypeStrings[(int)TimeStepTypeKey]),
                                                  thisVarPtr.ReportIDChr,
                                                  KeyedValue,
                                                  VarName,
                                                  thisRvar.timeStepType,
                                                  thisRvar.units,
                                                  thisRvar.unitNameCustomEMS);
            }
        }
    }
}

void SetupOutputVariable(EnergyPlusData &state,
                         std::string_view const VariableName,                    // String Name of variable
                         OutputProcessor::Unit const VariableUnit,               // Actual units corresponding to the actual variable
                         int &ActualVariable,                                    // Actual Variable, used to set up pointer
                         OutputProcessor::SOVTimeStepType const TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                         OutputProcessor::SOVStoreType const VariableTypeKey,    // State, Average=1, NonState, Sum=2
                         std::string_view const KeyedValue                       // Associated Key for this variable
)
{
    SetupOutputVariable(state,
                        VariableName,    // String Name of variable
                        VariableUnit,    // Actual units corresponding to the actual variable
                        ActualVariable,  // Actual Variable, used to set up pointer
                        TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                        VariableTypeKey, // State, Average=1, NonState, Sum=2
                        KeyedValue,      // Associated Key for this variable
                        ""               // Internal use -- causes reporting at this freqency
                                         // indexGroupKey // Group identifier for SQL output
    );
}

void SetupOutputVariable(EnergyPlusData &state,
                         std::string_view const VariableName,                    // String Name of variable
                         OutputProcessor::Unit const VariableUnit,               // Actual units corresponding to the actual variable
                         int &ActualVariable,                                    // Actual Variable, used to set up pointer
                         OutputProcessor::SOVTimeStepType const TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                         OutputProcessor::SOVStoreType const VariableTypeKey,    // State, Average=1, NonState, Sum=2
                         std::string_view const KeyedValue,                      // Associated Key for this variable
                         OutputProcessor::ReportingFrequency const ReportFreq,   // Internal use -- causes reporting at this freqency
                         int const indexGroupKey                                 // Group identifier for SQL output
)
{
    if (ReportFreq == OutputProcessor::ReportingFrequency::EachCall) // This is valid
    {
        SetupOutputVariable(
            state, VariableName, VariableUnit, ActualVariable, TimeStepTypeKey, VariableTypeKey, KeyedValue, "DETAILED", indexGroupKey);
    } else {
        SetupOutputVariable(state,
                            VariableName,
                            VariableUnit,
                            ActualVariable,
                            TimeStepTypeKey,
                            VariableTypeKey,
                            KeyedValue,
                            OutputProcessor::ReportingFrequencyNames[static_cast<int>(ReportFreq)],
                            indexGroupKey);
    }
}

void SetupOutputVariable(EnergyPlusData &state,
                         std::string_view const VariableName,                    // String Name of variable
                         OutputProcessor::Unit const VariableUnit,               // Actual units corresponding to the actual variable
                         int &ActualVariable,                                    // Actual Variable, used to set up pointer
                         OutputProcessor::SOVTimeStepType const TimeStepTypeKey, // Zone, HeatBalance=1, HVAC, System, Plant=2
                         OutputProcessor::SOVStoreType const VariableTypeKey,    // State, Average=1, NonState, Sum=2
                         std::string_view const KeyedValue,                      // Associated Key for this variable
                         std::string_view const ReportFreq,                      // Internal use -- causes reporting at this freqency
                         int const indexGroupKey                                 // Group identifier for SQL output
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   December 1998
    //       MODIFIED       August 2008; Added SQL output capability
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine sets up the variable data structure that will be used
    // to track values of the output variables of EnergyPlus.

    // METHODOLOGY EMPLOYED:
    // Pointers (as pointers), pointers (as indices), and lots of other KEWL data stuff.

    // Using/Aliasing
    using namespace OutputProcessor;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int CV;
    TimeStepType TimeStepType; // 1=TimeStepZone, 2=TimeStepSys
    StoreType VariableType;    // 1=Average, 2=Sum, 3=Min/Max
    int Loop;
    ReportingFrequency RepFreq(ReportingFrequency::Hourly);
    auto &op = state.dataOutputProcessor;

    if (!op->OutputInitialized) InitializeOutput(state);

    // Variable name without units
    const std::string_view VarName = VariableName;

    // Determine whether to Report or not
    CheckReportVariable(state, KeyedValue, VarName);

    if (op->NumExtraVars == 0) {
        op->NumExtraVars = 1;
        op->ReportList = -1;
    }

    // If ReportFreq present, overrides input
    if (!ReportFreq.empty()) {
        RepFreq = determineFrequency(state, ReportFreq);
        op->NumExtraVars = 1;
        op->ReportList = 0;
    }

    // DataOutputs::OutputVariablesForSimulation is case-insentitive
    bool const ThisOneOnTheList = DataOutputs::FindItemInVariableList(state, KeyedValue, VarName);

    for (Loop = 1; Loop <= op->NumExtraVars; ++Loop) {

        if (Loop == 1) ++op->NumOfIVariable_Setup;

        TimeStepType = ValidateTimeStepType(state, TimeStepTypeKey);
        VariableType = validateVariableType(state, VariableTypeKey);

        AddToOutputVariableList(state, VarName, TimeStepType, VariableType, VariableType::Integer, VariableUnit);
        ++op->NumTotalIVariable;

        if (!ThisOneOnTheList) continue;

        ++op->NumOfIVariable;
        if (Loop == 1 && VariableType == StoreType::Summed) {
            ++op->NumOfIVariable_Sum;
        }
        if (op->NumOfIVariable > op->MaxIVariable) {
            ReallocateIVar(state);
        }

        CV = op->NumOfIVariable;
        auto &thisIVar = op->IVariableTypes(CV);
        thisIVar.timeStepType = TimeStepType;
        thisIVar.storeType = VariableType;
        thisIVar.VarName = fmt::format("{}:{}", KeyedValue, VarName);
        thisIVar.VarNameOnly = VarName;
        thisIVar.VarNameOnlyUC = Util::makeUPPER(VarName);
        thisIVar.VarNameUC = Util::makeUPPER(thisIVar.VarName);
        thisIVar.KeyNameOnlyUC = Util::makeUPPER(KeyedValue);
        thisIVar.units = VariableUnit;
        AssignReportNumber(state, op->CurrentReportNumber);
        std::string const IDOut = fmt::to_string(op->CurrentReportNumber);
        thisIVar.ReportID = op->CurrentReportNumber;
        auto &thisVarPtr = thisIVar.VarPtr;
        thisVarPtr.Value = 0.0;
        thisVarPtr.StoreValue = 0.0;
        thisVarPtr.TSValue = 0.0;
        thisVarPtr.NumStored = 0.0;
        //    IVariable%LastTSValue=0
        thisVarPtr.MaxValue = IMaxSetValue;
        thisVarPtr.maxValueDate = 0;
        thisVarPtr.MinValue = IMinSetValue;
        thisVarPtr.minValueDate = 0;
        thisVarPtr.Which = &ActualVariable;
        thisVarPtr.ReportID = op->CurrentReportNumber;
        thisVarPtr.ReportIDChr = IDOut.substr(0, 15);
        thisVarPtr.storeType = VariableType;
        thisVarPtr.Stored = false;
        thisVarPtr.Report = false;
        thisVarPtr.frequency = ReportingFrequency::Hourly;
        thisVarPtr.SchedPtr = 0;

        if (op->ReportList(Loop) == -1) continue;

        thisVarPtr.Report = true;

        if (op->ReportList(Loop) == 0) {
            thisVarPtr.frequency = RepFreq;
            thisVarPtr.SchedPtr = 0;
        } else {
            thisVarPtr.frequency = op->ReqRepVars(op->ReportList(Loop)).frequency;
            thisVarPtr.SchedPtr = op->ReqRepVars(op->ReportList(Loop)).SchedPtr;
        }

        if (thisVarPtr.Report) {

            if (thisVarPtr.SchedPtr != 0) {
                WriteReportVariableDictionaryItem(state,
                                                  thisVarPtr.frequency,
                                                  thisVarPtr.storeType,
                                                  thisVarPtr.ReportID,
                                                  indexGroupKey,
                                                  std::string(sovTimeStepTypeStrings[(int)TimeStepTypeKey]),
                                                  thisVarPtr.ReportIDChr,
                                                  KeyedValue,
                                                  VarName,
                                                  thisIVar.timeStepType,
                                                  thisIVar.units,
                                                  op->ReqRepVars(op->ReportList(Loop)).SchedName);
            } else {
                WriteReportVariableDictionaryItem(state,
                                                  thisVarPtr.frequency,
                                                  thisVarPtr.storeType,
                                                  thisVarPtr.ReportID,
                                                  indexGroupKey,
                                                  std::string(sovTimeStepTypeStrings[(int)TimeStepTypeKey]),
                                                  thisVarPtr.ReportIDChr,
                                                  KeyedValue,
                                                  VarName,
                                                  thisIVar.timeStepType,
                                                  thisIVar.units);
            }
        }
    }
}

void UpdateDataandReport(EnergyPlusData &state, OutputProcessor::TimeStepType const t_TimeStepTypeKey) // What kind of data to update (Zone, HVAC)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   December 1998
    //       MODIFIED       January 2001; Resolution integrated at the Zone TimeStep intervals
    //       MODIFIED       August 2008; Added SQL output capability
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine writes the actual report variable (for user requested
    // Report Variables) strings to the standard output file.

    // METHODOLOGY EMPLOYED:
    // na

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;
    using General::EncodeMonDayHrMin;
    using ScheduleManager::GetCurrentScheduleValue;

    // Locals
    // SUBROUTINE ARGUMENT DEFINITIONS:

    // SUBROUTINE PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    bool TimePrint(true);        // True if the time needs to be printed
    bool EndTimeStepFlag(false); // True when it's the end of the Zone Time Step
    auto &op = state.dataOutputProcessor;

    if (t_TimeStepTypeKey != TimeStepType::Zone && t_TimeStepTypeKey != TimeStepType::System) {
        ShowFatalError(state, "Invalid reporting requested -- UpdateDataAndReport");
    }

    // Basic record keeping and report out if "detailed"
    Real64 StartMinute = op->TimeValue.at(t_TimeStepTypeKey).CurMinute; // StartMinute for UpdateData call
    op->TimeValue.at(t_TimeStepTypeKey).CurMinute += (*op->TimeValue.at(t_TimeStepTypeKey).TimeStep) * 60.0;
    if (t_TimeStepTypeKey == TimeStepType::System &&
        (op->TimeValue.at(TimeStepType::System).CurMinute == op->TimeValue.at(TimeStepType::Zone).CurMinute)) {
        EndTimeStepFlag = true;
    } else if (t_TimeStepTypeKey == TimeStepType::Zone) {
        EndTimeStepFlag = true;
    } else {
        EndTimeStepFlag = false;
    }
    Real64 MinuteNow = op->TimeValue.at(t_TimeStepTypeKey).CurMinute; // What minute it is now

    int MDHM; // Month,Day,Hour,Minute
    EncodeMonDayHrMin(MDHM, state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, int(MinuteNow));
    TimePrint = true;

    Real64 rxTime = (MinuteNow - StartMinute) /
                    double(state.dataGlobal->MinutesPerTimeStep); // (MinuteNow-StartMinute)/REAL(MinutesPerTimeStep,r64) - for execution time

    if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
        // R and I data frames for TimeStepType::TimeStepZone
        if (t_TimeStepTypeKey == TimeStepType::Zone && !state.dataResultsFramework->resultsFramework->RIDetailedZoneTSData.rVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                ReportingFrequency::EachCall, op->RVariableTypes, op->NumOfRVariable, TimeStepType::Zone);
        }
        if (t_TimeStepTypeKey == TimeStepType::Zone && !state.dataResultsFramework->resultsFramework->RIDetailedZoneTSData.iVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                ReportingFrequency::EachCall, op->IVariableTypes, op->NumOfIVariable, TimeStepType::Zone);
        }

        // R and I data frames for TimeStepType::TimeStepSystem
        if (t_TimeStepTypeKey == TimeStepType::System && !state.dataResultsFramework->resultsFramework->RIDetailedHVACTSData.rVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                ReportingFrequency::EachCall, op->RVariableTypes, op->NumOfRVariable, TimeStepType::System);
        }
        if (t_TimeStepTypeKey == TimeStepType::System && !state.dataResultsFramework->resultsFramework->RIDetailedHVACTSData.iVariablesScanned()) {
            state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                ReportingFrequency::EachCall, op->IVariableTypes, op->NumOfIVariable, TimeStepType::System);
        }
    }

    if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
        if (t_TimeStepTypeKey == TimeStepType::Zone) {
            state.dataResultsFramework->resultsFramework->RIDetailedZoneTSData.newRow(state.dataEnvrn->Month,
                                                                                      state.dataEnvrn->DayOfMonth,
                                                                                      state.dataGlobal->HourOfDay,
                                                                                      op->TimeValue.at(TimeStepType::Zone).CurMinute,
                                                                                      state.dataGlobal->CalendarYear);
        }
        if (t_TimeStepTypeKey == TimeStepType::System) {
            // TODO this was an error probably, was using TimeValue(1)
            state.dataResultsFramework->resultsFramework->RIDetailedHVACTSData.newRow(state.dataEnvrn->Month,
                                                                                      state.dataEnvrn->DayOfMonth,
                                                                                      state.dataGlobal->HourOfDay,
                                                                                      op->TimeValue.at(TimeStepType::System).CurMinute,
                                                                                      state.dataGlobal->CalendarYear);
        }
    }

    // Main "Record Keeping" Loops for R and I variables
    for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
        if (op->RVariableTypes(Loop).timeStepType != t_TimeStepTypeKey) continue;

        // Act on the RVariables variable
        auto &rVar(op->RVariableTypes(Loop).VarPtr);
        rVar.Stored = true;
        if (rVar.storeType == StoreType::Averaged) {
            Real64 CurVal = (*rVar.Which) * rxTime;
            //        CALL SetMinMax(RVar%Which,MDHM,RVar%MaxValue,RVar%maxValueDate,RVar%MinValue,RVar%minValueDate)
            if ((*rVar.Which) > rVar.MaxValue) {
                rVar.MaxValue = (*rVar.Which);
                rVar.maxValueDate = MDHM;
            }
            if ((*rVar.Which) < rVar.MinValue) {
                rVar.MinValue = (*rVar.Which);
                rVar.minValueDate = MDHM;
            }
            rVar.TSValue += CurVal;
            rVar.EITSValue = rVar.TSValue; // CR - 8481 fix - 09/06/2011
        } else {
            //        CurVal=RVar%Which
            if ((*rVar.Which) > rVar.MaxValue) {
                rVar.MaxValue = (*rVar.Which);
                rVar.maxValueDate = MDHM;
            }
            if ((*rVar.Which) < rVar.MinValue) {
                rVar.MinValue = (*rVar.Which);
                rVar.minValueDate = MDHM;
            }
            rVar.TSValue += (*rVar.Which);
            rVar.EITSValue = rVar.TSValue; // CR - 8481 fix - 09/06/2011
        }

        // End of "record keeping"  Report if applicable
        if (!rVar.Report) continue;
        bool ReportNow = true;
        if (rVar.SchedPtr > 0) ReportNow = (GetCurrentScheduleValue(state, rVar.SchedPtr) != 0.0); // SetReportNow(RVar%SchedPtr)
        if (!ReportNow) continue;
        rVar.tsStored = true;
        if (!rVar.thisTSStored) {
            ++rVar.thisTSCount;
            rVar.thisTSStored = true;
        }

        if (rVar.frequency == ReportingFrequency::EachCall) {
            if (TimePrint) {
                if (op->LHourP != state.dataGlobal->HourOfDay || std::abs(op->LStartMin - StartMinute) > 0.001 ||
                    std::abs(op->LEndMin - op->TimeValue.at(t_TimeStepTypeKey).CurMinute) > 0.001) {
                    int CurDayType = state.dataEnvrn->DayOfWeek;
                    if (state.dataEnvrn->HolidayIndex > 0) {
                        CurDayType = state.dataEnvrn->HolidayIndex;
                    }
                    WriteTimeStampFormatData(state,
                                             state.files.eso,
                                             ReportingFrequency::EachCall,
                                             op->TimeStepStampReportNbr,
                                             op->TimeStepStampReportChr,
                                             state.dataGlobal->DayOfSimChr,
                                             true,
                                             state.dataEnvrn->Month,
                                             state.dataEnvrn->DayOfMonth,
                                             state.dataGlobal->HourOfDay,
                                             op->TimeValue.at(t_TimeStepTypeKey).CurMinute,
                                             StartMinute,
                                             state.dataEnvrn->DSTIndicator,
                                             ScheduleManager::dayTypeNames[CurDayType]);
                    op->LHourP = state.dataGlobal->HourOfDay;
                    op->LStartMin = StartMinute;
                    op->LEndMin = op->TimeValue.at(t_TimeStepTypeKey).CurMinute;
                }
                TimePrint = false;
            }
            WriteNumericData(state, rVar.ReportID, rVar.ReportIDChr, *rVar.Which);
            ++state.dataGlobal->StdOutputRecordCount;

            if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
                if (t_TimeStepTypeKey == TimeStepType::Zone) {
                    state.dataResultsFramework->resultsFramework->RIDetailedZoneTSData.pushVariableValue(rVar.ReportID, *rVar.Which);
                }
                if (t_TimeStepTypeKey == TimeStepType::System) {
                    state.dataResultsFramework->resultsFramework->RIDetailedHVACTSData.pushVariableValue(rVar.ReportID, *rVar.Which);
                }
            }
        }
    }

    for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
        if (op->IVariableTypes(Loop).timeStepType != t_TimeStepTypeKey) continue;

        // Act on the IVariables variable
        auto &iVar = op->IVariableTypes(Loop).VarPtr;
        iVar.Stored = true;
        //      ICurVal=IVar%Which
        if (iVar.storeType == StoreType::Averaged) {
            Real64 ICurVal = (*iVar.Which) * rxTime;
            iVar.TSValue += ICurVal;
            iVar.EITSValue = iVar.TSValue; // CR - 8481 fix - 09/06/2011
            if (nint(ICurVal) > iVar.MaxValue) {
                iVar.MaxValue = nint(ICurVal); // Record keeping for date and time go here too
                iVar.maxValueDate = MDHM;      //+ TimeValue.at(t_TimeStepTypeKey)%TimeStep
            }
            if (nint(ICurVal) < iVar.MinValue) {
                iVar.MinValue = nint(ICurVal);
                iVar.minValueDate = MDHM; //+ TimeValue.at(t_TimeStepTypeKey)%TimeStep
            }
        } else {
            if ((*iVar.Which) > iVar.MaxValue) {
                iVar.MaxValue = (*iVar.Which); // Record keeping for date and time go here too
                iVar.maxValueDate = MDHM;      //+ TimeValue(TimeStepType)%TimeStep
            }
            if ((*iVar.Which) < iVar.MinValue) {
                iVar.MinValue = (*iVar.Which);
                iVar.minValueDate = MDHM; //+ TimeValue(TimeStepType)%TimeStep
            }
            iVar.TSValue += (*iVar.Which);
            iVar.EITSValue = iVar.TSValue; // CR - 8481 fix - 09/06/2011
        }

        if (!iVar.Report) continue;
        bool ReportNow = true;
        if (iVar.SchedPtr > 0) ReportNow = (GetCurrentScheduleValue(state, iVar.SchedPtr) != 0.0); // SetReportNow(IVar%SchedPtr)
        if (!ReportNow) continue;
        iVar.tsStored = true;
        if (!iVar.thisTSStored) {
            ++iVar.thisTSCount;
            iVar.thisTSStored = true;
        }

        if (iVar.frequency == ReportingFrequency::EachCall) {
            if (TimePrint) {
                if (op->LHourP != state.dataGlobal->HourOfDay || std::abs(op->LStartMin - StartMinute) > 0.001 ||
                    std::abs(op->LEndMin - op->TimeValue.at(t_TimeStepTypeKey).CurMinute) > 0.001) {
                    int CurDayType = state.dataEnvrn->DayOfWeek;
                    if (state.dataEnvrn->HolidayIndex > 0) {
                        CurDayType = state.dataEnvrn->HolidayIndex;
                    }
                    WriteTimeStampFormatData(state,
                                             state.files.eso,
                                             ReportingFrequency::EachCall,
                                             op->TimeStepStampReportNbr,
                                             op->TimeStepStampReportChr,
                                             state.dataGlobal->DayOfSimChr,
                                             true,
                                             state.dataEnvrn->Month,
                                             state.dataEnvrn->DayOfMonth,
                                             state.dataGlobal->HourOfDay,
                                             op->TimeValue.at(t_TimeStepTypeKey).CurMinute,
                                             StartMinute,
                                             state.dataEnvrn->DSTIndicator,
                                             ScheduleManager::dayTypeNames[CurDayType]);
                    op->LHourP = state.dataGlobal->HourOfDay;
                    op->LStartMin = StartMinute;
                    op->LEndMin = op->TimeValue.at(t_TimeStepTypeKey).CurMinute;
                }
                TimePrint = false;
            }
            // only time integer vars actual report as integer only is "detailed"
            WriteNumericData(state, iVar.ReportID, iVar.ReportIDChr, *iVar.Which);
            ++state.dataGlobal->StdOutputRecordCount;

            if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
                if (t_TimeStepTypeKey == TimeStepType::Zone) {
                    state.dataResultsFramework->resultsFramework->RIDetailedZoneTSData.pushVariableValue(iVar.ReportID, *iVar.Which);
                }
                if (t_TimeStepTypeKey == TimeStepType::System) {
                    state.dataResultsFramework->resultsFramework->RIDetailedHVACTSData.pushVariableValue(iVar.ReportID, *iVar.Which);
                }
            }
        }
    }

    if (t_TimeStepTypeKey == TimeStepType::System) return; // All other stuff happens at the "zone" time step call to this routine.

    // TimeStep Block (Report on Zone TimeStep)

    if (EndTimeStepFlag) {
        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            if (!state.dataResultsFramework->resultsFramework->RITimestepTSData.rVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                    ReportingFrequency::TimeStep, op->RVariableTypes, op->NumOfRVariable);
            }
            if (!state.dataResultsFramework->resultsFramework->RITimestepTSData.iVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                    ReportingFrequency::TimeStep, op->IVariableTypes, op->NumOfIVariable);
            }
            state.dataResultsFramework->resultsFramework->RITimestepTSData.newRow(state.dataEnvrn->Month,
                                                                                  state.dataEnvrn->DayOfMonth,
                                                                                  state.dataGlobal->HourOfDay,
                                                                                  op->TimeValue.at(TimeStepType::Zone).CurMinute,
                                                                                  state.dataGlobal->CalendarYear);
        }

        for (TimeStepType thisTimeStepType : {TimeStepType::Zone, TimeStepType::System}) { // Zone, HVAC
            for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
                if (op->RVariableTypes(Loop).timeStepType != thisTimeStepType) continue;
                auto &rVar = op->RVariableTypes(Loop).VarPtr;
                // Update meters on the TimeStep  (Zone)
                if (rVar.MeterArrayPtr != 0 && !state.dataOutputProcessor->MeterValue.empty()) {
                    Real64 TimeStepValue = rVar.TSValue * rVar.ZoneMult * rVar.ZoneListMult;
                    for (int i = 1; i <= op->VarMeterArrays(rVar.MeterArrayPtr).NumOnMeters; i++) {
                        int index = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(i);
                        state.dataOutputProcessor->MeterValue(index) += TimeStepValue;
                    }
                    for (int i = 1; i <= op->VarMeterArrays(rVar.MeterArrayPtr).NumOnCustomMeters; i++) {
                        int index = op->VarMeterArrays(rVar.MeterArrayPtr).OnCustomMeters(i);
                        state.dataOutputProcessor->MeterValue(index) += TimeStepValue;
                    }
                }

                bool ReportNow = true;
                if (rVar.SchedPtr > 0) ReportNow = (GetCurrentScheduleValue(state, rVar.SchedPtr) != 0.0); // SetReportNow(RVar%SchedPtr)
                if (!ReportNow || !rVar.Report) {
                    rVar.TSValue = 0.0;
                }
                //        IF (RVar%StoreType == AveragedVar) THEN
                //          RVar%Value=RVar%Value+RVar%TSValue/NumOfTimeStepInHour
                //        ELSE
                rVar.Value += rVar.TSValue;
                //        ENDIF

                if (!ReportNow || !rVar.Report) continue;

                if (rVar.frequency == ReportingFrequency::TimeStep) {
                    if (TimePrint) {
                        if (op->LHourP != state.dataGlobal->HourOfDay || std::abs(op->LStartMin - StartMinute) > 0.001 ||
                            std::abs(op->LEndMin - op->TimeValue.at(thisTimeStepType).CurMinute) > 0.001) {
                            int CurDayType = state.dataEnvrn->DayOfWeek;
                            if (state.dataEnvrn->HolidayIndex > 0) {
                                CurDayType = state.dataEnvrn->HolidayIndex;
                            }
                            WriteTimeStampFormatData(state,
                                                     state.files.eso,
                                                     ReportingFrequency::EachCall,
                                                     op->TimeStepStampReportNbr,
                                                     op->TimeStepStampReportChr,
                                                     state.dataGlobal->DayOfSimChr,
                                                     true,
                                                     state.dataEnvrn->Month,
                                                     state.dataEnvrn->DayOfMonth,
                                                     state.dataGlobal->HourOfDay,
                                                     op->TimeValue.at(thisTimeStepType).CurMinute,
                                                     StartMinute,
                                                     state.dataEnvrn->DSTIndicator,
                                                     ScheduleManager::dayTypeNames[CurDayType]);
                            op->LHourP = state.dataGlobal->HourOfDay;
                            op->LStartMin = StartMinute;
                            op->LEndMin = op->TimeValue.at(thisTimeStepType).CurMinute;
                        }
                        TimePrint = false;
                    }

                    WriteNumericData(state, rVar.ReportID, rVar.ReportIDChr, rVar.TSValue);
                    ++state.dataGlobal->StdOutputRecordCount;

                    if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
                        state.dataResultsFramework->resultsFramework->RITimestepTSData.pushVariableValue(rVar.ReportID, rVar.TSValue);
                    }
                }
                rVar.TSValue = 0.0;
                rVar.thisTSStored = false;
            } // Number of R Variables

            for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
                if (op->IVariableTypes(Loop).timeStepType != thisTimeStepType) continue;
                auto &iVar = op->IVariableTypes(Loop).VarPtr;
                bool ReportNow = true;
                if (iVar.SchedPtr > 0) ReportNow = (GetCurrentScheduleValue(state, iVar.SchedPtr) != 0.0); // SetReportNow(IVar%SchedPtr)
                if (!ReportNow) {
                    iVar.TSValue = 0.0;
                }
                //        IF (IVar%StoreType == AveragedVar) THEN
                //          IVar%Value=IVar%Value+REAL(IVar%TSValue,r64)/REAL(NumOfTimeStepInHour,r64)
                //        ELSE
                iVar.Value += iVar.TSValue;
                //        ENDIF

                if (!ReportNow || !iVar.Report) continue;

                if (iVar.frequency == ReportingFrequency::TimeStep) {
                    if (TimePrint) {
                        if (op->LHourP != state.dataGlobal->HourOfDay || std::abs(op->LStartMin - StartMinute) > 0.001 ||
                            std::abs(op->LEndMin - op->TimeValue.at(thisTimeStepType).CurMinute) > 0.001) {
                            int CurDayType = state.dataEnvrn->DayOfWeek;
                            if (state.dataEnvrn->HolidayIndex > 0) {
                                CurDayType = state.dataEnvrn->HolidayIndex;
                            }
                            WriteTimeStampFormatData(state,
                                                     state.files.eso,
                                                     ReportingFrequency::EachCall,
                                                     op->TimeStepStampReportNbr,
                                                     op->TimeStepStampReportChr,
                                                     state.dataGlobal->DayOfSimChr,
                                                     true,
                                                     state.dataEnvrn->Month,
                                                     state.dataEnvrn->DayOfMonth,
                                                     state.dataGlobal->HourOfDay,
                                                     op->TimeValue.at(thisTimeStepType).CurMinute,
                                                     StartMinute,
                                                     state.dataEnvrn->DSTIndicator,
                                                     ScheduleManager::dayTypeNames[CurDayType]);
                            op->LHourP = state.dataGlobal->HourOfDay;
                            op->LStartMin = StartMinute;
                            op->LEndMin = op->TimeValue.at(thisTimeStepType).CurMinute;
                        }
                        TimePrint = false;
                    }

                    WriteNumericData(state, iVar.ReportID, iVar.ReportIDChr, iVar.TSValue);
                    ++state.dataGlobal->StdOutputRecordCount;

                    if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
                        state.dataResultsFramework->resultsFramework->RITimestepTSData.pushVariableValue(iVar.ReportID, iVar.TSValue);
                    }
                }
                iVar.TSValue = 0.0;
                iVar.thisTSStored = false;
            } // Number of I Variables
        }     // Index Type (Zone or HVAC)

        UpdateMeters(state, MDHM);

        ReportTSMeters(state, StartMinute, op->TimeValue.at(TimeStepType::Zone).CurMinute, TimePrint, TimePrint);

    } // TimeStep Block

    // Hour Block
    if (state.dataGlobal->EndHourFlag) {
        if (op->TrackingHourlyVariables) {
            int CurDayType = state.dataEnvrn->DayOfWeek;
            if (state.dataEnvrn->HolidayIndex > 0) {
                CurDayType = state.dataEnvrn->HolidayIndex;
            }
            WriteTimeStampFormatData(state,
                                     state.files.eso,
                                     ReportingFrequency::Hourly,
                                     op->TimeStepStampReportNbr,
                                     op->TimeStepStampReportChr,
                                     state.dataGlobal->DayOfSimChr,
                                     true,
                                     state.dataEnvrn->Month,
                                     state.dataEnvrn->DayOfMonth,
                                     state.dataGlobal->HourOfDay,
                                     _,
                                     _,
                                     state.dataEnvrn->DSTIndicator,
                                     ScheduleManager::dayTypeNames[CurDayType]);
            TimePrint = false;
        }

        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            if (!state.dataResultsFramework->resultsFramework->RIHourlyTSData.rVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                    ReportingFrequency::Hourly, op->RVariableTypes, op->NumOfRVariable);
            }
            if (!state.dataResultsFramework->resultsFramework->RIHourlyTSData.iVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                    ReportingFrequency::Hourly, op->IVariableTypes, op->NumOfIVariable);
            }
            state.dataResultsFramework->resultsFramework->RIHourlyTSData.newRow(
                state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
        }

        for (TimeStepType thisTimeStepType : {TimeStepType::Zone, TimeStepType::System}) { // Zone, HVAC
            op->TimeValue.at(thisTimeStepType).CurMinute = 0.0;
            for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
                if (op->RVariableTypes(Loop).timeStepType != thisTimeStepType) continue;
                auto &rVar = op->RVariableTypes(Loop).VarPtr;
                //        ReportNow=.TRUE.
                //        IF (RVar%SchedPtr > 0) &
                //          ReportNow=(GetCurrentScheduleValue(state, RVar%SchedPtr) /= 0.0)  !SetReportNow(RVar%SchedPtr)

                //        IF (ReportNow) THEN
                if (rVar.tsStored) {
                    if (rVar.storeType == StoreType::Averaged) {
                        rVar.Value /= double(rVar.thisTSCount);
                    }
                    if (rVar.Report && rVar.frequency == ReportingFrequency::Hourly && rVar.Stored) {
                        WriteNumericData(state, rVar.ReportID, rVar.ReportIDChr, rVar.Value);
                        ++state.dataGlobal->StdOutputRecordCount;
                        rVar.Stored = false;
                        // add time series value for hourly to data store
                        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
                            state.dataResultsFramework->resultsFramework->RIHourlyTSData.pushVariableValue(rVar.ReportID, rVar.Value);
                        }
                    }
                    rVar.StoreValue += rVar.Value;
                    ++rVar.NumStored;
                }
                rVar.tsStored = false;
                rVar.thisTSStored = false;
                rVar.thisTSCount = 0;
                rVar.Value = 0.0;
            } // Number of R Variables

            for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
                if (op->IVariableTypes(Loop).timeStepType != thisTimeStepType) continue;
                auto &iVar = op->IVariableTypes(Loop).VarPtr;
                //        ReportNow=.TRUE.
                //        IF (IVar%SchedPtr > 0) &
                //          ReportNow=(GetCurrentScheduleValue(state, IVar%SchedPtr) /= 0.0)  !SetReportNow(IVar%SchedPtr)
                //        IF (ReportNow) THEN
                if (iVar.tsStored) {
                    if (iVar.storeType == StoreType::Averaged) {
                        iVar.Value /= double(iVar.thisTSCount);
                    }
                    if (iVar.Report && iVar.frequency == ReportingFrequency::Hourly && iVar.Stored) {
                        WriteNumericData(state, iVar.ReportID, iVar.ReportIDChr, iVar.Value);
                        ++state.dataGlobal->StdOutputRecordCount;
                        iVar.Stored = false;
                        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
                            state.dataResultsFramework->resultsFramework->RIHourlyTSData.pushVariableValue(iVar.ReportID, iVar.Value);
                        }
                    }
                    iVar.StoreValue += iVar.Value;
                    ++iVar.NumStored;
                }
                iVar.tsStored = false;
                iVar.thisTSStored = false;
                iVar.thisTSCount = 0;
                iVar.Value = 0.0;
            } // Number of I Variables
        }     // thisTimeStepType (Zone or HVAC)

        ReportHRMeters(state, TimePrint);

    } // Hour Block

    if (!state.dataGlobal->EndHourFlag) return;

    // Day Block
    if (state.dataGlobal->EndDayFlag) {
        if (op->TrackingDailyVariables) {
            int CurDayType = state.dataEnvrn->DayOfWeek;
            if (state.dataEnvrn->HolidayIndex > 0) {
                CurDayType = state.dataEnvrn->HolidayIndex;
            }
            WriteTimeStampFormatData(state,
                                     state.files.eso,
                                     ReportingFrequency::Daily,
                                     op->DailyStampReportNbr,
                                     op->DailyStampReportChr,
                                     state.dataGlobal->DayOfSimChr,
                                     true,
                                     state.dataEnvrn->Month,
                                     state.dataEnvrn->DayOfMonth,
                                     _,
                                     _,
                                     _,
                                     state.dataEnvrn->DSTIndicator,
                                     ScheduleManager::dayTypeNames[CurDayType]);
            TimePrint = false;
        }
        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            if (!state.dataResultsFramework->resultsFramework->RIDailyTSData.rVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                    ReportingFrequency::Daily, op->RVariableTypes, op->NumOfRVariable);
            }
            if (!state.dataResultsFramework->resultsFramework->RIDailyTSData.iVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                    ReportingFrequency::Daily, op->IVariableTypes, op->NumOfIVariable);
            }
            state.dataResultsFramework->resultsFramework->RIDailyTSData.newRow(
                state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
        }

        op->NumHoursInMonth += 24;
        for (TimeStepType thisTimeStepType : {TimeStepType::Zone, TimeStepType::System}) { // Zone, HVAC
            for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
                if (op->RVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteRealVariableOutput(state, op->RVariableTypes(Loop).VarPtr, ReportingFrequency::Daily);
                }
            } // Number of R Variables

            for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
                if (op->IVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteIntegerVariableOutput(state, op->IVariableTypes(Loop).VarPtr, ReportingFrequency::Daily);
                }
            } // Number of I Variables
        }     // thisTimeStepType (Zone or HVAC)

        ReportDYMeters(state, TimePrint);

    } // Day Block

    // Only continue if EndDayFlag is set
    if (!state.dataGlobal->EndDayFlag) return;

    // Month Block
    if (state.dataEnvrn->EndMonthFlag || state.dataGlobal->EndEnvrnFlag) {
        if (op->TrackingMonthlyVariables) {
            WriteTimeStampFormatData(state,
                                     state.files.eso,
                                     ReportingFrequency::Monthly,
                                     op->MonthlyStampReportNbr,
                                     op->MonthlyStampReportChr,
                                     state.dataGlobal->DayOfSimChr,
                                     true,
                                     state.dataEnvrn->Month);
            TimePrint = false;
        }

        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            if (!state.dataResultsFramework->resultsFramework->RIMonthlyTSData.rVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                    ReportingFrequency::Monthly, op->RVariableTypes, op->NumOfRVariable);
            }
            if (!state.dataResultsFramework->resultsFramework->RIMonthlyTSData.iVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                    ReportingFrequency::Monthly, op->IVariableTypes, op->NumOfIVariable);
            }
            state.dataResultsFramework->resultsFramework->RIMonthlyTSData.newRow(
                state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
        }

        op->NumHoursInSim += op->NumHoursInMonth;
        state.dataEnvrn->EndMonthFlag = false;
        for (TimeStepType thisTimeStepType : {TimeStepType::Zone, TimeStepType::System}) { // Zone, HVAC
            for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
                if (op->RVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteRealVariableOutput(state, op->RVariableTypes(Loop).VarPtr, ReportingFrequency::Monthly);
                }
            } // Number of R Variables

            for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
                if (op->IVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteIntegerVariableOutput(state, op->IVariableTypes(Loop).VarPtr, ReportingFrequency::Monthly);
                }
            } // Number of I Variables
        }     // thisTimeStepType (Zone, HVAC)

        ReportMNMeters(state, TimePrint);

        op->NumHoursInMonth = 0;
    } // Month Block

    // Sim/Environment Block
    if (state.dataGlobal->EndEnvrnFlag) {
        if (op->TrackingRunPeriodVariables) {
            WriteTimeStampFormatData(state,
                                     state.files.eso,
                                     ReportingFrequency::Simulation,
                                     op->RunPeriodStampReportNbr,
                                     op->RunPeriodStampReportChr,
                                     state.dataGlobal->DayOfSimChr,
                                     true);
            TimePrint = false;
        }

        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            if (!state.dataResultsFramework->resultsFramework->RIRunPeriodTSData.rVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                    ReportingFrequency::Simulation, op->RVariableTypes, op->NumOfRVariable);
            }
            if (!state.dataResultsFramework->resultsFramework->RIRunPeriodTSData.iVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                    ReportingFrequency::Simulation, op->IVariableTypes, op->NumOfIVariable);
            }
            state.dataResultsFramework->resultsFramework->RIRunPeriodTSData.newRow(
                state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
        }
        for (TimeStepType thisTimeStepType : {TimeStepType::Zone, TimeStepType::System}) { // Zone, HVAC
            for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
                if (op->RVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteRealVariableOutput(state, op->RVariableTypes(Loop).VarPtr, ReportingFrequency::Simulation);
                }
            } // Number of R Variables

            for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
                if (op->IVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteIntegerVariableOutput(state, op->IVariableTypes(Loop).VarPtr, ReportingFrequency::Simulation);
                }
            } // Number of I Variables
        }     // thisTimeStepType (Zone, HVAC)

        ReportSMMeters(state, TimePrint);

        op->NumHoursInSim = 0;
    }

    // Yearly Block
    if (state.dataEnvrn->EndYearFlag) {
        if (op->TrackingYearlyVariables) {
            WriteYearlyTimeStamp(state, state.files.eso, op->YearlyStampReportChr, state.dataGlobal->CalendarYearChr, true);
            TimePrint = false;
        }
        if (state.dataResultsFramework->resultsFramework->timeSeriesEnabled()) {
            if (!state.dataResultsFramework->resultsFramework->RIYearlyTSData.rVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeRTSDataFrame(
                    ReportingFrequency::Yearly, op->RVariableTypes, op->NumOfRVariable);
            }
            if (!state.dataResultsFramework->resultsFramework->RIYearlyTSData.iVariablesScanned()) {
                state.dataResultsFramework->resultsFramework->initializeITSDataFrame(
                    ReportingFrequency::Yearly, op->IVariableTypes, op->NumOfIVariable);
            }
            state.dataResultsFramework->resultsFramework->RIYearlyTSData.newRow(
                state.dataEnvrn->Month, state.dataEnvrn->DayOfMonth, state.dataGlobal->HourOfDay, 0, state.dataGlobal->CalendarYear);
        }
        for (TimeStepType thisTimeStepType : {TimeStepType::Zone, TimeStepType::System}) { // Zone, HVAC
            for (int Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
                if (op->RVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteRealVariableOutput(state, op->RVariableTypes(Loop).VarPtr, ReportingFrequency::Yearly);
                }
            } // Number of R Variables

            for (int Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
                if (op->IVariableTypes(Loop).timeStepType == thisTimeStepType) {
                    WriteIntegerVariableOutput(state, op->IVariableTypes(Loop).VarPtr, ReportingFrequency::Yearly);
                }
            } // Number of I Variables
        }     // thisTimeStepType (Zone, HVAC)

        ReportYRMeters(state, TimePrint);

        state.dataGlobal->CalendarYear += 1;
        state.dataGlobal->CalendarYearChr = fmt::to_string(state.dataGlobal->CalendarYear);
    }
}

void AssignReportNumber(EnergyPlusData &state, int &ReportNumber)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   December 1997
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine returns the next report number available.  The report number
    // is used in output reports as a key.

    // METHODOLOGY EMPLOYED:
    // Use internal ReportNumberCounter to maintain current report numbers.

    // REFERENCES:
    // na

    // USE STATEMENTS:
    using namespace OutputProcessor;

    // Locals
    // SUBROUTINE ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:

    ++state.dataOutputProcessor->ReportNumberCounter;
    ReportNumber = state.dataOutputProcessor->ReportNumberCounter;
}

void GenOutputVariablesAuditReport(EnergyPlusData &state)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   February 2000
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine reports (to the .err file) any report variables
    // which were requested but not "setup" during the run.  These will
    // either be items that were not used in the IDF file or misspellings
    // of report variable names.

    // METHODOLOGY EMPLOYED:
    // Use flagged data structure in OutputProcessor.

    // Using/Aliasing
    using namespace OutputProcessor;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop;
    auto &op = state.dataOutputProcessor;
    std::map<ReportingFrequency, std::string> reportFrequency({{ReportingFrequency::EachCall, "Detailed"},
                                                               {ReportingFrequency::TimeStep, "TimeStep"},
                                                               {ReportingFrequency::Hourly, "Hourly"},
                                                               {ReportingFrequency::Daily, "Daily"},
                                                               {ReportingFrequency::Monthly, "Monthly"},
                                                               {ReportingFrequency::Yearly, "Annual"}});

    for (Loop = 1; Loop <= op->NumOfReqVariables; ++Loop) {
        if (op->ReqRepVars(Loop).Used) continue;
        if (op->ReqRepVars(Loop).Key.empty()) op->ReqRepVars(Loop).Key = "*";
        if (has(op->ReqRepVars(Loop).VarName, "OPAQUE SURFACE INSIDE FACE CONDUCTION") && !state.dataGlobal->DisplayAdvancedReportVariables &&
            !state.dataOutputProcessor->OpaqSurfWarned) {
            ShowWarningError(state, R"(Variables containing "Opaque Surface Inside Face Conduction" are now "advanced" variables.)");
            ShowContinueError(state, "You must enter the \"Output:Diagnostics,DisplayAdvancedReportVariables;\" statement to view.");
            ShowContinueError(state, "First, though, read cautionary statements in the \"InputOutputReference\" document.");
            state.dataOutputProcessor->OpaqSurfWarned = true;
        }
        if (!state.dataOutputProcessor->Rept) {
            ShowWarningError(state, "The following Report Variables were requested but not generated -- check.rdd file");
            ShowContinueError(state, "Either the IDF did not contain these elements, the variable name is misspelled,");
            ShowContinueError(state,
                              "or the requested variable is an advanced output which requires Output : Diagnostics, DisplayAdvancedReportVariables;");
            state.dataOutputProcessor->Rept = true;
        }
        ShowMessage(state,
                    format("Key={}, VarName={}, Frequency={}",
                           op->ReqRepVars(Loop).Key,
                           op->ReqRepVars(Loop).VarName,
                           reportFrequency[op->ReqRepVars(Loop).frequency]));
    }
}

void UpdateMeterReporting(EnergyPlusData &state)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   January 2001
    //       MODIFIED       February 2007 -- add cumulative meter reporting
    //                      January 2012 -- add predefined tabular meter reporting
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine is called at the end of the first HVAC iteration and
    // sets up the reporting for the Energy Meters.  It also may show a fatal error
    // if errors occurred during initial SetupOutputVariable processing.  It "gets"
    // the Report Meter input:
    // Report Meter,
    //        \memo Meters requested here show up on eplusout.eso and eplusout.mtr
    //   A1 , \field Meter_Name
    //        \required-field
    //        \note Form is EnergyUseType:..., e.g. Electricity:* for all Electricity meters
    //        \note or EndUse:..., e.g. InteriorLights:* for all interior lights
    //        \note Report MeterFileOnly puts results on the eplusout.mtr file only
    //   A2 ; \field Reporting_Frequency
    //        \type choice
    //        \key timestep
    //        \note timestep refers to the zone timestep/timestep in hour value
    //        \note runperiod, environment, and annual are the same
    //        \key hourly
    //        \key daily
    //        \key monthly
    //        \key runperiod
    //        \key environment
    //        \key annual
    //        \note runperiod, environment, and annual are synonymous
    // Report MeterFileOnly,
    //        \memo same reporting as Report Meter -- goes to eplusout.mtr only
    //   A1 , \field Meter_Name
    //        \required-field
    //        \note Form is EnergyUseType:..., e.g. Electricity:* for all Electricity meters
    //        \note or EndUse:..., e.g. InteriorLights:* for all interior lights
    //        \note Report MeterFileOnly puts results on the eplusout.mtr file only
    //   A2 ; \field Reporting_Frequency
    //        \type choice
    //        \key timestep
    //        \note timestep refers to the zone timestep/timestep in hour value
    //        \note runperiod, environment, and annual are the same
    //        \key hourly
    //        \key daily
    //        \key monthly
    //        \key runperiod
    //        \key environment
    //        \key annual
    //        \note runperiod, environment, and annual are synonymous

    // Using/Aliasing
    using namespace OutputProcessor;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop;
    Array1D_string Alphas(2);
    Array1D<Real64> Numbers(1);
    int NumAlpha;
    int NumNumbers;
    int IOStat;
    int NumReqMeters;
    int NumReqMeterFOs;

    bool ErrorsFound(false); // If errors detected in input
    auto &op = state.dataOutputProcessor;

    GetCustomMeterInput(state, ErrorsFound);
    if (ErrorsFound) {
        op->ErrorsLogged = true;
    }

    // Helper lambda to locate a meter index from its name. Returns a negative value if not found
    auto setupMeterFromMeterName = // (AUTO_OK_LAMBDA)
        [&state](std::string &name, std::string const &freqString, bool MeterFileOnlyIndicator, bool CumulativeIndicator) -> bool {
        bool result = false;

        size_t varnameLen = index(name, '[');
        if (varnameLen != std::string::npos) {
            name.erase(varnameLen);
        }

        auto &op = state.dataOutputProcessor;

        std::string::size_type wildCardPosition = index(name, '*');

        if (wildCardPosition == std::string::npos) {
            int meterIndex = Util::FindItem(name, op->EnergyMeters);
            if (meterIndex > 0) {
                ReportingFrequency ReportFreq = determineFrequency(state, freqString);
                SetInitialMeterReportingAndOutputNames(state, meterIndex, MeterFileOnlyIndicator, ReportFreq, CumulativeIndicator);
                result = true;
            }
        } else { // Wildcard input
            ReportingFrequency ReportFreq = determineFrequency(state, freqString);
            for (int meterIndex = 1; meterIndex <= op->NumEnergyMeters; ++meterIndex) {
                if (Util::SameString(op->EnergyMeters(meterIndex).Name.substr(0, wildCardPosition), name.substr(0, wildCardPosition))) {
                    SetInitialMeterReportingAndOutputNames(state, meterIndex, MeterFileOnlyIndicator, ReportFreq, CumulativeIndicator);
                    result = true;
                }
            }
        }

        return result;
    };

    auto &cCurrentModuleObject = state.dataIPShortCut->cCurrentModuleObject;
    cCurrentModuleObject = "Output:Meter";
    NumReqMeters = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);

    for (Loop = 1; Loop <= NumReqMeters; ++Loop) {

        state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                 cCurrentModuleObject,
                                                                 Loop,
                                                                 Alphas,
                                                                 NumAlpha,
                                                                 Numbers,
                                                                 NumNumbers,
                                                                 IOStat,
                                                                 state.dataIPShortCut->lNumericFieldBlanks,
                                                                 state.dataIPShortCut->lAlphaFieldBlanks,
                                                                 state.dataIPShortCut->cAlphaFieldNames,
                                                                 state.dataIPShortCut->cNumericFieldNames);

        bool meterFileOnlyIndicator = false;
        bool cumulativeIndicator = false;
        if (!setupMeterFromMeterName(Alphas(1), Alphas(2), meterFileOnlyIndicator, cumulativeIndicator)) {
            ShowWarningError(
                state, format("{}: invalid {}=\"{}\" - not found.", cCurrentModuleObject, state.dataIPShortCut->cAlphaFieldNames(1), Alphas(1)));
        }
    }

    cCurrentModuleObject = "Output:Meter:MeterFileOnly";
    NumReqMeterFOs = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);
    for (Loop = 1; Loop <= NumReqMeterFOs; ++Loop) {

        state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                 cCurrentModuleObject,
                                                                 Loop,
                                                                 Alphas,
                                                                 NumAlpha,
                                                                 Numbers,
                                                                 NumNumbers,
                                                                 IOStat,
                                                                 state.dataIPShortCut->lNumericFieldBlanks,
                                                                 state.dataIPShortCut->lAlphaFieldBlanks,
                                                                 state.dataIPShortCut->cAlphaFieldNames,
                                                                 state.dataIPShortCut->cNumericFieldNames);

        bool meterFileOnlyIndicator = true;
        bool cumulativeIndicator = false;
        if (!setupMeterFromMeterName(Alphas(1), Alphas(2), meterFileOnlyIndicator, cumulativeIndicator)) {
            ShowWarningError(
                state, format("{}: invalid {}=\"{}\" - not found.", cCurrentModuleObject, state.dataIPShortCut->cAlphaFieldNames(1), Alphas(1)));
        }
    }

    cCurrentModuleObject = "Output:Meter:Cumulative";
    NumReqMeters = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);

    for (Loop = 1; Loop <= NumReqMeters; ++Loop) {

        state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                 cCurrentModuleObject,
                                                                 Loop,
                                                                 Alphas,
                                                                 NumAlpha,
                                                                 Numbers,
                                                                 NumNumbers,
                                                                 IOStat,
                                                                 state.dataIPShortCut->lNumericFieldBlanks,
                                                                 state.dataIPShortCut->lAlphaFieldBlanks,
                                                                 state.dataIPShortCut->cAlphaFieldNames,
                                                                 state.dataIPShortCut->cNumericFieldNames);

        bool meterFileOnlyIndicator = false;
        bool cumulativeIndicator = true;
        if (!setupMeterFromMeterName(Alphas(1), Alphas(2), meterFileOnlyIndicator, cumulativeIndicator)) {
            ShowWarningError(
                state, format("{}: invalid {}=\"{}\" - not found.", cCurrentModuleObject, state.dataIPShortCut->cAlphaFieldNames(1), Alphas(1)));
        }
    }

    cCurrentModuleObject = "Output:Meter:Cumulative:MeterFileOnly";
    NumReqMeterFOs = state.dataInputProcessing->inputProcessor->getNumObjectsFound(state, cCurrentModuleObject);
    for (Loop = 1; Loop <= NumReqMeterFOs; ++Loop) {

        state.dataInputProcessing->inputProcessor->getObjectItem(state,
                                                                 cCurrentModuleObject,
                                                                 Loop,
                                                                 Alphas,
                                                                 NumAlpha,
                                                                 Numbers,
                                                                 NumNumbers,
                                                                 IOStat,
                                                                 state.dataIPShortCut->lNumericFieldBlanks,
                                                                 state.dataIPShortCut->lAlphaFieldBlanks,
                                                                 state.dataIPShortCut->cAlphaFieldNames,
                                                                 state.dataIPShortCut->cNumericFieldNames);

        bool meterFileOnlyIndicator = true;
        bool cumulativeIndicator = true;
        if (!setupMeterFromMeterName(Alphas(1), Alphas(2), meterFileOnlyIndicator, cumulativeIndicator)) {
            ShowWarningError(
                state, format("{}: invalid {}=\"{}\" - not found.", cCurrentModuleObject, state.dataIPShortCut->cAlphaFieldNames(1), Alphas(1)));
        }
    }

    ReportMeterDetails(state);

    if (op->ErrorsLogged) {
        ShowFatalError(state, "UpdateMeterReporting: Previous Meter Specification errors cause program termination.");
    }

    op->MeterValue.dimension(op->NumEnergyMeters, 0.0);
}

void SetInitialMeterReportingAndOutputNames(EnergyPlusData &state,
                                            int const WhichMeter,              // Which meter number
                                            bool const MeterFileOnlyIndicator, // true if this is a meter file only reporting
                                            OutputProcessor::ReportingFrequency const FrequencyIndicator, // at what frequency is the meter reported
                                            bool const CumulativeIndicator // true if this is a Cumulative meter reporting
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   February 2007
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // Set values and output initial names to output files.

    // Using/Aliasing
    using namespace OutputProcessor;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int indexGroupKey;
    std::string indexGroup;
    auto &op = state.dataOutputProcessor;

    if ((FrequencyIndicator == ReportingFrequency::EachCall) ||
        (FrequencyIndicator == ReportingFrequency::TimeStep)) { // roll "detailed" into TimeStep
        if (!CumulativeIndicator) {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptTS) {
                    ShowWarningError(
                        state,
                        format(
                            "Output:Meter:MeterFileOnly requested for \"{}\" (TimeStep), already on \"Output:Meter\". Will report to both {} and {}",
                            op->EnergyMeters(WhichMeter).Name,
                            state.files.eso.filePath.filename().string(),
                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptTS) {
                op->EnergyMeters(WhichMeter).RptTS = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptTSFO = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).TSRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         op->EnergyMeters(WhichMeter).TSRptNumChr,
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         false,
                                         MeterFileOnlyIndicator);
            }
        } else {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptAccTS) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"Cumulative {}\" (TimeStep), already on \"Output:Meter\". "
                                            "Will report to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptAccTS) {
                op->EnergyMeters(WhichMeter).RptAccTS = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptAccTSFO = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).TSAccRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         fmt::to_string(op->EnergyMeters(WhichMeter).TSAccRptNum),
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         true,
                                         MeterFileOnlyIndicator);
            }
        }
    } else if (FrequencyIndicator == ReportingFrequency::Hourly) {
        if (!CumulativeIndicator) {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptHR) {
                    ShowWarningError(
                        state,
                        format("Output:Meter:MeterFileOnly requested for \"{}\" (Hourly), already on \"Output:Meter\". Will report to both {} and {}",
                               op->EnergyMeters(WhichMeter).Name,
                               state.files.eso.filePath.filename().string(),
                               state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptHR) {
                op->EnergyMeters(WhichMeter).RptHR = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptHRFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingHourlyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).HRRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         op->EnergyMeters(WhichMeter).HRRptNumChr,
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         false,
                                         MeterFileOnlyIndicator);
            }
        } else {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptAccHR) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"Cummulative {}\" (Hourly), already on \"Output:Meter\". Will "
                                            "report to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptAccHR) {
                op->EnergyMeters(WhichMeter).RptAccHR = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptAccHRFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingHourlyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).HRAccRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         fmt::to_string(op->EnergyMeters(WhichMeter).HRAccRptNum),
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         true,
                                         MeterFileOnlyIndicator);
            }
        }
    } else if (FrequencyIndicator == ReportingFrequency::Daily) {
        if (!CumulativeIndicator) {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptDY) {
                    ShowWarningError(
                        state,
                        format("Output:Meter:MeterFileOnly requested for \"{}\" (Daily), already on \"Output:Meter\". Will report to both {} and {}",
                               op->EnergyMeters(WhichMeter).Name,
                               state.files.eso.filePath.filename().string(),
                               state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptDY) {
                op->EnergyMeters(WhichMeter).RptDY = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptDYFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingDailyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).DYRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         op->EnergyMeters(WhichMeter).DYRptNumChr,
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         false,
                                         MeterFileOnlyIndicator);
            }
        } else {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptAccDY) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"Cumulative {}\" (Daily), already on \"Output:Meter\". Will "
                                            "report to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptAccDY) {
                op->EnergyMeters(WhichMeter).RptAccDY = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptAccDYFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingDailyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).DYAccRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         fmt::to_string(op->EnergyMeters(WhichMeter).DYAccRptNum),
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         true,
                                         MeterFileOnlyIndicator);
            }
        }
    } else if (FrequencyIndicator == ReportingFrequency::Monthly) {
        if (!CumulativeIndicator) {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptMN) {
                    ShowWarningError(
                        state,
                        format(
                            "Output:Meter:MeterFileOnly requested for \"{}\" (Monthly), already on \"Output:Meter\". Will report to both {} and {}",
                            op->EnergyMeters(WhichMeter).Name,
                            state.files.eso.filePath.filename().string(),
                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptMN) {
                op->EnergyMeters(WhichMeter).RptMN = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptMNFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingMonthlyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).MNRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         op->EnergyMeters(WhichMeter).MNRptNumChr,
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         false,
                                         MeterFileOnlyIndicator);
            }
        } else {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptAccMN) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"Cumulative {}\" (Monthly), already on \"Output:Meter\". Will "
                                            "report to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptAccMN) {
                op->EnergyMeters(WhichMeter).RptAccMN = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptAccMNFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingMonthlyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).MNAccRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         fmt::to_string(op->EnergyMeters(WhichMeter).MNAccRptNum),
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         true,
                                         MeterFileOnlyIndicator);
            }
        }
    } else if (FrequencyIndicator == ReportingFrequency::Yearly) {
        if (!CumulativeIndicator) {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptYR) {
                    ShowWarningError(
                        state,
                        format("Output:Meter:MeterFileOnly requested for \"{}\" (Annual), already on \"Output:Meter\". Will report to both {} and {}",
                               op->EnergyMeters(WhichMeter).Name,
                               state.files.eso.filePath.filename().string(),
                               state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptYR) {
                op->EnergyMeters(WhichMeter).RptYR = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptYRFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingYearlyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).YRRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         op->EnergyMeters(WhichMeter).YRRptNumChr,
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         false,
                                         MeterFileOnlyIndicator);
            }
        } else {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptAccYR) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"Cumulative {}\" (Annual), already on \"Output:Meter\". Will "
                                            "report to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptAccYR) {
                op->EnergyMeters(WhichMeter).RptAccYR = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptAccYRFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingYearlyVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).YRAccRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         fmt::to_string(op->EnergyMeters(WhichMeter).YRAccRptNum),
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         true,
                                         MeterFileOnlyIndicator);
            }
        }
    } else if (FrequencyIndicator == ReportingFrequency::Simulation) {
        if (!CumulativeIndicator) {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptSM) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"{}\" (RunPeriod), already on \"Output:Meter\". Will report "
                                            "to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptSM) {
                op->EnergyMeters(WhichMeter).RptSM = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptSMFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingRunPeriodVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).SMRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         op->EnergyMeters(WhichMeter).SMRptNumChr,
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         false,
                                         MeterFileOnlyIndicator);
            }
        } else {
            if (MeterFileOnlyIndicator) {
                if (op->EnergyMeters(WhichMeter).RptAccSM) {
                    ShowWarningError(state,
                                     format("Output:Meter:MeterFileOnly requested for \"Cumulative {}\" (RunPeriod), already on \"Output:Meter\". "
                                            "Will report to both {} and {}",
                                            op->EnergyMeters(WhichMeter).Name,
                                            state.files.eso.filePath.filename().string(),
                                            state.files.mtr.filePath.filename().string()));
                }
            }
            if (!op->EnergyMeters(WhichMeter).RptAccSM) {
                op->EnergyMeters(WhichMeter).RptAccSM = true;
                if (MeterFileOnlyIndicator) op->EnergyMeters(WhichMeter).RptAccSMFO = true;
                if (!MeterFileOnlyIndicator) op->TrackingRunPeriodVariables = true;
                indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(WhichMeter).Name);
                indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(WhichMeter));
                WriteMeterDictionaryItem(state,
                                         FrequencyIndicator,
                                         StoreType::Summed,
                                         op->EnergyMeters(WhichMeter).SMAccRptNum,
                                         indexGroupKey,
                                         indexGroup,
                                         fmt::to_string(op->EnergyMeters(WhichMeter).SMAccRptNum),
                                         op->EnergyMeters(WhichMeter).Name,
                                         op->EnergyMeters(WhichMeter).Units,
                                         true,
                                         MeterFileOnlyIndicator);
            }
        }
    } else {
    }
}

int GetMeterIndex(EnergyPlusData &state, std::string_view const MeterName)
{

    // FUNCTION INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   August 2002
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function returns a index to the meter "number" (aka assigned report number)
    // for the meter name.  If none active for this run, a zero is returned.  This is used later to
    // obtain a meter "value".

    // Using/Aliasing
    using namespace OutputProcessor;
    using SortAndStringUtilities::SetupAndSort;

    // Return value
    int MeterIndex;

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    // Valid Meter names because matching case insensitive
    //////////// hoisted into namespace changed to GetMeterIndexFirstCall////////////
    // static bool FirstCall( true );
    ////////////////////////////////////////////////
    int Found;
    auto &op = state.dataOutputProcessor;

    if (op->GetMeterIndexFirstCall || (state.dataOutputProcessor->NumValidMeters != op->NumEnergyMeters)) {
        state.dataOutputProcessor->NumValidMeters = op->NumEnergyMeters;
        state.dataOutputProcessor->ValidMeterNames.allocate(state.dataOutputProcessor->NumValidMeters);
        for (Found = 1; Found <= state.dataOutputProcessor->NumValidMeters; ++Found) {
            state.dataOutputProcessor->ValidMeterNames(Found) = Util::makeUPPER(op->EnergyMeters(Found).Name);
        }
        state.dataOutputProcessor->iValidMeterNames.allocate(state.dataOutputProcessor->NumValidMeters);
        SetupAndSort(state.dataOutputProcessor->ValidMeterNames, state.dataOutputProcessor->iValidMeterNames);
        op->GetMeterIndexFirstCall = false;
    }

    MeterIndex = Util::FindItemInSortedList(MeterName, state.dataOutputProcessor->ValidMeterNames, state.dataOutputProcessor->NumValidMeters);
    if (MeterIndex != 0) MeterIndex = state.dataOutputProcessor->iValidMeterNames(MeterIndex);

    return MeterIndex;
}

std::string GetMeterResourceType(EnergyPlusData &state, int const MeterNumber) // Which Meter Number (from GetMeterIndex)
{

    // FUNCTION INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   August 2002
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function returns the character string of Resource Type for the
    // given meter number/index. If MeterNumber is 0, ResourceType=Invalid/Unknown.

    // METHODOLOGY EMPLOYED:
    // na

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;

    // Return value
    std::string ResourceType;

    // Locals
    // FUNCTION ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    if (MeterNumber > 0) {
        ResourceType = state.dataOutputProcessor->EnergyMeters(MeterNumber).ResourceType;
    } else {
        ResourceType = "Invalid/Unknown";
    }

    return ResourceType;
}

Real64 GetCurrentMeterValue(EnergyPlusData &state, int const MeterNumber) // Which Meter Number (from GetMeterIndex)
{

    // FUNCTION INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   August 2002
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function returns the current meter value (timestep) for the meter number indicated.

    // METHODOLOGY EMPLOYED:
    // Uses internal EnergyMeters structure to get value.

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;

    // Return value
    Real64 CurrentMeterValue;

    // Locals
    // FUNCTION ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    // na

    if (MeterNumber > 0) {
        CurrentMeterValue = state.dataOutputProcessor->EnergyMeters(MeterNumber).CurTSValue;
    } else {
        CurrentMeterValue = 0.0;
    }

    return CurrentMeterValue;
}

Real64 GetInstantMeterValue(EnergyPlusData &state,
                            int const MeterNumber,                             // Which Meter Number (from GetMeterIndex)
                            OutputProcessor::TimeStepType const t_timeStepType // Whether this is zone of HVAC
)
{

    // FUNCTION INFORMATION:
    //       AUTHOR         Richard Liesen
    //       DATE WRITTEN   February 2003
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function returns the Instantaneous meter value (timestep) for the meter number indicated
    //  using TimeStepType to differentiate between Zone and HVAC values.

    // METHODOLOGY EMPLOYED:
    // Uses internal EnergyMeters structure to get value.

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;

    // Return value
    Real64 InstantMeterValue(0.0);

    // Locals
    // FUNCTION ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:

    //      EnergyMeters(Meter)%TSValue=EnergyMeters(EnergyMeters(Meter)%SourceMeter)%TSValue-MeterValue(Meter)

    if (MeterNumber == 0) return InstantMeterValue;
    auto &op = state.dataOutputProcessor;

    auto &energy_meter = op->EnergyMeters(MeterNumber);
    auto &cache_beg = energy_meter.InstMeterCacheStart;
    auto &cache_end = energy_meter.InstMeterCacheEnd;
    if (energy_meter.TypeOfMeter != MtrType::CustomDec) {
        // section added to speed up the execution of this routine
        // instead of looping through all the VarMeterArrays to see if a RVariableType is used for a
        // specific meter, create a list of all the indexes for RVariableType that are used for that
        // meter.
        if (cache_beg == 0) { // not yet added to the cache
            for (int Loop = 1; Loop <= op->NumVarMeterArrays; ++Loop) {
                auto const &var_meter_on = op->VarMeterArrays(Loop).OnMeters;
                for (int Meter = 1, Meter_end = op->VarMeterArrays(Loop).NumOnMeters; Meter <= Meter_end; ++Meter) {
                    if (var_meter_on(Meter) == MeterNumber) {
                        IncrementInstMeterCache(state);
                        cache_end = op->InstMeterCacheLastUsed;
                        if (cache_beg == 0) cache_beg = op->InstMeterCacheLastUsed;
                        op->InstMeterCache(op->InstMeterCacheLastUsed) = op->VarMeterArrays(Loop).RepVariable;
                        break;
                    }
                }
                auto const &var_meter_on_custom = op->VarMeterArrays(Loop).OnCustomMeters;
                for (int Meter = 1, Meter_end = op->VarMeterArrays(Loop).NumOnCustomMeters; Meter <= Meter_end; ++Meter) {
                    if (var_meter_on_custom(Meter) == MeterNumber) {
                        IncrementInstMeterCache(state);
                        cache_end = op->InstMeterCacheLastUsed;
                        if (cache_beg == 0) cache_beg = op->InstMeterCacheLastUsed;
                        op->InstMeterCache(op->InstMeterCacheLastUsed) = op->VarMeterArrays(Loop).RepVariable;
                        break;
                    }
                } // End Number of Meters Loop
            }
        }
        for (int Loop = cache_beg; Loop <= cache_end; ++Loop) {
            auto &r_var_loop = op->RVariableTypes(op->InstMeterCache(Loop));
            // Separate the Zone variables from the HVAC variables using TimeStepType
            if (r_var_loop.timeStepType == t_timeStepType) {
                auto &rVar = r_var_loop.VarPtr;
                // Add to the total all of the appropriate variables
                InstantMeterValue += (*rVar.Which) * rVar.ZoneMult * rVar.ZoneListMult;
            }
        }
    } else { // MeterType_CustomDec
        // Get Source Meter value
        // Loop through all report meters to find correct report variables to add to instant meter total
        for (int Loop = 1; Loop <= op->NumVarMeterArrays; ++Loop) {
            auto &r_var_loop = op->RVariableTypes(op->VarMeterArrays(Loop).RepVariable);

            auto const &var_meter_on = op->VarMeterArrays(Loop).OnMeters;
            for (int Meter = 1, Meter_end = op->VarMeterArrays(Loop).NumOnMeters; Meter <= Meter_end; ++Meter) {
                if (var_meter_on(Meter) == energy_meter.SourceMeter) {
                    // Separate the Zone variables from the HVAC variables using TimeStepType
                    if (r_var_loop.timeStepType == t_timeStepType) {
                        auto &rVar = r_var_loop.VarPtr;
                        // Add to the total all of the appropriate variables
                        InstantMeterValue += (*rVar.Which) * rVar.ZoneMult * rVar.ZoneListMult;
                        break;
                    }
                }
            }

            auto const &var_meter_on_custom = op->VarMeterArrays(Loop).OnCustomMeters;
            for (int Meter = 1, Meter_end = op->VarMeterArrays(Loop).NumOnCustomMeters; Meter <= Meter_end; ++Meter) {
                if (var_meter_on_custom(Meter) == energy_meter.SourceMeter) {
                    // Separate the Zone variables from the HVAC variables using TimeStepType
                    if (r_var_loop.timeStepType == t_timeStepType) {
                        auto &rVar = r_var_loop.VarPtr;
                        // Add to the total all of the appropriate variables
                        InstantMeterValue += (*rVar.Which) * rVar.ZoneMult * rVar.ZoneListMult;
                        break;
                    }
                }
            }

        } // End Number of Meters Loop
        for (int Loop = 1; Loop <= op->NumVarMeterArrays; ++Loop) {
            auto &r_var_loop = op->RVariableTypes(op->VarMeterArrays(Loop).RepVariable);

            auto const &var_meter_on = op->VarMeterArrays(Loop).OnMeters;
            for (int Meter = 1, Meter_end = op->VarMeterArrays(Loop).NumOnMeters; Meter <= Meter_end; ++Meter) {
                if (var_meter_on(Meter) == MeterNumber) {
                    // Separate the Zone variables from the HVAC variables using TimeStepType
                    if (r_var_loop.timeStepType == t_timeStepType) {
                        auto &rVar = r_var_loop.VarPtr;
                        // Add to the total all of the appropriate variables
                        InstantMeterValue -= (*rVar.Which) * rVar.ZoneMult * rVar.ZoneListMult;
                        break;
                    }
                }
            }

            auto const &var_meter_on_custom = op->VarMeterArrays(Loop).OnCustomMeters;
            for (int Meter = 1, Meter_end = op->VarMeterArrays(Loop).NumOnCustomMeters; Meter <= Meter_end; ++Meter) {
                if (var_meter_on_custom(Meter) == MeterNumber) {
                    // Separate the Zone variables from the HVAC variables using TimeStepType
                    if (r_var_loop.timeStepType == t_timeStepType) {
                        auto &rVar = r_var_loop.VarPtr;
                        // Add to the total all of the appropriate variables
                        InstantMeterValue -= (*rVar.Which) * rVar.ZoneMult * rVar.ZoneListMult;
                        break;
                    }
                }
            }

        } // End Number of Meters Loop
    }

    return InstantMeterValue;
}

void IncrementInstMeterCache(EnergyPlusData &state)
{
    // SUBROUTINE INFORMATION:
    //       AUTHOR         Jason Glazer
    //       DATE WRITTEN   January 2013
    //       MODIFIED
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // Manage the InstMeterCache array

    // METHODOLOGY EMPLOYED:
    // When the array grows to large, double it.

    auto &op = state.dataOutputProcessor;

    if (!allocated(op->InstMeterCache)) {
        op->InstMeterCache.dimension(op->InstMeterCacheSizeInc, 0); // zero the entire array
        op->InstMeterCacheLastUsed = 1;
    } else {
        ++op->InstMeterCacheLastUsed;
        // if larger than current size grow the array
        if (op->InstMeterCacheLastUsed > op->InstMeterCacheSize) {
            op->InstMeterCache.redimension(op->InstMeterCacheSize += op->InstMeterCacheSizeInc, 0);
        }
    }
}

Real64 GetInternalVariableValue(EnergyPlusData &state,
                                OutputProcessor::VariableType const varType, // 1=integer, 2=real, 3=meter
                                int const keyVarIndex                        // Array index
)
{
    // FUNCTION INFORMATION:
    //       AUTHOR         Linda K. Lawrie
    //       DATE WRITTEN   December 2000
    //       MODIFIED       August 2003, M. J. Witte
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function returns the current value of the Internal Variable assigned to
    // the varType and keyVarIndex.  Values may be accessed for REAL(r64) and integer
    // report variables and meter variables.  The variable type (varType) may be
    // determined by calling subroutine and GetVariableKeyCountandType.  The
    // index (keyVarIndex) may be determined by calling subroutine GetVariableKeys.

    // METHODOLOGY EMPLOYED:
    // Uses Internal OutputProcessor data structure to return value.

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;
    using ScheduleManager::GetCurrentScheduleValue;

    // Return value
    Real64 resultVal; // value returned

    // Locals
    // FUNCTION ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    // na

    auto &op = state.dataOutputProcessor;

    // Select based on variable type:  integer, real, or meter
    if (varType == VariableType::NotFound) { // Variable not a found variable
        resultVal = 0.0;
    } else if (varType == VariableType::Integer) {
        if (keyVarIndex > op->NumOfIVariable) {
            ShowFatalError(state, "GetInternalVariableValue: Integer variable passed index beyond range of array.");
            ShowContinueError(state, format("Index = {} Number of integer variables = {}", keyVarIndex, op->NumOfIVariable));
        }
        if (keyVarIndex < 1) {
            ShowFatalError(state, format("GetInternalVariableValue: Integer variable passed index <1. Index = {}", keyVarIndex));
        }

        // must use %Which, %Value is always zero if variable is not a requested report variable
        resultVal = double(*op->IVariableTypes(keyVarIndex).VarPtr.Which);
    } else if (varType == VariableType::Real) {
        if (keyVarIndex > op->NumOfRVariable) {
            ShowFatalError(state, "GetInternalVariableValue: Real variable passed index beyond range of array.");
            ShowContinueError(state, format("Index = {} Number of real variables = {}", keyVarIndex, op->NumOfRVariable));
        }
        if (keyVarIndex < 1) {
            ShowFatalError(state, format("GetInternalVariableValue: Integer variable passed index <1. Index = {}", keyVarIndex));
        }

        // must use %Which, %Value is always zero if variable is not a requested report variable
        resultVal = *op->RVariableTypes(keyVarIndex).VarPtr.Which;
    } else if (varType == VariableType::Meter) {
        resultVal = GetCurrentMeterValue(state, keyVarIndex);
    } else if (varType == VariableType::Schedule) {
        resultVal = GetCurrentScheduleValue(state, keyVarIndex);
    } else {
        resultVal = 0.0;
    }

    return resultVal;
}

Real64 GetInternalVariableValueExternalInterface(EnergyPlusData &state,
                                                 OutputProcessor::VariableType const varType, // 1=integer, 2=REAL(r64), 3=meter
                                                 int const keyVarIndex                        // Array index
)
{
    // FUNCTION INFORMATION:
    //       AUTHOR         Thierry S. Nouidui
    //       DATE WRITTEN   August 2011
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function returns the last zone-timestep value of the Internal Variable assigned to
    // the varType and keyVarIndex.  Values may be accessed for REAL(r64) and integer
    // report variables and meter variables.  The variable type (varType) may be
    // determined by calling subroutine and GetVariableKeyCountandType.  The
    // index (keyVarIndex) may be determined by calling subroutine GetVariableKeys.

    // METHODOLOGY EMPLOYED:
    // Uses Internal OutputProcessor data structure to return value.

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;
    using ScheduleManager::GetCurrentScheduleValue;

    // Return value
    Real64 resultVal; // value returned

    // Locals
    // FUNCTION ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    // na

    auto &op = state.dataOutputProcessor;

    // Select based on variable type:  integer, REAL(r64), or meter
    if (varType == VariableType::NotFound) { // Variable not a found variable
        resultVal = 0.0;
    } else if (varType == VariableType::Integer) {
        if (keyVarIndex > op->NumOfIVariable) {
            ShowFatalError(state, "GetInternalVariableValueExternalInterface: passed index beyond range of array.");
        }
        if (keyVarIndex < 1) {
            ShowFatalError(state, "GetInternalVariableValueExternalInterface: passed index beyond range of array.");
        }

        // must use %EITSValue, %This is the last-zonetimestep value
        resultVal = double(op->IVariableTypes(keyVarIndex).VarPtr.EITSValue);
    } else if (varType == VariableType::Real) {
        if (keyVarIndex > op->NumOfRVariable) {
            ShowFatalError(state, "GetInternalVariableValueExternalInterface: passed index beyond range of array.");
        }
        if (keyVarIndex < 1) {
            ShowFatalError(state, "GetInternalVariableValueExternalInterface: passed index beyond range of array.");
        }

        // must use %EITSValue, %This is the last-zonetimestep value
        resultVal = op->RVariableTypes(keyVarIndex).VarPtr.EITSValue;
    } else if (varType == VariableType::Meter) {
        resultVal = GetCurrentMeterValue(state, keyVarIndex);
    } else if (varType == VariableType::Schedule) {
        resultVal = GetCurrentScheduleValue(state, keyVarIndex);
    } else {
        resultVal = 0.0;
    }

    return resultVal;
}

int GetNumMeteredVariables(EnergyPlusData &state,
                           [[maybe_unused]] std::string const &ComponentType, // Given Component Type
                           std::string const &ComponentName                   // Given Component Name (user defined)
)
{

    // FUNCTION INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   May 2005
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function counts the number of metered variables associated with the
    // given ComponentType/Name.   This resultant number would then be used to
    // allocate arrays for a call the GetMeteredVariables routine.

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    int Loop;
    int NumVariables = 0;
    auto &op = state.dataOutputProcessor;

    for (Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
        //    Pos=INDEX(RVariableTypes(Loop)%VarName,':')
        //    IF (ComponentName /= RVariableTypes(Loop)%VarNameUC(1:Pos-1)) CYCLE
        if (ComponentName != op->RVariableTypes(Loop).KeyNameOnlyUC) continue;
        auto &rVar = op->RVariableTypes(Loop).VarPtr;
        if (rVar.MeterArrayPtr == 0) {
            continue;
        }
        if (op->VarMeterArrays(rVar.MeterArrayPtr).NumOnMeters > 0) {
            ++NumVariables;
        }
    }

    return NumVariables;
}

void GetMeteredVariables(EnergyPlusData &state,
                         std::string const &ComponentType,                      // Given Component Type
                         std::string const &ComponentName,                      // Given Component Name (user defined)
                         Array1D_int &VarIndexes,                               // Variable Numbers
                         Array1D<OutputProcessor::VariableType> &VarTypes,      // Variable Types (1=integer, 2=real, 3=meter)
                         Array1D<OutputProcessor::TimeStepType> &TimeStepTypes, // Variable Index Types (1=Zone,2=HVAC)
                         Array1D<OutputProcessor::Unit> &unitsForVar,           // units from enum for each variable
                         Array1D<Constant::eResource> &ResourceTypes,           // ResourceTypes for each variable
                         Array1D_string &EndUses,                               // EndUses for each variable
                         Array1D_string &Groups,                                // Groups for each variable
                         Array1D_string &Names,                                 // Variable Names for each variable
                         int &NumFound                                          // Number Found
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   May 2005
    //       MODIFIED       Jason DeGraw 2/12/2020, de-optionalized
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This routine gets the variable names and other associated information
    // for metered variables associated with the given ComponentType/Name.

    // Using/Aliasing
    using namespace OutputProcessor;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop;
    int NumVariables;
    int MeterPtr;
    int NumOnMeterPtr;
    int MeterNum;
    auto &op = state.dataOutputProcessor;

    NumVariables = 0;

    for (Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
        //    Pos=INDEX(RVariableTypes(Loop)%VarName,':')
        //    IF (ComponentName /= RVariableTypes(Loop)%VarNameUC(1:Pos-1)) CYCLE
        if (ComponentName != op->RVariableTypes(Loop).KeyNameOnlyUC) continue;
        auto &rVar = op->RVariableTypes(Loop).VarPtr;
        if (rVar.MeterArrayPtr == 0) continue;
        NumOnMeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).NumOnMeters;
        MeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(1);
        if (MeterPtr) {
            ++NumVariables;
            VarIndexes(NumVariables) = Loop;
            VarTypes(NumVariables) = VariableType::Real;
            TimeStepTypes(NumVariables) = op->RVariableTypes(Loop).timeStepType;
            unitsForVar(NumVariables) = op->RVariableTypes(Loop).units;

            ResourceTypes(NumVariables) =
                static_cast<Constant::eResource>(getEnumValue(Constant::eResourceNamesUC, Util::makeUPPER(op->EnergyMeters(MeterPtr).ResourceType)));
            Names(NumVariables) = op->RVariableTypes(Loop).VarNameUC;

            for (MeterNum = 1; MeterNum <= NumOnMeterPtr; ++MeterNum) {
                MeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(MeterNum);
                if (!op->EnergyMeters(MeterPtr).EndUse.empty()) {
                    EndUses(NumVariables) = Util::makeUPPER(op->EnergyMeters(MeterPtr).EndUse);
                    break;
                }
            }

            for (MeterNum = 1; MeterNum <= NumOnMeterPtr; ++MeterNum) {
                MeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(MeterNum);
                if (!op->EnergyMeters(MeterPtr).Group.empty()) {
                    Groups(NumVariables) = Util::makeUPPER(op->EnergyMeters(MeterPtr).Group);
                    break;
                }
            }

        } else {
            ShowWarningError(state,
                             format("Referenced variable or meter used in the wrong context \"{}\" of type \"{}\"", ComponentName, ComponentType));
        }
    }

    NumFound = NumVariables; // Should just return this
}

void GetMeteredVariables(EnergyPlusData &state,
                         std::string const &ComponentType,                      // Given Component Type
                         std::string const &ComponentName,                      // Given Component Name (user defined)
                         Array1D_int &VarIndexes,                               // Variable Numbers
                         Array1D<OutputProcessor::VariableType> &VarTypes,      // Variable Types (1=integer, 2=real, 3=meter)
                         Array1D<OutputProcessor::TimeStepType> &TimeStepTypes, // Variable Index Types (1=Zone,2=HVAC)
                         Array1D<OutputProcessor::Unit> &unitsForVar,           // units from enum for each variable
                         Array1D<Constant::eResource> &ResourceTypes,           // ResourceTypes for each variable
                         Array1D_string &EndUses,                               // EndUses for each variable
                         Array1D_string &Groups,                                // Groups for each variable
                         Array1D_string &Names,                                 // Variable Names for each variable
                         Array1D_int &VarIDs                                    // Variable Report Numbers
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   May 2005
    //       MODIFIED       Jason DeGraw 2/12/2020, de-optionalized
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This routine gets the variable names and other associated information
    // for metered variables associated with the given ComponentType/Name.

    // Using/Aliasing
    using namespace OutputProcessor;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop;
    int NumVariables;
    int MeterPtr;
    int NumOnMeterPtr;
    int MeterNum;
    auto &op = state.dataOutputProcessor;

    NumVariables = 0;

    for (Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
        //    Pos=INDEX(RVariableTypes(Loop)%VarName,':')
        //    IF (ComponentName /= RVariableTypes(Loop)%VarNameUC(1:Pos-1)) CYCLE
        if (ComponentName != op->RVariableTypes(Loop).KeyNameOnlyUC) continue;
        auto &rVar = op->RVariableTypes(Loop).VarPtr;
        if (rVar.MeterArrayPtr == 0) continue;
        NumOnMeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).NumOnMeters;
        MeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(1);
        if (MeterPtr) {
            ++NumVariables;
            VarIndexes(NumVariables) = Loop;
            VarTypes(NumVariables) = VariableType::Real;
            TimeStepTypes(NumVariables) = op->RVariableTypes(Loop).timeStepType;
            unitsForVar(NumVariables) = op->RVariableTypes(Loop).units;

            ResourceTypes(NumVariables) =
                static_cast<Constant::eResource>(getEnumValue(Constant::eResourceNamesUC, Util::makeUPPER(op->EnergyMeters(MeterPtr).ResourceType)));
            Names(NumVariables) = op->RVariableTypes(Loop).VarNameUC;

            for (MeterNum = 1; MeterNum <= NumOnMeterPtr; ++MeterNum) {
                MeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(MeterNum);
                if (!op->EnergyMeters(MeterPtr).EndUse.empty()) {
                    EndUses(NumVariables) = Util::makeUPPER(op->EnergyMeters(MeterPtr).EndUse);
                    break;
                }
            }

            for (MeterNum = 1; MeterNum <= NumOnMeterPtr; ++MeterNum) {
                MeterPtr = op->VarMeterArrays(rVar.MeterArrayPtr).OnMeters(MeterNum);
                if (!op->EnergyMeters(MeterPtr).Group.empty()) {
                    Groups(NumVariables) = Util::makeUPPER(op->EnergyMeters(MeterPtr).Group);
                    break;
                }
            }

            VarIDs(NumVariables) = rVar.ReportID;

        } else {
            ShowWarningError(state,
                             format("Referenced variable or meter used in the wrong context \"{}\" of type \"{}\"", ComponentName, ComponentType));
        }
    }
}

void GetVariableKeyCountandType(EnergyPlusData &state,
                                std::string const &varName, // Standard variable name
                                int &numKeys,               // Number of keys found
                                OutputProcessor::VariableType &varType,
                                OutputProcessor::StoreType &varAvgSum,      // Variable  is Averaged=1 or Summed=2
                                OutputProcessor::TimeStepType &varStepType, // Variable time step is Zone=1 or HVAC=2
                                OutputProcessor::Unit &varUnits             // Units enumeration
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Michael J. Witte
    //       DATE WRITTEN   August 2003
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine returns the variable TYPE (Real, integer, meter, schedule, etc.)
    // (varType) whether it is an averaged or summed variable (varAvgSum),
    // whether it is a zone or HVAC time step (varStepType),
    // and the number of keynames for a given report variable or report meter name
    // (varName).  The variable type (varType) and number of keys (numKeys) are
    // used when calling subroutine GetVariableKeys to obtain a list of the
    // keynames for a particular variable and a corresponding list of indexes.

    // METHODOLOGY EMPLOYED:
    // Uses Internal OutputProcessor data structure to search for varName
    // in each of the three output data arrays:
    //       RVariableTypes - real report variables
    //       IVariableTypes - integer report variables
    //       EnergyMeters   - report meters (via GetMeterIndex function)
    //       Schedules      - specific schedule values
    // When the variable is found, the variable type (varType) is set and the
    // number of associated keys is counted.
    // varType is assigned as follows:
    //       0 = not found
    //       1 = integer
    //       2 = real
    //       3 = meter
    //       4 = schedule
    //  varAvgSum is assigned as follows:
    //       1 = averaged
    //       2 = summed
    //  varStepType is assigned as follows:
    //       1 = zone time step
    //       2 = HVAC time step

    // Using/Aliasing
    using namespace OutputProcessor;
    using ScheduleManager::GetScheduleIndex;
    using ScheduleManager::GetScheduleType;
    using SortAndStringUtilities::SetupAndSort;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop; // Loop counters
    int Loop2;
    std::string::size_type Position; // Starting point of search string
    int VFound;                      // Found integer/real variable attributes
    bool Found;                      // True if varName is found
    bool Duplicate;                  // True if keyname is a duplicate
    std::string VarKeyPlusName;      // Full variable name including keyname and units
    std::string varNameUpper;        // varName pushed to all upper case
    auto &op = state.dataOutputProcessor;

    // INITIALIZATIONS
    if (op->InitFlag) {
        op->curKeyVarIndexLimit = 1000;
        op->keyVarIndexes.allocate(op->curKeyVarIndexLimit);
        op->numVarNames = op->NumVariablesForOutput;
        op->varNames.allocate(op->numVarNames);
        for (Loop = 1; Loop <= op->NumVariablesForOutput; ++Loop) {
            op->varNames(Loop) = Util::makeUPPER(op->DDVariableTypes(Loop).VarNameOnly);
        }
        op->ivarNames.allocate(op->numVarNames);
        SetupAndSort(op->varNames, op->ivarNames);
        op->InitFlag = false;
    }

    if (op->numVarNames != op->NumVariablesForOutput) {
        op->numVarNames = op->NumVariablesForOutput;
        op->varNames.allocate(op->numVarNames);
        for (Loop = 1; Loop <= op->NumVariablesForOutput; ++Loop) {
            op->varNames(Loop) = Util::makeUPPER(op->DDVariableTypes(Loop).VarNameOnly);
        }
        op->ivarNames.allocate(op->numVarNames);
        SetupAndSort(op->varNames, op->ivarNames);
    }

    op->keyVarIndexes = 0;
    varType = VariableType::NotFound;
    numKeys = 0;
    varAvgSum = StoreType::Averaged;
    varStepType = TimeStepType::Zone;
    varUnits = OutputProcessor::Unit::None;
    Found = false;
    Duplicate = false;
    varNameUpper = varName;

    // Search Variable List First
    VFound = Util::FindItemInSortedList(varNameUpper, op->varNames, op->numVarNames);
    if (VFound != 0) {
        varType = op->DDVariableTypes(op->ivarNames(VFound)).variableType;
    }

    if (varType == VariableType::Integer) {
        // Search Integer Variables
        for (Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
            VarKeyPlusName = op->IVariableTypes(Loop).VarNameUC;
            Position = index(VarKeyPlusName, ':' + varNameUpper, true);
            if (Position != std::string::npos) {
                if (VarKeyPlusName.substr(Position + 1) == varNameUpper) {
                    Found = true;
                    varType = VariableType::Integer;
                    Duplicate = false;
                    // Check if duplicate - duplicates happen if the same report variable/key name
                    // combination is requested more than once in the idf at different reporting
                    // frequencies
                    for (Loop2 = 1; Loop2 <= numKeys; ++Loop2) {
                        if (VarKeyPlusName == op->IVariableTypes(op->keyVarIndexes(Loop2)).VarNameUC) Duplicate = true;
                    }
                    if (!Duplicate) {
                        ++numKeys;
                        if (numKeys > op->curKeyVarIndexLimit) {
                            op->keyVarIndexes.redimension(op->curKeyVarIndexLimit += 500, 0);
                        }
                        op->keyVarIndexes(numKeys) = Loop;
                        varAvgSum = op->DDVariableTypes(op->ivarNames(VFound)).storeType;
                        varStepType = op->DDVariableTypes(op->ivarNames(VFound)).timeStepType;
                        varUnits = op->DDVariableTypes(op->ivarNames(VFound)).units;
                    }
                }
            }
        }
    } else if (varType == VariableType::Real) {
        // Search real Variables Next
        for (Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
            if (op->RVariableTypes(Loop).VarNameOnlyUC == varNameUpper) {
                Found = true;
                varType = VariableType::Real;
                Duplicate = false;
                // Check if duplicate - duplicates happen if the same report variable/key name
                // combination is requested more than once in the idf at different reporting
                // frequencies
                VarKeyPlusName = op->RVariableTypes(Loop).VarNameUC;
                for (Loop2 = 1; Loop2 <= numKeys; ++Loop2) {
                    if (VarKeyPlusName == op->RVariableTypes(op->keyVarIndexes(Loop2)).VarNameUC) Duplicate = true;
                }
                if (!Duplicate) {
                    ++numKeys;
                    if (numKeys > op->curKeyVarIndexLimit) {
                        op->keyVarIndexes.redimension(op->curKeyVarIndexLimit += 500, 0);
                    }
                    op->keyVarIndexes(numKeys) = Loop;
                    varAvgSum = op->DDVariableTypes(op->ivarNames(VFound)).storeType;
                    varStepType = op->DDVariableTypes(op->ivarNames(VFound)).timeStepType;
                    varUnits = op->DDVariableTypes(op->ivarNames(VFound)).units;
                }
            }
        }
    }

    // Search Meters if not found in integers or reals
    // Use the GetMeterIndex function
    // Meters do not have keys, so only one will be found
    if (!Found) {
        op->keyVarIndexes(1) = GetMeterIndex(state, varName);
        if (op->keyVarIndexes(1) > 0) {
            Found = true;
            numKeys = 1;
            varType = VariableType::Meter;
            varUnits = op->EnergyMeters(op->keyVarIndexes(1)).Units;
            varAvgSum = StoreType::Summed;
            varStepType = TimeStepType::Zone;
        }
    }

    // Search schedules if not found in integers, reals, or meters
    // Use the GetScheduleIndex function
    // Schedules do not have keys, so only one will be found
    if (!Found) {
        op->keyVarIndexes(1) = GetScheduleIndex(state, varName);
        if (op->keyVarIndexes(1) > 0) {
            Found = true;
            numKeys = 1;
            varType = VariableType::Schedule;
            varUnits = unitStringToEnum(GetScheduleType(state, op->keyVarIndexes(1)));
            varAvgSum = StoreType::Averaged;
            varStepType = TimeStepType::Zone;
        }
    }
}

void GetVariableKeys(EnergyPlusData &state,
                     std::string const &varName,                  // Standard variable name
                     OutputProcessor::VariableType const varType, // 1=integer, 2=real, 3=meter
                     Array1D_string &keyNames,                    // Specific key name
                     Array1D_int &keyVarIndexes                   // Array index for
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Michael J. Witte
    //       DATE WRITTEN   August 2003
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine returns a list of keynames and indexes associated
    // with a particular report variable or report meter name (varName).
    // This routine assumes that the variable TYPE (Real, integer, meter, etc.)
    // may be determined by calling GetVariableKeyCountandType.  The variable type
    // and index can then be used with function GetInternalVariableValue to
    // to retrieve the current value of a particular variable/keyname combination.

    // METHODOLOGY EMPLOYED:
    // Uses Internal OutputProcessor data structure to search for varName
    // and build list of keynames and indexes.  The indexes are the array index
    // in the data array for the

    // Using/Aliasing
    using namespace OutputProcessor;
    using ScheduleManager::GetScheduleIndex;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop; // Loop counters
    int Loop2;
    std::string::size_type Position; // Starting point of search string
    bool Duplicate;                  // True if keyname is a duplicate
    int maxKeyNames;                 // Max allowable # of key names=size of keyNames array
    int maxkeyVarIndexes;            // Max allowable # of key indexes=size of keyVarIndexes array
    int numKeys;                     // Number of keys found
    std::string VarKeyPlusName;      // Full variable name including keyname and units
    std::string varNameUpper;        // varName pushed to all upper case

    // INITIALIZATIONS
    keyNames = "";
    keyVarIndexes = 0;
    numKeys = 0;
    Duplicate = false;
    maxKeyNames = size(keyNames);
    maxkeyVarIndexes = size(keyVarIndexes);
    varNameUpper = Util::makeUPPER(varName);
    auto &op = state.dataOutputProcessor;

    // Select based on variable type:  integer, real, or meter
    if (varType == VariableType::Integer) { // Integer
        for (Loop = 1; Loop <= op->NumOfIVariable; ++Loop) {
            VarKeyPlusName = op->IVariableTypes(Loop).VarNameUC;
            Position = index(VarKeyPlusName, ':' + varNameUpper, true);
            if (Position != std::string::npos) {
                if (VarKeyPlusName.substr(Position + 1) == varNameUpper) {
                    Duplicate = false;
                    // Check if duplicate - duplicates happen if the same report variable/key name
                    // combination is requested more than once in the idf at different reporting
                    // frequencies
                    for (Loop2 = 1; Loop2 <= numKeys; ++Loop2) {
                        if (VarKeyPlusName == op->IVariableTypes(keyVarIndexes(Loop2)).VarNameUC) Duplicate = true;
                    }
                    if (!Duplicate) {
                        ++numKeys;
                        if ((numKeys > maxKeyNames) || (numKeys > maxkeyVarIndexes)) {
                            ShowFatalError(state, "Invalid array size in GetVariableKeys");
                        }
                        keyNames(numKeys) = op->IVariableTypes(Loop).VarNameUC.substr(0, Position);
                        keyVarIndexes(numKeys) = Loop;
                    }
                }
            }
        }
    } else if (varType == VariableType::Real) { // Real
        for (Loop = 1; Loop <= op->NumOfRVariable; ++Loop) {
            if (op->RVariableTypes(Loop).VarNameOnlyUC == varNameUpper) {
                Duplicate = false;
                // Check if duplicate - duplicates happen if the same report variable/key name
                // combination is requested more than once in the idf at different reporting
                // frequencies
                VarKeyPlusName = op->RVariableTypes(Loop).VarNameUC;
                for (Loop2 = 1; Loop2 <= numKeys; ++Loop2) {
                    if (VarKeyPlusName == op->RVariableTypes(keyVarIndexes(Loop2)).VarNameUC) Duplicate = true;
                }
                if (!Duplicate) {
                    ++numKeys;
                    if ((numKeys > maxKeyNames) || (numKeys > maxkeyVarIndexes)) {
                        ShowFatalError(state, "Invalid array size in GetVariableKeys");
                    }
                    keyNames(numKeys) = op->RVariableTypes(Loop).KeyNameOnlyUC;
                    keyVarIndexes(numKeys) = Loop;
                }
            }
        }
    } else if (varType == VariableType::Meter) { // Meter
        numKeys = 1;
        if ((numKeys > maxKeyNames) || (numKeys > maxkeyVarIndexes)) {
            ShowFatalError(state, "Invalid array size in GetVariableKeys");
        }
        keyNames(1) = "Meter";
        keyVarIndexes(1) = GetMeterIndex(state, varName);
    } else if (varType == VariableType::Schedule) { // Schedule
        numKeys = 1;
        if ((numKeys > maxKeyNames) || (numKeys > maxkeyVarIndexes)) {
            ShowFatalError(state, "Invalid array size in GetVariableKeys");
        }
        keyNames(1) = "Environment";
        keyVarIndexes(1) = GetScheduleIndex(state, varName);
    } else {
        // do nothing
    }
}

bool ReportingThisVariable(EnergyPlusData &state, std::string const &RepVarName)
{

    // FUNCTION INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   October 2008
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS FUNCTION:
    // This function scans the report variables and reports back
    // if user has requested this variable be reported.

    // METHODOLOGY EMPLOYED:
    // na

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;

    // Return value
    bool BeingReported;

    // Locals
    // FUNCTION ARGUMENT DEFINITIONS:

    // FUNCTION PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // FUNCTION LOCAL VARIABLE DECLARATIONS:
    int Found;
    auto &op = state.dataOutputProcessor;

    BeingReported = false;
    Found = Util::FindItem(RepVarName, op->ReqRepVars, &ReqReportVariables::VarName);
    if (Found > 0) {
        BeingReported = true;
    }

    if (!BeingReported) { // check meter names too
        Found = Util::FindItem(RepVarName, op->EnergyMeters);
        if (Found > 0) {
            if (op->EnergyMeters(Found).RptTS || op->EnergyMeters(Found).RptHR || op->EnergyMeters(Found).RptDY || op->EnergyMeters(Found).RptMN ||
                op->EnergyMeters(Found).RptSM || op->EnergyMeters(Found).RptTSFO || op->EnergyMeters(Found).RptHRFO ||
                op->EnergyMeters(Found).RptDYFO || op->EnergyMeters(Found).RptMNFO || op->EnergyMeters(Found).RptSMFO ||
                op->EnergyMeters(Found).RptAccTS || op->EnergyMeters(Found).RptAccHR || op->EnergyMeters(Found).RptAccDY ||
                op->EnergyMeters(Found).RptAccMN || op->EnergyMeters(Found).RptAccSM || op->EnergyMeters(Found).RptAccTSFO ||
                op->EnergyMeters(Found).RptAccHRFO || op->EnergyMeters(Found).RptAccDYFO || op->EnergyMeters(Found).RptAccMNFO ||
                op->EnergyMeters(Found).RptAccSMFO) {
                BeingReported = true;
            }
        }
    }

    return BeingReported;
}

void InitPollutionMeterReporting(EnergyPlusData &state, std::string const &ReportFreqName)
{

    // SUBROUTINE INFORMATION:Richard Liesen
    //       DATE WRITTEN   July 2002
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This subroutine is called at the end of the first HVAC iteration and
    // sets up the reporting for the Pollution Meters.
    // ReportPollutionOutput,
    //   A1 ; \field Reporting_Frequency
    //        \type choice
    //        \key timestep
    //        \key hourly
    //        \key daily
    //        \key monthly
    //        \key runperiod
    // METHODOLOGY EMPLOYED:
    // The program tries to setup all of the following meters if the Pollution Report is initiated.
    //       Electricity:Facility [J]
    //       Diesel:Facility [J]
    //       DistrictCooling:Facility [J]
    //       DistrictHeating:Facility [J]
    //       Gas:Facility [J]
    //       GASOLINE:Facility [J]
    //       COAL:Facility [J]
    //       FuelOilNo1:Facility [J]
    //       FuelOilNo2:Facility [J]
    //       Propane:Facility [J]
    //       ElectricityProduced:Facility [J]
    //       Pollutant:CO2
    //       Pollutant:CO
    //       Pollutant:CH4
    //       Pollutant:NOx
    //       Pollutant:N2O
    //       Pollutant:SO2
    //       Pollutant:PM
    //       Pollutant:PM10
    //       Pollutant:PM2.5
    //       Pollutant:NH3
    //       Pollutant:NMVOC
    //       Pollutant:Hg
    //       Pollutant:Pb
    //       Pollutant:WaterEnvironmentalFactors
    //       Pollutant:Nuclear High
    //       Pollutant:Nuclear Low
    //       Pollutant:Carbon Equivalent

    // Using/Aliasing
    using namespace OutputProcessor;
    // SUBROUTINE PARAMETER DEFINITIONS:
    //             Now for the Pollution Meters
    static Array1D_string const PollutionMeters({1, 29},
                                                {"Electricity:Facility",
                                                 "Diesel:Facility",
                                                 "DistrictCooling:Facility",
                                                 "DistrictHeatingWater:Facility",
                                                 "DistrictHeatingSteam:Facility",
                                                 "NaturalGas:Facility",
                                                 "GASOLINE:Facility",
                                                 "COAL:Facility",
                                                 "FuelOilNo1:Facility",
                                                 "FuelOilNo2:Facility",
                                                 "Propane:Facility",
                                                 "ElectricityProduced:Facility",
                                                 "CO2:Facility",
                                                 "CO:Facility",
                                                 "CH4:Facility",
                                                 "NOx:Facility",
                                                 "N2O:Facility",
                                                 "SO2:Facility",
                                                 "PM:Facility",
                                                 "PM10:Facility",
                                                 "PM2.5:Facility",
                                                 "NH3:Facility",
                                                 "NMVOC:Facility",
                                                 "Hg:Facility",
                                                 "Pb:Facility",
                                                 "WaterEnvironmentalFactors:Facility",
                                                 "Nuclear High:Facility",
                                                 "Nuclear Low:Facility",
                                                 "Carbon Equivalent:Facility"});

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    int Loop;
    int NumReqMeters;
    int Meter;
    ReportingFrequency ReportFreq(determineFrequency(state, ReportFreqName));

    int indexGroupKey;
    std::string indexGroup;
    auto &op = state.dataOutputProcessor;

    NumReqMeters = 29;

    for (Loop = 1; Loop <= NumReqMeters; ++Loop) {

        Meter = Util::FindItem(PollutionMeters(Loop), op->EnergyMeters);
        if (Meter > 0) { // All the active meters for this run are set, but all are still searched for.

            indexGroupKey = DetermineIndexGroupKeyFromMeterName(state, op->EnergyMeters(Meter).Name);
            indexGroup = DetermineIndexGroupFromMeterGroup(op->EnergyMeters(Meter));
            // All of the specified meters are checked and the headers printed to the meter file if this
            //  has not been done previously
            if (ReportFreq == ReportingFrequency::TimeStep) {
                if (op->EnergyMeters(Meter).RptTS) {
                    op->EnergyMeters(Meter).RptTS = true;
                } else {
                    op->EnergyMeters(Meter).RptTS = true;
                    WriteMeterDictionaryItem(state,
                                             ReportFreq,
                                             StoreType::Summed,
                                             op->EnergyMeters(Meter).TSRptNum,
                                             indexGroupKey,
                                             indexGroup,
                                             op->EnergyMeters(Meter).TSRptNumChr,
                                             op->EnergyMeters(Meter).Name,
                                             op->EnergyMeters(Meter).Units,
                                             false,
                                             false);
                }
            } else if (ReportFreq == ReportingFrequency::Hourly) {
                if (op->EnergyMeters(Meter).RptHR) {
                    op->EnergyMeters(Meter).RptHR = true;
                    op->TrackingHourlyVariables = true;
                } else {
                    op->EnergyMeters(Meter).RptHR = true;
                    op->TrackingHourlyVariables = true;
                    WriteMeterDictionaryItem(state,
                                             ReportFreq,
                                             StoreType::Summed,
                                             op->EnergyMeters(Meter).HRRptNum,
                                             indexGroupKey,
                                             indexGroup,
                                             op->EnergyMeters(Meter).HRRptNumChr,
                                             op->EnergyMeters(Meter).Name,
                                             op->EnergyMeters(Meter).Units,
                                             false,
                                             false);
                }
            } else if (ReportFreq == ReportingFrequency::Daily) {
                if (op->EnergyMeters(Meter).RptDY) {
                    op->EnergyMeters(Meter).RptDY = true;
                    op->TrackingDailyVariables = true;
                } else {
                    op->EnergyMeters(Meter).RptDY = true;
                    op->TrackingDailyVariables = true;
                    WriteMeterDictionaryItem(state,
                                             ReportFreq,
                                             StoreType::Summed,
                                             op->EnergyMeters(Meter).DYRptNum,
                                             indexGroupKey,
                                             indexGroup,
                                             op->EnergyMeters(Meter).DYRptNumChr,
                                             op->EnergyMeters(Meter).Name,
                                             op->EnergyMeters(Meter).Units,
                                             false,
                                             false);
                }
            } else if (ReportFreq == ReportingFrequency::Monthly) {
                if (op->EnergyMeters(Meter).RptMN) {
                    op->EnergyMeters(Meter).RptMN = true;
                    op->TrackingMonthlyVariables = true;
                } else {
                    op->EnergyMeters(Meter).RptMN = true;
                    op->TrackingMonthlyVariables = true;
                    WriteMeterDictionaryItem(state,
                                             ReportFreq,
                                             StoreType::Summed,
                                             op->EnergyMeters(Meter).MNRptNum,
                                             indexGroupKey,
                                             indexGroup,
                                             op->EnergyMeters(Meter).MNRptNumChr,
                                             op->EnergyMeters(Meter).Name,
                                             op->EnergyMeters(Meter).Units,
                                             false,
                                             false);
                }
            } else if (ReportFreq == ReportingFrequency::Yearly) {
                if (op->EnergyMeters(Meter).RptYR) {
                    op->EnergyMeters(Meter).RptYR = true;
                    op->TrackingYearlyVariables = true;
                } else {
                    op->EnergyMeters(Meter).RptYR = true;
                    op->TrackingMonthlyVariables = true;
                    WriteMeterDictionaryItem(state,
                                             ReportFreq,
                                             StoreType::Summed,
                                             op->EnergyMeters(Meter).YRRptNum,
                                             indexGroupKey,
                                             indexGroup,
                                             op->EnergyMeters(Meter).YRRptNumChr,
                                             op->EnergyMeters(Meter).Name,
                                             op->EnergyMeters(Meter).Units,
                                             false,
                                             false);
                }
            } else if (ReportFreq == ReportingFrequency::Simulation) {
                if (op->EnergyMeters(Meter).RptSM) {
                    op->EnergyMeters(Meter).RptSM = true;
                    op->TrackingRunPeriodVariables = true;
                } else {
                    op->EnergyMeters(Meter).RptSM = true;
                    op->TrackingRunPeriodVariables = true;
                    WriteMeterDictionaryItem(state,
                                             ReportFreq,
                                             StoreType::Summed,
                                             op->EnergyMeters(Meter).SMRptNum,
                                             indexGroupKey,
                                             indexGroup,
                                             op->EnergyMeters(Meter).SMRptNumChr,
                                             op->EnergyMeters(Meter).Name,
                                             op->EnergyMeters(Meter).Units,
                                             false,
                                             false);
                }
            } else {
            }
        }
    }
}

void ProduceRDDMDD(EnergyPlusData &state)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   March 2009
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // provide a single call for writing out the Report Data Dictionary and Meter Data Dictionary.

    // Using/Aliasing
    using namespace OutputProcessor;
    using General::ScanForReports;
    using SortAndStringUtilities::SetupAndSort;

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    std::string VarOption1;
    std::string VarOption2;
    bool DoReport;
    int Item;
    bool SortByName;
    int ItemPtr;

    struct VariableTypes
    {
        // Members
        int RealIntegerType; // Real= 1, Integer=2
        int VarPtr;          // pointer to real/integer VariableTypes structures
        int TimeStepType;
        int StoreType;
        std::string UnitsString;

        // Default Constructor
        VariableTypes() : RealIntegerType(0), VarPtr(0), TimeStepType(0), StoreType(0)
        {
        }
    };

    auto &op = state.dataOutputProcessor;

    //  See if Report Variables should be turned on
    SortByName = false;
    ScanForReports(state, "VariableDictionary", DoReport, _, VarOption1, VarOption2);
    //  IF (.not. DoReport) RETURN

    if (DoReport) {
        op->ProduceReportVDD = ReportVDD::Yes;
        if (VarOption1 == std::string("IDF")) {
            op->ProduceReportVDD = ReportVDD::IDF;
        }
        if (!VarOption2.empty()) {
            if (Util::SameString(VarOption2, "Name") || Util::SameString(VarOption2, "AscendingName")) {
                SortByName = true;
            }
        }
    }

    state.files.rdd.ensure_open(state, "ProduceRDDMDD", state.files.outputControl.rdd);
    state.files.mdd.ensure_open(state, "ProduceRDDMDD", state.files.outputControl.mdd);
    if (op->ProduceReportVDD == ReportVDD::Yes) {
        print(state.files.rdd, "Program Version,{},{}{}", state.dataStrGlobals->VerStringVar, state.dataStrGlobals->IDDVerString, '\n');
        print(state.files.rdd, "Var Type (reported time step),Var Report Type,Variable Name [Units]{}", '\n');

        print(state.files.mdd, "Program Version,{},{}{}", state.dataStrGlobals->VerStringVar, state.dataStrGlobals->IDDVerString, '\n');
        print(state.files.mdd, "Var Type (reported time step),Var Report Type,Variable Name [Units]{}", '\n');
    } else if (op->ProduceReportVDD == ReportVDD::IDF) {
        print(state.files.rdd, "! Program Version,{},{}{}", state.dataStrGlobals->VerStringVar, state.dataStrGlobals->IDDVerString, '\n');
        print(state.files.rdd, "! Output:Variable Objects (applicable to this run){}", '\n');

        print(state.files.mdd, "! Program Version,{},{}{}", state.dataStrGlobals->VerStringVar, state.dataStrGlobals->IDDVerString, '\n');
        print(state.files.mdd, "! Output:Meter Objects (applicable to this run){}", '\n');
    }

    Array1D_string VariableNames(op->NumVariablesForOutput);
    for (int i = 1; i <= op->NumVariablesForOutput; ++i)
        VariableNames(i) = op->DDVariableTypes(i).VarNameOnly;
    Array1D_int iVariableNames(op->NumVariablesForOutput);

    if (SortByName) {
        SetupAndSort(VariableNames, iVariableNames);
    } else {
        for (Item = 1; Item <= op->NumVariablesForOutput; ++Item) {
            iVariableNames(Item) = Item;
        }
    }

    for (Item = 1; Item <= op->NumVariablesForOutput; ++Item) {
        if (op->ProduceReportVDD == ReportVDD::Yes) {
            ItemPtr = iVariableNames(Item);
            if (!op->DDVariableTypes(ItemPtr).ReportedOnDDFile) {
                print(state.files.rdd,
                      "{},{},{}{}{}",
                      StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType),
                      standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType),
                      VariableNames(Item),
                      unitStringFromDDitem(state, ItemPtr),
                      '\n');
                state.dataResultsFramework->resultsFramework->RDD.push_back(StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType) + "," +
                                                                            standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType) + "," +
                                                                            VariableNames(Item) + unitStringFromDDitem(state, ItemPtr));
                op->DDVariableTypes(ItemPtr).ReportedOnDDFile = true;
                while (op->DDVariableTypes(ItemPtr).Next != 0) {
                    if (SortByName) {
                        ++ItemPtr;
                    } else {
                        ItemPtr = op->DDVariableTypes(ItemPtr).Next;
                    }
                    print(state.files.rdd,
                          "{},{},{}{}{}",
                          StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType),
                          standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType),
                          VariableNames(Item),
                          unitStringFromDDitem(state, ItemPtr),
                          '\n');
                    state.dataResultsFramework->resultsFramework->RDD.push_back(StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType) +
                                                                                "," +
                                                                                standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType) +
                                                                                "," + VariableNames(Item) + unitStringFromDDitem(state, ItemPtr));
                    op->DDVariableTypes(ItemPtr).ReportedOnDDFile = true;
                }
            }
        } else if (op->ProduceReportVDD == ReportVDD::IDF) {
            ItemPtr = iVariableNames(Item);
            if (!op->DDVariableTypes(ItemPtr).ReportedOnDDFile) {
                print(state.files.rdd,
                      "Output:Variable,*,{},hourly; !- {} {}{}{}",
                      VariableNames(Item),
                      StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType),
                      standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType),
                      unitStringFromDDitem(state, ItemPtr),
                      '\n');
                state.dataResultsFramework->resultsFramework->RDD.push_back(StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType) + "," +
                                                                            standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType) + "," +
                                                                            VariableNames(Item) + unitStringFromDDitem(state, ItemPtr));
                op->DDVariableTypes(ItemPtr).ReportedOnDDFile = true;
                while (op->DDVariableTypes(ItemPtr).Next != 0) {
                    if (SortByName) {
                        ++ItemPtr;
                    } else {
                        ItemPtr = op->DDVariableTypes(ItemPtr).Next;
                    }
                    print(state.files.rdd,
                          "Output:Variable,*,{},hourly; !- {} {}{}{}",
                          VariableNames(Item),
                          StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType),
                          standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType),
                          unitStringFromDDitem(state, ItemPtr),
                          '\n');
                    state.dataResultsFramework->resultsFramework->RDD.push_back(StandardTimeStepTypeKey(op->DDVariableTypes(ItemPtr).timeStepType) +
                                                                                "," +
                                                                                standardVariableTypeKey(op->DDVariableTypes(ItemPtr).storeType) +
                                                                                "," + VariableNames(Item) + unitStringFromDDitem(state, ItemPtr));
                    op->DDVariableTypes(ItemPtr).ReportedOnDDFile = true;
                }
            }
        }
    }
    state.files.rdd.close();

    //  Now EnergyMeter variables
    VariableNames.allocate(op->NumEnergyMeters);
    iVariableNames.allocate(op->NumEnergyMeters);
    if (SortByName) {
        for (Item = 1; Item <= op->NumEnergyMeters; ++Item) {
            VariableNames(Item) = op->EnergyMeters(Item).Name;
        }
        SetupAndSort(VariableNames, iVariableNames);
    } else {
        for (Item = 1; Item <= op->NumEnergyMeters; ++Item) {
            VariableNames(Item) = op->EnergyMeters(Item).Name;
            iVariableNames(Item) = Item;
        }
    }

    for (Item = 1; Item <= op->NumEnergyMeters; ++Item) {
        ItemPtr = iVariableNames(Item);
        if (op->ProduceReportVDD == ReportVDD::Yes) {
            print(state.files.mdd,
                  "Zone,Meter,{}{}{}",
                  op->EnergyMeters(ItemPtr).Name,
                  unitEnumToStringBrackets(op->EnergyMeters(ItemPtr).Units),
                  '\n');
            state.dataResultsFramework->resultsFramework->MDD.push_back("Zone,Meter," + op->EnergyMeters(ItemPtr).Name +
                                                                        unitEnumToStringBrackets(op->EnergyMeters(ItemPtr).Units));
        } else if (op->ProduceReportVDD == ReportVDD::IDF) {
            print(state.files.mdd,
                  "Output:Meter,{},hourly; !-{}{}",
                  op->EnergyMeters(ItemPtr).Name,
                  unitEnumToStringBrackets(op->EnergyMeters(ItemPtr).Units),
                  '\n');
            state.dataResultsFramework->resultsFramework->MDD.push_back("Output:Meter," + op->EnergyMeters(ItemPtr).Name +
                                                                        unitEnumToStringBrackets(op->EnergyMeters(ItemPtr).Units));
            print(state.files.mdd,
                  "Output:Meter:Cumulative,{},hourly; !-{}{}",
                  op->EnergyMeters(ItemPtr).Name,
                  unitEnumToStringBrackets(op->EnergyMeters(ItemPtr).Units),
                  '\n');
            state.dataResultsFramework->resultsFramework->MDD.push_back("Output:Meter:Cumulative," + op->EnergyMeters(ItemPtr).Name +
                                                                        unitEnumToStringBrackets(op->EnergyMeters(ItemPtr).Units));
        }
    }
    state.files.mdd.close();
}

void AddToOutputVariableList(EnergyPlusData &state,
                             std::string_view const VarName, // Variable Name
                             OutputProcessor::TimeStepType const TimeStepType,
                             OutputProcessor::StoreType const StateType,
                             OutputProcessor::VariableType const VariableType,
                             OutputProcessor::Unit const unitsForVar,
                             std::string_view const customUnitName // the custom name for the units from EMS definition of units
)
{

    // SUBROUTINE INFORMATION:
    //       AUTHOR         Linda Lawrie
    //       DATE WRITTEN   August 2010
    //       MODIFIED       na
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS SUBROUTINE:
    // This routine maintains a unique list of Output Variables for the
    // Variable Dictionary output.

    // METHODOLOGY EMPLOYED:
    // na

    // REFERENCES:
    // na

    // Using/Aliasing
    using namespace OutputProcessor;

    // Locals
    // SUBROUTINE ARGUMENT DEFINITIONS:

    // SUBROUTINE PARAMETER DEFINITIONS:
    // na

    // INTERFACE BLOCK SPECIFICATIONS:
    // na

    // DERIVED TYPE DEFINITIONS:
    // na

    // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
    auto &op = state.dataOutputProcessor;

    int dup = 0; // for duplicate variable name
    if (op->NumVariablesForOutput > 0) {
        dup = Util::FindItemInList(VarName, op->DDVariableTypes, &VariableTypeForDDOutput::VarNameOnly, op->NumVariablesForOutput);
    } else {
        op->DDVariableTypes.allocate(LVarAllocInc);
        op->MaxVariablesForOutput = LVarAllocInc;
    }
    if (dup == 0) {
        ++op->NumVariablesForOutput;
        if (op->NumVariablesForOutput > op->MaxVariablesForOutput) {
            op->DDVariableTypes.redimension(op->MaxVariablesForOutput += LVarAllocInc);
        }
        op->DDVariableTypes(op->NumVariablesForOutput).timeStepType = TimeStepType;
        op->DDVariableTypes(op->NumVariablesForOutput).storeType = StateType;
        op->DDVariableTypes(op->NumVariablesForOutput).variableType = VariableType;
        op->DDVariableTypes(op->NumVariablesForOutput).VarNameOnly = VarName;
        op->DDVariableTypes(op->NumVariablesForOutput).units = unitsForVar;
        if (!customUnitName.empty() && unitsForVar == OutputProcessor::Unit::customEMS) {
            op->DDVariableTypes(op->NumVariablesForOutput).unitNameCustomEMS = customUnitName;
        }
    } else if (unitsForVar != op->DDVariableTypes(dup).units) { // not the same as first units
        int dup2 = 0;                                           // for duplicate variable name
        while (op->DDVariableTypes(dup).Next != 0) {
            if (unitsForVar != op->DDVariableTypes(op->DDVariableTypes(dup).Next).units) {
                dup = op->DDVariableTypes(dup).Next;
                continue;
            }
            dup2 = op->DDVariableTypes(dup).Next;
            break;
        }
        if (dup2 == 0) {
            ++op->NumVariablesForOutput;
            if (op->NumVariablesForOutput > op->MaxVariablesForOutput) {
                op->DDVariableTypes.redimension(op->MaxVariablesForOutput += LVarAllocInc);
            }
            op->DDVariableTypes(op->NumVariablesForOutput).timeStepType = TimeStepType;
            op->DDVariableTypes(op->NumVariablesForOutput).storeType = StateType;
            op->DDVariableTypes(op->NumVariablesForOutput).variableType = VariableType;
            op->DDVariableTypes(op->NumVariablesForOutput).VarNameOnly = VarName;
            op->DDVariableTypes(op->NumVariablesForOutput).units = unitsForVar;
            if (!customUnitName.empty() && unitsForVar == OutputProcessor::Unit::customEMS) {
                op->DDVariableTypes(op->NumVariablesForOutput).unitNameCustomEMS = customUnitName;
            }
            op->DDVariableTypes(dup).Next = op->NumVariablesForOutput;
        }
    }
}

int initErrorFile(EnergyPlusData &state)
{
    state.files.err_stream = std::make_unique<std::ofstream>(state.files.outputErrFilePath);
    if (state.files.err_stream->bad()) {
        DisplayString(state, "ERROR: Could not open file " + state.files.outputErrFilePath.string() + " for output (write).");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

} // namespace EnergyPlus
