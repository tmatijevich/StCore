/*******************************************************************************
 * File: StCore\Pallet.c
 * Author: Tyler Matijevich
 * Date: 2022-05-19
*******************************************************************************/

#include "Main.h"

/* Get pallet status */
long StCorePalletStatus(unsigned char Pallet, StCorePalletStatusType *Status) {
	
	/*********************** 
	 Declare Local Variables
	***********************/
	SuperTrakPalletInfo_t *pPalletData;
	static long timestamp;
	static short velocity[UCHAR_MAX];
	static unsigned short destination[UCHAR_MAX];
	static unsigned short setSection[UCHAR_MAX];
	static long setPosition[UCHAR_MAX];
	static float setVelocity[UCHAR_MAX];
	static float setAcceleration[UCHAR_MAX];
	
	/*****
	 Clear
	*****/
	memset(Status, 0, sizeof(*Status));
	
	/******
	 Verify
	******/
	/* Check reference */
	if(core.pPalletData == NULL)
		return stCORE_ERROR_ALLOC;
		
	/* Check ID mapping */
	if(core.palletMap[Pallet] == -1)
		return stCORE_ERROR_PALLET;
		
	/******
	 Status
	******/
	/* Access the pallet information structure */
	pPalletData = core.pPalletData + core.palletMap[Pallet];
	
	/* Copy data */
	Status->Present = GET_BIT(pPalletData->status, stPALLET_PRESENT);
	Status->Recovering = GET_BIT(pPalletData->status, stPALLET_RECOVERING);
	Status->AtTarget = GET_BIT(pPalletData->status, stPALLET_AT_TARGET);
	Status->InPosition = GET_BIT(pPalletData->status, stPALLET_IN_POSITION);
	Status->ServoEnabled = GET_BIT(pPalletData->status, stPALLET_SERVO_ENABLED);
	Status->Initializing = GET_BIT(pPalletData->status, stPALLET_INITIALIZING);
	Status->Lost = GET_BIT(pPalletData->status, stPALLET_LOST);
	
	Status->Section = pPalletData->section;
	Status->Position = ((double)pPalletData->position) / 1000.0;
	Status->Info.PositionUm = pPalletData->position;
	Status->Info.ControlMode = pPalletData->controlMode;
	
	/* Wait for new scan to read parameters */
	if(timestamp != AsIOTimeCyclicStart()) {
		timestamp = AsIOTimeCyclicStart();
		SuperTrakServChanRead(0, 1314, 0, core.palletCount, (unsigned long)&velocity, sizeof(velocity));
		SuperTrakServChanRead(0, 1339, 0, core.palletCount, (unsigned long)&destination, sizeof(destination));
		SuperTrakServChanRead(0, 1306, 0, core.palletCount, (unsigned long)&setSection, sizeof(setSection));
		SuperTrakServChanRead(0, 1311, 0, core.palletCount, (unsigned long)&setPosition, sizeof(setPosition));
		SuperTrakServChanRead(0, 1313, 0, core.palletCount, (unsigned long)&setVelocity, sizeof(setVelocity));
		SuperTrakServChanRead(0, 1312, 0, core.palletCount, (unsigned long)&setAcceleration, sizeof(setAcceleration));
	}
	
	/* Copy data */
	Status->Info.Velocity = (float)velocity[core.palletMap[Pallet]];
	Status->Info.DestinationTarget = (unsigned char)destination[core.palletMap[Pallet]];
	Status->Info.SetSection = (unsigned char)setSection[core.palletMap[Pallet]];
	Status->Info.SetPosition = ((double)setPosition[core.palletMap[Pallet]]) / 1000.0;
	Status->Info.SetPositionUm = setPosition[core.palletMap[Pallet]];
	Status->Info.SetVelocity = setVelocity[core.palletMap[Pallet]];
	Status->Info.SetAcceleration = setAcceleration[core.palletMap[Pallet]] * 1000.0;
	
	return 0;
	
} /* End function */
