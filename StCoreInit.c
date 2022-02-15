/*******************************************************************************
 * File: StCoreInit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "StCoreMain.h"

struct StCorePLCControlIFConfig_typ plcControlIFConfig;
unsigned char configError = false;

/* Read layout and targets. Configure PLC control interface */
long StCoreInit(char *storagePath, char *simIPAddress, char *ethernetInterfaces, unsigned char palletCount, unsigned char networkIOCount) {
	
	/* Declare local variables */
	struct FormatStringArgumentsType args;
	struct SuperTrakSystemLayoutType layout;
	struct SuperTrakPositionInfoType positionInfo;
	short i;
	unsigned char targetCount = 0;
	unsigned short dataUInt16[255];
	
	/* Create StCore logbook */
	if(CreateCustomLogbook("StCoreLog", 200000) == arEVENTLOG_ERR_INTERNAL) { /* Not enough memory */
		configError = true;
		/* Description: StCore library unable to create StCoreLog logbook */
		args.i[0] = arEVENTLOG_ERR_INTERNAL;
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, 65000, "ArEventLog error %i. Check user partition size", &args);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 0, 65000);
	}
	
	/* Initialize SuperTrak */
	SuperTrakInit(storagePath, simIPAddress, ethernetInterfaces);
	
	/* Read system layout */
	if(args.i[0] = SuperTrakSystemLayout(&layout, &positionInfo) != stPOS_ERROR_NONE) {
		configError = true;
		/* Description: Unable to read system layout */
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 100, "StPos error %i from SuperTrakSystemLayout() call", &args, "StCoreLog", 1);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 100);
	}
	
	/* Read section count */
	args.i[0] = layout.sectionCount;
	CustomFormatMessage(USERLOG_SEVERITY_INFORMATION, 101, "%i sections found in system layout", &args, "StCoreLog", 1);
	
	/* Determine target count */
	args.i[0] = SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 1, sizeof(dataUInt16) / sizeof(dataUInt16[0]), (unsigned long)&dataUInt16, sizeof(dataUInt16));
	if(args.i[0] != scERR_SUCCESS) {
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 102, "Service channel error %i when reading target section numbers", &args, "StCoreLog", 1);
	}
	for(i = 0, targetCount = 0; i < sizeof(dataUInt16) / sizeof(dataUInt16[0]); i++) {
		if(dataUInt16[i])
			targetCount = i + 1;
	}
	args.i[0] = targetCount;
	CustomFormatMessage(USERLOG_SEVERITY_INFORMATION, 103, "%i targets found in global parameters", &args, "StCoreLog", 1);

	return 0;
	
} /* Function definition */
