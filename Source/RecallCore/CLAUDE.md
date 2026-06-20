# RecallCore Module

## Purpose
Core interfaces, types, and utilities for the Recall ECS framework.

## Key Components

### Essential Interfaces
- **RecallSimulationReactSystemInterface**: Core system lifecycle (Start, Reset, Save/Restore)
- **RecallInputQueueInterface**: Input handling and queuing
- **RecallPlayerQueueInterface**: Player management
- **RecallRandomNumberInterface**: Deterministic random number generation
- **RecallSnapshotInterface**: System state saving/loading

### Core Data Types
- **RecallGameRuleTypes**: Base game rule definitions
- **RecallInputActionTableRow**: Input action configurations
- **RecallPhysicsLayerDataAsset**: Physics layer management
- **RecallCharacterDataTable**: Character configuration data

### Settings & Configuration
- **RecallSimulationSettings**: Framework-wide simulation configuration
- **RecallWorldSettings**: World-specific Recall settings
- **RecallStreamingConfig**: Entity streaming configuration

## File Organization
- **Classes/**: Framework interfaces and core data types
- **Private/Utility/**: Internal utility implementations
- **Public/Utility/**: Public utility functions (e.g., Input utils)

## Common Patterns
- All interfaces inherit from UInterface for Blueprint compatibility
- Use RECALLCORE_API for public exports
- Subsystem interfaces follow Start/Reset/Save/Restore lifecycle
- Input utilities provide action mapping and validation

## Integration Points
- **World Settings**: Extend AWorldSettings with RecallWorldSettings
- **Game Instance**: Implement core interfaces in game instance
- **Player Controller**: Use player interfaces for input/state management