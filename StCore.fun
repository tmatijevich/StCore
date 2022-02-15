(*******************************************************************************
 * File: StCore.fun
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************)
(*SuperTrak controller program*)

FUNCTION StCoreInit : USINT (*Read layout and targets. Configure PLC control interface*)
	VAR_INPUT
		storagePath : STRING[127]; (*Specifies the filesystem location for conveyor configuration data*)
		simIPAddress : STRING[15]; (*Simulation IP address as specified in CPU Configuration*)
		ethernetInterfaces : STRING[63]; (*Comma-delimited list of Ethernet interfaces. Example: 'IF3,IF4'*)
		maxPallet : USINT; (*Maximum number of pallets intended for use on system*)
		maxNetworkIO : USINT; (*Maximum number of network I/O channels intended for use on system*)
	END_VAR
END_FUNCTION

FUNCTION StCoreCyclic : USINT (*Process cyclic control and status data using PLC communication protocal*)
END_FUNCTION

FUNCTION StCoreExit : USINT (*Free allocated memory*)
END_FUNCTION
(*SuperTrak statuses*)

FUNCTION StCoreSystemStatus : USINT (*Get system level status information*)
END_FUNCTION

FUNCTION StCoreSectionStatus : USINT (*Get section level status information*)
END_FUNCTION

FUNCTION StCoreTargetStatus : USINT (*Get target status information*)
END_FUNCTION

FUNCTION StCorePalletStatus : USINT (*Get pallet status information*)
END_FUNCTION
(*SuperTrak power and error handling functions*)

FUNCTION StCoreSystemControl : USINT (*Enable control of the SuperTrak system. Reset system, section, and core errors*)
	VAR_INPUT
		enable : BOOL; (*Enable/disable SuperTrak system. The SuperTrak must be configured for system control*)
		errorReset : BOOL;
		verbosityLevel : USINT;
	END_VAR
END_FUNCTION

FUNCTION StCoreSectionControl : USINT (*Enable control on a SuperTrak section*)
	VAR_INPUT
		section : USINT; (*Specify the section to enable or disable*)
		enable : BOOL; (*Enable/disable section. The SuperTrak must be configured for section control*)
	END_VAR
END_FUNCTION
