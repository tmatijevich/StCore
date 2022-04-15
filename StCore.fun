(*******************************************************************************
 * File: StCore.fun
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************)
(*SuperTrak control program*)

FUNCTION StCoreInit : DINT (*Initialize SuperTrak, verify layout, read targets, and size control interface*)
	VAR_INPUT
		StoragePath : STRING[127]; (*Filesystem location for SuperTrak configuration data*)
		SimIPAddress : STRING[15]; (*CPU configuration's simulation IP address*)
		EthernetInterfaceList : STRING[63]; (*Comma-separated list of ethernet interfaces ('IF3,IF4')*)
		PalletCount : USINT; (*Maximum number of pallets on system*)
		NetworkIOCount : USINT; (*Maximum number of network I/O channels on system*)
	END_VAR
END_FUNCTION

FUNCTION StCoreCyclic : DINT (*Process SuperTrak control interface*)
END_FUNCTION

FUNCTION StCoreExit : DINT (*Free internal memory*)
END_FUNCTION
(*******************************************************************************
SuperTrak control interface
*******************************************************************************)
(*System and sections*)

FUNCTION_BLOCK StCoreSystem (*SuperTrak system interface function*)
	VAR_INPUT
		Enable : BOOL; (*Enable function execution*)
		ErrorReset : BOOL; (*Reset function error*)
		EnableAllSections : BOOL; (*(IF) Enable all sections*)
		AcknowledgeFaults : BOOL; (*(IF) Clear SuperTrak system faults or warnings*)
	END_VAR
	VAR_OUTPUT
		Valid : BOOL; (*Successful interface function execution*)
		Error : BOOL; (*An error has occurred with the function*)
		StatusID : DINT; (*Function error identifier. See logger for more details*)
		PalletsStopped : BOOL; (*(IF) All pallets are currently not moving*)
		WarningPresent : BOOL; (*(IF) A system warning message has been recorded*)
		FaultPresent : BOOL; (*(IF) A system fault message has been recorded*)
		PalletCount : USINT; (*(IF) Total number of pallets on the system*)
		Info : StCoreSystemInfoType; (*Extended system information*)
	END_VAR
	VAR
		Internal : StCoreSystemInternalType; (*Local internal data*)
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK StCoreSection (*SuperTrak section interface function*)
	VAR_INPUT
		Enable : BOOL; (*Enable function execution*)
		Section : USINT; (*Select section*)
		ErrorReset : BOOL; (*Reset function error*)
		EnableSection : BOOL; (*(IF) Enable section control*)
		AcknowledgeFaults : BOOL; (*(IF) Clear SuperTrak section faults or warnings*)
	END_VAR
	VAR_OUTPUT
		Valid : BOOL; (*Successful interface function execution*)
		Error : BOOL; (*An error has occurred with the function*)
		StatusID : DINT; (*Function error identifier. See logger for more details*)
		Enabled : BOOL; (*(IF) Section control is enabled*)
		UnrecognizedPallets : BOOL; (*(IF) Section has one or more pallets with undetermined position. The section will automatically jog these pallets to locate them*)
		MotorPowerOn : BOOL; (*(IF) Motor supply voltage is within range*)
		PalletsRecovering : BOOL; (*(IF) Pallets must return to their last controlled position or pallets are moving to the load target*)
		LocatingPallets : BOOL; (*(IF) Section is automatically jogging pallets to determine their location*)
		DisabledExternally : BOOL; (*(IF) Section is disabled due to an external condition*)
		WarningPresent : BOOL; (*(IF) A section warning message has been recorded*)
		FaultPresent : BOOL; (*(IF) A section fault message has been recorded*)
		Info : StCoreSectionInfoType; (*Extended section information*)
	END_VAR
	VAR
		Internal : StCoreSectionInternalType; (*Local internal data*)
	END_VAR
END_FUNCTION_BLOCK
(*Release commands*)

FUNCTION StCoreReleaseToTarget : DINT (*Release pallet to target*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		Pallet : USINT; (*Pallet ID*)
		Direction : UINT; (*Direction of motion (stDIRECTION_RIGHT or stDIRECTION_LEFT)*)
		DestinationTarget : USINT; (*Destination target*)
	END_VAR
END_FUNCTION

FUNCTION StCoreReleaseToOffset : DINT (*Release pallet to target + offset*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		Pallet : USINT; (*Pallet ID*)
		Direction : UINT; (*Direction of motion (stDIRECTION_RIGHT or stDIRECTION_LEFT)*)
		DestinationTarget : USINT; (*Destination target*)
		TargetOffset : LREAL; (*[-500.0, 500.0] mm Absolute offset from destination target*)
	END_VAR
END_FUNCTION

FUNCTION StCoreIncrementOffset : DINT (*Increment pallet offset*)
	VAR_INPUT
		Target : USINT; (*Target with pallet present*)
		Pallet : USINT; (*Pallet ID*)
		IncrementalOffset : LREAL; (*[-500.0, 500.0] mm Relative offset from current destination*)
	END_VAR
END_FUNCTION

FUNCTION StCoreResumeMove : DINT (*Resume pallet movement when at mandatory stop*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		Pallet : USINT; (*Pallet ID*)
	END_VAR
END_FUNCTION
(*Configuration commands*)

FUNCTION StCoreSetPalletID : DINT (*Set ID of pallet at target*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		PalletID : USINT; (*Pallet ID to assign*)
	END_VAR
END_FUNCTION

FUNCTION StCoreSetMotionParameters : DINT (*Set pallet velocity and/or acceleration*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		Pallet : USINT; (*Pallet ID*)
		Velocity : LREAL; (*mm/s Set velocity (0.0 for unchanged)*)
		Acceleration : LREAL; (*mm/s^2 Set acceleration (0.0 for unchanged)*)
	END_VAR
END_FUNCTION

FUNCTION StCoreSetMechanicalParameters : DINT (*Set pallet shelf width and offset*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		Pallet : USINT; (*Pallet ID*)
		ShelfWidth : LREAL; (*mm*)
		CenterOffset : LREAL;
	END_VAR
END_FUNCTION

FUNCTION StCoreSetControlParameters : DINT (*Set pallet control parameters*)
	VAR_INPUT
		Target : USINT; (*Target (with pallet present)*)
		Pallet : USINT; (*Pallet ID*)
		ControlGainSet : USINT; (*[0, 15] Control gain set defined in TrakMaster*)
		MovingFilter : LREAL; (*[0.0, 1.0) Moving control weight filter*)
		StationaryFilter : LREAL; (*[0.0, 1.0) Stationary control weight filter*)
	END_VAR
END_FUNCTION
(*Targets and pallets*)

FUNCTION StCoreTargetStatus : DINT (*Get target status*)
	VAR_INPUT
		Target : USINT; (*Target number*)
	END_VAR
	VAR_IN_OUT
		Status : StCoreTargetStatusType; (*Target status reference*)
	END_VAR
END_FUNCTION

FUNCTION_BLOCK StCoreTarget (*Target core interface*)
	VAR_INPUT
		Enable : BOOL; (*Enable function execution*)
		Target : USINT; (*Select target*)
		ErrorReset : BOOL; (*Reset function error*)
	END_VAR
	VAR_OUTPUT
		Valid : BOOL; (*Successful function execution*)
		Error : BOOL; (*An error has occurred with the function*)
		StatusID : DINT; (*Function error identifier*)
		PalletPresent : BOOL; (*(IF) A pallet has arrived, entered the in-position window, and is not yet released*)
		PalletInPosition : BOOL; (*(IF) A pallet is currently within the in-position window*)
		PalletPreArrival : BOOL; (*(IF) A pallet is expected to arrive at the stop within the configured time*)
		PalletOverTarget : BOOL; (*(IF) A pallet's shelf is over the target (requires configuration to report status)*)
		PalletPositionUncertain : BOOL; (*(IF) A pallet has arrived but is not reporting in-position*)
		PalletID : USINT; (*(IF) ID of the pallet present at the target*)
		Info : StCoreTargetInfoType; (*Extended target information*)
	END_VAR
	VAR
		Internal : StCoreFunctionInternalType; (*Local internal data*)
	END_VAR
END_FUNCTION_BLOCK
