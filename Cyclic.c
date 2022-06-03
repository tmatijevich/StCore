/*******************************************************************************
 * File: StCore\Cyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Cyclic"

/* Prototypes */
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);

/* Process SuperTrak control interface */
long StCoreCyclic(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static unsigned char taskVerified;
	RTInfo_typ fbRTInfo;
	coreFormatArgumentType args;
	static unsigned long timerSave, saveParameters;
	long status, i;
	SuperTrakControlIfConfig_t currentInterfaceConfig;
	static SuperTrakControlInterface_t controlInterface;
	SuperTrakPalletInfo_t *pPalletData;
	unsigned short palletPresentCount, *pSystemPalletCount;
	
	/************
	 Verification
	************/
	SuperTrakGetControlIfConfig(0, &currentInterfaceConfig); /* Get current control interface configuration */
	
	/* 1. Existing error */
	if(core.error) {
		/* Do not proceed with further checks */
	}
		
	/* 2. Verify task class and cycle time */
	else if(!taskVerified) {
		memset(&fbRTInfo, 0, sizeof(fbRTInfo));
		fbRTInfo.enable = true;
		RTInfo(&fbRTInfo);
		if(fbRTInfo.task_class != 1 || fbRTInfo.cycle_time != 800) {
			args.i[0] = fbRTInfo.task_class;
			args.i[1] = fbRTInfo.cycle_time;
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_CYCLE), "Invalid task class %i or cycle time %i us", &args);
			core.error = true;
			core.statusID = stCORE_ERROR_CYCLE;
		}
		else 
			taskVerified = true;
	}
		
	/* 3. Delay and save parameters */
	else if(!core.ready) {
		if(timerSave >= 500000) {
			saveParameters = 0x000020; /* Global parameters and PLC interface configuration */
			status = SuperTrakServChanWrite(0, stPAR_SAVE_PARAMETERS, 0, 1, (unsigned long)&saveParameters, sizeof(saveParameters));
			if(status != scERR_SUCCESS) {
				coreLogServiceChannel((unsigned short)status, stPAR_SAVE_PARAMETERS, LOG_OBJECT);
				core.error = true;
				core.statusID = stCORE_ERROR_PARAMETER;
			}
			else
				core.ready = true;
		}
		else
			timerSave += 800;
	}
		
	/* 4. Monitor changes to control interface configuration */
	else if(memcmp(&currentInterfaceConfig, &core.interface, sizeof(currentInterfaceConfig)) != 0) {
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_PROTOCAL), "Please restart controller and do not modify control interface configuration", NULL);
		core.error = true;
		core.statusID = stCORE_ERROR_PROTOCAL;
	}
	
	/*****************************
	 Process SuperTrak Cyclic Data
	*****************************/
	/* Check references */
	if((core.pCyclicControl == NULL || core.pCyclicStatus == NULL || core.pPalletData == NULL) && !core.error) {
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOCATION), "StCoreCyclic cannot reference allocated cyclic or pallet data", NULL);
		core.error = true;
		core.statusID = stCORE_ERROR_ALLOCATION;
	}
	
	/* Process StCore commands */
	coreCommandManager();
	
	/* Reference control interface */
	controlInterface.pControl = (unsigned long)core.pCyclicControl;
	controlInterface.controlSize = core.interface.controlSize;
	controlInterface.pStatus = (unsigned long)core.pCyclicStatus;
	controlInterface.statusSize = core.interface.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	/* Process SuperTrak control */
	if(core.pCyclicControl && core.pCyclicStatus && core.ready) { /* Only process control data if the references are valid */
		if(core.error)
			memset(core.pCyclicControl, 0, core.interface.controlSize); /* Clear control data on critical error */
		SuperTrakProcessControl(0, &controlInterface);
	}
		
	/* Process SuperTrak interface */
	SuperTrakCyclic1();
	
	/* Process SuperTrak status */
	if(core.pCyclicControl && core.pCyclicStatus && core.ready) /* Only process status data if the references are valid */
		SuperTrakProcessStatus(0, &controlInterface);
	
	/**************
	 Pallet Manager
	**************/
	/* Reset mapping */
	memset(&core.palletMap, -1, sizeof(core.palletMap));
	
	/* Do not proceed if error or not ready */
	if(core.error) {
		if(core.pPalletData)
			memset(core.pPalletData, 0, sizeof(SuperTrakPalletInfo_t) * core.palletCount); /* Clear pallet data on critical error */
		return core.statusID;
	}
	
	/* Read pallet information */
	SuperTrakGetPalletInfo((unsigned long)core.pPalletData, core.palletCount, false); /* Read memory structure from 0 to palletCount - 1 */
	
	/* Update pallet mapping */
	palletPresentCount = 0;
	for(i = 0; i < core.palletCount; i++) {
		pPalletData = core.pPalletData + i;
		/* Mapping the pallet ID if assigned (non-zero) to offset in memory structure (i) */
		if(pPalletData->palletID)
			core.palletMap[pPalletData->palletID] = i;
		
		/* Count the number of pallets present up to the user allocated amount */
		if(GET_BIT(pPalletData->status, stPALLET_PRESENT))
			palletPresentCount++;
	}
	
	/* Compare pallets present from allocated data to system total pallets */
	pSystemPalletCount = (unsigned short*)(core.pCyclicStatus + core.interface.systemStatusOffset + 2);
	if(palletPresentCount != *pSystemPalletCount && !core.error && core.ready) {
		args.i[0] = *pSystemPalletCount;
		args.i[1] = palletPresentCount;
		args.i[2] = core.palletCount;
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_PALLET), "%i total pallets on system but only %i are allocated for - increase PalletCount %i in StCoreInit", &args);
		core.error = true;
		return core.statusID = stCORE_ERROR_PALLET;
	}
	
	return 0;
	
} /* End function */

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}
