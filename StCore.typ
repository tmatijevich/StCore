(*******************************************************************************
 * File: StCore.typ
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************)

TYPE
	StCoreSystemCommandType : 	STRUCT  (*System control input structure*)
		Enable : BOOL; (*Enable control of entire SuperTrak system. Configure enable signal source to system control*)
		ErrorReset : BOOL; (*Reset SuperTrak system faults and warnings*)
	END_STRUCT;
	StCoreSectionCommandType : 	STRUCT  (*Section control input structure*)
		Enable : BOOL; (*Enable control of section. Configure enable signal source to section control*)
		ErrorReset : BOOL; (*Reset section faults and warnings*)
	END_STRUCT;
END_TYPE
