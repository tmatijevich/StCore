# StCore

The StCore library is a full set of core function to interface with SuperTrak (St).  The functions are designed to be robust, lightweight, and user friendly.  This library is only intended for use with B&R Automation Studio and is provided as-is under the Apache 2.0 License.

Programming the SuperTrak is best understood when orgnized in four components: System, Section, Target, and Pallet.  The system is the collection of all sections, targets, and pallets. Sections are the motor drive hardware assembled to form the SuperTrak.  Targets are a software definition representating a point on the SuperTrak workspace.  Finally, pallets are the product carrier hardware controlled by the sections in the SuperTrak workspace.

[Download the library here](https://github.com/tmatijevich/StCore/releases/latest/download/StCore.zip).

## Features

- Dynamic sizing of sections and targets
- Automatic section and pallet mapping
- Motion commands from functions or function blocks
- Command buffering per pallet
- Standard, extended, and diagnostic information for all objects
- Extensive logging with fault and warning context
- Robust error handling
- Network IO functions
- PLC communication protocal revision 3.0
