/*******************************************************************************
 * File: StCore\Cyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "Main.h"

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
			coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_CYCLE), "Invalid task class %i or cycle time %i us", &args);
			core.error = true;
			core.statusID = stCORE_ERROR_CYCLE;
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
				core.error = true;
				core.statusID = stCORE_ERROR_COMM;
			}
		}
		else
			timerSave += 800;
	}
		
	/* 4. Monitor changes to control interface configuration */
	else if(memcmp(&currentInterfaceConfig, &core.interface, sizeof(currentInterfaceConfig)) != 0) {
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_INTERFACE), "Please restart controller and do not modify control interface configuration");
		core.error = true;
		core.statusID = stCORE_ERROR_INTERFACE;
	}
	
	/*****************************
	 Process SuperTrak cyclic data
	*****************************/
	/* Process StCore command buffers */
	coreCommandManager();
	
	/* Reference control interface */
	controlInterface.pControl = (unsigned long)core.pCyclicControl;
	controlInterface.controlSize = core.interface.controlSize;
	controlInterface.pStatus = (unsigned long)core.pCyclicStatus;
	controlInterface.statusSize = core.interface.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	/* Process SuperTrak control */
	if(core.error || saveParameters == 0) {
		/* Do nothing */
	}
	else if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
		coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_ALLOC), "StCoreCyclic() cannot reference cyclic control or status data due to null pointer");
		core.error = true;
		core.statusID = stCORE_ERROR_ALLOC;
	}
	else {
		/* Only call if there is no error and valid references */
		SuperTrakProcessControl(0, &controlInterface);
	}
		
	/* Process SuperTrak interface */
	SuperTrakCyclic1();
	
	/* Process SuperTrak status */
	if(!core.error && saveParameters) 
		SuperTrakProcessStatus(0, &controlInterface);
	
	return core.statusID;
	
} /* Function definition */
