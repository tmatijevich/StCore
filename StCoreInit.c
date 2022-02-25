/*******************************************************************************
 * File: StCoreInit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "StCoreMain.h"

SuperTrakControlIfConfig_t configPLCInterface;
unsigned char configPalletCount, configNetworkIOCount, configError = false;

/* Read layout and targets. Configure PLC control interface */
long StCoreInit(char *storagePath, char *simIPAddress, char *ethernetInterfaces, unsigned char palletCount, unsigned char networkIOCount) {
	
	/***********************
	 Declare local variables 
	***********************/
	struct FormatStringArgumentsType args;
	RTInfo_typ fbRTInfo;
	struct SuperTrakSystemLayoutType layout;
	struct SuperTrakPositionInfoType positionInfo;
	unsigned short i, targetCount, dataUInt16[255];
	
	/* Assume configuration error is true until finished */
	configError = true;
	
	/******************** 
	 Initialize SuperTrak 
	********************/
	/* Required to call SuperTrakInit() or else SuperTrakCyclic1() will have cycle time violation */
	if(!SuperTrakInit(storagePath, simIPAddress, ethernetInterfaces)) {
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, 65000, "Check storagePath, simIPAddress, or ethernetInferfaces input parameters to StCoreInit()", &args);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 0, 65000);
	}
	
	/*********************
	 Create StCore logbook 
	*********************/
	args.i[0] = CreateCustomLogbook("StCoreLog", 200000);
	if(args.i[0] != 0 && args.i[0] != arEVENTLOG_ERR_LOGBOOK_EXISTS) { 
		/* Description: StCore library unable to create StCoreLog logbook */
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, 65001, "CreateCustomLogbook() error %i. Check naming conflict with StCoreLog or insufficient user partition", &args);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 0, 65001);
	}
	
	/********************************
	 Verify task class and cycle time
	********************************/
	memset(&fbRTInfo, 0, sizeof(fbRTInfo));
	fbRTInfo.enable = true;
	RTInfo(&fbRTInfo);
	if(fbRTInfo.task_class != 1 || fbRTInfo.cycle_time != 800) {
		args.i[0] = fbRTInfo.task_class;
		args.i[1] = fbRTInfo.cycle_time;
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1000, "SuperTrak initialized in invalid task class %i or invalid cycle time %i us. Use TC#1 with 800 us", &args, "StCoreLog", 1);
		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1000);
	}
	
	/****************** 
	 Read system layout
	******************/
	if((args.i[0] = SuperTrakSystemLayout(&layout, &positionInfo)) != stPOS_ERROR_NONE) {
		/* Description: StCoreInit() is unable to read system layout */
		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1100, "StPos error %i from SuperTrakSystemLayout(). Check storagePath, simIPAddress, or ethernetInferfaces inputs", &args, "StCoreLog", 1);
		StCoreLogPosition(args.i[0], positionInfo);
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
	args.i[0] = SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 1, COUNT_OF(dataUInt16), (unsigned long)&dataUInt16, sizeof(dataUInt16));
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

	/*******************************
	 Configure PLC control interface
	*******************************/
	configPalletCount = palletCount;
	configNetworkIOCount = networkIOCount;
	memset(&configPLCInterface, 0, sizeof(configPLCInterface));
	
	/* Options */
	/* Enable interface (0) and use system control & status */
	configPLCInterface.options = (1 << stCONTROL_IF_ENABLED) + (1 << stCONTROL_IF_SYSTEM_ENABLED);
	/* Assign to interface 0 */
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_OPTIONS, 0, 1, (unsigned long)&configPLCInterface.options, sizeof(configPLCInterface.options));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_OPTIONS);
	
	/* Section start */
	configPLCInterface.sectionStartIndex = 0;
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_START, 0, 1, (unsigned long)&configPLCInterface.sectionStartIndex, sizeof(configPLCInterface.sectionStartIndex));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_SECTION_START);
	
	/* Section count */
	configPLCInterface.sectionCount = layout.sectionCount;
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_COUNT, 0, 1, (unsigned long)&configPLCInterface.sectionCount, sizeof(configPLCInterface.sectionCount));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_SECTION_COUNT);
	
	/* Target start */
	configPLCInterface.targetStartIndex = 0;
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_START, 0, 1, (unsigned long)&configPLCInterface.targetStartIndex, sizeof(configPLCInterface.targetStartIndex));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_TARGET_START);
	
	/* Target count */
	configPLCInterface.targetCount = ROUND_UP_MULTIPLE(targetCount + 1, 4);
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_COUNT, 0, 1, (unsigned long)&configPLCInterface.targetCount, sizeof(configPLCInterface.targetCount));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_TARGET_COUNT);
	
	/* Command count */
	configPLCInterface.commandCount = ROUND_UP_MULTIPLE(palletCount + 1, 8);
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_COMMAND_COUNT, 0, 1, (unsigned long)&configPLCInterface.commandCount, sizeof(configPLCInterface.commandCount));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_COMMAND_COUNT);
	
	/* Network IO start */
	configPLCInterface.networkIoStartIndex = 0;
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_START, 0, 1, (unsigned long)&configPLCInterface.networkIoStartIndex, sizeof(configPLCInterface.networkIoStartIndex));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_NETWORK_IO_START);
	
	/* Network IO count */
	configPLCInterface.networkIoCount = ROUND_UP_MULTIPLE(networkIOCount + 1, 8);
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_COUNT, 0, 1, (unsigned long)&configPLCInterface.networkIoCount, sizeof(configPLCInterface.networkIoCount));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_NETWORK_IO_COUNT);
	
	/* Revision */
	configPLCInterface.revision = 0;
	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_REVISION, 0, 1, (unsigned long)&configPLCInterface.revision, sizeof(configPLCInterface.revision));
	if(args.i[0] != scERR_SUCCESS) 
		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_REVISION);
		
	/* Set offset table */
	configPLCInterface.systemControlOffset = 0;
	configPLCInterface.sectionControlOffset = 4;
	configPLCInterface.targetControlOffset = configPLCInterface.sectionControlOffset + ROUND_UP_MULTIPLE(configPLCInterface.sectionCount, 4);
	configPLCInterface.commandTriggerOffset = configPLCInterface.targetControlOffset + ROUND_UP_MULTIPLE(configPLCInterface.targetCount / 4, 4);
	configPLCInterface.commandDataOffset = configPLCInterface.commandTriggerOffset + ROUND_UP_MULTIPLE(configPLCInterface.commandCount / 8, 4);
	configPLCInterface.networkInputOffset = configPLCInterface.commandDataOffset + configPLCInterface.commandCount * 8;
	
	configError = false;
	return 0;
	
} /* Function definition */
