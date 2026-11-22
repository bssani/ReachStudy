# ReachStudy

VR reach check for a seated driver (Unreal Engine 5.7.4, Meta Quest 3 over Link, hand tracking only).
The participant sits in a physical seat and touches target meshes on the virtual driver door and instrument panel; a target changes material while it is touched. Two driver door variants can be switched; the instrument panel stays.

Status: project skeleton (S0). See [Docs/Plan.md](Docs/Plan.md) (Korean).

## Setup

1. Unreal Engine 5.7.4, Visual Studio 2022 with the Game development with C++ workload, Meta Quest Link.
2. **Meta plugins are not in this repo** (parts are under the Oculus SDK License). Copy these two folders into `Plugins/`:
   - `Plugins/MetaXR` — Meta XR Plugin 1.205.0
   - `Plugins/MetaXRInteraction` — Meta XR Interaction SDK 1.205.0
3. Build:
   ```
   "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ReachStudyEditor Win64 Development -Project="<abs path>\ReachStudy.uproject" -WaitMutex
   ```

Vehicle CAD is never committed: imported CAD lives under `Content/CAD/` (git-ignored).
