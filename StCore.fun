(*******************************************************************************
 * File: StCore.fun
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************)
(*SuperTrak controller program*)

FUNCTION StCoreInit : DINT (*Read layout and targets. Configure PLC control interface*)
	VAR_INPUT
		storagePath : STRING[127]; (*Specifies the filesystem location for conveyor configuration data*)
		simIPAddress : STRING[15]; (*Simulation IP address as specified in CPU Configuration*)
		ethernetInterfaces : STRING[63]; (*Comma-delimited list of Ethernet interfaces. Example: 'IF3,IF4'*)
		palletCount : USINT; (*Maximum number of pallets intended for use on system*)
		networkIOCount : USINT; (*Maximum number of network I/O channels intended for use on system*)
	END_VAR
END_FUNCTION

FUNCTION StCoreCyclic : DINT (*Process PLC communication protocol*)
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
