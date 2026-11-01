# Original mode graphics and wait icons

Only original game.exe evidence is used. Canonical consumers:
`instructions2/AutoModeGraphic442720.json`, `SkipModeGraphic4427d0.json`,
`ModeIndicators442880.json`; registration handlers446830/446880. Raw supporting
bytes are archived in `ModeVisualConsumersAssembly.txt`.

`ModeVisuals` owns registration, captured-resource backup and applied latches.
It does not own a Scene or silently simulate missing source layers. The host
supplies resource lookup, image reload and actual icon create/remove callbacks.
Missing layer lookup returnsnullopt and leaves the pending latch untouched.
The reload callback must implement416860's resource replacement with an empty
external mask, retaining ordinary layer coordinates and attributes and applying
native resource-specific centering separately. This operation does not swap
whole layer nodes.416860 does not itself call the color-effect rebuild416a00;
preserving stored color settings is distinct from applying them again to the
fresh image. Callback errors propagate.

On first active byte==1, consumer saves the current resource name, loads the
registered replacement and sets its applied latch. Repeated active updates do
nothing. On any other active byte, it reloads the captured name and clears the
latch. A missing destination delays both operations. No-argument registration
only disables the consumer; it does not restore a currently applied image or
clear its backup/latch. Re-registering similarly retains the old backup/latch,
even if the new layer ID differs. These are native observed state transitions,
not a cleaned-up replacement policy.

442880 updates Auto, AlreadySkip and Skip icons separately, then applies Auto
image replacement before Skip replacement. Skip's active byte is alreadySkip OR
skip. The two consumers have independent backups even when registered on the
same layer. Enabling both saves original→Auto→Skip; disabling both restores
original then the Skip backup (Auto). The final resource can therefore still be
the Auto image although both flags are false. The model preserves this order.
Other direct Auto consumer calls include41159b (mode cancellation),444b51
(KeyWait state refresh),443be1 and44b7b0.

Four five-int indicator records are parent layer, localx, localy, source layer,
and sprite group. A record is configured when any field is nonzero. Presence
query432a40 requires both a configured record and a retained native instance
pointer. Creation432860 looks up both layers, creates the requested group with
mode0/visible1, sets kind1 and sprite sort-order1, and appends to the parent.
It never creates a duplicate while an instance pointer is retained. Delete
4329a0 clears the pointer before attempting removal from the parent. Host scene
teardown must clear model instance state via`clear_icon_instances()`.

The Already (read-text) icon is different from skip: at444632..444656 the desired
state is the previous-read bytectx+80111!=0. It is removed at KeyWait exit4453af.
Auto/AlreadySkip/Skip reflect their own global mode flags, including updates
while selectors are active (for example44826f); do not turn Already on merely
because AlreadySkip is selected. The model exposes`update_already()` separately.

KeyWait icon settings are also separate.434170 stores its group index in
ctx+80134.434120 stores position mode in textlayer+138 and offsets in+130/+134.
The source object is global557048, loaded from original literal`system.ykg`
at427ef7.444768 creates a mode2/visible1 sprite in the current text layer only
whenctx+8924d is exactly1; that temporary sprite is removed when waiting ends.
Normal early skip/no-wait branches can avoid creating it. Position table445428
is decoded directly from original bytes:

| Mode | Layer-local position |
| --- | --- |
| 0 | text bounds right-30, bottom-30; supplied offsets ignored |
| 1 | supplied x,y |
| 2 | text bounds left+x, top+y |
| 3 | text bounds right+x, bottom+y |
| 4 | current native text cursor+x, cursor+y |
| Other | retained caller-local fallback coordinates |

`key_wait_icon_position()` uses native wrapping int32 additions and returns
these coordinates from TextFlow. Parent layer translation is applied by the
host renderer, not a second addition inside this helper.

# INI alpha exclusions

The vector object addresses are SaveDataWithoutGraphicTransmittance56c89c,
SaveDataWithoutGraphicLayer56c8ac and SpriteWithoutGraphicAttribute56c8cc. Their
begin pointers are respectively56c8a0,56c8b0 and56c8d0; confusing object/begin
addresses changes which option is being modeled.

The transmittance exclusion is a **load policy**, not altered writer fields.
447acf saves the destination's current native+1100 int and+1108 double, then
447b82 loads its saved header through416fe0.447c1c scans the exclusion IDs; when
matched,447c50..447c5e restores those two previous values. Thus loading retains
the current opacity for those IDs. It does not force0/1000 or omit header74.
The saved tween velocity/target are still reset by416fe0; this later branch
only restores current opacity values.

SpriteWithoutGraphicAttribute marks layer+12b8 in graphic load handlers. Its
only located reads are4137de (GPU) and413b39 (software), forcing the sprite's
passed parent transmittance to0. Both paths still compute parent position,
scale and rotation afterward. This is evidence for excluding inherited parent
opacity, not excluding all transforms or color behavior. That flag itself is
not serialized by415c00; its lifecycle must follow native layer creation/load
rather than inventing a header bit.

`mode_visuals_tests` covers latches, deferred missing layers, disabling and
re-registration, shared-layer ordering, four indicator types, raw-byte mode
comparisons, icon reset/state transfer and all wait-position modes. Independent
C++17 ASan/UBSan execution passed without game assets or SDL.
