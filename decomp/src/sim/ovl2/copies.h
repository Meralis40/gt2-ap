// OVL2's copies of functions of the resident code, OVL4 and OVL5: the
// src/sim/ovl2 files that #include the original sources, and the OVL2 sources
// that call the copies, include this first, so that the original names are the
// OVL2 ones.
#ifndef OVL2_COPIES_H
#define OVL2_COPIES_H

// Resident code: menuquad.cpp, grounddisc.cpp, lerp.cpp.
#define Menu_AddGreyBand Menu_AddGreyBand_OVL2
#define Menu_AddGouraudQuad Menu_AddGouraudQuad_OVL2
#define GroundDisc_Init GroundDisc_Init_OVL2
#define GroundDisc_Draw GroundDisc_Draw_OVL2
#define Lerp_Clamped Lerp_Clamped_OVL2

// OVL4: timload.cpp, menubgmbank.cpp.
#define Tim_LoadFile Tim_LoadFile_OVL2
#define MenuBgmBank_Init MenuBgmBank_Init_OVL2
#define FUN_OVL4__800226f4 FUN_OVL2__80025668
#define MenuBgmBank_Load MenuBgmBank_Load_OVL2
#define MenuBgmBank_LoadSeq MenuBgmBank_LoadSeq_OVL2
#define MenuBgmBank_Play MenuBgmBank_Play_OVL2
#define MenuBgmBank_Stop MenuBgmBank_Stop_OVL2

// OVL5: the movie player, from movieset.cpp to decodevlc.cpp, playing
// g_ArcStrPlayer's player. g_Movie is OVL2's, not defined by movieset.cpp, and
// D_80052908 is the VLC table's place in OVL2.
#define MOVIE_PLAYER_EXTERN
#define g_Movie g_ArcStrPlayer
#define D_800114CC D_80052908
#define Movie_Init Movie_Init_OVL2
#define Movie_Shutdown Movie_Shutdown_OVL2
#define Movie_SetRing Movie_SetRing_OVL2
#define Movie_SetVlcBuffers Movie_SetVlcBuffers_OVL2
#define Movie_SetSliceBuffers Movie_SetSliceBuffers_OVL2
#define Movie_Open Movie_Open_OVL2
#define Movie_SetCdRange Movie_SetCdRange_OVL2
#define Movie_Start Movie_Start_OVL2
#define Movie_GetStatus Movie_GetStatus_OVL2
#define Movie_Stop Movie_Stop_OVL2
#define Movie_RestartRead Movie_RestartRead_OVL2
#define Movie_StartMdec Movie_StartMdec_OVL2
#define Movie_DecodeNext Movie_DecodeNext_OVL2
#define Movie_Step Movie_Step_OVL2
#define Movie_DecodeThenStart Movie_DecodeThenStart_OVL2
#define Movie_CdSectorCb Movie_CdSectorCb_OVL2
#define Movie_OnSector Movie_OnSector_OVL2
#define Movie_MdecOutCb Movie_MdecOutCb_OVL2
#define Movie_OnMdecSlice Movie_OnMdecSlice_OVL2
#define Movie_GpuDmaIrq Movie_GpuDmaIrq_OVL2
#define Movie_OnGpuDma Movie_OnGpuDma_OVL2
#define MovieStream_Clear MovieStream_Clear_OVL2
#define MovieStream_Start MovieStream_Start_OVL2
#define MovieStream_Stop MovieStream_Stop_OVL2
#define MovieStream_ReadSector MovieStream_ReadSector_OVL2
#define MovieStream_PopFrame MovieStream_PopFrame_OVL2
#define MovieStream_ReleaseFrame MovieStream_ReleaseFrame_OVL2
#define MdecOut_Init MdecOut_Init_OVL2
#define MdecOut_SetSize MdecOut_SetSize_OVL2
#define MdecOut_Start MdecOut_Start_OVL2
#define MdecOut_Stop MdecOut_Stop_OVL2
#define Movie_DecodeVlc Movie_DecodeVlc_OVL2

#endif
