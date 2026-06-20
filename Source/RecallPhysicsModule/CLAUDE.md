# RecallPhysicsModule

## Purpose
Deterministic physics integration with Jolt Physics for Recall ECS framework.

## Physics Objects

### Character Physics
- **RecallPhysicsCharacterObject**: Character controller with collision
- **RecallPhysicsCharacterVirtualObject**: Kinematic character simulation
- **Character Shape Types**: Capsule, box, and custom character shapes

### Vehicle Physics  
- **RecallPhysicsVehicleObject**: Full vehicle simulation
- **Vehicle Shape Types**: Chassis, wheel, and suspension definitions
- **Vehicle Types**: Drive modes, steering, and physics parameters

### Common Objects
- **RecallPhysicsCommonObjects**: Static and dynamic rigid bodies
- **Common Shape Types**: Box, sphere, capsule, convex, and mesh shapes

## Core Systems

### Physics Integration
- **RecallPhysicsSubsystem**: Main physics world management
- **RecallPhysicsUtils**: Physics calculation utilities
- **Object Factories**: Centralized physics object creation

### ECS Integration
- **Physics Processors**: Character, vehicle, and collider processors
- **Physics Fragments**: Body, character, vehicle, and sensor data
- **Physics Traits**: Entity physics setup and configuration

## Key Patterns

### Physics Object Creation
```cpp
// Character physics setup
FRecallPhysicsCharacterObjectCreateInfo CreateInfo;
CreateInfo.Position = StartLocation;
CreateInfo.Shape = CharacterShapeSettings;

TSharedPtr<FRecallPhysicsCharacterObject> PhysicsObject = 
    PhysicsSubsystem->CreateCharacterObject(CreateInfo);
```

### Fragment Integration
- **RecallPhysicsBodyFragment**: Core physics body data
- **RecallPhysicsCharacterFragments**: Character-specific physics state
- **RecallPhysicsVehicleFragments**: Vehicle physics parameters
- **RecallPhysicsSensorFragment**: Overlap and sensor detection

### Deterministic Requirements
- All physics operations must be deterministic
- Use Jolt Physics for consistent cross-platform results
- Fixed timestep simulation for network synchronization
- Deterministic random number generation for physics

## File Organization
- **Classes/Physics/**: Physics object definitions and factories
- **Simulation/Physics/**: ECS processors and traits
- **System/Physics/**: Physics subsystem and snapshot support
- **Utility/Physics/**: Physics calculation utilities

## Common Operations
- Collision detection and response
- Character movement and grounding
- Vehicle dynamics and control
- Sensor triggers and overlap events
- Physics body manipulation and queries