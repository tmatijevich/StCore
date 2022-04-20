/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

/* Process SuperTrak control interface */
long StCoreCyclic(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static unsigned char taskVerified;
	RTInfo_typ fbRTInfo;
	FormatStringArgumentsType args;
	static unsigned long timerSave, saveParameters;
	long status;
	SuperTrakControlIfConfig_t currentInterfaceConfig;
	static SuperTrakControlInterface_t controlInterface;
	
	/************
	 Verification
	************/
	SuperTrakGetControlIfConfig(0, &currentInterfaceConfig); /* Get current control interface configuration */
	
	/* 1. Existing error */
	if(coreError) {
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
			coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_TASK), "Invalid task class %i or cycle time %i us", &args);
			coreError = true;
			coreStatusID = stCORE_ERROR_TASK;
		}
		else 
			taskVerified = true;
	}
		
	/* 3. Delay and save parameters */
	else if(saveParameters == 0) {
		if(timerSave >= 500000) {
			saveParameters = 0x000020; /* Global parameters and PLC interface configuration */
			status = SuperTrakServChanWrite(0, stPAR_SAVE_PARAMETERS, 0, 1, (unsigned long)&saveParameters, sizeof(saveParameters));
			if(status != scERR_SUCCESS) {
				coreLogServiceChannel((unsigned short)status, stPAR_SAVE_PARAMETERS);
				coreError = true;
				coreStatusID = stCORE_ERROR_SERVCHAN;
			}
		}
		else
			timerSave += 800;
	}
		
	/* 4. Monitor changes to control interface configuration */
	else if(memcmp(&currentInterfaceConfig, &coreInterfaceConfig, sizeof(currentInterfaceConfig)) != 0) {
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_INTERFACE), "Please restart controller and do not modify control interface configuration");
		coreError = true;
		coreStatusID = stCORE_ERROR_INTERFACE;
	}
	
	/*****************************
	 Process SuperTrak cyclic data
	*****************************/
	/* Process StCore command buffers */
	coreCommandManager();
	
	/* Reference control interface */
	controlInterface.pControl = (unsigned long)pCoreCyclicControl;
	controlInterface.controlSize = coreInterfaceConfig.controlSize;
	controlInterface.pStatus = (unsigned long)pCoreCyclicStatus;
	controlInterface.statusSize = coreInterfaceConfig.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	/* Process SuperTrak control */
	if(coreError || saveParameters == 0) {
		/* Do nothing */
	}
	else if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL) {
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_ALLOC), "StCoreCyclic() cannot reference cyclic control or status data due to null pointer");
		coreError = true;
		coreStatusID = stCORE_ERROR_ALLOC;
	}
	else {
		/* Only call if there is no error and valid references */
		SuperTrakProcessControl(0, &controlInterface);
	}
		
	/* Process SuperTrak interface */
	SuperTrakCyclic1();
	
	/* Process SuperTrak status */
	if(!coreError && saveParameters) 
		SuperTrakProcessStatus(0, &controlInterface);
	
	return coreStatusID;
	
} /* Function definition */
