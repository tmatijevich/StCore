/*******************************************************************************
 * File: StCore\Init.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "Main.h"

/* Prototypes */
void logMemoryManagement(unsigned short result, unsigned long size, char *name);

/* Global variable declaration */
struct coreGlobalType core = {.error = true, .statusID = stCORE_ERROR_INIT};

/*********************
 StCoreInit definition
*********************/
/* Initialize SuperTrak, verify layout, read targets, and size control interface */
long StCoreInit(char *StoragePath, char *SimIPAddress, char *EthernetInterfaceList, unsigned char PalletCount, unsigned char NetworkIOCount) {
	
	/***********************
	 Declare local variables 
	***********************/
	long status, i, j, k;
	FormatStringArgumentsType args;
	unsigned short sectionCount, networkOrder[CORE_SECTION_MAX], headSection, flowDirection, flowOrder[CORE_SECTION_MAX];
	unsigned short dataUInt16[255];
	unsigned long allocationSize;
	
	/********************
	 Initialize SuperTrak
	********************/
	SuperTrakInit(StoragePath, SimIPAddress, EthernetInterfaceList);
	
	/* Assmume initialization error until the routine completes successfully (for transfer with init/exit routines) */
	core.error = true;
	core.statusID = stCORE_ERROR_INIT;
	
	/**************
	 Create logbook
	**************/
	status = CreateCustomLogbook(CORE_LOGBOOK_NAME, 200000);
	if(status != 0 && status != arEVENTLOG_ERR_LOGBOOK_EXISTS) {
		args.i[0] = status;
		coreStringCopy(args.s[0], CORE_LOGBOOK_NAME, sizeof(args.s[0]));
		LogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_LOGBOOK), "ArEventLog error %i. Possible naming conflict with %s or insufficient user partition size", &args);
		return core.statusID = stCORE_ERROR_LOGBOOK;
	}
	
	/********************
	 Verify system layout
	********************/
	/* Read section count */
	status = SuperTrakServChanRead(0, stPAR_SECTION_COUNT, 0, 1, (unsigned long)&sectionCount, sizeof(sectionCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_SECTION_COUNT);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
		
	/* Verify section count */
	if(sectionCount < 1 || CORE_SECTION_MAX < sectionCount) {
		coreStringCopy(args.s[0], StoragePath, sizeof(args.s[0]));
		args.i[0] = stPAR_SECTION_COUNT;
		args.i[1] = sectionCount;
		args.i[2] = CORE_SECTION_MAX;
		/* An invalid storage path can result in a section count of 0 */
		coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_LAYOUT), "Verify path \"%s\" to configuration files. Section count (Par %i):%i exceeds limits [1, %i]", &args);
		return core.statusID = stCORE_ERROR_LAYOUT;
	}
	
	/* Read section addresses */
	status = SuperTrakServChanRead(0, stPAR_SECTION_ADDRESS, 0, sectionCount, (unsigned long)&networkOrder, sizeof(networkOrder));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_SECTION_ADDRESS);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Read head section */
	status = SuperTrakServChanRead(0, stPAR_LOGICAL_HEAD_SECTION, 0, 1, (unsigned long)&headSection, sizeof(headSection));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_LOGICAL_HEAD_SECTION);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Verify head section */
	if(headSection != 1) {
		args.i[0] = stPAR_LOGICAL_HEAD_SECTION;
		args.i[1] = headSection;
		coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_LAYOUT), "Logical head section (Par %i):%i is not equal to 1", &args);
		return core.statusID = stCORE_ERROR_LAYOUT;
	}
	
	/* Read flow direction */
	status = SuperTrakServChanRead(0, stPAR_FLOW_DIRECTION, 0, 1, (unsigned long)&flowDirection, sizeof(flowDirection));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_FLOW_DIRECTION);
		return core.statusID = stCORE_ERROR_SERVCHAN;
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
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_LAYOUT), "Unable to match head section with system layout");
		return core.statusID = stCORE_ERROR_LAYOUT;
	}
	
	j = 1; k = 0;
	while(networkOrder[i] != headSection) { /* Build flow order following network order in flow direction */
		if(++k > sectionCount) {
			coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_LAYOUT), "Unexpected system layout section order");
			return core.statusID = stCORE_ERROR_LAYOUT;
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
			coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_LAYOUT), "Non-increasing numbering in flow direction. Section %i %s of section %i", &args);
			return core.statusID = stCORE_ERROR_LAYOUT;
		}
	}
	
	/************
	 Read targets
	************/
	/* Read section of 255 targets, indexed 0..254 */
	status = SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 1, COUNT_OF(dataUInt16), (unsigned long)&dataUInt16, sizeof(dataUInt16));
	
	/* Find the last target to be defined (non-zero section number) */
	for(i = 0, core.targetCount = 0; i < COUNT_OF(dataUInt16); i++) {
		if(dataUInt16[i])
			core.targetCount = i + 1;
	}

	/*******************************
	 Configure PLC control interface
	*******************************/
	core.palletCount = PalletCount;
	core.networkIOCount = NetworkIOCount;
	
	/* Options */
	/* Enable interface (0) and use system control & status */
	core.interface.options = (1 << stCONTROL_IF_ENABLED) + (1 << stCONTROL_IF_SYSTEM_ENABLED);
	/* Assign to interface 0 */
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_OPTIONS, 0, 1, (unsigned long)&core.interface.options, sizeof(core.interface.options));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_OPTIONS);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Section start */
	core.interface.sectionStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_START, 0, 1, (unsigned long)&core.interface.sectionStartIndex, sizeof(core.interface.sectionStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_SECTION_START);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Section count */
	core.interface.sectionCount = sectionCount;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_COUNT, 0, 1, (unsigned long)&core.interface.sectionCount, sizeof(core.interface.sectionCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_SECTION_COUNT);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Target start */
	core.interface.targetStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_START, 0, 1, (unsigned long)&core.interface.targetStartIndex, sizeof(core.interface.targetStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_TARGET_START);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Target count */
	core.interface.targetCount = ROUND_UP_MULTIPLE(core.targetCount + 1, 4);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_COUNT, 0, 1, (unsigned long)&core.interface.targetCount, sizeof(core.interface.targetCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_TARGET_COUNT);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Command count */
	core.interface.commandCount = ROUND_UP_MULTIPLE(CORE_COMMAND_COUNT, 8);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_COMMAND_COUNT, 0, 1, (unsigned long)&core.interface.commandCount, sizeof(core.interface.commandCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_COMMAND_COUNT);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Network IO start */
	core.interface.networkIoStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_START, 0, 1, (unsigned long)&core.interface.networkIoStartIndex, sizeof(core.interface.networkIoStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_NETWORK_IO_START);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Network IO count */
	core.interface.networkIoCount = ROUND_UP_MULTIPLE(core.networkIOCount + 1, 8);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_COUNT, 0, 1, (unsigned long)&core.interface.networkIoCount, sizeof(core.interface.networkIoCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_NETWORK_IO_COUNT);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/* Revision */
	core.interface.revision = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_REVISION, 0, 1, (unsigned long)&core.interface.revision, sizeof(core.interface.revision));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_REVISION);
		return core.statusID = stCORE_ERROR_SERVCHAN;
	}
	
	/**************************
	 Read PLC control interface
	**************************/
	SuperTrakGetControlIfConfig(0, &core.interface);
	
	/***************
	 Allocate memory
	***************/
	allocationSize = core.interface.controlSize;
	if(core.pCyclicControl)
		TMP_free(allocationSize, (void**)core.pCyclicControl);
	status = TMP_alloc(allocationSize, (void**)&core.pCyclicControl);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "cyclic control data");
		return stCORE_ERROR_ALLOC;
	}
	memset(core.pCyclicControl, 0, allocationSize); /* Initialization memory to zero */
	
	allocationSize = core.interface.statusSize;
	if(core.pCyclicStatus)
		TMP_free(allocationSize, (void**)core.pCyclicStatus);
	status = TMP_alloc(allocationSize, (void**)&core.pCyclicStatus);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "cyclic status data");
		return stCORE_ERROR_ALLOC;
	}
	memset(core.pCyclicStatus, 0, allocationSize); /* Initialization memory to zero */
	
	/* Memory for simple target release */
	allocationSize = sizeof(coreSimpleTargetReleaseType) * core.targetCount;
	if(core.pSimpleRelease)
		TMP_free(allocationSize, (void**)core.pSimpleRelease);
	status = TMP_alloc(allocationSize, (void**)&core.pSimpleRelease);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "simple target release");
		return stCORE_ERROR_ALLOC;
	}
	memset(core.pSimpleRelease, 0, allocationSize); /* Initialization memory to zero */
	
	/* Memory for pallet command buffers */
	allocationSize = sizeof(coreCommandBufferType) * core.palletCount;
	if(core.pCommandBuffer)
		TMP_free(allocationSize, (void**)core.pCommandBuffer);
	status = TMP_alloc(allocationSize, (void**)&core.pCommandBuffer);
	if(status) {
		logMemoryManagement((unsigned short)status, allocationSize, "pallet command buffers");
		return stCORE_ERROR_ALLOC;
	}
	memset(core.pCommandBuffer, 0, allocationSize); /* Initialization memory to zero */
	
	args.i[0] = sectionCount;
	args.i[1] = core.targetCount;
	coreLogFormat(USERLOG_SEVERITY_SUCCESS, 1200, "%i sections and %i targets defined in TrakMaster", &args);
	core.error = false;
	return core.statusID = 0;
	
} /* End function */


/* Log error from memory management (TMP_alloc) calls */
void logMemoryManagement(unsigned short result, unsigned long size, char *name) {
	
	/* Declare local variables */
	FormatStringArgumentsType args;
	
	/* Set arguments */
	args.i[0] = result;
	args.i[1] = (long)size;
	coreStringCopy(args.s[0], name, sizeof(args.s[0]));

	/* Log message */
	coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_ALLOC), "TMP_alloc() error %i when allocating %i bytes for %s", &args);
	
} /* End function */
