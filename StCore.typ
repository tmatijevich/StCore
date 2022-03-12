(*******************************************************************************
 * File: StCore.typ
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************)

TYPE
	StCoreSystemInfoType : 	STRUCT 
		Warnings : UDINT;
		Faults : UDINT;
		SectionCount : USINT;
		ExternallyDisabled : BOOL;
		MotorPower : BOOL;
		Enabled : BOOL;
		Disabled : BOOL;
		SectionWarningPresent : BOOL;
		SectionFaultPresent : BOOL;
	END_STRUCT;
END_TYPE
