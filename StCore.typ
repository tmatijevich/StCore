(*******************************************************************************
 * File: StCore.typ
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************)

TYPE
	StCoreSystemInfoType : 	STRUCT  (*Extended SuperTrak system information*)
		Warnings : UDINT; (*(Par 1460) Active SuperTrak system warnings*)
		Faults : UDINT; (*(Par 1460) Active SuperTrak system faults*)
		SectionCount : USINT; (*(Par 1080) Section count*)
		Enabled : BOOL; (*(Derived) All sections are enabled*)
		Disabled : BOOL; (*(Derived) All sections are disabled*)
		MotorPower : BOOL; (*(Derived) All sections' motor voltage within range*)
		DisabledExternally : BOOL; (*(Derived) One or more section disabled from external condition*)
		SectionWarningPresent : BOOL; (*(Derived) One or more active SuperTrak section warning*)
		SectionFaultPresent : BOOL; (*(Derived) One or more active SuperTrak section fault*)
		PalletRecoveringCount : USINT; (*(Derived) Total number of pallets reporting recovering status*)
		PalletInitializingCount : USINT; (*(Derived) Total number of palelts reporting initializing (moving to load target) status*)
	END_STRUCT;
END_TYPE
