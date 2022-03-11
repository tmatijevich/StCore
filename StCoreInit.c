/*******************************************************************************
 * File: StCoreInit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "StCoreMain.h"

/****************************
 Global variable declarationss
****************************/
/* Cyclic communication data */
unsigned char *pCyclicControlData, *pCyclicStatusData;

/* Control interface configuration */
SuperTrakControlIfConfig_t coreControlInterfaceConfig;

/* StCore configuration */
unsigned char coreError = true, coreInitialTargetCount, userPalletCount, userNetworkIOCount;

/* Read layout and targets, then size control interface */
long StCoreInit(char *storagePath, char *simIPAddress, char *ethernetInterfaces, unsigned char palletCount, unsigned char networkIOCount) {
	
	/***********************
	 Declare local variables 
	***********************/
	static unsigned char firstCall = true;
	long statusID, i, j, k;
	FormatStringArgumentsType args;
	unsigned short sectionCount, networkOrder[stCORE_SECTION_MAX], headSection, flowDirection, flowOrder[stCORE_SECTION_MAX];
	unsigned short dataUInt16[255];
	unsigned long size;
	
	/********************
	 Initialize SuperTrak
	********************/
	/* SuperTrakCyclic1() has cycle-time violation is SuperTrakInit() is not called */
	/* Only call SuperTrakInit() if it's the first call to StCoreInit(), if multiple calls are made it will be caught in the next step */
	if(firstCall) {
		/* SuperTrakInit() does not appear to return status information, if this fails the service channel will error */
		SuperTrakInit(storagePath, simIPAddress, ethernetInterfaces);
	}
	
	/**************
	 Create logbook
	**************/
	statusID = CreateCustomLogbook(stCORE_LOGBOOK_NAME, 200000);
	if(statusID != 0 && statusID != arEVENTLOG_ERR_LOGBOOK_EXISTS) {
		args.i[0] = statusID;
	
		/* Safely copy logbook name to string argument */
		strncpy(args.s[0], stCORE_LOGBOOK_NAME, IECSTRING_FORMATARGS_LEN);
		args.s[0][IECSTRING_FORMATARGS_LEN] = '\0';
		
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LOGBOOK), "ArEventLog error %i. Possible naming conflict with %s or insufficient user partition size", &args);
		return stCORE_ERROR_LOGBOOK;
	}
	
	/***********
	 Verify call
	***********/
	if(!firstCall) {
		StCoreLogMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INIT), "Call StCoreInit() once during _INIT");
		return stCORE_ERROR_INIT;
	}
	firstCall = false;
	
	/********************
	 Verify system layout
	********************/
	/* Read section count */
	statusID = SuperTrakServChanRead(0, stPAR_SECTION_COUNT, 0, 1, (unsigned long)&sectionCount, sizeof(sectionCount));
	
	/* Verify section count */
	if(sectionCount < 1 || stCORE_SECTION_MAX < sectionCount) {
		args.i[0] = stPAR_SECTION_COUNT;
		args.i[1] = sectionCount;
		args.i[2] = stCORE_SECTION_MAX;
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_SYSTEMLAYOUT), "Section count (Par %i):%i exceeds limits [1, %i]", &args);
		return stCORE_ERROR_SYSTEMLAYOUT;
	}
	
	/* Read section addresses */
	statusID = SuperTrakServChanRead(0, stPAR_SECTION_ADDRESS, 0, sectionCount, (unsigned long)&networkOrder, sizeof(networkOrder));
	
	/* Read head section */
	statusID = SuperTrakServChanRead(0, stPAR_LOGICAL_HEAD_SECTION, 0, 1, (unsigned long)&headSection, sizeof(headSection));
	
	/* Verify head section */
	if(headSection != 1) {
		args.i[0] = stPAR_LOGICAL_HEAD_SECTION;
		args.i[1] = headSection;
		StCoreLogMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_SYSTEMLAYOUT), "Logical head section (Par %i):%i is not equal to 1");
		return stCORE_ERROR_SYSTEMLAYOUT;
	}
	
	/* Read flow direction */
	statusID = SuperTrakServChanRead(0, stPAR_FLOW_DIRECTION, 0, 1, (unsigned long)&flowDirection, sizeof(flowDirection));
	
	/* Verify flow order */
	memset(&flowOrder, 0, sizeof(flowOrder));
	for(i = 0; i < sectionCount; i++) { /* Find the head section in the network order */
		if(networkOrder[i] == headSection) {
			flowOrder[0] = networkOrder[i];
			if(flowDirection == stDIRECTION_RIGHT) {
				if(i == sectionCount - 1) i = 0;
				else i += 1;
			}
			else {
				if(i == 0) i = sectionCount - 1;
				else i -= 1;
			}
			break;
		}
	}
	if(i >= sectionCount)
		return stCORE_ERROR_SYSTEMLAYOUT;
	
	j = 1; k = 0;
	while(networkOrder[i] != headSection) { /* Build flow order following network order in flow direction */
		if(++k > sectionCount)
			return stCORE_ERROR_SYSTEMLAYOUT;
		flowOrder[j++] = networkOrder[i];
		if(flowDirection == stDIRECTION_RIGHT) {
			if(i == sectionCount - 1) i = 0;
			else i += 1;
		}
		else {
			if(i == 0) i = sectionCount - 1;
			else i -= 1;
		}
	}
	
	for(i = 1; i < sectionCount; i++) { 
		if(flowOrder[i] != flowOrder[i - 1] + 1) { /* Check flow order is in ascending order (system layout easy setup) */
			args.i[0] = flowOrder[i];
			args.i[1] = flowOrder[i - 1];
			if(flowDirection == stDIRECTION_LEFT) strcpy(args.s[0], "left");
			else strcpy(args.s[0], "right");
			StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_SYSTEMLAYOUT), "Section %i downstream of section %i in pallet flow direction:%s. Use system layout's easy setup", &args);
			return stCORE_ERROR_SYSTEMLAYOUT;
		}
	}
	
	args.i[0] = sectionCount;
	/* SuperTrak system layout verified */
	StCoreFormatMessage(USERLOG_SEVERITY_SUCCESS, 1201, "%i sections read from TrakMaster system layout", &args);
	
	/************
	 Read targets
	************/
	/* Read section of 255 targets, indexed 0..254 */
	statusID = SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 1, COUNT_OF(dataUInt16), (unsigned long)&dataUInt16, sizeof(dataUInt16));
	
	/* Find the last target to be defined (non-zero section number) */
	for(i = 0, coreInitialTargetCount = 0; i < COUNT_OF(dataUInt16); i++) {
		if(dataUInt16[i])
			coreInitialTargetCount = i + 1;
	}
	args.i[0] = coreInitialTargetCount;
	StCoreFormatMessage(USERLOG_SEVERITY_SUCCESS, 1300, "%i targets defined in TrakMaster global parameters", &args);

//	/*******************************
//	 Configure PLC control interface
//	*******************************/
//	configUserPalletCount = palletCount;
//	configUserNetworkIOCount = networkIOCount;
//	memset(&configPLCInterface, 0, sizeof(configPLCInterface));
//	
//	/* Options */
//	/* Enable interface (0) and use system control & status */
//	configPLCInterface.options = (1 << stCONTROL_IF_ENABLED) + (1 << stCONTROL_IF_SYSTEM_ENABLED);
//	/* Assign to interface 0 */
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_OPTIONS, 0, 1, (unsigned long)&configPLCInterface.options, sizeof(configPLCInterface.options));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_OPTIONS);
//	
//	/* Section start */
//	configPLCInterface.sectionStartIndex = 0;
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_START, 0, 1, (unsigned long)&configPLCInterface.sectionStartIndex, sizeof(configPLCInterface.sectionStartIndex));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_SECTION_START);
//	
//	/* Section count */
//	configPLCInterface.sectionCount = layout.sectionCount;
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_COUNT, 0, 1, (unsigned long)&configPLCInterface.sectionCount, sizeof(configPLCInterface.sectionCount));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_SECTION_COUNT);
//	
//	/* Target start */
//	configPLCInterface.targetStartIndex = 0;
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_START, 0, 1, (unsigned long)&configPLCInterface.targetStartIndex, sizeof(configPLCInterface.targetStartIndex));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_TARGET_START);
//	
//	/* Target count */
//	configPLCInterface.targetCount = ROUND_UP_MULTIPLE(configInitialTargetCount + 1, 4);
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_COUNT, 0, 1, (unsigned long)&configPLCInterface.targetCount, sizeof(configPLCInterface.targetCount));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_TARGET_COUNT);
//	
//	/* Command count */
//	configPLCInterface.commandCount = ROUND_UP_MULTIPLE(palletCount + 1, 8);
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_COMMAND_COUNT, 0, 1, (unsigned long)&configPLCInterface.commandCount, sizeof(configPLCInterface.commandCount));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_COMMAND_COUNT);
//	
//	/* Network IO start */
//	configPLCInterface.networkIoStartIndex = 0;
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_START, 0, 1, (unsigned long)&configPLCInterface.networkIoStartIndex, sizeof(configPLCInterface.networkIoStartIndex));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_NETWORK_IO_START);
//	
//	/* Network IO count */
//	configPLCInterface.networkIoCount = ROUND_UP_MULTIPLE(networkIOCount + 1, 8);
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_COUNT, 0, 1, (unsigned long)&configPLCInterface.networkIoCount, sizeof(configPLCInterface.networkIoCount));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_NETWORK_IO_COUNT);
//	
//	/* Revision */
//	configPLCInterface.revision = 0;
//	args.i[0] = SuperTrakServChanWrite(0, stPAR_PLC_IF_REVISION, 0, 1, (unsigned long)&configPLCInterface.revision, sizeof(configPLCInterface.revision));
//	if(args.i[0] != scERR_SUCCESS) 
//		return StCoreLogServChan(args.i[0], stPAR_PLC_IF_REVISION);
//		
//	/***************************************
//	 Get PLC control interface configuration
//	***************************************/
//	SuperTrakGetControlIfConfig(0, &configPLCInterface);
//	
//	/************************
//	 Get enable signal source
//	************************/
//	if((args.i[0] = SuperTrakServChanRead(0, 1107, 0, 1, (unsigned long)&configEnableSource, sizeof(configEnableSource))) != scERR_SUCCESS)
//		return StCoreLogServChan(args.i[0], 1107);
//	
//	/***************
//	 Allocate memory
//	***************/
//	if((args.i[0] = TMP_alloc(configPLCInterface.controlSize, (void**)&controlData))) {
//		args.i[1] = configPLCInterface.controlSize;
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1300, "TMP_alloc error %i. Unable to allocate %i bytes of memory for cyclic control data", &args, "StCoreLog", 1);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1300);
//	}
//	memset(controlData, 0, configPLCInterface.controlSize);
//	
//	if((args.i[0] = TMP_alloc(configPLCInterface.statusSize, (void**)&statusData))) {
//		args.i[1] = configPLCInterface.statusSize;
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1301, "TMP_alloc error %i. Unable to allocate %i bytes of memory for cyclic status data", &args, "StCoreLog", 1);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1301);
//	}
//	memset(statusData, 0, configPLCInterface.statusSize);
//	
//	if((args.i[0] = TMP_alloc(sizeof(StCoreSectionCommandType*) * configPLCInterface.sectionCount, (void**)&user.section.command))) {
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1310, "TMP_alloc error %i. Unable to allocate %i bytes of memory for user section command references", &args, "StCoreLog", 1);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1310);
//	}
//	memset(user.section.command, 0, sizeof(StCoreSectionCommandType*) * configPLCInterface.sectionCount);
//	
//	/* Memory for command buffers */
//	size = sizeof(SuperTrakCommand_t) * stCORE_COMMANDBUFFER_SIZE * configPLCInterface.commandCount; /* Command data * commands per pallet * pallets */
//	if((args.i[0] = TMP_alloc(size, (void**)&pCommandBuffer))) {
//		args.i[1] = (long)size;
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1320, "TMP_alloc() error %i. Unable to allocate %i bytes for command buffers", &args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, stCORE_LOGBOOK_FACILITY, 1320);
//	}
//	memset(pCommandBuffer, 0, size);
//	
//	/* Memory for buffer write indices */
//	size = sizeof(unsigned char) * configPLCInterface.commandCount;
//	args.i[1] = (long)size;
//	if((args.i[0] = TMP_alloc(size, (void**)&pCommandWriteIndex))) {
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1320, "TMP_alloc() error %i. Unable to allocate %i bytes for command buffer write indices", &args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, stCORE_LOGBOOK_FACILITY, 1320);
//	}
//	memset(pCommandWriteIndex, 0, size);
//	
//	/* Memory for buffer read indices */
//	if((args.i[0] = TMP_alloc(size, (void**)&pCommandReadIndex))) {
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1320, "TMP_alloc() error %i. Unable to allocate %i bytes for command buffer read indices", &args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, stCORE_LOGBOOK_FACILITY, 1320);
//	}
//	memset(pCommandReadIndex, 0, size);
//	
//	/* Memory for buffer full flags */
//	if((args.i[0] = TMP_alloc(size, (void**)&pCommandBufferFull))) {
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1320, "TMP_alloc() error %i. Unable to allocate %i bytes for command buffer full flags", &args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, stCORE_LOGBOOK_FACILITY, 1320);
//	}
//	memset(pCommandBufferFull, 0, size);
//	
//	configError = false;
	return 0;
	
} /* Function definition */
