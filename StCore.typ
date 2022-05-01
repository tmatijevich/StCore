(*******************************************************************************
 * File: StCore.typ
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************)

TYPE
	StCoreSystemInfoType : 	STRUCT  (*Extended system information*)
		Warnings : UDINT; (*(Par 1460) Active SuperTrak system warnings*)
		Faults : UDINT; (*(Par 1460) Active SuperTrak system faults*)
		SectionCount : USINT; (*(Par 1080) Section count*)
		Enabled : BOOL; (*(Derived) All sections are enabled*)
		Disabled : BOOL; (*(Derived) All sections are disabled*)
		MotorPower : BOOL; (*(Derived) All sections' motor voltage within range*)
		DisabledExternally : BOOL; (*(Derived) One or more section disabled from external condition*)
		SectionWarningPresent : BOOL; (*(Derived) One or more active SuperTrak section warning*)
		SectionFaultPresent : BOOL; (*(Derived) One or more active SuperTrak section fault*)
		PalletRecoveringCount : USINT; (*(Derived) Total number of pallets reporting recovering status*)
		PalletInitializingCount : USINT; (*(Derived) Total number of palelts reporting initializing (moving to load target) status*)
	END_STRUCT;
	StCoreSystemInternalType : 	STRUCT  (*Local internal system information*)
		State : USINT; (*Execution state*)
		PreviousErrorReset : BOOL; (*Previous ErrorReset value*)
	END_STRUCT;
	StCoreSectionInfoType : 	STRUCT  (*Extended section information*)
		Warnings : UDINT; (*(Par 1480) Active SuperTrak section warnings*)
		Faults : UDINT; (*(Par 1480) Active SuperTrak section faults*)
		PalletCount : USINT; (*(Par 1502) Number of pallets on the section*)
	END_STRUCT;
	StCoreSectionInternalType : 	STRUCT  (*Local internal section information*)
		State : USINT; (*Execution state*)
		PreviousErrorReset : BOOL; (*Previous ErrorReset value*)
	END_STRUCT;
	StCoreFunctionInternalType : 	STRUCT  (*Local internal function information*)
		State : USINT; (*Execution state*)
		Select : USINT; (*Select index (Section, Target, or Pallet)*)
		PreviousSelect : USINT; (*Previous select index value*)
		PreviousErrorReset : BOOL; (*Previous ErrorReset value*)
		PreviousCommand : UINT; (*Previous command inputs bitwise*)
		CommandSelect : USINT; (*Active command index*)
		pCommand : UDINT; (*Address of buffered core command*)
	END_STRUCT;
	StCoreTargetParameterType : 	STRUCT  (*Target interface parameters*)
		Release : StCoreReleaseParameterType; (*Release command parameters*)
		PalletID : USINT := 1; (*Pallet ID to assign*)
		Motion : StCoreMotionParameterType; (*Motion parameters for configuration command*)
		Mechanical : StCoreMechanicalParameterType; (*Mechanical parameters for configuration command*)
		Control : StCoreControlParameterType; (*Control parameters for configuration command*)
	END_STRUCT;
	StCoreReleaseParameterType : 	STRUCT  (*Release command parameter structure*)
		Direction : UINT := stDIRECTION_RIGHT; (*Direction of motion (stDIRECTION_RIGHT or stDIRECTION_LEFT)*)
		DestinationTarget : USINT := 1; (*Destination target*)
		Offset : LREAL; (*(mm) [-500, 500] Target offset or incremental offset*)
	END_STRUCT;
	StCoreMotionParameterType : 	STRUCT  (*Motion parameter structure*)
		Velocity : REAL; (*(mm/s) [5, 4000] Set velocity*)
		Acceleration : REAL; (*(mm/s/s) [500, 60000] Set acceleration and deceleration*)
	END_STRUCT;
	StCoreMechanicalParameterType : 	STRUCT  (*Mechanical parameter structure*)
		ShelfWidth : LREAL; (*(mm) [152, 600] Physical pallet shelf length*)
		CenterOffset : LREAL; (*(mm) Physical pallet shelf center offset*)
	END_STRUCT;
	StCoreControlParameterType : 	STRUCT  (*Control parameter structure*)
		ControlGainSet : USINT; (*[0, 15] Select control gain set*)
		MovingFilter : REAL := 0.5; (*[0, 1) Moving control filter weight*)
		StationaryFilter : REAL := 0.5; (*[0, 1) Stationary control filter weight*)
	END_STRUCT;
	StCoreTargetInfoType : 	STRUCT  (*Extended target information*)
		Section : USINT; (*(Par 1650) Target section number*)
		Position : LREAL; (*mm (Derived) Target section position*)
		PositionUm : DINT; (*um (Par 1651) Target integer section position*)
		PalletCount : USINT; (*(Derived) The number of pallets destined to this target*)
	END_STRUCT;
	StCoreTargetStatusType : 	STRUCT  (*Target status information*)
		PalletPresent : BOOL; (*(IF) A pallet has arrived, entered the in-position window, and is not yet released*)
		PalletInPosition : BOOL; (*(IF) A pallet is currently within the in-position window*)
		PalletPreArrival : BOOL; (*(IF) A pallet is expected to arrive at the stop within the configured time*)
		PalletOverTarget : BOOL; (*(IF) A pallet's shelf is over the target (requires configuration to report status)*)
		PalletPositionUncertain : BOOL; (*(IF) A pallet has arrived but is not reporting in-position*)
		PalletID : USINT; (*(IF) ID of the pallet present at the target*)
		Info : StCoreTargetInfoType; (*Extended target status information*)
	END_STRUCT;
END_TYPE
