# RecallSimulation Module

## Purpose
Core simulation systems, processors, and entity management for Recall ECS.

## Key Systems

### Entity Management
- **RecallEntitySubsystem**: Entity lifecycle and validation
- **RecallEntityAsyncSpawnSubsystem**: Asynchronous entity spawning
- **RecallActorSubsystem**: Actor representation management

### Game Logic
- **RecallGameRuleSubsystem**: Game rule processing and validation
- **RecallStateTreeSubsystem**: StateTree behavior execution
- **RecallSlowMotionSubsystem**: Time dilation effects

### Core Processors
- **Controller Processors**: Input handling and command processing
- **Transform Processors**: Position, rotation, and scale updates
- **Representation Processors**: Visual representation synchronization
- **GameplayTag Processors**: Tag-based entity management

## Fragment Types
- **RecallControllerFragments**: Player input and control data
- **RecallGameplayTagFragment**: Gameplay tag management
- **Actor Representation**: Visual actor binding and updates

## Key Patterns

### Processor Structure
```cpp
class UMyProcessor : public UMassProcessor
{
public:
    UMyProcessor();
    
protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager& EntityManager, 
                        FMassExecutionContext& Context) override;
    
private:
    FMassEntityQuery EntityQuery;
};
```

### StateTree Integration
- **RecallStateTreeInstanceTypes**: StateTree entity bindings
- **RecallStateTreeTaskBase**: Base class for custom tasks
- Use external data handles for subsystem access

### Utility Organization
- **Debug Utils**: Debug visualization and logging
- **Entity Utils**: Entity validation and management helpers
- **Camera Utils**: Camera positioning and targeting utilities

## File Organization
- **Simulation/**: Domain-specific processors and fragments
- **System/**: Framework subsystems and core logic
- **Utility/**: Shared utility functions and helpers
- **Classes/**: Public interfaces and actor implementations