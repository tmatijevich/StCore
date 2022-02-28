/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

unsigned long saveParameters;

long StCoreCyclic(void) {
	
	/***********************
	 Declare local variables 
	***********************/
	struct FormatStringArgumentsType args;
	static unsigned long timerSave;
	static SuperTrakControlInterface_t controlInterface;
	
	configPLCInterface;
	
	/***************
	 Save parameters
	***************/
	if(timerSave >= 500000 && saveParameters == 0) {
		/* Save parameters */
		saveParameters = 0x000020; /* Global parameters, PLC interface configuration */
		args.i[0] = SuperTrakServChanWrite(0, stPAR_SAVE_PARAMETERS, 0, 1, (unsigned long)&saveParameters, sizeof(saveParameters));
		if(args.i[0] != scERR_SUCCESS) {
			configError = true;
			StCoreLogServChan(args.i[0], stPAR_SAVE_PARAMETERS);
		}
		else {
			args.i[0] = stPAR_SAVE_PARAMETERS;
			args.i[1] = saveParameters;
			CustomFormatMessage(USERLOG_SEVERITY_DEBUG, 2000, "Successful serv. chan. write to par %i with value %i", &args, "StCoreLog", 1);
		}
	}
	else
		timerSave += 800;
		
	controlInterface.pControl = (unsigned long)control;
	controlInterface.controlSize = configPLCInterface.controlSize;
	controlInterface.pStatus = (unsigned long)status;
	controlInterface.statusSize = configPLCInterface.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	StCoreRunSystemControl();
	StCoreRunSectionControl();
	
	SuperTrakProcessControl(0, &controlInterface);
	SuperTrakCyclic1();
	SuperTrakProcessStatus(0, &controlInterface);
	
	return 0;
	
} /* Function definition */
