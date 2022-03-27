/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

unsigned long coreSaveParameters;

/* Process SuperTrak control interface */
long StCoreCyclic(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static unsigned char taskVerified, logAllocation = true;
	RTInfo_typ fbRTInfo;
	FormatStringArgumentsType args;
	static unsigned long timerSave;
	long status, i;
	SuperTrakControlIfConfig_t currentInterfaceConfig;
	SuperTrakPalletInfo_t palletInfo[256];
	static SuperTrakControlInterface_t controlInterface;
	
	/******************************************************************************
	 Error check:
	 1. Existing error
	 2. Task class and cycle time
	 3. Delay and save parameters
	 4. Interface configuration changes
	 5. Pallet IDs within configured limit
	******************************************************************************/
	/* 1. Do not run anything if core error from Init, Cyclic, or Exit functions */
	/* A core error should always come with a status ID */
	if(coreError)
		return coreStatusID;
		
	/* 2. Verify task class and cycle time */
	if(!taskVerified) {
		memset(&fbRTInfo, 0, sizeof(fbRTInfo));
		fbRTInfo.enable = true;
		RTInfo(&fbRTInfo);
		if(fbRTInfo.task_class != 1 || fbRTInfo.cycle_time != 800) {
			args.i[0] = fbRTInfo.task_class;
			args.i[1] = fbRTInfo.cycle_time;
			coreLogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_TASK), "Invalid task class %i or cycle time %i us", &args);
			coreError = true;
			return coreStatusID = stCORE_ERROR_TASK;
		}
		taskVerified = true;
	}
		
	/* 3. Save parameters */
	if(timerSave >= 500000 && coreSaveParameters == 0) {
		coreSaveParameters = 0x000020; /* Global parameters and PLC interface configuration */
		status = SuperTrakServChanWrite(0, stPAR_SAVE_PARAMETERS, 0, 1, (unsigned long)&coreSaveParameters, sizeof(coreSaveParameters));
		if(status != scERR_SUCCESS) {
			coreLogServiceChannel((unsigned short)status, stPAR_SAVE_PARAMETERS);
			coreError = true;
			return coreStatusID = stCORE_ERROR_SERVCHAN;
		}
	}
	else if(coreSaveParameters == 0)
		timerSave += 800;
		
	/* Wait until interface configuration is saved */
//	if(coreSaveParameters == 0)
//		return 0;
		
	/* 4. Monitor for changes to control interface configuration from initilization */
	SuperTrakGetControlIfConfig(0, &currentInterfaceConfig);
	if(memcmp(&currentInterfaceConfig, &coreControlInterface, sizeof(currentInterfaceConfig)) != 0) {
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_INTERFACE), "Please restart controller and do not modify control interface configuration");
		coreError = true;
		return coreStatusID = stCORE_ERROR_INTERFACE;
	}
	
	/* 5. Monitor pallet IDs present on system for limit */
	/*SuperTrakGetPalletInfo((unsigned long)&palletInfo, (unsigned char)COUNT_OF(palletInfo), false);
	for(i = corePalletCount; i < COUNT_OF(palletInfo); i++) {
		if(GET_BIT(palletInfo[i].status, 0)) {
			args.i[0] = palletInfo[i].palletID;
			args.i[1] = i;
			args.i[2] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_MAXPALLET), "Pallet ID %i index %i preset on system exceeding configured limit of %i pallets", &args);
			coreError = true;
			return coreStatusID = stCORE_ERROR_MAXPALLET;
		}
	}*/
	
	/*****************************
	 Process SuperTrak cyclic data
	*****************************/
	/* Guard pointers */
	if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL) {
		if(logAllocation) 
			coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_ALLOC), "StCoreCyclic() cannot reference cyclic control or status data due to null pointer");
		logAllocation = false;
		coreError = true;
		return coreStatusID = stCORE_ERROR_ALLOC;
	}
	logAllocation = true;
		
	/* Process StCore commands */
	coreProcessCommand();
	
	controlInterface.pControl = (unsigned long)pCoreCyclicControl;
	controlInterface.controlSize = coreControlInterface.controlSize;
	controlInterface.pStatus = (unsigned long)pCoreCyclicStatus;
	controlInterface.statusSize = coreControlInterface.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	SuperTrakProcessControl(0, &controlInterface);
	SuperTrakCyclic1();
	SuperTrakProcessStatus(0, &controlInterface);
	
	return 0;
	
} /* Function definition */
