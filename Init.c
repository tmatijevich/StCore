/*******************************************************************************
 * File: StCore\Init.c
 * Author: Tyler Matijevich
 * Date: 2022-02-15
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Init"

/* Prototypes */
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);
void logMemoryManagement(unsigned short result, unsigned long size, char *name);

/* Global variable declaration */
struct coreGlobalType core = {.error = true, .statusID = stCORE_ERROR_INIT};

/*********************
 StCoreInit definition
*********************/
/* Initialize SuperTrak, verify layout, read targets, and size control interface */
long StCoreInit(char *StoragePath, char *SimIPAddress, char *EthernetInterfaceList, unsigned char PalletCount, unsigned char NetworkIOCount) {
	
	/***********************
	 Declare Local Variables 
	***********************/
	ArEventLogCreate_typ fbCreate;
	ArEventLogGetIdent_typ fbGetIdent;
	long status, i, j, k;
	coreFormatArgumentType args;
	unsigned short sectionCount, networkOrder[CORE_SECTION_MAX], headSection, flowDirection;
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
	 Create Logbook
	**************/
	/* Set inputs */
	memset(&fbCreate, 0, sizeof(fbCreate));
	coreStringCopy(fbCreate.Name, CORE_LOGBOOK_NAME, sizeof(fbCreate.Name));
	fbCreate.Size = 200000;
	fbCreate.Persistence = arEVENTLOG_PERSISTENCE_PERSIST;
	fbCreate.Execute = true;
	
	/* Run until finished */
	ArEventLogCreate(&fbCreate);
	while(fbCreate.Busy)
		ArEventLogCreate(&fbCreate);
	
	/* Verify results */
	status = fbCreate.StatusID;
	if(status != 0 && status != arEVENTLOG_ERR_LOGBOOK_EXISTS) {
		/* Get user logbook ident */
		memset(&fbGetIdent, 0, sizeof(fbGetIdent));
		coreStringCopy(fbGetIdent.Name, "$arlogusr", sizeof(fbGetIdent.Name));
		fbGetIdent.Execute = true;
		ArEventLogGetIdent(&fbGetIdent);
		
		/* Log error to user logbook */
		args.i[0] = status;
		coreStringCopy(args.s[0], CORE_LOGBOOK_NAME, sizeof(args.s[0]));
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_LOGBOOK), "ArEventLog error %i due to possible naming conflict with %s or insufficient user partition size", &args);
		
		fbGetIdent.Execute = false;
		ArEventLogGetIdent(&fbGetIdent);
		
		return core.statusID = stCORE_ERROR_LOGBOOK;
	}
	else if(status == arEVENTLOG_ERR_LOGBOOK_EXISTS) {
		/* Assign the ident from existing logbook */
		memset(&fbGetIdent, 0, sizeof(fbGetIdent));
		coreStringCopy(fbGetIdent.Name, CORE_LOGBOOK_NAME, sizeof(fbGetIdent.Name));
		fbGetIdent.Execute = true;
		ArEventLogGetIdent(&fbGetIdent);
		
		core.ident = fbGetIdent.Ident;
		
		fbGetIdent.Execute = false;
		ArEventLogGetIdent(&fbGetIdent);
	}
	else {
		/* Record the ident of created logbook */
		core.ident = fbCreate.Ident;
	}
	fbCreate.Execute = false;
	ArEventLogCreate(&fbCreate);
	
	/********************
	 Verify system layout
	********************/
	/* Read section count */
	status = SuperTrakServChanRead(0, stPAR_SECTION_COUNT, 0, 1, (unsigned long)&sectionCount, sizeof(sectionCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_SECTION_COUNT, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
		
	/* Verify section count */
	if(sectionCount < 1 || CORE_SECTION_MAX < sectionCount) {
		coreStringCopy(args.s[0], StoragePath, sizeof(args.s[0]));
		args.i[0] = sectionCount;
		args.i[1] = CORE_SECTION_MAX;
		/* An invalid storage path can result in a section count of 0 */
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_LAYOUT), "Verify path \"%s\" to configuration files. Section count %i exceeds limits [1, %i]", &args);
		return core.statusID = stCORE_ERROR_LAYOUT;
	}
	
	/* Read section addresses */
	status = SuperTrakServChanRead(0, stPAR_SECTION_ADDRESS, 0, sectionCount, (unsigned long)&networkOrder, sizeof(networkOrder));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_SECTION_ADDRESS, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Read head section */
	status = SuperTrakServChanRead(0, stPAR_LOGICAL_HEAD_SECTION, 0, 1, (unsigned long)&headSection, sizeof(headSection));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_LOGICAL_HEAD_SECTION, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Read flow direction */
	status = SuperTrakServChanRead(0, stPAR_FLOW_DIRECTION, 0, 1, (unsigned long)&flowDirection, sizeof(flowDirection));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_FLOW_DIRECTION, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Clear mapping table */
	memset(&core.sectionMap, -1, sizeof(core.sectionMap));
	
	/* Find the head in user addresses */
	for(i = 0; i < sectionCount; i++) {
		/* Match head */
		if(networkOrder[i] == headSection) {
			/* Check address */
			if(networkOrder[i] < 1 || CORE_SECTION_ADDRESS_MAX < networkOrder[i]) {
				args.i[0] = networkOrder[i];
				args.i[1] = CORE_SECTION_ADDRESS_MAX;
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_LAYOUT), "Section address %i exceeds limits [1, %i]", &args);
				return core.statusID = stCORE_ERROR_LAYOUT;
			}
			
			/* Register in mapping table */
			core.sectionMap[networkOrder[i]] = 0;
			
			/* Adjust index in flow direction */
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
	
	/* Walk from head in flow direction through network order */
	j = 1; k = 1;
	while(networkOrder[i] != headSection) {
		/* Monitor loop */
		if(++k > sectionCount) { 
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_LAYOUT), "Unexpected system layout section order", NULL);
			return core.statusID = stCORE_ERROR_LAYOUT;
		}
		
		/* Check address */
		if(networkOrder[i] < 1 || CORE_SECTION_ADDRESS_MAX < networkOrder[i]) {
			args.i[0] = networkOrder[i];
			args.i[1] = CORE_SECTION_ADDRESS_MAX;
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_LAYOUT), "Section address %i exceeds limits [1, %i]", &args);
			return core.statusID = stCORE_ERROR_LAYOUT;
		}
		
		/* Register in mapping table */
		core.sectionMap[networkOrder[i]] = j++;
		
		/* Adjust index in flow direction */
		if(flowDirection == stDIRECTION_RIGHT) {
			if(i == sectionCount - 1) i = 0;
			else i += 1;
		}
		else {
			if(i == 0) i = sectionCount - 1;
			else i -= 1;
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
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_OPTIONS, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Section start */
	core.interface.sectionStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_START, 0, 1, (unsigned long)&core.interface.sectionStartIndex, sizeof(core.interface.sectionStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_SECTION_START, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Section count */
	core.interface.sectionCount = sectionCount;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_SECTION_COUNT, 0, 1, (unsigned long)&core.interface.sectionCount, sizeof(core.interface.sectionCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_SECTION_COUNT, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Target start */
	core.interface.targetStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_START, 0, 1, (unsigned long)&core.interface.targetStartIndex, sizeof(core.interface.targetStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_TARGET_START, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Target count */
	core.interface.targetCount = ROUND_UP_MULTIPLE(core.targetCount + 1, CORE_TARGET_RELEASE_PER_BYTE);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_TARGET_COUNT, 0, 1, (unsigned long)&core.interface.targetCount, sizeof(core.interface.targetCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_TARGET_COUNT, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Command count */
	core.interface.commandCount = ROUND_UP_MULTIPLE(CORE_COMMAND_COUNT, CORE_COMMAND_DATA_BYTE_COUNT);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_COMMAND_COUNT, 0, 1, (unsigned long)&core.interface.commandCount, sizeof(core.interface.commandCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_COMMAND_COUNT, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Network IO start */
	core.interface.networkIoStartIndex = 0;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_START, 0, 1, (unsigned long)&core.interface.networkIoStartIndex, sizeof(core.interface.networkIoStartIndex));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_NETWORK_IO_START, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Network IO count */
	core.interface.networkIoCount = ROUND_UP_MULTIPLE(core.networkIOCount + 1, CORE_NETWORK_IO_PER_BYTE);
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_NETWORK_IO_COUNT, 0, 1, (unsigned long)&core.interface.networkIoCount, sizeof(core.interface.networkIoCount));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_NETWORK_IO_COUNT, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
	}
	
	/* Revision */
	core.interface.revision = 3;
	status = SuperTrakServChanWrite(0, stPAR_PLC_IF_REVISION, 0, 1, (unsigned long)&core.interface.revision, sizeof(core.interface.revision));
	if(status != scERR_SUCCESS) {
		coreLogServiceChannel((unsigned short)status, stPAR_PLC_IF_REVISION, LOG_OBJECT);
		return core.statusID = stCORE_ERROR_COMM;
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
	allocationSize = sizeof(coreCommandType) * core.targetCount;
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
	logMessage(CORE_LOG_SEVERITY_SUCCESS, 1200, "%i sections and %i targets defined in TrakMaster", &args);
	core.error = false;
	return core.statusID = 0;
	
} /* End function */

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}

/* Log error from memory management (TMP_alloc) calls */
void logMemoryManagement(unsigned short result, unsigned long size, char *name) {
	
	/* Declare local variables */
	coreFormatArgumentType args;
	
	/* Set arguments */
	args.i[0] = result;
	args.i[1] = (long)size;
	coreStringCopy(args.s[0], name, sizeof(args.s[0]));

	/* Log message */
	logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "TMP_alloc() error %i when allocating %i bytes for %s", &args);
	
} /* End function */
