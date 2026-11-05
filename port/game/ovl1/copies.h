// OVL1's copies of functions of the resident code, OVL2, OVL3 and OVL4: the
// src/sim/ovl1 files that #include the original sources, and the OVL1 sources
// that call the copies, include this first, so that the original names are the
// OVL1 ones. timload2.cpp and timupload2.cpp name their second copies themselves.
#ifndef OVL1_COPIES_H
#define OVL1_COPIES_H

// Resident code: negconcalib.cpp.
#define Options_ApplyNeGconCalib Options_Apply

// OVL2: timupload.cpp.
#define Tim_UploadToPage_OVL2 Gpu_UploadImageBlock

// OVL3: initevent.cpp (minmax.cpp and randompick.cpp name theirs themselves).
#define RaceSetup_InitEvent RaceSetup_BuildArcade

// OVL4: timload.cpp, ghostfile.cpp.
#define Tim_LoadFile VolId_ReadFileAndGetSize
#define Event_GetGhostFileId GhostEvent_GetFileId

#endif
