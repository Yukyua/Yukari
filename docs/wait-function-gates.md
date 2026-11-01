# Wait and selection function gates

This is a focused original-executable semantic audit, not a count of implemented entry points. Evidence: KeyWait core 0x4435c0, KeyVoiceWait core 0x444530, Select/MouseSelect core 0x4480c0; original executable assembly resolves ambiguous item setters.

## Function masks

KeyWait and KeyVoiceWait read context +0x8012c. Select type 1 reads +0x80124; MouseSelect type 2 reads +0x80128. The masks control built-in auxiliary actions and script areas, not the actual Case choices.

| Bit | Original consumer |
| --- | --- |
| 0x001 | Create History area item (context +0x88e00) |
| 0x002 | Create WindowErase area item (+0x88f54) |
| 0x004 | Create right-click SubMenu item, only when submenu script is nonempty and current frame +0xc8 is 1 |
| 0x008 | Include registered script area list (+0x89240), excluding history-indicator entries with bit31 |
| 0x010 | Create AutoMode area item: +0x88ecc when off, +0x88f10 when on |
| 0x020 | Create Skip area item (+0x88e44) |
| 0x040 | Create AlreadySkip area item (+0x88e88) |
| 0x080 | Attach SystemKeySetWindowErase keys to the WindowErase item, which must also exist via bit 0x002 |
| 0x100 | Attach mouse wheel-up to the History item, which must also exist via bit 0x001 |
| 0x200 | Attach SystemKeySetHistoryMode keys to the History item, which must also exist via bit 0x001 |
| 0x400 | No direct consumer established in the inspected wait and selector bodies; do not assign a guessed action |

For Select/MouseSelect the SubMenu creation additionally requires zero arguments at this branch and global 0x56caf4 (the TTE wrapper-active flag) not equal to 1 (assembly 0x44b7b8..0x44b80d). Gate action creation and the corresponding key/area hit processing together. Unconditionally checking the area first and only gating a visible button is insufficient.

`FUN_0x418380(item,4)` sets item kind=1 (MouseKey), mouse_key=4. It does **not** set kind=4 (Message). MouseKey4 is WM_MOUSEWHEEL positive delta; MouseKey5 is negative delta; MouseKey2 is right click. Baseline left-click, wheel-down and SystemKeySetKeyInSkip registration remain independent of these optional bits in the wait core. Actual Case items/navigation are likewise independent.

## Voice wait scheduling

KeyWait calls 0x4435c0; KeyVoiceWait calls 0x444530. Ordinary KeyWait with auto off uses infinite user-input wait. With auto on it queries 0x45dcd0; pending voice sets a 100ms wait and timeout messages repeat. After voice completion it uses the configured auto delay.

KeyVoiceWait queries 0x45dcd0 even with auto off. Pending voice uses the same 100ms retry loop. With no pending voice and auto off its timeout is zero, so it completes without waiting for an additional user click. Input can still finish a pending wait early.

`Audio::voice_wait_pending()` implements 0x45dcd0 with 0x45da40/0x45dae0/0x45d340: an actually playing nonlooping source, at least one matching assigned group with flag exactly 1 or bit 4, and no matching silent group. Volume zero and host mute are not silent-group flags. Skip-suppression's group bit 2 is a different predicate and must not replace the voice-wait bit 4.

## Timing wrappers and load hook

KeyWaitNA 0x445480 calls ordinary wait only if context +0x80111 is zero (current statement not already read). Otherwise it returns without waiting.

SelectTTE 0x44c690 and MouseSelectTTE 0x44c750 only record wall time around the underlying selection; they do not introduce a leading timeout parameter or automatically select a result on a deadline. TTELoop 0x44c6e0/0x44c7a0 stores elapsed modulo 10000 and replaces a zero result with 1. KeyWaitTTE 0x4454b0 records elapsed; KeyWaitTTELoop 0x4454f0 takes modulo 10000 but does not force 1. The TTE 0x4387e0 predicate checks nonzero elapsed and, with an argument, elapsed strictly below its unsigned limit; a passing argument form clears elapsed, failure skips the next block. Its no-argument form preserves elapsed.

SaveDataLoadToCallScript 0x4467d0 registers a nonempty string, clears it for an empty string, and leaves it unchanged with no arguments. The consumer at 0x447d70 runs after restoring the saved frame chain: 0x43f140 pushes a new VM frame (0x432ad0), then loads the registered script into that frame (0x43cf20). Return resumes the restored script. Do not replace the restored chain, mark it as a menu/event frame, or change NowScriptState for this hook. Load then restores saved save/load permissions and related context fields.
