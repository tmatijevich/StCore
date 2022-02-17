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
	
	/***********************
	 Declare local variables 
	***********************/
	struct FormatStringArgumentsType args;
	struct SuperTrakSystemLayoutType layout;
	struct SuperTrakPositionInfoType positionInfo;
	unsigned short i, targetCount, dataUInt16[255];
	
	/* Assume configuration error is true until finished */
	configError = true;
	
	/*********************
	 Create StCore logbook 
	*********************/
	/* NAME_INVALID, SIZE_INVALID, and PERSISTENCE_INVAL should not apply */
	/* LOGBOOK_EXISTS, MODULE_EXISTS could be naming conflict */
	/* INTERNAL could mean insufficient memory */
	if(args.i[0] = CreateCustomLogbook("StCoreLog", 200000)) { /* Returns 0 if LOGBOOK_EXISTS error */
		/* Description: StCore library unable to create StCoreLog logbook */
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, 65000, "ArEventLog error %i. Check naming conflict with StCoreLog or insufficient user partition", &args);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 0, 65000);
	}
	
	/******************** 
	 Initialize SuperTrak 
	********************/
	/* Check null pointers */
	if(storagePath == NULL || simIPAddress == NULL || ethernetInterfaces == NULL) {
		CustomMessage(USERLOG_SEVERITY_CRITICAL, 1000, "Check storagePath, simIPAddress, or ethernetInferfaces input parameters to StCoreInit()", "StCoreLog", 1);
		return -1;
	}
	if(!SuperTrakInit(storagePath, simIPAddress, ethernetInterfaces)) {
		CustomMessage(USERLOG_SEVERITY_CRITICAL, 1001, "Check storagePath, simIPAddress, or ethernetInferfaces input parameters to StCoreInit()", "StCoreLog", 1);
		return -1;
	}
	
	/****************** 
	 Read system layout
	******************/
	if(args.i[0] = SuperTrakSystemLayout(&layout, &positionInfo) != stPOS_ERROR_NONE) {
		/* Description: StCoreInit() is unable to read system layout */
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1100, "StPos error %i from SuperTrakSystemLayout()", &args, "StCoreLog", 1);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1100);
	}
	
	/* Check logical head section */
	if(layout.flowOrder[0] != 1) {
		/* Description: SuperTrak system layout has invalid logical head section */
		args.i[0] = layout.flowOrder[0];
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1101, "Invalid head section %i. Use system layout's easy setup starting with section number 1", &args, "StCoreLog", 1);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1101);
	}
	
	/* Check flow order */
	for(i = 1; i < layout.sectionCount; i++) {
		if(layout.flowOrder[i] != layout.flowOrder[i - 1] + 1) {
			/* Description: SuperTrak system layout section numbering non-ascending in flow direction */
			args.i[0] = layout.flowOrder[i];
			args.i[1] = layout.flowOrder[i - 1];
			CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1102, "Section %i downstream of section %i in pallet flow direction. Use system layout's easy setup", &args, "StCoreLog", 1);
			return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1102);
		}
	}
	
	/* Read section count */
	args.i[0] = layout.sectionCount;
	CustomFormatMessage(USERLOG_SEVERITY_SUCCESS, 1103, "%i sections found in system layout", &args, "StCoreLog", 1);
	
	/**********************
	 Determine target count
	**********************/
	/* Read section of 255 targets, indexed 0..254 */
	args.i[0] = SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 1, sizeof(dataUInt16) / sizeof(dataUInt16[0]), (unsigned long)&dataUInt16, sizeof(dataUInt16));
	if(args.i[0] != scERR_SUCCESS) {
		args.i[1] = stPAR_TARGET_SECTION;
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1200, "Service channel error %i when reading target section number parameter %i", &args, "StCoreLog", 1);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1200);
	}
	
	/* Find the last target to be defined (non-zero section number) */
	for(i = 0, targetCount = 0; i < sizeof(dataUInt16) / sizeof(dataUInt16[0]); i++) {
		if(dataUInt16[i])
			targetCount = i + 1;
	}
	args.i[0] = targetCount;
	CustomFormatMessage(USERLOG_SEVERITY_SUCCESS, 1201, "%i targets found in global parameters", &args, "StCoreLog", 1);

	configError = false;
	return 0;
	
} /* Function definition */
