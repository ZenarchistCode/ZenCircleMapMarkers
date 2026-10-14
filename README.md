# ZenCircleMapMarkers

This mod adds configurable circle overlays to DayZ's vanilla and Expansion maps. Useful for highlighting mission areas, danger zones, trader boundaries and building restrictions etc.

It does NOT add map markers - you'll need to add the map marker separately via a different mod; this mod just adds the circles. So for example, if you're running Expansion Bundle or Navigation, you would create the map marker in Expansion's map marker config then simply link a circle marker to it via my circle config (instructions below).

Or if you're using my ZenMap mod you'd create the marker in that config and create the circle around it separately in this mod.

**Requires Community Framework (CF). Install on both server and clients. Expansion is optional.**

## Features

- Static circles with configurable position, radius, colour, transparency and outline thickness.
- Optional diagonal caution hatching.
- Matching overlapping circles merge into a continuous area without internal borders or doubled opacity.
- Smaller differently coloured circles cut through larger overlays to keep boundaries readable.
- Link circles to existing Expansion server markers by UID or name.
- Automatic player-specific Expansion Quest travel circles, using the objective's actual radius and marker colour.
- Optional automatic no-build circles from Expansion BaseBuilding and ZenModPack.
- Server-side scripting API for adding, updating and removing circles.

Expansion's optional **CIRCLE MARKERS** category provides an **All Circle Markers** toggle and an optional **No Build Zones** toggle in the server marker list widget for players to show/hide circle markers.

## Configuration

Config file is created automatically at:

`$profile:Zenarchist\ZenCircleMapMarkersConfig.json`

Edit while the server is stopped, then restart. Boolean settings use `1` for enabled and `0` for disabled.

| Setting | Description |
|---|---|
| `Enabled` | Enables the mod. Default: `1`. |
| `RenderIntervalSeconds` | Client redraw interval. Default: `0.016` seconds. |
| `OutlineSegments` | Outline smoothness. Higher values increase detail and rendering cost. Default: `36`. |
| `FillResolutionPixels` | Fill detail: `1` is smoothest; larger values are coarser and cheaper. `0` disables fills. Default: `2`. |
| `RespectExpansionMarkerVisibility` | Linked circles follow Expansion marker visibility controls. Default: `1`. |
| `CautionLineAlpha` | Hatch opacity, from `0–255`. Default: `190`. |
| `CautionLineSpacingPixels` | Distance between hatch lines. Default: `18`. |
| `CautionLineWidthPixels` | Hatch thickness. Default: `2`. |
| `CautionAngleDegrees` | Hatch angle. Default: `-45`; `0` gives horizontal lines. |
| `EnableDynamicExpansionQuestCircles` | Automatic Expansion Quest travel circles. Default: `1`. |
| `ExpansionQuestCircleOutlineAlpha`, `ExpansionQuestCircleFillAlpha`, `ExpansionQuestCircleOutlineWidthPixels` | Quest circle appearance. Defaults: `255`, `35`, `2`. |
| `EnableNoBuildZoneCircles` | Imports supported no-build zones. Default: `0`. |
| `NoBuildZoneRefreshSeconds` | Checks for added, changed or removed zones. Default: `10` seconds. |
| `NoBuildZoneCircleColorRed`, `NoBuildZoneCircleColorGreen`, `NoBuildZoneCircleColorBlue` | No-build circle RGB colour. Defaults: `128`, `0`, `32`. |
| `NoBuildZoneCircleOutlineAlpha`, `NoBuildZoneCircleFillAlpha`, `NoBuildZoneCircleOutlineWidthPixels` | No-build circle appearance. Defaults: `255`, `35`, `2`. |
| `DebugLogging` | Enables diagnostic logging. Default: `0`. |
| `Markers` | Array of custom circles, as shown below. |

## Example Marker

Add an entry like this inside the `Markers` array:

```json
{
    "ID": "DangerZone",
    "Enabled": 1,
    "Position": [7500.0, 0.0, 9500.0],
    "RadiusMeters": 500.0,
    "ColorARGB": 0,
    "ColorRed": 255,
    "ColorGreen": 0,
    "ColorBlue": 0,
    "OutlineAlpha": 255,
    "FillAlpha": 35,
    "OutlineWidthPixels": 2.0,
    "CautionEnabled": 1,
    "EnableOverlap": 1,
    "ExpansionMarkerUID": "",
    "ExpansionMarkerName": ""
}
```

- Use a unique `ID`. `Position` is `[X, Y, Z]`; `RadiusMeters` is the radius in world metres.
- RGB and alpha values range from `0–255`. Alpha `0` is transparent; `255` is opaque.
- `ColorARGB: 0` uses the RGB fields. A non-zero Expansion-style ARGB value overrides RGB; the separate alpha settings still control transparency.
- `CautionEnabled` applies the global hatch style.
- `EnableOverlap` merges overlapping circles with matching colours, opacity, outline width and caution state.
- Set `ExpansionMarkerUID` or `ExpansionMarkerName` to follow an existing Expansion server marker instead of `Position`. UID takes priority.

## Scripting

Create, move and remove circles from server-side scripts:

```c
ZenCircleMapMarkers.AddCircleMarker("MissionZone", "7500 0 9500", 500.0, ARGB(255, 255, 0, 0), 35, 2.0, true, true);
ZenCircleMapMarkers.UpdateCircleMarkerPosition("MissionZone", "8000 0 9500");
ZenCircleMapMarkers.RemoveCircleMarker("MissionZone");
```

The final two creation arguments enable caution hatching and overlap merging. Runtime circles are not saved across server restarts.