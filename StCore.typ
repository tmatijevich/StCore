(*******************************************************************************
 * File: StCore.typ
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************)

TYPE
	StCoreSystemCommandType : 	STRUCT  (*System control input structure*)
		EnableAllSections : BOOL; (*Enable control of entire SuperTrak system. Configure enable signal source to system control*)
		AcknowledgeFaults : BOOL; (*Reset SuperTrak system faults and warnings*)
	END_STRUCT;
	StCoreSystemStatusType : 	STRUCT  (*System status output structure*)
		PalletsStopped : BOOL;
		WarningPresent : BOOL;
		FaultPresent : BOOL;
		PalletCount : USINT;
	END_STRUCT;
	StCoreSectionCommandType : 	STRUCT  (*Section control input structure*)
		EnableSection : BOOL; (*Enable control of section. Configure enable signal source to section control*)
		ResumePallets : BOOL; (*Resume operation of disabled pallets*)
		AcknowledgeFaults : BOOL; (*Reset section faults and warnings*)
	END_STRUCT;
END_TYPE
