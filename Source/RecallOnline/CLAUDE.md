# RecallOnline Module

## Purpose
Multiplayer networking, rollback, and synchronization for deterministic Recall gameplay.

## Core Systems

### Rollback & Synchronization
- **RecallRollbackSubsystem**: Frame-based rollback management
- **RecallRollbackFrameManager**: Frame storage and retrieval
- **RecallSynchronizationUtils**: Cross-client sync validation

### Replay & Restoration
- **RecallReplaySubsystem**: Gameplay recording and playback
- **RecallRestoreSubsystem**: State restoration from snapshots
- **RecallSnapshotFileSubsystem**: File-based snapshot management

### Online Components
- **RecallGameSimulationComponent**: Core game simulation coordination
- **RecallInputControllerComponent**: Network input handling
- **RecallClientRestoreComponent**: Client-side state restoration
- **RecallLatencyGameComponent**: Network latency compensation

## Player Controllers & Game Framework
- **RecallPlayerControllerBase**: Base multiplayer controller
- **RecallGameMode**: Authoritative game state management
- **RecallGameState_InGame**: Replicated game state
- **RecallPlayerState_InGame**: Per-player networked state

## Key Patterns

### Rollback Integration
```cpp
// Save current frame state
RollbackSubsystem->SaveFrame(CurrentFrame);

// Process input and simulation
ProcessGameLogic();

// Restore to previous frame if needed
if (bNeedsRollback)
{
    RollbackSubsystem->RestoreFrame(TargetFrame);
}
```

### Component Architecture
- Game state components handle global simulation state
- Player state components track per-player data
- Controller components manage input and view targets
- Pawn components handle local representation

### Debug & Development
- **RecallCheatManager**: Development cheats and commands
- **RecallDebugMenuSubsystem**: In-game debug interface
- **RecallDebugRollbackManager**: Rollback debugging tools

## Networking Considerations
- All simulation must be deterministic
- Use provided random number generators
- Input handling through Recall input queue
- State synchronization via snapshot system
- Frame-based rollback for conflict resolution