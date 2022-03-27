/*******************************************************************************
 * File: StCoreInit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "StCoreMain.h"

/* Prototypes */
void logMemoryManagement(unsigned short result, unsigned long size, char *name);

/****************************
 Global variable declarations
****************************/
/* Cyclic communication data */
unsigned char *pCoreCyclicControl, *pCoreCyclicStatus;

/* Control interface configuration */
SuperTrakControlIfConfig_t coreInterfaceConfig;

/* StCore configuration */
unsigned char coreError = true, coreTargetCount, corePalletCount, coreNetworkIOCount;
long coreStatusID = stCORE_ERROR_INIT;

/* Command buffer */
SuperTrakCommand_t *pCoreCommandBuffer;
coreBufferControlType *pCoreBufferControl;

/*********************
 StCoreInit definition
*********************/
/* Initialize SuperTrak, verify layout, read targets, and size control interface */
long StCoreInit(char *StoragePath, char *SimIPAddress, char *EthernetInterfaceList, unsigned char PalletCount, unsigned char NetworkIOCount) {
	
	/***********************
	 Declare local variables 
	***********************/
	static unsigned char firstCall = true, firstLog = true;
	long status, i, j, k;
	FormatStringArgumentsType args;
	unsigned short sectionCount, networkOrder[CORE_SECTION_MAX], headSection, flowDirection, flowOrder[CORE_SECTION_MAX];
	unsigned short dataUInt16[255];
	unsigned long allocationSize;
	
	/********************
	 Initialize SuperTrak
	********************/
	/* SuperTrakCyclic1() has cycle-time violation if SuperTrakInit() is not called */
	/* Only call SuperTrakInit() if it's the first call to StCoreInit(), if multiple calls are made it will be caught in the next step */
	if(firstCall) {
		firstCall = false;
		/* SuperTrakInit() does not appear to return status information, if this fails the service channel will error */
		SuperTrakInit(StoragePath, SimIPAddress, EthernetInterfaceList);
	}
	else {
		/* This does not change the error status or StCore, but notifies the user of multiple calls */
		if(firstLog) {
			firstLog = false;
			coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_INIT), "Call StCoreInit() once during _INIT");
		}
		return stCORE_ERROR_INIT; /* Don't set the global error status */
	}
	
	/**************
	 Create logbook
	**************/
	status = CreateCustomLogbook(CORE_LOGBOOK_NAME, 200000);
	if(status != 0 && status != arEVENTLOG_ERR_LOGBOOK_EXISTS) {
		args.i[0] = status;
		coreStringCopy(args.s[0], CORE_LOGBOOK_NAME, sizeof(args.s[0]));
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_LOGBOOK), "ArEventLog error %i. Possible naming conflict with %s or insufficient user partition size", &args);
		return coreStatusID = stCORE_ERROR_LOGBOOK;
	}
	
	/********************
	 Verify system layout
	********************/
	/* Read section count */
	status = SuperTrakServChanRead(0, stPAR_SECTION_COUNT, 0, 1, (unsigned long)&sectionCount, sizeof(sectionCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_SECTION_COUNT);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
		
	/* Verify section count */
	if(sectionCount < 1 || CORE_SECTION_MAX < sectionCount) {
		coreStringCopy(args.s[0], StoragePath, sizeof(args.s[0]));
		args.i[0] = stPAR_SECTION_COUNT;
		args.i[1] = sectionCount;
		args.i[2] = CORE_SECTION_MAX;
		/* An invalid storage path can result in a section count of 0 */
		coreLogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_LAYOUT), "Verify path \"%s\" to configuration files. Section count (Par %i):%i exceeds limits [1, %i]", &args);
		return coreStatusID = stCORE_ERROR_LAYOUT;
	}
	
	/* Read section addresses */
	status = SuperTrakServChanRead(0, stPAR_SECTION_ADDRESS, 0, sectionCount, (unsigned long)&networkOrder, sizeof(networkOrder));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_SECTION_ADDRESS);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Read head section */
	status = SuperTrakServChanRead(0, stPAR_LOGICAL_HEAD_SECTION, 0, 1, (unsigned long)&headSection, sizeof(headSection));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_LOGICAL_HEAD_SECTION);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Verify head section */
	if(headSection != 1) {
		args.i[0] = stPAR_LOGICAL_HEAD_SECTION;
		args.i[1] = headSection;
		coreLogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_LAYOUT), "Logical head section (Par %i):%i is not equal to 1", &args);
		return coreStatusID = stCORE_ERROR_LAYOUT;
	}
	
	/* Read flow direction */
	status = SuperTrakServChanRead(0, stPAR_FLOW_DIRECTION, 0, 1, (unsigned long)&flowDirection, sizeof(flowDirection));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_FLOW_DIRECTION);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
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
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_LAYOUT), "Unable to match head section with system layout");
		return coreStatusID = stCORE_ERROR_LAYOUT;
	}
	
	j = 1; k = 0;
	while(networkOrder[i] != headSection) { /* Build flow order following network order in flow direction */
		if(++k > sectionCount) {
			coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_LAYOUT), "Unexpected system layout section order");
			return coreStatusID = stCORE_ERROR_LAYOUT;
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
			if(flowDirection == stDIRECTION_LEFT) coreStringCopy(args.s[0], "left", sizeof(args.s[0]));
			else coreStringCopy(args.s[0], "right", sizeof(args.s[0]));
			coreLogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_LAYOUT), "Non-increasing numbering in flow direction. Section %i %s of section %i", &args);
			return coreStatusID = stCORE_ERROR_LAYOUT;
		}
	}
	
	/************
	 Read targets
	************/
	/* Read section of 255 targets, indexed 0..254 */
	status = SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 1, COUNT_OF(dataUInt16), (unsigned long)&dataUInt16, sizeof(dataUInt16));
	
	/* Find the last target to be defined (non-zero section number) */
	for(i = 0, coreTargetCount = 0; i < COUNT_OF(dataUInt16); i++) {
		if(dataUInt16[i])
			coreTargetCount = i + 1;
	}

	/*******************************
	 Configure PLC control interface
	*******************************/
	corePalletCount = PalletCount;
	coreNetworkIOCount = NetworkIOCount;
	
	/* Options */
	/* Enable interface (0) and use system control & status */
	coreInterfaceConfig.options = (1 << stCONTROL_IF_ENABLED) + (1 << stCONTROL_IF_SYSTEM_ENABLED);
	/* Assign to interface 0 */
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_OPTIONS, 0, 1, (unsigned long)&coreInterfaceConfig.options, sizeof(coreInterfaceConfig.options));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_OPTIONS);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Section start */
	coreInterfaceConfig.sectionStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_START, 0, 1, (unsigned long)&coreInterfaceConfig.sectionStartIndex, sizeof(coreInterfaceConfig.sectionStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_SECTION_START);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Section count */
	coreInterfaceConfig.sectionCount = sectionCount;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_COUNT, 0, 1, (unsigned long)&coreInterfaceConfig.sectionCount, sizeof(coreInterfaceConfig.sectionCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_SECTION_COUNT);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Target start */
	coreInterfaceConfig.targetStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_START, 0, 1, (unsigned long)&coreInterfaceConfig.targetStartIndex, sizeof(coreInterfaceConfig.targetStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_TARGET_START);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Target count */
	coreInterfaceConfig.targetCount = ROUND_UP_MULTIPLE(coreTargetCount + 1, 4);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_COUNT, 0, 1, (unsigned long)&coreInterfaceConfig.targetCount, sizeof(coreInterfaceConfig.targetCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_TARGET_COUNT);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Command count */
	coreInterfaceConfig.commandCount = ROUND_UP_MULTIPLE(corePalletCount + 1, 8);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_COMMAND_COUNT, 0, 1, (unsigned long)&coreInterfaceConfig.commandCount, sizeof(coreInterfaceConfig.commandCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_COMMAND_COUNT);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Network IO start */
	coreInterfaceConfig.networkIoStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_START, 0, 1, (unsigned long)&coreInterfaceConfig.networkIoStartIndex, sizeof(coreInterfaceConfig.networkIoStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_NETWORK_IO_START);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Network IO count */
	coreInterfaceConfig.networkIoCount = ROUND_UP_MULTIPLE(coreNetworkIOCount + 1, 8);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_COUNT, 0, 1, (unsigned long)&coreInterfaceConfig.networkIoCount, sizeof(coreInterfaceConfig.networkIoCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_NETWORK_IO_COUNT);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Revision */
	coreInterfaceConfig.revision = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_REVISION, 0, 1, (unsigned long)&coreInterfaceConfig.revision, sizeof(coreInterfaceConfig.revision));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_REVISION);
		return coreStatusID = stCORE_ERROR_SERVCHAN;
	}
	
	/**************************
	 Read PLC control interface
	**************************/
	SuperTrakGetControlIfConfig(0, &coreInterfaceConfig);
	
	/***************
	 Allocate memory
	***************/
	allocationSize = coreInterfaceConfig.controlSize;
	status = TMP_alloc(allocationSize, (void**)&pCoreCyclicControl);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "cyclic control data");
		return stCORE_ERROR_ALLOC;
	}
	memset(pCoreCyclicControl, 0, allocationSize); /* Initialization memory to zero */
	
	allocationSize = coreInterfaceConfig.statusSize;
	status = TMP_alloc(allocationSize, (void**)&pCoreCyclicStatus);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "cyclic status data");
		return stCORE_ERROR_ALLOC;
	}
	memset(pCoreCyclicStatus, 0, allocationSize); /* Initialization memory to zero */
	
	/* Memory for command buffers */
	allocationSize = sizeof(SuperTrakCommand_t) * CORE_COMMANDBUFFER_SIZE * corePalletCount; /* Command data * commands per pallet * pallets */
	status = TMP_alloc(allocationSize, (void**)&pCoreCommandBuffer);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "pallet command buffers");
		return stCORE_ERROR_ALLOC;
	}
	memset(pCoreCommandBuffer, 0, allocationSize); /* Initialization memory to zero */
	
	/* Memory for buffer write indices */
	allocationSize = sizeof(coreBufferControlType) * corePalletCount;
	status = TMP_alloc(allocationSize, (void**)&pCoreBufferControl);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "command buffer control");
		return stCORE_ERROR_ALLOC;
	}
	memset(pCoreBufferControl, 0, allocationSize); /* Initialization memory to zero */
	
	args.i[0] = sectionCount;
	args.i[1] = coreTargetCount;
	coreLogFormatMessage(USERLOG_SEVERITY_SUCCESS, 1200, "%i sections and %i targets defined in TrakMaster", &args);
	coreError = false;
	return coreStatusID = 0;
	
} /* Function definition */


/* Log error from memory management (TMP_alloc) calls */
void logMemoryManagement(unsigned short result, unsigned long size, char *name) {
	
	/* Declare local variables */
	FormatStringArgumentsType args;
	
	/* Set arguments */
	args.i[0] = result;
	args.i[1] = (long)size;
	coreStringCopy(args.s[0], name, sizeof(args.s[0]));

	/* Log message */
	coreLogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_ALLOC), "TMP_alloc() error %i when allocating %i bytes for %s", &args);
	
} /* Function definition */
