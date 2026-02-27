# UE5 Edge Debris Scatter Tool

An Unreal Engine 5 editor tool for environment artists. 
Detects building edges via sphere sweep and procedurally 
scatters instanced static mesh debris with organic density 
falloff, weighted mesh selection, and deterministic seeding.

## Features
- Auto-detects vertical surfaces within a radius
- Density falloff from wall edge outward
- Weighted random mesh selection from user-defined set
- Deterministic seed for reproducible results
- Non-uniform scale variation
- Auto-refresh on actor move in editor
- Instance cap to prevent editor freezing

## Parameters
| Parameter | Description |
|-----------|-------------|
| Radius | Detection sphere size |
| EdgeNormalThreshold | What counts as a vertical face |
| SpreadDistance | How far debris spreads along the wall |
| DebrisReach | How far debris spills away from wall |
| FalloffExponent | Controls density gradient steepness |
| RandomSeed | Reproducible scatter results |
| MaxTotalInstances | Performance safety cap |

## Usage
1. Place actor near building geometry
2. Assign meshes to the Debris Meshes array
3. Hit Generate or enable Auto Refresh
