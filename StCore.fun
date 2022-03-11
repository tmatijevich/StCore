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
