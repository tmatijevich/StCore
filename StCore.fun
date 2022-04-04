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
(*SuperTrak control interface*)

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
		Internal : ARRAY[0..1] OF USINT; (*Internal data*)
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

FUNCTION StCoreReleaseToTarget : DINT (*Command target or pallet to release to target*)
	VAR_INPUT
		Target : USINT; (*Target with pallet present*)
		Pallet : USINT; (*Pallet ID*)
		Direction : UINT; (*Direction of motion stDIRECTION_RIGHT or stDIRECTION_LEFT*)
		DestinationTarget : USINT; (*Destination target*)
	END_VAR
END_FUNCTION

FUNCTION StCoreReleaseToOffset : DINT (*Command target or pallet to release to offset*)
	VAR_INPUT
		Target : USINT; (*Target with pallet present*)
		Pallet : USINT; (*Pallet ID*)
		Direction : UINT; (*Direction of motion stDIRECTION_RIGHT or stDIRECTION_LEFT*)
		DestinationTarget : USINT; (*Destination target*)
		TargetOffset : DINT; (*[-500000,500000] um Relative offset from target position*)
	END_VAR
END_FUNCTION
