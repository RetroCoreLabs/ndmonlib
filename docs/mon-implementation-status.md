# MON Call Implementation Status

> **Auto-generated** by `tools/generate_mon_status.py`. Do not edit by hand - regenerate with:
> ```bash
> python3 tools/generate_mon_status.py
> ```

Generated: 2026-09-28

Status is taken from the authoritative registration table in `src/core/mon_registry.c` (the status the dispatcher acts on at runtime), cross-checked against each handler's C source.

## Summary

| Status | Count | Share |
|--------|-------|-------|
| Validated (OK) | 45 | 19.2% |
| In progress (WIP) | 17 | 7.3% |
| Stub (not implemented) (STUB) | 172 | 73.5% |
| **Total** | **234** | 100% |

Legend: **OK** = validated / working, **WIP** = in progress (partial), **STUB** = not implemented (dispatcher returns not-implemented without calling the handler).

## Validated (OK) - 45

| MON | Name | Status | LOC | Handler | Notes |
|-----|------|--------|-----|---------|-------|
| `0B` | ExitFromProgram (LEAVE) | OK | 11 | `mon_0B_ExitFromProgram` |  |
| `1B` | InByte (INBT) | OK | 98 | `mon_1B_InByte` |  |
| `2B` | OutByte (OUTBT) | OK | 64 | `mon_2B_OutByte` |  |
| `3B` | SetEcho (ECHOM) | OK | 38 | `mon_3B_SetEcho` |  |
| `4B` | SetBreak (BRKM) | OK | 43 | `mon_4B_SetBreak` |  |
| `11B` | GetBasicTime (TIME) | OK | 25 | `mon_11B_GetBasicTime` |  |
| `12B` | SetCommandBuffer (SETCM) | OK | 21 | `mon_12B_SetCommandBuffer` |  |
| `13B` | ClearInBuffer (CIBUF) | OK | 14 | `mon_13B_ClearInBuffer` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `16B` | GetTerminalType (MGTTY) | OK | 31 | `mon_16B_GetTerminalType` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed); registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `17B` | SetTerminalType (MSTTY) | OK | 20 | `mon_17B_SetTerminalType` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `30B` | GetOwnRTAddress (GETRT) | OK | 57 | `mon_30B_GetOwnRTAddress` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `32B` | OutMessage (MSG) | OK | 32 | `mon_32B_OutMessage` |  |
| `41B` | ReadObjectEntry (ROBJE) | OK | 107 | `mon_41B_ReadObjectEntry` |  |
| `43B` | CloseFile (CLOSE) | OK | 61 | `mon_43B_CloseFile` |  |
| `45B` | GetTypeRing (GTYPR) | OK | 38 | `mon_45B_GetTypeRing` |  |
| `50B` | OpenFile (OPEN) | OK | 65 | `mon_50B_OpenFile` |  |
| `52B` | TerminalMode (TERMO) | OK | 20 | `mon_52B_TerminalMode` |  |
| `54B` | DeleteFile (MDLFI) | OK | 42 | `mon_54B_DeleteFile` |  |
| `62B` | GetBytesInFile (RMAX) | OK | 42 | `mon_62B_GetBytesInFile` |  |
| `64B` | WarningMessage (ERMSG) | OK | 14 | `mon_64B_WarningMessage` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `73B` | SetMaxBytes (SMAX) | OK | 51 | `mon_73B_SetMaxBytes` |  |
| `74B` | SetStartByte (SETBT) | OK | 40 | `mon_74B_SetStartByte` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `76B` | SetBlockSize (SETBS) | OK | 40 | `mon_76B_SetBlockSize` |  |
| `113B` | GetCurrentTime (CLOCK) | OK | 37 | `mon_113B_GetCurrentTime` |  |
| `114B` | GetTimeUsed (TUSED) | OK | 17 | `mon_114B_GetTimeUsed` |  |
| `117B` | ReadFromFile (RFILE) | OK | 132 | `mon_117B_ReadFromFile` |  |
| `120B` | WriteToFile (WFILE) | OK | 116 | `mon_120B_WriteToFile` |  |
| `122B` | ReserveResource (RESRV) | OK | 35 | `mon_122B_ReserveResource` |  |
| `123B` | ReleaseResource (RELES) | OK | 35 | `mon_123B_ReleaseResource` |  |
| `142B` | ToErrorDevice (ERMON) | OK | 22 | `mon_142B_ToErrorDevice` |  |
| `143B` | ExecutionInfo (RSIO) | OK | 34 | `mon_143B_ExecutionInfo` |  |
| `221B` | CreateFile (CRALF) | OK | 63 | `mon_221B_CreateFile` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `256B` | FullFileName (DEABF) | OK | 102 | `mon_256B_FullFileName` |  |
| `262B` | GetSystemInfo (CPUST) | OK | 42 | `mon_262B_GetSystemInfo` |  |
| `312B` | CheckMonCall (MOINF) | OK | 29 | `mon_312B_CheckMonCall` |  |
| `317B` | ExecuteCommand (UECOM) | OK | 61 | `mon_317B_ExecuteCommand` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `320B` | UELogin (UELOG) | OK | 9 | `mon_320B_UELogin` |  |
| `321B` | UEAdministrator (UEADM) | OK | 25 | `mon_321B_UEAdministrator` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `327B` | FileSystemFunction (FSMTY) | OK | 73 | `mon_327B_FileSystemFunction` |  |
| `412B` | FileAsSegment (FSCNT) | OK | 72 | `mon_412B_FileAsSegment` |  |
| `413B` | FileNotAsSegment (FSCDNT) | OK | 68 | `mon_413B_FileNotAsSegment` |  |
| `416B` | SaveND500Segment (WSEGN) | OK | 30 | `mon_416B_SaveND500Segment` |  |
| `422B` | GetScratchSegment (GSWSP) | OK | 31 | `mon_422B_GetScratchSegment` |  |
| `503B` | InputString (DVINST) | OK | 279 | `mon_503B_InputString` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |
| `504B` | OutputString (DVOUTS) | OK | 118 | `mon_504B_OutputString` | registered VALIDATED but source contains caveat keywords (TODO/UNPROVEN/etc.) - verify completeness |

## In progress (WIP) - 17

| MON | Name | Status | LOC | Handler | Notes |
|-----|------|--------|-----|---------|-------|
| `22B` | OutUpTo8Bytes (M8OUT) | WIP | 48 | `mon_22B_OutUpTo8Bytes` |  |
| `24B` | Out8Bytes (B8OUT) | WIP | 73 | `mon_24B_Out8Bytes` |  |
| `35B` | OutNumber (IOUT) | WIP | 76 | `mon_35B_OutNumber` |  |
| `67B` | OutBufferSpace (OSIZE) | WIP | 31 | `mon_67B_OutBufferSpace` |  |
| `71B` | DisableEscape (DESCF) | WIP | 12 | `mon_71B_DisableEscape` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `72B` | EnableEscape (EESCF) | WIP | 12 | `mon_72B_EnableEscape` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `144B` | DeviceFunction (MAGTP) | WIP | 79 | `mon_144B_DeviceFunction` |  |
| `162B` | OutString (OUTST) | WIP | 82 | `mon_162B_OutString` |  |
| `214B` | GetUserName (GUSNA) | WIP | 31 | `mon_214B_GetUserName` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `257B` | OpenFileInfo (FOPEN) | WIP | 83 | `mon_257B_OpenFileInfo` |  |
| `263B` | GetDeviceType (GDEVT) | WIP | 40 | `mon_263B_GetDeviceType` |  |
| `300B` | SetEscapeHandling (EUSEL) | WIP | 11 | `mon_300B_SetEscapeHandling` |  |
| `301B` | StopEscapeHandling (DUSEL) | WIP | 9 | `mon_301B_StopEscapeHandling` |  |
| `313B` | InBufferState (IBRISZ) | WIP | 18 | `mon_313B_InBufferState` | stale 'AUTO-GENERATED STUB' header comment (handler is implemented; comment can be removed) |
| `336B` | Terminal (IOMTY) | WIP | 63 | `mon_336B_Terminal` |  |
| `405B` | SwitchUserBreak (USTRK) | WIP | 14 | `mon_405B_SwitchUserBreak` |  |
| `511B` | DeviceInputOutput (DVIO) | WIP | 52 | `mon_511B_DVIO` |  |

## Stub (not implemented) (STUB) - 172

| MON | Name | Status | LOC | Handler | Notes |
|-----|------|--------|-----|---------|-------|
| `5B` | ReadScratchFile (RDISK) | STUB | 6 | `mon_5B_ReadScratchFile` |  |
| `6B` | WriteScratchFile (WDISK) | STUB | 7 | `mon_6B_WriteScratchFile` |  |
| `7B` | ReadBlock (RPAGE) | STUB | 7 | `mon_7B_ReadBlock` |  |
| `10B` | WriteBlock (WPAGE) | STUB | 8 | `mon_10B_WriteBlock` |  |
| `14B` | ClearOutBuffer (COBUF) | STUB | 6 | `mon_14B_ClearOutBuffer` |  |
| `21B` | InUpTo8Bytes (M8INB) | STUB | 6 | `mon_21B_InUpTo8Bytes` |  |
| `23B` | In8Bytes (B8INB) | STUB | 6 | `mon_23B_In8Bytes` |  |
| `26B` | GetLastByte (LASTC) | STUB | 6 | `mon_26B_GetLastByte` |  |
| `27B` | GetRTDescr (RTDSC) | STUB | 6 | `mon_27B_GetRTDescr` |  |
| `31B` | IOInstruction (EXIOX) | STUB | 7 | `mon_31B_IOInstruction` |  |
| `33B` | AltPageTable (ALTON) | STUB | 6 | `mon_33B_AltPageTable` |  |
| `34B` | NormalPageTable (ALTOFF) | STUB | 5 | `mon_34B_NormalPageTable` |  |
| `36B` | NoWaitSwitch (NOWT) | STUB | 8 | `mon_36B_NoWaitSwitch` |  |
| `37B` | ReadADChannel (AIRDW) | STUB | 7 | `mon_37B_ReadADChannel` |  |
| `40B` | CloseSpoolingFile (SPCLO) | STUB | 9 | `mon_40B_CloseSpoolingFile` |  |
| `44B` | GetUserEntry (RUSER) | STUB | 6 | `mon_44B_GetUserEntry` |  |
| `53B` | GetSegmentEntry (RSEGM) | STUB | 6 | `mon_53B_GetSegmentEntry` |  |
| `55B` | GetSpoolingEntry (RSQPE) | STUB | 6 | `mon_55B_GetSpoolingEntry` |  |
| `56B` | SetUserParam (PASET) | STUB | 6 | `mon_56B_SetUserParam` |  |
| `57B` | GetUserParam (PAGET) | STUB | 5 | `mon_57B_GetUserParam` |  |
| `61B` | MemoryAllocation (FIXC5) | STUB | 6 | `mon_61B_MemoryAllocation` |  |
| `63B` | In4x2Bytes (B41NW) | STUB | 6 | `mon_63B_In4x2Bytes` |  |
| `65B` | ErrorMessage (QERMS) | STUB | 6 | `mon_65B_ErrorMessage` |  |
| `66B` | InBufferSpace (ISIZE) | STUB | 6 | `mon_66B_InBufferSpace` |  |
| `70B` | CallCommand (COMMND) | STUB | 6 | `mon_70B_CallCommand` |  |
| `75B` | GetStartByte (REABT) | STUB | 6 | `mon_75B_GetStartByte` |  |
| `77B` | SetStartBlock (SETBL) | STUB | 7 | `mon_77B_SetStartBlock` |  |
| `100B` | StartRTProgram (RT) | STUB | 6 | `mon_100B_StartRTProgram` |  |
| `101B` | DelayStart (SET) | STUB | 8 | `mon_101B_DelayStart` |  |
| `102B` | StartupTime (ABSET) | STUB | 9 | `mon_102B_StartupTime` |  |
| `103B` | StartupInterval (INTV) | STUB | 8 | `mon_103B_StartupInterval` |  |
| `104B` | SuspendProgram (HOLD) | STUB | 7 | `mon_104B_SuspendProgram` |  |
| `105B` | StopRTProgram (ABORT) | STUB | 6 | `mon_105B_StopRTProgram` |  |
| `106B` | StartOnInterrupt (CONCT) | STUB | 7 | `mon_106B_StartOnInterrupt` |  |
| `107B` | NoInterruptStart (DSCNT) | STUB | 6 | `mon_107B_NoInterruptStart` |  |
| `110B` | SetRTPriority (PRIOR) | STUB | 7 | `mon_110B_SetRTPriority` |  |
| `111B` | SetClock (UPDAT) | STUB | 10 | `mon_111B_SetClock` |  |
| `112B` | AdjustClock (CLADJ) | STUB | 7 | `mon_112B_AdjustClock` |  |
| `115B` | FixScattered (FIX) | STUB | 6 | `mon_115B_FixScattered` |  |
| `116B` | UnfixSegment (UNFIX) | STUB | 6 | `mon_116B_UnfixSegment` |  |
| `121B` | AwaitFileTransfer (WAITF) | STUB | 7 | `mon_121B_AwaitFileTransfer` |  |
| `124B` | ForceReserve (PRSRV) | STUB | 8 | `mon_124B_ForceReserve` |  |
| `125B` | ForceRelease (PRLRS) | STUB | 7 | `mon_125B_ForceRelease` |  |
| `126B` | ExactDelayStart (DSET) | STUB | 7 | `mon_126B_ExactDelayStart` |  |
| `127B` | ExactStartup (DABST) | STUB | 7 | `mon_127B_ExactStartup` |  |
| `130B` | ExactInterval (DINTV) | STUB | 7 | `mon_130B_ExactInterval` |  |
| `131B` | DataTransfer (ABSTR) | STUB | 10 | `mon_131B_DataTransfer` |  |
| `132B` | JumpToSegment (MCALL) | STUB | 7 | `mon_132B_JumpToSegment` |  |
| `133B` | ExitFromSegment (MEXIT) | STUB | 6 | `mon_133B_ExitFromSegment` |  |
| `134B` | ExitRTProgram (RTEXT) | STUB | 5 | `mon_134B_ExitRTProgram` |  |
| `135B` | WaitForRestart (RTWT) | STUB | 5 | `mon_135B_WaitForRestart` |  |
| `136B` | EnableRTStart (RTON) | STUB | 6 | `mon_136B_EnableRTStart` |  |
| `137B` | DisableRTStart (RTOFF) | STUB | 6 | `mon_137B_DisableRTStart` |  |
| `140B` | ReservationInfo (WHDEV) | STUB | 7 | `mon_140B_ReservationInfo` |  |
| `141B` | DeviceControl (IOSET) | STUB | 9 | `mon_141B_DeviceControl` |  |
| `146B` | PrivInstruction (IPRIV) | STUB | 6 | `mon_146B_PrivInstruction` |  |
| `147B` | CAMACFunction (CAMAC) | STUB | 10 | `mon_147B_CAMACFunction` |  |
| `150B` | CAMACGLRegister (GL) | STUB | 7 | `mon_150B_CAMACGLRegister` |  |
| `151B` | GetRTAddress (GRTDA) | STUB | 6 | `mon_151B_GetRTAddress` |  |
| `152B` | GetRTName (GRTNA) | STUB | 6 | `mon_152B_GetRTName` |  |
| `153B` | CAMACIOInstruction (IOXN) | STUB | 7 | `mon_153B_CAMACIOInstruction` |  |
| `154B` | AssignCAMACLAM (ASSIG) | STUB | 8 | `mon_154B_AssignCAMACLAM` |  |
| `155B` | GraphicFunction (GRAPH) | STUB | 10 | `mon_155B_GraphicFunction` |  |
| `157B` | SegmentToPageTable (ENTSG) | STUB | 9 | `mon_157B_SegmentToPageTable` |  |
| `160B` | FixContiguous (FIXC) | STUB | 7 | `mon_160B_FixContiguous` |  |
| `161B` | InString (INSTR) | STUB | 8 | `mon_161B_InString` |  |
| `164B` | SaveSegment (WSEG) | STUB | 6 | `mon_164B_SaveSegment` |  |
| `165B` | GetInRegisters (DIW) | STUB | 8 | `mon_165B_GetInRegisters` |  |
| `167B` | AttachSegment (REENT) | STUB | 6 | `mon_167B_AttachSegment` |  |
| `170B` | UserDef0 (US0) | STUB | 5 | `mon_170B_UserDef0` |  |
| `171B` | UserDef1 (US1) | STUB | 5 | `mon_171B_UserDef1` |  |
| `172B` | UserDef2 (US2) | STUB | 5 | `mon_172B_UserDef2` |  |
| `173B` | UserDef3 (US3) | STUB | 5 | `mon_173B_UserDef3` |  |
| `174B` | UserDef4 (US4) | STUB | 5 | `mon_174B_UserDef4` |  |
| `175B` | UserDef5 (US5) | STUB | 5 | `mon_175B_UserDef5` |  |
| `176B` | UserDef6 (US6) | STUB | 5 | `mon_176B_UserDef6` |  |
| `177B` | UserDef7 (US7) | STUB | 5 | `mon_177B_UserDef7` |  |
| `200B` | XMSGFunction (XMSG) | STUB | 5 | `mon_200B_XMSGFunction` |  |
| `201B` | HDLCfunction (MHDLC) | STUB | 10 | `mon_201B_HDLCfunction` |  |
| `206B` | TerminationHandling (EDTRM) | STUB | 7 | `mon_206B_TerminationHandling` |  |
| `207B` | GetErrorInfo (RERRP) | STUB | 5 | `mon_207B_GetErrorInfo` |  |
| `212B` | ReentrantSegment (SREEN) | STUB | 6 | `mon_212B_ReentrantSegment` |  |
| `213B` | GetDirUserIndexes (MUIDI) | STUB | 6 | `mon_213B_GetDirUserIndexes` |  |
| `215B` | GetObjectEntry (DROBJ) | STUB | 8 | `mon_215B_GetObjectEntry` |  |
| `216B` | SetObjectEntry (DWOBJ) | STUB | 10 | `mon_216B_SetObjectEntry` |  |
| `217B` | GetAllFileIndexes (GUIOI) | STUB | 6 | `mon_217B_GetAllFileIndexes` |  |
| `220B` | DirectOpen (DOPEN) | STUB | 8 | `mon_220B_DirectOpen` |  |
| `222B` | GetAddressArea (GBSIZ) | STUB | 5 | `mon_222B_GetAddressArea` |  |
| `227B` | SetEscLocalChars (MSDAE) | STUB | 8 | `mon_227B_SetEscLocalChars` |  |
| `230B` | GetEscLocalChars (MGDAE) | STUB | 6 | `mon_230B_GetEscLocalChars` |  |
| `231B` | ExpandFile (EXPFI) | STUB | 7 | `mon_231B_ExpandFile` |  |
| `232B` | RenameFile (MRNFI) | STUB | 7 | `mon_232B_RenameFile` |  |
| `233B` | SetTemporaryFile (STEFI) | STUB | 6 | `mon_233B_SetTemporaryFile` |  |
| `234B` | SetPeripheralName (SPEFI) | STUB | 7 | `mon_234B_SetPeripheralName` |  |
| `235B` | ScratchOpen (SCROP) | STUB | 8 | `mon_235B_ScratchOpen` |  |
| `236B` | SetPermanentOpen (SPERD) | STUB | 6 | `mon_236B_SetPermanentOpen` |  |
| `237B` | SetFileAccess (SFACC) | STUB | 9 | `mon_237B_SetFileAccess` |  |
| `240B` | AppendSpooling (APSPE) | STUB | 9 | `mon_240B_AppendSpooling` |  |
| `241B` | NewUser (SUSCN) | STUB | 8 | `mon_241B_NewUser` |  |
| `242B` | OldUser (RUSCN) | STUB | 5 | `mon_242B_OldUser` |  |
| `243B` | GetDirNameIndex (FDINA) | STUB | 6 | `mon_243B_GetDirNameIndex` |  |
| `244B` | GetDirEntry (GDIEN) | STUB | 6 | `mon_244B_GetDirEntry` |  |
| `245B` | GetNameEntry (GNAEN) | STUB | 6 | `mon_245B_GetNameEntry` |  |
| `246B` | ReserveDir (REDIR) | STUB | 6 | `mon_246B_ReserveDir` |  |
| `247B` | ReleaseDir (RLDIR) | STUB | 6 | `mon_247B_ReleaseDir` |  |
| `250B` | GetDefaultDir (FDFDI) | STUB | 6 | `mon_250B_GetDefaultDir` |  |
| `251B` | CopyPage (COPAG) | STUB | 9 | `mon_251B_CopyPage` |  |
| `252B` | BackupClose (BCLOS) | STUB | 7 | `mon_252B_BackupClose` |  |
| `253B` | NewFileVersion (CRALN) | STUB | 8 | `mon_253B_NewFileVersion` |  |
| `254B` | GetErrorDevice (GERDV) | STUB | 5 | `mon_254B_GetErrorDevice` |  |
| `255B` | PIOCFunction (PIOCM) | STUB | 11 | `mon_255B_PIOCFunction` |  |
| `267B` | TimeOut (TMOUT) | STUB | 7 | `mon_267B_TimeOut` |  |
| `270B` | ReadDiskPage (RDPAG) | STUB | 8 | `mon_270B_ReadDiskPage` |  |
| `271B` | WriteDiskPage (WDPAG) | STUB | 9 | `mon_271B_WriteDiskPage` |  |
| `272B` | DeletePage (DELPG) | STUB | 8 | `mon_272B_DeletePage` |  |
| `273B` | GetFileName (MGFIL) | STUB | 10 | `mon_273B_GetFileName` |  |
| `274B` | GetFileIndexes (FOBJN) | STUB | 7 | `mon_274B_GetFileIndexes` |  |
| `275B` | SetTerminalName (STRFI) | STUB | 6 | `mon_275B_SetTerminalName` |  |
| `276B` | EnableLocal (ELOFU) | STUB | 6 | `mon_276B_EnableLocal` |  |
| `277B` | DisableLocal (DLOFU) | STUB | 5 | `mon_277B_DisableLocal` |  |
| `302B` | OnEscLocalFunction (ELON) | STUB | 5 | `mon_302B_OnEscLocalFunction` |  |
| `303B` | OffEscLocalFunction (ELOFF) | STUB | 5 | `mon_303B_OffEscLocalFunction` |  |
| `306B` | GetTerminalMode (GTMOD) | STUB | 6 | `mon_306B_GetTerminalMode` |  |
| `307B` | TerminalNoWait (TNOWAI) | STUB | 8 | `mon_307B_TerminalNoWait` |  |
| `310B` | In8AndFlag (TBIN8) | STUB | 6 | `mon_310B_In8AndFlag` |  |
| `311B` | WriteDirEntry (WDIEN) | STUB | 7 | `mon_311B_WriteDirEntry` |  |
| `314B` | DefaultRemoteSystem (SRUSI) | STUB | 9 | `mon_314B_DefaultRemoteSystem` |  |
| `315B` | LAMUFunction (MLAMU) | STUB | 9 | `mon_315B_LAMUFunction` |  |
| `316B` | SetRemoteAccess (SRLMO) | STUB | 6 | `mon_316B_SetRemoteAccess` |  |
| `322B` | GetSegmentNo (GSGNO) | STUB | 6 | `mon_322B_GetSegmentNo` |  |
| `323B` | SegmentOverlay (SPLRE) | STUB | 11 | `mon_323B_SegmentOverlay` |  |
| `324B` | OctobusFunction (OCTIO) | STUB | 8 | `mon_324B_OctobusFunction` |  |
| `325B` | BatchModeEcho (MBECH) | STUB | 5 | `mon_325B_BatchModeEcho` |  |
| `326B` | LogInStart (MLOGI) | STUB | 11 | `mon_326B_LogInStart` |  |
| `330B` | TerminalStatus (TERST) | STUB | 6 | `mon_330B_TerminalStatus` |  |
| `332B` | TerminalLineInfo (TREPP) | STUB | 7 | `mon_332B_TerminalLineInfo` |  |
| `333B` | DMAFunction (UDMA) | STUB | 9 | `mon_333B_DMAFunction` |  |
| `334B` | GetErrorMessage (GETXM) | STUB | 6 | `mon_334B_GetErrorMessage` |  |
| `335B` | TransferData (EXABS) | STUB | 10 | `mon_335B_TransferData` |  |
| `337B` | ChangeSegment (SPCHG) | STUB | 5 | `mon_337B_ChangeSegment` |  |
| `340B` | ReadSystemRecord (RSREC) | STUB | 8 | `mon_340B_ReadSystemRecord` |  |
| `341B` | SegmentFunction (SGMTY) | STUB | 10 | `mon_341B_SegmentFunction` |  |
| `400B` | ErrorReturn (MACROE) | STUB | 5 | `mon_400B_ErrorReturn` |  |
| `401B` | DisAssemble (DISASS) | STUB | 7 | `mon_401B_DisAssemble` |  |
| `402B` | GetInputFlags (RFLAG) | STUB | 5 | `mon_402B_GetInputFlags` |  |
| `403B` | SetOutputFlags (WFLAG) | STUB | 6 | `mon_403B_SetOutputFlags` |  |
| `404B` | FixIOArea (IOFIX) | STUB | 7 | `mon_404B_FixIOArea` |  |
| `406B` | AccessRTCommon (RWRTC) | STUB | 9 | `mon_406B_AccessRTCommon` |  |
| `410B` | FixInMemory (FIXMEM) | STUB | 9 | `mon_410B_FixInMemory` |  |
| `411B` | MemoryUnfix (UNFIXM) | STUB | 6 | `mon_411B_MemoryUnfix` |  |
| `414B` | BCNAFCAMAC (BCNAF) | STUB | 8 | `mon_414B_BCNAFCAMAC` |  |
| `415B` | BCNAF1CAMAC (BCNAF1) | STUB | 8 | `mon_415B_BCNAF1CAMAC` |  |
| `417B` | MaxPagesInMemory (MXPISG) | STUB | 8 | `mon_417B_MaxPagesInMemory` |  |
| `420B` | GetUserRegisters (GRBLK) | STUB | 5 | `mon_420B_GetUserRegisters` |  |
| `421B` | GetActiveSegment (GASGM) | STUB | 5 | `mon_421B_GetActiveSegment` |  |
| `423B` | CopyCapability (CAPCOP) | STUB | 10 | `mon_423B_CopyCapability` |  |
| `424B` | ClearCapability (CAPCLE) | STUB | 7 | `mon_424B_ClearCapability` |  |
| `425B` | SetProcessName (SPRNAM) | STUB | 6 | `mon_425B_SetProcessName` |  |
| `426B` | GetProcessNo (GPRNAM) | STUB | 6 | `mon_426B_GetProcessNo` |  |
| `427B` | GetOwnProcessInfo (GPRNME) | STUB | 5 | `mon_427B_GetOwnProcessInfo` |  |
| `430B` | TranslateAddress (ADR100) | STUB | 6 | `mon_430B_TranslateAddress` |  |
| `431B` | AwaitTransfer (MWAITF) | STUB | 7 | `mon_431B_AwaitTransfer` |  |
| `435B` | ForceTrap (PRT) | STUB | 7 | `mon_435B_ForceTrap` |  |
| `436B` | SetND500Param (5PASET) | STUB | 6 | `mon_436B_SetND500Param` |  |
| `437B` | GetND500Param (5PAGET) | STUB | 5 | `mon_437B_GetND500Param` |  |
| `440B` | Attach500Segment (AT5SGM) | STUB | 15 | `mon_440B_Attach500Segment` |  |
| `500B` | StartProcess (STARTP) | STUB | 6 | `mon_500B_StartProcess` |  |
| `501B` | StopProcess (STOPPR) | STUB | 5 | `mon_501B_StopProcess` |  |
| `502B` | SwitchProcess (SWITCHP) | STUB | 6 | `mon_502B_SwitchProcess` |  |
| `505B` | GetTrapReason (GERRCOD) | STUB | 5 | `mon_505B_GetTrapReason` |  |
| `507B` | SetProcessPriority (SPRIO) | STUB | 6 | `mon_507B_SetProcessPriority` |  |
| `514B` | ND500TimeOut (5TMOUT) | STUB | 7 | `mon_514B_ND500TimeOut` |  |

