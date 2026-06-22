# Recall - Unreal Engine Plugin

**Recall** is a comprehensive ECS framework built directly on Epic Games' Mass system, providing fixed step simulation, rollback netcode, and deterministic multi-threaded processing for competitive multiplayer games.

## About Recall

Recall replaces the MassExtended approach used in prior iterations. Rather than wrapping Mass in a separate plugin, Recall patches the engine directly using the **UnrealEnginePatch** plugin. This gives access to the full Mass feature set including StateTree integration, Mass debug tools, and all engine-level Mass internals — without maintaining a fork or a parallel ECS layer.

A **custom engine build is required**. Patches are managed through JSON patch definitions in the `EnginePatch/` folder and applied automatically at build time by `UnrealEnginePatch`.

## Core Architecture Decision

Previous iterations of this framework relied on **MassExtended**, a plugin that duplicated and wrapped the Mass ECS layer to extend it. Recall drops that approach entirely — instead, the engine itself is patched using the **UnrealEnginePatch** plugin. This eliminates the maintenance cost of a parallel ECS layer and gives direct access to the full Mass feature set.

| | MassExtended (previous) | Recall (current) |
|---|---|---|
| Mass integration | Wrapper plugin duplicating Mass | Direct engine patches |
| StateTree support | Limited | Full (engine-level) |
| Mass debug tools | Partial | Full |
| Custom engine | Not required | **Required** |
| Patch management | Source copy | JSON patch definitions |

## Engine Patches

Recall's engine patches live in `EnginePatch/` as JSON files consumed by the `UnrealEnginePatch` plugin. Patches are applied/unapplied via:

```bat
# Unapply all patches (to edit original source lines)
CCR\Plugins\UnrealEnginePatch\Scripts\UnapplyPatches.bat <project_dir> <engine_dir>

# Apply (or re-apply after editing patch definitions)
CCR\Plugins\UnrealEnginePatch\Scripts\SyncPatches.bat <project_dir> <engine_dir>
```

### Patch List

| Patch | Description |
|---|---|
| `mass-entity-handle-remove-transient` | Makes `FMassEntityHandle` `BlueprintType`, removes `Transient` from properties for serialization |
| `mass-entity-manager` | Serial number accessors (`GetSerialNumberGenerator`, `SetSerialNumberGenerator`, `GetAllEntities`) |
| `mass-entity-manager-extended` | Snapshot/restore methods: `DestroyEntity(bSignalObservers)`, `BuildEntity(ChunkIndex)`, `DestroyAllEntities`, `RestoreEntitySerialNumber`, shared fragment map/hash/reset helpers, archetype chunk accessors, deterministic `GetStructCrc32`, `DefaultSharedFragments` reset-to-default caching |
| `mass-entity-storage-free-index-order` | Deterministic free index ordering for reproducible entity slot reuse |
| `mass-entity-query-deterministic-sort` | Deterministic archetype sort in `CacheArchetypes` for rollback reproducibility |
| `mass-processing-parallel-always` | Forces `MASS_DO_PARALLEL 1` regardless of server flag |
| `mass-processing-render-phase` | Adds `Render` phase to `EMassProcessingPhase` for render-tick processors |
| `mass-processing-manual-step` | Adds `ExecuteStep()` and `DisableTickFunctions()` for manual fixed-step control |
| `mass-processing-types-gc-guard` | Wraps off-game-thread `NewObject` processor copies in `FGCScopeGuard` for GC safety |
| `mass-processor-current-thread` | `GetDesiredThread()` returns current thread instead of forcing game thread + high priority |
| `statetree-instance-storage-unique-id` | Unique ID accessors on `FStateTreeInstanceStorage` for snapshot identification |

## Modules

### RecallCore (Runtime)
Core ECS extensions, base classes, synchronization, player integration, deterministic input handling, and snapshot interfaces.

### RecallSimulation (Runtime)
Entity spawning, lifecycle management, actor pooling and representation, gameplay system processors, deterministic transform processing.

### RecallOnline (Runtime)
Rollback netcode implementation, input synchronization, replay recording/playback, drop-in client restoration, multiplayer game framework.

### RecallPhysicsModule (Runtime)
Deterministic physics via Jolt Physics: character controllers, vehicle simulation, collision detection, physics-based sensors.

### RecallSignals (Runtime)
Type-safe event/signal system, time-based signal scheduling, inter-system communication.

### RecallSnapshot (Runtime)
Complete game state capture and restoration, efficient serialization for rollback, snapshot file management.

### RecallFrontend (Runtime)
UI integration, replay player widgets, debug interface systems.

### RecallEntityStreaming (Runtime)
Entity LOD management, streaming systems, culling and performance optimization.

## Key Features

### Deterministic Simulation
- Fixed timestep across all clients
- Deterministic RNG with reproducible seeds
- Deterministic archetype ordering for consistent entity processing
- Custom `GetStructCrc32` for stable shared fragment hashing across sessions

### Rollback Netcode
- Frame-accurate game state snapshots and restoration
- Client-side input prediction and rollback
- Drop-in mid-game client restoration
- Desync detection and logging

### Multi-Threading
- Parallel processor execution (`MASS_DO_PARALLEL` always enabled)
- GC-safe off-game-thread processor instantiation
- Deterministic job scheduling

### Full Mass Integration
- StateTree processors and AI integration (engine-level access)
- Mass debug tools and visual logger
- `FMassEntityHandle` exposed to Blueprints
- Render-phase processors for visual-tick separation
- Manual step control for fixed-step game loop

## Plugin Dependencies

| Plugin | Required | Notes |
|---|---|---|
| `StateTree` | Yes | AI and behavior tree integration |
| `JoltPhysics` | Yes | Deterministic physics |
| `EasyOnline` | Yes | Networking and multiplayer |
| `DebugMenu` | Yes | Development and debug tooling |
| `EnhancedInput` | Yes | Input processing |
| `MassGameplay` | Yes | Epic's Mass gameplay utilities |
| `Niagara` | Yes | Particle systems |
| `VariableCollection` | Yes | Variable management |
| `MultiWorld` | Optional | Multi-world support |

## Setup

1. **Custom engine required** — clone and build the CCR engine branch. The engine must have all `Recall` patches applied before compiling.
2. **Apply patches** — run `SyncPatches.bat` pointing at your project and engine directories.
3. **Enable plugin** — add `Recall` to your `.uproject` plugins list (`"EnabledByDefault": false` by default).
4. **Configure dependencies** — ensure `JoltPhysics`, `EasyOnline`, and other required plugins are present.

## Author

**Bastien Van de Walle** — Recall framework, engine patches, deterministic ECS architecture.  
Based on Epic Games' Mass Entity Component System.