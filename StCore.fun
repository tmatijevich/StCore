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

FUNCTION_BLOCK StCoreSystem
	VAR_INPUT
		Enable : BOOL;
		EnableAllSections : BOOL;
		AcknowledgeFaults : BOOL;
	END_VAR
	VAR_OUTPUT
		Valid : BOOL;
		Error : BOOL;
		StatusID : DINT;
		PalletsStopped : BOOL;
		WarningPresent : BOOL;
		FaultPresent : BOOL;
		PalletCount : USINT;
		Info : StCoreSystemInfoType;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK StCoreSection
	VAR_INPUT
		Enable : BOOL;
		Section : USINT;
		EnableSection : BOOL;
		AcknowledgeFaults : BOOL;
	END_VAR
	VAR_OUTPUT
		Valid : BOOL;
		Error : BOOL;
		StatusID : DINT;
		Enabled : BOOL;
		UnrecognizedPalletsPresent : BOOL;
		MotorPowerOn : BOOL;
		PalletsRecovering : BOOL;
		LocatingPallets : BOOL;
		DisabledExternally : BOOL;
		WarningPresent : BOOL;
		FaultPresent : BOOL;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION StCoreReleaseToTarget : DINT
	VAR_INPUT
		target : USINT;
		palletID : USINT;
		direction : UINT;
		destinationTarget : USINT;
	END_VAR
END_FUNCTION
