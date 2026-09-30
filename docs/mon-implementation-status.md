# MON Call Implementation Status

> **Auto-generated** by `tools/generate_mon_status.py`. Do not edit by hand - regenerate with:
> ```bash
> python3 tools/generate_mon_status.py
> ```

Generated: 2026-09-30

Status is taken from the authoritative registration table in `src/core/mon_registry.c` (the status the dispatcher acts on at runtime), cross-checked against each handler's C source.

## Summary

| Status | Count | Share |
|--------|-------|-------|
| Validated | 45 | 19.2% |
| In progress | 17 | 7.3% |
| Stub | 172 | 73.5% |
| **Total** | **234** | 100% |

Status values:

- **Validated** - registered `MON_STATUS_VALIDATED`: tested and working.
- **In progress** - registered `MON_STATUS_IN_PROGRESS`: partly implemented.
- **Stub** - registered `MON_STATUS_NOT_IMPLEMENTED`: the dispatcher in `src/core/mon_dispatch.c` returns not-implemented without calling the handler.

Table columns:

- **MON** - monitor call number in octal (the `B` suffix means octal).
- **Name** - the call's name in the manual *SINTRAN III Monitor Calls* (ND-860228.2 EN), with its short mnemonic in parentheses.
- **Status** - see above.
- **Code lines** - number of lines in the handler's `.c` file that are neither blank nor comment lines. It includes `#include` lines, declarations and braces, so it is only a rough size: the stubs have 5 to 15, and it says nothing about whether the code is correct.
- **Handler** - the C function; its source is `src/handlers/<Handler>.c`.
- **Notes** - problems the generator found by comparing the registered status with the handler source.

## Validated - 45

| MON | Name | Status | Code lines | Handler | Notes |
|-----|------|--------|------------|---------|-------|
| `0B` | ExitFromProgram (LEAVE) | Validated | 11 | `mon_0B_ExitFromProgram` |  |
| `1B` | InByte (INBT) | Validated | 98 | `mon_1B_InByte` |  |
| `2B` | OutByte (OUTBT) | Validated | 64 | `mon_2B_OutByte` |  |
| `3B` | SetEcho (ECHOM) | Validated | 38 | `mon_3B_SetEcho` |  |
| `4B` | SetBreak (BRKM) | Validated | 43 | `mon_4B_SetBreak` |  |
| `11B` | GetBasicTime (TIME) | Validated | 25 | `mon_11B_GetBasicTime` |  |
| `12B` | SetCommandBuffer (SETCM) | Validated | 21 | `mon_12B_SetCommandBuffer` |  |
| `13B` | ClearInBuffer (CIBUF) | Validated | 14 | `mon_13B_ClearInBuffer` |  |
| `16B` | GetTerminalType (MGTTY) | Validated | 31 | `mon_16B_GetTerminalType` |  |
| `17B` | SetTerminalType (MSTTY) | Validated | 20 | `mon_17B_SetTerminalType` |  |
| `30B` | GetOwnRTAddress (GETRT) | Validated | 57 | `mon_30B_GetOwnRTAddress` | registered VALIDATED but the source says 'TODO' - check whether the call is complete |
| `32B` | OutMessage (MSG) | Validated | 32 | `mon_32B_OutMessage` |  |
| `41B` | ReadObjectEntry (ROBJE) | Validated | 107 | `mon_41B_ReadObjectEntry` |  |
| `43B` | CloseFile (CLOSE) | Validated | 61 | `mon_43B_CloseFile` |  |
| `45B` | GetTypeRing (GTYPR) | Validated | 38 | `mon_45B_GetTypeRing` |  |
| `50B` | OpenFile (OPEN) | Validated | 65 | `mon_50B_OpenFile` |  |
| `52B` | TerminalMode (TERMO) | Validated | 20 | `mon_52B_TerminalMode` |  |
| `54B` | DeleteFile (MDLFI) | Validated | 42 | `mon_54B_DeleteFile` |  |
| `62B` | GetBytesInFile (RMAX) | Validated | 42 | `mon_62B_GetBytesInFile` |  |
| `64B` | WarningMessage (ERMSG) | Validated | 14 | `mon_64B_WarningMessage` | registered VALIDATED but the source says 'TODO' - check whether the call is complete |
| `73B` | SetMaxBytes (SMAX) | Validated | 51 | `mon_73B_SetMaxBytes` |  |
| `74B` | SetStartByte (SETBT) | Validated | 40 | `mon_74B_SetStartByte` |  |
| `76B` | SetBlockSize (SETBS) | Validated | 40 | `mon_76B_SetBlockSize` |  |
| `113B` | GetCurrentTime (CLOCK) | Validated | 37 | `mon_113B_GetCurrentTime` |  |
| `114B` | GetTimeUsed (TUSED) | Validated | 17 | `mon_114B_GetTimeUsed` |  |
| `117B` | ReadFromFile (RFILE) | Validated | 132 | `mon_117B_ReadFromFile` |  |
| `120B` | WriteToFile (WFILE) | Validated | 116 | `mon_120B_WriteToFile` |  |
| `122B` | ReserveResource (RESRV) | Validated | 35 | `mon_122B_ReserveResource` |  |
| `123B` | ReleaseResource (RELES) | Validated | 35 | `mon_123B_ReleaseResource` |  |
| `142B` | ToErrorDevice (ERMON) | Validated | 22 | `mon_142B_ToErrorDevice` |  |
| `143B` | ExecutionInfo (RSIO) | Validated | 34 | `mon_143B_ExecutionInfo` |  |
| `221B` | CreateFile (CRALF) | Validated | 63 | `mon_221B_CreateFile` |  |
| `256B` | FullFileName (DEABF) | Validated | 102 | `mon_256B_FullFileName` |  |
| `262B` | GetSystemInfo (CPUST) | Validated | 42 | `mon_262B_GetSystemInfo` |  |
| `312B` | CheckMonCall (MOINF) | Validated | 29 | `mon_312B_CheckMonCall` |  |
| `317B` | ExecuteCommand (UECOM) | Validated | 61 | `mon_317B_ExecuteCommand` |  |
| `320B` | UELogin (UELOG) | Validated | 9 | `mon_320B_UELogin` |  |
| `321B` | UEAdministrator (UEADM) | Validated | 25 | `mon_321B_UEAdministrator` | registered VALIDATED but the source says 'NOT YET' - check whether the call is complete |
| `327B` | FileSystemFunction (FSMTY) | Validated | 73 | `mon_327B_FileSystemFunction` |  |
| `412B` | FileAsSegment (FSCNT) | Validated | 72 | `mon_412B_FileAsSegment` |  |
| `413B` | FileNotAsSegment (FSCDNT) | Validated | 68 | `mon_413B_FileNotAsSegment` |  |
| `416B` | SaveND500Segment (WSEGN) | Validated | 30 | `mon_416B_SaveND500Segment` |  |
| `422B` | GetScratchSegment (GSWSP) | Validated | 31 | `mon_422B_GetScratchSegment` |  |
| `503B` | InputString (DVINST) | Validated | 279 | `mon_503B_InputString` | registered VALIDATED but the source says 'PLACEHOLDER' - check whether the call is complete |
| `504B` | OutputString (DVOUTS) | Validated | 118 | `mon_504B_OutputString` |  |

## In progress - 17

| MON | Name | Status | Code lines | Handler | Notes |
|-----|------|--------|------------|---------|-------|
| `22B` | OutUpTo8Bytes (M8OUT) | In progress | 48 | `mon_22B_OutUpTo8Bytes` |  |
| `24B` | Out8Bytes (B8OUT) | In progress | 73 | `mon_24B_Out8Bytes` |  |
| `35B` | OutNumber (IOUT) | In progress | 76 | `mon_35B_OutNumber` |  |
| `67B` | OutBufferSpace (OSIZE) | In progress | 31 | `mon_67B_OutBufferSpace` |  |
| `71B` | DisableEscape (DESCF) | In progress | 12 | `mon_71B_DisableEscape` |  |
| `72B` | EnableEscape (EESCF) | In progress | 12 | `mon_72B_EnableEscape` |  |
| `144B` | DeviceFunction (MAGTP) | In progress | 79 | `mon_144B_DeviceFunction` |  |
| `162B` | OutString (OUTST) | In progress | 82 | `mon_162B_OutString` |  |
| `214B` | GetUserName (GUSNA) | In progress | 31 | `mon_214B_GetUserName` |  |
| `257B` | OpenFileInfo (FOPEN) | In progress | 83 | `mon_257B_OpenFileInfo` |  |
| `263B` | GetDeviceType (GDEVT) | In progress | 40 | `mon_263B_GetDeviceType` |  |
| `300B` | SetEscapeHandling (EUSEL) | In progress | 11 | `mon_300B_SetEscapeHandling` |  |
| `301B` | StopEscapeHandling (DUSEL) | In progress | 9 | `mon_301B_StopEscapeHandling` |  |
| `313B` | InBufferState (IBRISZ) | In progress | 18 | `mon_313B_InBufferState` |  |
| `336B` | Terminal (IOMTY) | In progress | 63 | `mon_336B_Terminal` |  |
| `405B` | SwitchUserBreak (USTRK) | In progress | 14 | `mon_405B_SwitchUserBreak` |  |
| `511B` | DeviceInputOutput (DVIO) | In progress | 52 | `mon_511B_DVIO` |  |

## Stub - 172

| MON | Name | Status | Code lines | Handler | Notes |
|-----|------|--------|------------|---------|-------|
| `5B` | ReadScratchFile (RDISK) | Stub | 6 | `mon_5B_ReadScratchFile` |  |
| `6B` | WriteScratchFile (WDISK) | Stub | 7 | `mon_6B_WriteScratchFile` |  |
| `7B` | ReadBlock (RPAGE) | Stub | 7 | `mon_7B_ReadBlock` |  |
| `10B` | WriteBlock (WPAGE) | Stub | 8 | `mon_10B_WriteBlock` |  |
| `14B` | ClearOutBuffer (COBUF) | Stub | 6 | `mon_14B_ClearOutBuffer` |  |
| `21B` | InUpTo8Bytes (M8INB) | Stub | 6 | `mon_21B_InUpTo8Bytes` |  |
| `23B` | In8Bytes (B8INB) | Stub | 6 | `mon_23B_In8Bytes` |  |
| `26B` | GetLastByte (LASTC) | Stub | 6 | `mon_26B_GetLastByte` |  |
| `27B` | GetRTDescr (RTDSC) | Stub | 6 | `mon_27B_GetRTDescr` |  |
| `31B` | IOInstruction (EXIOX) | Stub | 7 | `mon_31B_IOInstruction` |  |
| `33B` | AltPageTable (ALTON) | Stub | 6 | `mon_33B_AltPageTable` |  |
| `34B` | NormalPageTable (ALTOFF) | Stub | 5 | `mon_34B_NormalPageTable` |  |
| `36B` | NoWaitSwitch (NOWT) | Stub | 8 | `mon_36B_NoWaitSwitch` |  |
| `37B` | ReadADChannel (AIRDW) | Stub | 7 | `mon_37B_ReadADChannel` |  |
| `40B` | CloseSpoolingFile (SPCLO) | Stub | 9 | `mon_40B_CloseSpoolingFile` |  |
| `44B` | GetUserEntry (RUSER) | Stub | 6 | `mon_44B_GetUserEntry` |  |
| `53B` | GetSegmentEntry (RSEGM) | Stub | 6 | `mon_53B_GetSegmentEntry` |  |
| `55B` | GetSpoolingEntry (RSQPE) | Stub | 6 | `mon_55B_GetSpoolingEntry` |  |
| `56B` | SetUserParam (PASET) | Stub | 6 | `mon_56B_SetUserParam` |  |
| `57B` | GetUserParam (PAGET) | Stub | 5 | `mon_57B_GetUserParam` |  |
| `61B` | MemoryAllocation (FIXC5) | Stub | 6 | `mon_61B_MemoryAllocation` |  |
| `63B` | In4x2Bytes (B41NW) | Stub | 6 | `mon_63B_In4x2Bytes` |  |
| `65B` | ErrorMessage (QERMS) | Stub | 6 | `mon_65B_ErrorMessage` |  |
| `66B` | InBufferSpace (ISIZE) | Stub | 6 | `mon_66B_InBufferSpace` |  |
| `70B` | CallCommand (COMMND) | Stub | 6 | `mon_70B_CallCommand` |  |
| `75B` | GetStartByte (REABT) | Stub | 6 | `mon_75B_GetStartByte` |  |
| `77B` | SetStartBlock (SETBL) | Stub | 7 | `mon_77B_SetStartBlock` |  |
| `100B` | StartRTProgram (RT) | Stub | 6 | `mon_100B_StartRTProgram` |  |
| `101B` | DelayStart (SET) | Stub | 8 | `mon_101B_DelayStart` |  |
| `102B` | StartupTime (ABSET) | Stub | 9 | `mon_102B_StartupTime` |  |
| `103B` | StartupInterval (INTV) | Stub | 8 | `mon_103B_StartupInterval` |  |
| `104B` | SuspendProgram (HOLD) | Stub | 7 | `mon_104B_SuspendProgram` |  |
| `105B` | StopRTProgram (ABORT) | Stub | 6 | `mon_105B_StopRTProgram` |  |
| `106B` | StartOnInterrupt (CONCT) | Stub | 7 | `mon_106B_StartOnInterrupt` |  |
| `107B` | NoInterruptStart (DSCNT) | Stub | 6 | `mon_107B_NoInterruptStart` |  |
| `110B` | SetRTPriority (PRIOR) | Stub | 7 | `mon_110B_SetRTPriority` |  |
| `111B` | SetClock (UPDAT) | Stub | 10 | `mon_111B_SetClock` |  |
| `112B` | AdjustClock (CLADJ) | Stub | 7 | `mon_112B_AdjustClock` |  |
| `115B` | FixScattered (FIX) | Stub | 6 | `mon_115B_FixScattered` |  |
| `116B` | UnfixSegment (UNFIX) | Stub | 6 | `mon_116B_UnfixSegment` |  |
| `121B` | AwaitFileTransfer (WAITF) | Stub | 7 | `mon_121B_AwaitFileTransfer` |  |
| `124B` | ForceReserve (PRSRV) | Stub | 8 | `mon_124B_ForceReserve` |  |
| `125B` | ForceRelease (PRLRS) | Stub | 7 | `mon_125B_ForceRelease` |  |
| `126B` | ExactDelayStart (DSET) | Stub | 7 | `mon_126B_ExactDelayStart` |  |
| `127B` | ExactStartup (DABST) | Stub | 7 | `mon_127B_ExactStartup` |  |
| `130B` | ExactInterval (DINTV) | Stub | 7 | `mon_130B_ExactInterval` |  |
| `131B` | DataTransfer (ABSTR) | Stub | 10 | `mon_131B_DataTransfer` |  |
| `132B` | JumpToSegment (MCALL) | Stub | 7 | `mon_132B_JumpToSegment` |  |
| `133B` | ExitFromSegment (MEXIT) | Stub | 6 | `mon_133B_ExitFromSegment` |  |
| `134B` | ExitRTProgram (RTEXT) | Stub | 5 | `mon_134B_ExitRTProgram` |  |
| `135B` | WaitForRestart (RTWT) | Stub | 5 | `mon_135B_WaitForRestart` |  |
| `136B` | EnableRTStart (RTON) | Stub | 6 | `mon_136B_EnableRTStart` |  |
| `137B` | DisableRTStart (RTOFF) | Stub | 6 | `mon_137B_DisableRTStart` |  |
| `140B` | ReservationInfo (WHDEV) | Stub | 7 | `mon_140B_ReservationInfo` |  |
| `141B` | DeviceControl (IOSET) | Stub | 9 | `mon_141B_DeviceControl` |  |
| `146B` | PrivInstruction (IPRIV) | Stub | 6 | `mon_146B_PrivInstruction` |  |
| `147B` | CAMACFunction (CAMAC) | Stub | 10 | `mon_147B_CAMACFunction` |  |
| `150B` | CAMACGLRegister (GL) | Stub | 7 | `mon_150B_CAMACGLRegister` |  |
| `151B` | GetRTAddress (GRTDA) | Stub | 6 | `mon_151B_GetRTAddress` |  |
| `152B` | GetRTName (GRTNA) | Stub | 6 | `mon_152B_GetRTName` |  |
| `153B` | CAMACIOInstruction (IOXN) | Stub | 7 | `mon_153B_CAMACIOInstruction` |  |
| `154B` | AssignCAMACLAM (ASSIG) | Stub | 8 | `mon_154B_AssignCAMACLAM` |  |
| `155B` | GraphicFunction (GRAPH) | Stub | 10 | `mon_155B_GraphicFunction` |  |
| `157B` | SegmentToPageTable (ENTSG) | Stub | 9 | `mon_157B_SegmentToPageTable` |  |
| `160B` | FixContiguous (FIXC) | Stub | 7 | `mon_160B_FixContiguous` |  |
| `161B` | InString (INSTR) | Stub | 8 | `mon_161B_InString` |  |
| `164B` | SaveSegment (WSEG) | Stub | 6 | `mon_164B_SaveSegment` |  |
| `165B` | GetInRegisters (DIW) | Stub | 8 | `mon_165B_GetInRegisters` |  |
| `167B` | AttachSegment (REENT) | Stub | 6 | `mon_167B_AttachSegment` |  |
| `170B` | UserDef0 (US0) | Stub | 5 | `mon_170B_UserDef0` |  |
| `171B` | UserDef1 (US1) | Stub | 5 | `mon_171B_UserDef1` |  |
| `172B` | UserDef2 (US2) | Stub | 5 | `mon_172B_UserDef2` |  |
| `173B` | UserDef3 (US3) | Stub | 5 | `mon_173B_UserDef3` |  |
| `174B` | UserDef4 (US4) | Stub | 5 | `mon_174B_UserDef4` |  |
| `175B` | UserDef5 (US5) | Stub | 5 | `mon_175B_UserDef5` |  |
| `176B` | UserDef6 (US6) | Stub | 5 | `mon_176B_UserDef6` |  |
| `177B` | UserDef7 (US7) | Stub | 5 | `mon_177B_UserDef7` |  |
| `200B` | XMSGFunction (XMSG) | Stub | 5 | `mon_200B_XMSGFunction` |  |
| `201B` | HDLCfunction (MHDLC) | Stub | 10 | `mon_201B_HDLCfunction` |  |
| `206B` | TerminationHandling (EDTRM) | Stub | 7 | `mon_206B_TerminationHandling` |  |
| `207B` | GetErrorInfo (RERRP) | Stub | 5 | `mon_207B_GetErrorInfo` |  |
| `212B` | ReentrantSegment (SREEN) | Stub | 6 | `mon_212B_ReentrantSegment` |  |
| `213B` | GetDirUserIndexes (MUIDI) | Stub | 6 | `mon_213B_GetDirUserIndexes` |  |
| `215B` | GetObjectEntry (DROBJ) | Stub | 8 | `mon_215B_GetObjectEntry` |  |
| `216B` | SetObjectEntry (DWOBJ) | Stub | 10 | `mon_216B_SetObjectEntry` |  |
| `217B` | GetAllFileIndexes (GUIOI) | Stub | 6 | `mon_217B_GetAllFileIndexes` |  |
| `220B` | DirectOpen (DOPEN) | Stub | 8 | `mon_220B_DirectOpen` |  |
| `222B` | GetAddressArea (GBSIZ) | Stub | 5 | `mon_222B_GetAddressArea` |  |
| `227B` | SetEscLocalChars (MSDAE) | Stub | 8 | `mon_227B_SetEscLocalChars` |  |
| `230B` | GetEscLocalChars (MGDAE) | Stub | 6 | `mon_230B_GetEscLocalChars` |  |
| `231B` | ExpandFile (EXPFI) | Stub | 7 | `mon_231B_ExpandFile` |  |
| `232B` | RenameFile (MRNFI) | Stub | 7 | `mon_232B_RenameFile` |  |
| `233B` | SetTemporaryFile (STEFI) | Stub | 6 | `mon_233B_SetTemporaryFile` |  |
| `234B` | SetPeripheralName (SPEFI) | Stub | 7 | `mon_234B_SetPeripheralName` |  |
| `235B` | ScratchOpen (SCROP) | Stub | 8 | `mon_235B_ScratchOpen` |  |
| `236B` | SetPermanentOpen (SPERD) | Stub | 6 | `mon_236B_SetPermanentOpen` |  |
| `237B` | SetFileAccess (SFACC) | Stub | 9 | `mon_237B_SetFileAccess` |  |
| `240B` | AppendSpooling (APSPE) | Stub | 9 | `mon_240B_AppendSpooling` |  |
| `241B` | NewUser (SUSCN) | Stub | 8 | `mon_241B_NewUser` |  |
| `242B` | OldUser (RUSCN) | Stub | 5 | `mon_242B_OldUser` |  |
| `243B` | GetDirNameIndex (FDINA) | Stub | 6 | `mon_243B_GetDirNameIndex` |  |
| `244B` | GetDirEntry (GDIEN) | Stub | 6 | `mon_244B_GetDirEntry` |  |
| `245B` | GetNameEntry (GNAEN) | Stub | 6 | `mon_245B_GetNameEntry` |  |
| `246B` | ReserveDir (REDIR) | Stub | 6 | `mon_246B_ReserveDir` |  |
| `247B` | ReleaseDir (RLDIR) | Stub | 6 | `mon_247B_ReleaseDir` |  |
| `250B` | GetDefaultDir (FDFDI) | Stub | 6 | `mon_250B_GetDefaultDir` |  |
| `251B` | CopyPage (COPAG) | Stub | 9 | `mon_251B_CopyPage` |  |
| `252B` | BackupClose (BCLOS) | Stub | 7 | `mon_252B_BackupClose` |  |
| `253B` | NewFileVersion (CRALN) | Stub | 8 | `mon_253B_NewFileVersion` |  |
| `254B` | GetErrorDevice (GERDV) | Stub | 5 | `mon_254B_GetErrorDevice` |  |
| `255B` | PIOCFunction (PIOCM) | Stub | 11 | `mon_255B_PIOCFunction` |  |
| `267B` | TimeOut (TMOUT) | Stub | 7 | `mon_267B_TimeOut` |  |
| `270B` | ReadDiskPage (RDPAG) | Stub | 8 | `mon_270B_ReadDiskPage` |  |
| `271B` | WriteDiskPage (WDPAG) | Stub | 9 | `mon_271B_WriteDiskPage` |  |
| `272B` | DeletePage (DELPG) | Stub | 8 | `mon_272B_DeletePage` |  |
| `273B` | GetFileName (MGFIL) | Stub | 10 | `mon_273B_GetFileName` |  |
| `274B` | GetFileIndexes (FOBJN) | Stub | 7 | `mon_274B_GetFileIndexes` |  |
| `275B` | SetTerminalName (STRFI) | Stub | 6 | `mon_275B_SetTerminalName` |  |
| `276B` | EnableLocal (ELOFU) | Stub | 6 | `mon_276B_EnableLocal` |  |
| `277B` | DisableLocal (DLOFU) | Stub | 5 | `mon_277B_DisableLocal` |  |
| `302B` | OnEscLocalFunction (ELON) | Stub | 5 | `mon_302B_OnEscLocalFunction` |  |
| `303B` | OffEscLocalFunction (ELOFF) | Stub | 5 | `mon_303B_OffEscLocalFunction` |  |
| `306B` | GetTerminalMode (GTMOD) | Stub | 6 | `mon_306B_GetTerminalMode` |  |
| `307B` | TerminalNoWait (TNOWAI) | Stub | 8 | `mon_307B_TerminalNoWait` |  |
| `310B` | In8AndFlag (TBIN8) | Stub | 6 | `mon_310B_In8AndFlag` |  |
| `311B` | WriteDirEntry (WDIEN) | Stub | 7 | `mon_311B_WriteDirEntry` |  |
| `314B` | DefaultRemoteSystem (SRUSI) | Stub | 9 | `mon_314B_DefaultRemoteSystem` |  |
| `315B` | LAMUFunction (MLAMU) | Stub | 9 | `mon_315B_LAMUFunction` |  |
| `316B` | SetRemoteAccess (SRLMO) | Stub | 6 | `mon_316B_SetRemoteAccess` |  |
| `322B` | GetSegmentNo (GSGNO) | Stub | 6 | `mon_322B_GetSegmentNo` |  |
| `323B` | SegmentOverlay (SPLRE) | Stub | 11 | `mon_323B_SegmentOverlay` |  |
| `324B` | OctobusFunction (OCTIO) | Stub | 8 | `mon_324B_OctobusFunction` |  |
| `325B` | BatchModeEcho (MBECH) | Stub | 5 | `mon_325B_BatchModeEcho` |  |
| `326B` | LogInStart (MLOGI) | Stub | 11 | `mon_326B_LogInStart` |  |
| `330B` | TerminalStatus (TERST) | Stub | 6 | `mon_330B_TerminalStatus` |  |
| `332B` | TerminalLineInfo (TREPP) | Stub | 7 | `mon_332B_TerminalLineInfo` |  |
| `333B` | DMAFunction (UDMA) | Stub | 9 | `mon_333B_DMAFunction` |  |
| `334B` | GetErrorMessage (GETXM) | Stub | 6 | `mon_334B_GetErrorMessage` |  |
| `335B` | TransferData (EXABS) | Stub | 10 | `mon_335B_TransferData` |  |
| `337B` | ChangeSegment (SPCHG) | Stub | 5 | `mon_337B_ChangeSegment` |  |
| `340B` | ReadSystemRecord (RSREC) | Stub | 8 | `mon_340B_ReadSystemRecord` |  |
| `341B` | SegmentFunction (SGMTY) | Stub | 10 | `mon_341B_SegmentFunction` |  |
| `400B` | ErrorReturn (MACROE) | Stub | 5 | `mon_400B_ErrorReturn` |  |
| `401B` | DisAssemble (DISASS) | Stub | 7 | `mon_401B_DisAssemble` |  |
| `402B` | GetInputFlags (RFLAG) | Stub | 5 | `mon_402B_GetInputFlags` |  |
| `403B` | SetOutputFlags (WFLAG) | Stub | 6 | `mon_403B_SetOutputFlags` |  |
| `404B` | FixIOArea (IOFIX) | Stub | 7 | `mon_404B_FixIOArea` |  |
| `406B` | AccessRTCommon (RWRTC) | Stub | 9 | `mon_406B_AccessRTCommon` |  |
| `410B` | FixInMemory (FIXMEM) | Stub | 9 | `mon_410B_FixInMemory` |  |
| `411B` | MemoryUnfix (UNFIXM) | Stub | 6 | `mon_411B_MemoryUnfix` |  |
| `414B` | BCNAFCAMAC (BCNAF) | Stub | 8 | `mon_414B_BCNAFCAMAC` |  |
| `415B` | BCNAF1CAMAC (BCNAF1) | Stub | 8 | `mon_415B_BCNAF1CAMAC` |  |
| `417B` | MaxPagesInMemory (MXPISG) | Stub | 8 | `mon_417B_MaxPagesInMemory` |  |
| `420B` | GetUserRegisters (GRBLK) | Stub | 5 | `mon_420B_GetUserRegisters` |  |
| `421B` | GetActiveSegment (GASGM) | Stub | 5 | `mon_421B_GetActiveSegment` |  |
| `423B` | CopyCapability (CAPCOP) | Stub | 10 | `mon_423B_CopyCapability` |  |
| `424B` | ClearCapability (CAPCLE) | Stub | 7 | `mon_424B_ClearCapability` |  |
| `425B` | SetProcessName (SPRNAM) | Stub | 6 | `mon_425B_SetProcessName` |  |
| `426B` | GetProcessNo (GPRNAM) | Stub | 6 | `mon_426B_GetProcessNo` |  |
| `427B` | GetOwnProcessInfo (GPRNME) | Stub | 5 | `mon_427B_GetOwnProcessInfo` |  |
| `430B` | TranslateAddress (ADR100) | Stub | 6 | `mon_430B_TranslateAddress` |  |
| `431B` | AwaitTransfer (MWAITF) | Stub | 7 | `mon_431B_AwaitTransfer` |  |
| `435B` | ForceTrap (PRT) | Stub | 7 | `mon_435B_ForceTrap` |  |
| `436B` | SetND500Param (5PASET) | Stub | 6 | `mon_436B_SetND500Param` |  |
| `437B` | GetND500Param (5PAGET) | Stub | 5 | `mon_437B_GetND500Param` |  |
| `440B` | Attach500Segment (AT5SGM) | Stub | 15 | `mon_440B_Attach500Segment` |  |
| `500B` | StartProcess (STARTP) | Stub | 6 | `mon_500B_StartProcess` |  |
| `501B` | StopProcess (STOPPR) | Stub | 5 | `mon_501B_StopProcess` |  |
| `502B` | SwitchProcess (SWITCHP) | Stub | 6 | `mon_502B_SwitchProcess` |  |
| `505B` | GetTrapReason (GERRCOD) | Stub | 5 | `mon_505B_GetTrapReason` |  |
| `507B` | SetProcessPriority (SPRIO) | Stub | 6 | `mon_507B_SetProcessPriority` |  |
| `514B` | ND500TimeOut (5TMOUT) | Stub | 7 | `mon_514B_ND500TimeOut` |  |

