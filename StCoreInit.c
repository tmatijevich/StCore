/*******************************************************************************
 * File: StCoreInit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "StCoreMain.h"

/****************************
 Global variable declarations
****************************/
/* Cyclic communication data */
unsigned char *pCyclicControlData, *pCyclicStatusData;

/* Control interface configuration */
SuperTrakControlIfConfig_t coreControlInterfaceConfig;

/* StCore configuration */
unsigned char coreError = true, coreInitialTargetCount, userPalletCount, userNetworkIOCount;
long coreStatusID = stCORE_ERROR_INITCALL;

/* Command buffer */
SuperTrakCommand_t *pCoreCommandBuffer;
StCoreBufferControlType *pCoreBufferControl;

/* Initialize SuperTrak, read layout and targets, then size control interface */
long StCoreInit(char *storagePath, char *simIPAddress, char *ethernetInterfaces, unsigned char palletCount, unsigned char networkIOCount) {
	
	/***********************
	 Declare local variables 
	***********************/
	static unsigned char firstCall = true;
	long statusID, i, j, k;
	FormatStringArgumentsType args;
	unsigned short sectionCount, networkOrder[stCORE_SECTION_MAX], headSection, flowDirection, flowOrder[stCORE_SECTION_MAX];
	unsigned short dataUInt16[255];
	unsigned long allocationSize;
	
	/********************
	 Initialize SuperTrak
	********************/
	/* SuperTrakCyclic1() has cycle-time violation if SuperTrakInit() is not called */
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
	
		/* Safely copy logbook name */
		strncpy(args.s[0], stCORE_LOGBOOK_NAME, IECSTRING_FORMATARGS_LEN);
		args.s[0][IECSTRING_FORMATARGS_LEN] = '\0';
		
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LOGBOOK), "ArEventLog error %i. Possible naming conflict with %s or insufficient user partition size", &args);
		coreStatusID = stCORE_ERROR_LOGBOOK;
		return stCORE_ERROR_LOGBOOK;
	}
	
	/***********
	 Verify call
	***********/
	if(!firstCall) {
		StCoreLogMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITCALL), "Call StCoreInit() once during _INIT");
		coreStatusID = stCORE_ERROR_INITCALL;
		return stCORE_ERROR_INITCALL;
	}
	firstCall = false;
	
	/********************
	 Verify system layout
	********************/
	/* Safely copy storage path */
	strncpy(args.s[0], storagePath, IECSTRING_FORMATARGS_LEN);
	args.s[0][IECSTRING_FORMATARGS_LEN] = '\0';
	
	/* Read section count */
	statusID = SuperTrakServChanRead(0, stPAR_SECTION_COUNT, 0, 1, (unsigned long)&sectionCount, sizeof(sectionCount));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_SECTION_COUNT);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Verify section count */
	if(sectionCount < 1 || stCORE_SECTION_MAX < sectionCount) {
		args.i[0] = stPAR_SECTION_COUNT;
		args.i[1] = sectionCount;
		args.i[2] = stCORE_SECTION_MAX;
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LAYOUT), "Section count (Par %i):%i exceeds limits [1, %i]", &args);
		coreStatusID = stCORE_ERROR_LAYOUT;
		return stCORE_ERROR_LAYOUT;
	}
	
	/* Read section addresses */
	statusID = SuperTrakServChanRead(0, stPAR_SECTION_ADDRESS, 0, sectionCount, (unsigned long)&networkOrder, sizeof(networkOrder));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_SECTION_ADDRESS);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Read head section */
	statusID = SuperTrakServChanRead(0, stPAR_LOGICAL_HEAD_SECTION, 0, 1, (unsigned long)&headSection, sizeof(headSection));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_LOGICAL_HEAD_SECTION);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Verify head section */
	if(headSection != 1) {
		args.i[0] = stPAR_LOGICAL_HEAD_SECTION;
		args.i[1] = headSection;
		StCoreLogMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LAYOUT), "Logical head section (Par %i):%i is not equal to 1");
		coreStatusID = stCORE_ERROR_LAYOUT;
		return stCORE_ERROR_LAYOUT;
	}
	
	/* Read flow direction */
	statusID = SuperTrakServChanRead(0, stPAR_FLOW_DIRECTION, 0, 1, (unsigned long)&flowDirection, sizeof(flowDirection));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_FLOW_DIRECTION);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
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
	if(i >= sectionCount) { /* Search incomplete */
		StCoreLogMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LAYOUT), "Unable to match head section with system layout");
		coreStatusID = stCORE_ERROR_LAYOUT;
		return stCORE_ERROR_LAYOUT;
	}
	
	j = 1; k = 0;
	while(networkOrder[i] != headSection) { /* Build flow order following network order in flow direction */
		if(++k > sectionCount) {
			StCoreLogMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LAYOUT), "Unexpected system layout section order");
			coreStatusID = stCORE_ERROR_LAYOUT;
			return stCORE_ERROR_LAYOUT;
		}
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
			StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_LAYOUT), "Section %i downstream of section %i in pallet flow direction:%s. Expecting increasing section numbers", &args);
			coreStatusID = stCORE_ERROR_LAYOUT;
			return stCORE_ERROR_LAYOUT;
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

	/*******************************
	 Configure PLC control interface
	*******************************/
	userPalletCount = palletCount;
	userNetworkIOCount = networkIOCount;
	
	/* Options */
	/* Enable interface (0) and use system control & status */
	coreControlInterfaceConfig.options = (1 << stCONTROL_IF_ENABLED) + (1 << stCONTROL_IF_SYSTEM_ENABLED);
	/* Assign to interface 0 */
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_OPTIONS, 0, 1, (unsigned long)&coreControlInterfaceConfig.options, sizeof(coreControlInterfaceConfig.options));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_OPTIONS);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Section start */
	coreControlInterfaceConfig.sectionStartIndex = 0;
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_START, 0, 1, (unsigned long)&coreControlInterfaceConfig.sectionStartIndex, sizeof(coreControlInterfaceConfig.sectionStartIndex));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_SECTION_START);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Section count */
	coreControlInterfaceConfig.sectionCount = sectionCount;
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_COUNT, 0, 1, (unsigned long)&coreControlInterfaceConfig.sectionCount, sizeof(coreControlInterfaceConfig.sectionCount));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_SECTION_COUNT);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Target start */
	coreControlInterfaceConfig.targetStartIndex = 0;
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_START, 0, 1, (unsigned long)&coreControlInterfaceConfig.targetStartIndex, sizeof(coreControlInterfaceConfig.targetStartIndex));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_TARGET_START);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Target count */
	coreControlInterfaceConfig.targetCount = ROUND_UP_MULTIPLE(coreInitialTargetCount + 1, 4);
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_COUNT, 0, 1, (unsigned long)&coreControlInterfaceConfig.targetCount, sizeof(coreControlInterfaceConfig.targetCount));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_TARGET_COUNT);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Command count */
	coreControlInterfaceConfig.commandCount = ROUND_UP_MULTIPLE(palletCount + 1, 8);
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_COMMAND_COUNT, 0, 1, (unsigned long)&coreControlInterfaceConfig.commandCount, sizeof(coreControlInterfaceConfig.commandCount));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_COMMAND_COUNT);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Network IO start */
	coreControlInterfaceConfig.networkIoStartIndex = 0;
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_START, 0, 1, (unsigned long)&coreControlInterfaceConfig.networkIoStartIndex, sizeof(coreControlInterfaceConfig.networkIoStartIndex));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_NETWORK_IO_START);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Network IO count */
	coreControlInterfaceConfig.networkIoCount = ROUND_UP_MULTIPLE(networkIOCount + 1, 8);
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_COUNT, 0, 1, (unsigned long)&coreControlInterfaceConfig.networkIoCount, sizeof(coreControlInterfaceConfig.networkIoCount));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_NETWORK_IO_COUNT);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/* Revision */
	coreControlInterfaceConfig.revision = 0;
	statusID = SuperTrakServChanWrite(0, stPAR_PLC_IF_REVISION, 0, 1, (unsigned long)&coreControlInterfaceConfig.revision, sizeof(coreControlInterfaceConfig.revision));
	if(statusID != scERR_SUCCESS) {
		StCoreFormatMessage(USERLOG_SEVERITY_CRITICAL, StCoreEventCode(stCORE_ERROR_INITSERV), "Verify storage path \"%s\" to configuration files", &args);
		StCoreLogServChan(statusID, stPAR_PLC_IF_REVISION);
		coreStatusID = stCORE_ERROR_INITSERV;
		return stCORE_ERROR_INITSERV;
	}
	
	/***************************************
	 Get PLC control interface configuration
	***************************************/
	SuperTrakGetControlIfConfig(0, &coreControlInterfaceConfig);
	
	/***************
	 Allocate memory
	***************/
	allocationSize = coreControlInterfaceConfig.controlSize;
	TMP_alloc(allocationSize, (void**)&pCyclicControlData);
//	if((args.i[0] = TMP_alloc(coreControlInterfaceConfig.controlSize, (void**)&pCyclicControlData))) {
//		args.i[1] = configPLCInterface.controlSize;
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1300, "TMP_alloc error %i. Unable to allocate %i bytes of memory for cyclic control data", &args, "StCoreLog", 1);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1300);
//	}
	memset(pCyclicControlData, 0, allocationSize);
	
	allocationSize = coreControlInterfaceConfig.statusSize;
	TMP_alloc(allocationSize, (void**)&pCyclicStatusData);
//	if((args.i[0] = TMP_alloc(coreControlInterfaceConfig.statusSize, (void**)&pCyclicStatusData))) {
//		args.i[1] = configPLCInterface.statusSize;
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1301, "TMP_alloc error %i. Unable to allocate %i bytes of memory for cyclic status data", &args, "StCoreLog", 1);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1301);
//	}
	memset(pCyclicStatusData, 0, allocationSize); 
//	
//	if((args.i[0] = TMP_alloc(sizeof(StCoreSectionCommandType*) * configPLCInterface.sectionCount, (void**)&user.section.command))) {
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1310, "TMP_alloc error %i. Unable to allocate %i bytes of memory for user section command references", &args, "StCoreLog", 1);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 1310);
//	}
//	memset(user.section.command, 0, sizeof(StCoreSectionCommandType*) * configPLCInterface.sectionCount);
//	
	/* Memory for command buffers */
	allocationSize = sizeof(SuperTrakCommand_t) * stCORE_COMMANDBUFFER_SIZE * userPalletCount; /* Command data * commands per pallet * pallets */
	TMP_alloc(allocationSize, (void**)&pCoreCommandBuffer);
//	if((args.i[0] = TMP_alloc(size, (void**)&pCommandBuffer))) {
//		args.i[1] = (long)size;
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1320, "TMP_alloc() error %i. Unable to allocate %i bytes for command buffers", &args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, stCORE_LOGBOOK_FACILITY, 1320);
//	}
	memset(pCoreCommandBuffer, 0, allocationSize);
	
	/* Memory for buffer write indices */
	allocationSize = sizeof(StCoreBufferControlType) * userPalletCount;
	TMP_alloc(allocationSize, (void**)&pCoreBufferControl);
//	args.i[1] = (long)size;
//	if((args.i[0] = TMP_alloc(size, (void**)&pCommandWriteIndex))) {
//		CustomFormatMessage(USERLOG_SEVERITY_CRITICAL, 1320, "TMP_alloc() error %i. Unable to allocate %i bytes for command buffer write indices", &args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
//		return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, stCORE_LOGBOOK_FACILITY, 1320);
//	}
	memset(pCoreBufferControl, 0, allocationSize);
	
//	configError = false;
	return 0;
	
} /* Function definition */
