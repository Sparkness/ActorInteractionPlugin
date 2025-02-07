﻿// All rights reserved Dominik Pavlicek 2022.

#include "Components/ActorInteractableComponentBase.h"

#include "Helpers/ActorInteractionPluginLog.h"

#if WITH_EDITOR
#include "EditorHelper.h"
#endif

#include "Interfaces/ActorInteractionWidget.h"
#include "Widgets/ActorInteractableWidget.h"

#include "Components/ActorInteractableComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/WidgetComponent.h"
#include "Helpers/ActorInteractionFunctionLibrary.h"
#include "Interfaces/ActorInteractorInterface.h"

#define LOCTEXT_NAMESPACE "InteractableComponentBase"

UActorInteractableComponentBase::UActorInteractableComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	
	DebugSettings.DebugMode = false;
	DebugSettings.EditorDebugMode = false;
	
	SetupType = ESetupType::EST_Quick;

	InteractableState = EInteractableStateV2::EIS_Awake;
	DefaultInteractableState = EInteractableStateV2::EIS_Awake;
	InteractionWeight = 1;
	
	bInteractionHighlight = true;
	StencilID = 133;

	LifecycleMode = EInteractableLifecycle::EIL_Cycled;
	LifecycleCount = -1;
	InteractionPeriod = 1.5;
	CooldownPeriod = 3.f;
	RemainingLifecycleCount = LifecycleCount;

	InteractionOwner = GetOwner();

	ComparisonMethod = ETimingComparison::ECM_None;
	TimeToStart = 0.001f;
	InteractableName = LOCTEXT("InteractableComponentBase", "Base");

	const FInteractionKeySetup GamepadKeys = FKey("Gamepad_FaceButton_Bottom");
	FInteractionKeySetup KeyboardKeys = GamepadKeys;
	KeyboardKeys.Keys.Add("E");
		
	InteractionKeysPerPlatform.Add((TEXT("Windows")), KeyboardKeys);
	InteractionKeysPerPlatform.Add((TEXT("Mac")), KeyboardKeys);
	InteractionKeysPerPlatform.Add((TEXT("PS4")), GamepadKeys);
	InteractionKeysPerPlatform.Add((TEXT("XboxOne")), GamepadKeys);

	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(64, 64);
	bDrawAtDesiredSize = false;
	
	UActorComponent::SetActive(true);
	SetHiddenInGame(true);

	CachedInteractionWeight = InteractionWeight;

#if WITH_EDITORONLY_DATA
	bVisualizeComponent = true;
#endif
}

void UActorInteractableComponentBase::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionOwner = GetOwner();

	// Interaction Events
	OnInteractableSelected.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableSelectedEvent);
	OnInteractorFound.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractorFound);
	OnInteractorLost.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractorLost);

	OnInteractorOverlapped.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableBeginOverlapEvent);
	OnInteractorStopOverlap.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableStopOverlapEvent);
	OnInteractorTraced.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableTraced);

	OnInteractionCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCompleted);
	OnInteractionCycleCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCycleCompleted);
	OnInteractionStarted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionStarted);
	OnInteractionStopped.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionStopped);
	OnInteractionCanceled.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCanceled);
	OnLifecycleCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionLifecycleCompleted);
	OnCooldownCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCooldownCompleted);
	
	// Attributes Events
	OnInteractableDependencyChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableDependencyChangedEvent);
	OnInteractableAutoSetupChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableAutoSetupChangedEvent);
	OnInteractableWeightChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableWeightChangedEvent);
	OnInteractableStateChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableStateChangedEvent);
	OnInteractableOwnerChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableOwnerChangedEvent);
	OnInteractableCollisionChannelChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableCollisionChannelChangedEvent);
	OnLifecycleModeChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnLifecycleModeChangedEvent);
	OnLifecycleCountChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnLifecycleCountChangedEvent);
	OnCooldownPeriodChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnCooldownPeriodChangedEvent);
	OnInteractorChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractorChangedEvent);

	// Ignored Classes Events
	OnIgnoredInteractorClassAdded.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnIgnoredClassAdded);
	OnIgnoredInteractorClassRemoved.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnIgnoredClassRemoved);

	// Highlight Events
	OnHighlightableComponentAdded.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnHighlightableComponentAddedEvent);
	OnHighlightableComponentRemoved.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnHighlightableComponentRemovedEvent);
	
	// Collision Events
	OnCollisionComponentAdded.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnCollisionComponentAddedEvent);
	OnCollisionComponentRemoved.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnCollisionComponentRemovedEvent);

	// Widget
	OnWidgetUpdated.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnWidgetUpdatedEvent);

	// Dependency
	InteractableDependencyStarted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableDependencyStartedCallback);
	InteractableDependencyStopped.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableDependencyStoppedCallback);
	
	RemainingLifecycleCount = LifecycleCount;
	
	SetState(DefaultInteractableState);

	AutoSetup();

#if WITH_EDITOR
	
	DrawDebug();

#endif
}

void UActorInteractableComponentBase::InitWidget()
{
	Super::InitWidget();

	UpdateInteractionWidget();
}

void UActorInteractableComponentBase::OnRegister()
{
	
#if WITH_EDITORONLY_DATA
	if (bVisualizeComponent && SpriteComponent == nullptr && GetOwner() && !GetWorld()->IsGameWorld() )
	{
		SpriteComponent = NewObject<UBillboardComponent>(GetOwner(), NAME_None, RF_Transactional | RF_Transient | RF_TextExportTransient);

		SpriteComponent->Sprite = LoadObject<UTexture2D>(nullptr, TEXT("/ActorInteractionPlugin/Textures/Editor/T_MounteaLogo"));
		SpriteComponent->SetRelativeScale3D_Direct(FVector(1.f));
		SpriteComponent->Mobility = EComponentMobility::Movable;
		SpriteComponent->AlwaysLoadOnClient = false;
		SpriteComponent->SetIsVisualizationComponent(true);
		SpriteComponent->SpriteInfo.Category = TEXT("Misc");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT( "SpriteCategory", "Misc", "Misc" );
		SpriteComponent->CreationMethod = CreationMethod;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->bUseInEditorScaling = true;

		SpriteComponent->SetupAttachment(this);
		SpriteComponent->RegisterComponent();
	}
#endif
	
	Super::OnRegister();
}

bool UActorInteractableComponentBase::DoesHaveInteractor() const
{
	return Interactor.GetObject() != nullptr;
}

#pragma region InteractionImplementations

bool UActorInteractableComponentBase::DoesAutoSetup() const
{ return SetupType != ESetupType::EST_None; }

void UActorInteractableComponentBase::ToggleAutoSetup(const ESetupType& NewValue)
{
	SetupType = NewValue;
}

bool UActorInteractableComponentBase::ActivateInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Active);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Active:
			ErrorMessage.Append(TEXT("Interactable Component is already Active"));
			break;
		case EInteractableStateV2::EIS_Awake:
			ErrorMessage.Append(TEXT("Interactable Component has been Activated"));
			return true;
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Completed:
		case EInteractableStateV2::EIS_Disabled:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Activated"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::WakeUpInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Awake);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Awake:
			ErrorMessage.Append(TEXT("Interactable Component is already Awake"));
			break;
		case EInteractableStateV2::EIS_Active:
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Disabled:
			ErrorMessage.Append(TEXT("Interactable Component has been Awaken"));
			return true;
		case EInteractableStateV2::EIS_Completed:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Awaken"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::SnoozeInteractable(FString& ErrorMessage)
{
	DeactivateInteractable();

	ErrorMessage.Append(TEXT("Interactable Component has been Deactivated. Asleep state is now deprecated."));
	return true;
}

bool UActorInteractableComponentBase::CompleteInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Completed);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Active:
			ErrorMessage.Append(TEXT("Interactable Component is Completed"));
			return true;
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Awake:
		case EInteractableStateV2::EIS_Disabled:
		case EInteractableStateV2::EIS_Cooldown:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Completed"));
			break;
		case EInteractableStateV2::EIS_Completed:
			ErrorMessage.Append(TEXT("Interactable Component is already Completed"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

void UActorInteractableComponentBase::DeactivateInteractable()
{
	SetState(EInteractableStateV2::EIS_Disabled);
}

void UActorInteractableComponentBase::PauseInteraction(const float ExpirationTime, const FKey UsedKey, const TScriptInterface<IActorInteractorInterface>& CausingInteractor)
{
	if (!GetWorld()) return;
	
	SetState(EInteractableStateV2::EIS_Paused);
	const bool bIsUnlimited = FMath::IsWithinInclusive(InteractionProgressExpiration, -1.f, 0.f) || FMath::IsNearlyZero(InteractionProgressExpiration, 0.001f);
	
	if (bIsUnlimited)
	{
		GetWorld()->GetTimerManager().PauseTimer(Timer_Interaction);
		return;
	}
	
	if (!bIsUnlimited)
	{
		FTimerDelegate TimerDelegate_ProgressExpiration;
		TimerDelegate_ProgressExpiration.BindUFunction(this, "OnInteractionProgressExpired", ExpirationTime, UsedKey, CausingInteractor);

		const float ClampedExpiration = FMath::Max(InteractionProgressExpiration, 0.01f);
		
		GetWorld()->GetTimerManager().SetTimer(Timer_ProgressExpiration, TimerDelegate_ProgressExpiration, ClampedExpiration, false);
		GetWorld()->GetTimerManager().PauseTimer(Timer_Interaction);
	}
	else
	{
		OnInteractionProgressExpired(ExpirationTime, UsedKey, CausingInteractor);
	}
}

bool UActorInteractableComponentBase::CanInteract() const
{
	if (!GetWorld()) return false;
	
	switch (InteractableState)
	{
		case EInteractableStateV2::EIS_Awake:
		case EInteractableStateV2::EIS_Active:
		case EInteractableStateV2::EIS_Paused:
			return GetInteractor().GetInterface() != nullptr;
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Disabled:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Completed:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::Default: 
		default: break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::CanBeTriggered() const
{
	if (!GetWorld()) return false;
	
	switch (InteractableState)
	{
		case EInteractableStateV2::EIS_Awake:
		case EInteractableStateV2::EIS_Active:
		case EInteractableStateV2::EIS_Paused:
			return true;
		//	return !(GetWorld()->GetTimerManager().IsTimerActive(Timer_Interaction));
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Disabled:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Completed:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::Default: 
		default: break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::IsInteracting() const
{
	if (GetWorld())
	{
		return GetWorld()->GetTimerManager().IsTimerActive(Timer_Interaction);
	}

	AIntP_LOG(Error, TEXT("[IsInteracting] Cannot find World!"))
	return false;
}

EInteractableStateV2 UActorInteractableComponentBase::GetDefaultState() const
{ return DefaultInteractableState; }

void UActorInteractableComponentBase::SetDefaultState(const EInteractableStateV2 NewState)
{
	if
	(
		DefaultInteractableState == EInteractableStateV2::EIS_Active ||
		DefaultInteractableState == EInteractableStateV2::EIS_Completed ||
		DefaultInteractableState == EInteractableStateV2::EIS_Cooldown
	)
	{
		AIntP_LOG(Error, TEXT("[SetDefaultState] Tried to set invalid Default State!"))
		return;
	}
	DefaultInteractableState = NewState;
}

EInteractableStateV2 UActorInteractableComponentBase::GetState() const
{ return InteractableState; }

void UActorInteractableComponentBase::CleanupComponent()
{
	StopHighlight();
	OnInteractableStateChanged.Broadcast(InteractableState);
	if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	OnInteractorLost.Broadcast(Interactor);

	RemoveHighlightableComponents(HighlightableComponents);
	RemoveCollisionComponents(CollisionComponents);
}

void UActorInteractableComponentBase::SetState(const EInteractableStateV2 NewState)
{
	switch (NewState)
	{
		case EInteractableStateV2::EIS_Active:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Paused:
				case EInteractableStateV2::EIS_Awake:
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Active:
					break;
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::Default:
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Awake:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::EIS_Paused:
					{
						InteractableState = NewState;
						OnInteractableStateChanged.Broadcast(InteractableState);

						for (const auto& Itr : CollisionComponents)
						{
							BindCollisionShape(Itr);
						}
					}
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Asleep:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Paused:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
					{
						InteractableState = NewState;

						// Replacing Cleanup
						StopHighlight();
						OnInteractableStateChanged.Broadcast(InteractableState);
						if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
						OnInteractorLost.Broadcast(Interactor);
						
						for (const auto& Itr : CollisionComponents)
						{
							UnbindCollisionShape(Itr);
						}
					}
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Cooldown:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Active:
					InteractableState = NewState;
					StopHighlight();
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Disabled:
					{
						InteractableState = NewState;

						// Replacing Cleanup
						StopHighlight();
						OnInteractableStateChanged.Broadcast(InteractableState);
						if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
						OnInteractorLost.Broadcast(Interactor);
						
						for (const auto& Itr : CollisionComponents)
						{
							UnbindCollisionShape(Itr);
						}
					}
					break;
				case EInteractableStateV2::EIS_Paused:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Completed:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
					{
						InteractableState = NewState;
						CleanupComponent();
					}
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Paused:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Disabled:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Paused:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Asleep:
					{
						InteractableState = NewState;

						// Replacing Cleanup
						StopHighlight();
						OnInteractableStateChanged.Broadcast(InteractableState);
						if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
						OnInteractorLost.Broadcast(Interactor);
						
						for (const auto& Itr : CollisionComponents)
						{
							UnbindCollisionShape(Itr);
						}
					}
					break;
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Suppressed:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::EIS_Paused:
					OnInteractionCanceled.Broadcast();
					InteractableState = NewState;
					StopHighlight();
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Cooldown:
					OnInteractionCanceled.Broadcast();
					InteractableState = NewState;
					StopHighlight();
					OnInteractableStateChanged.Broadcast(InteractableState);
					GetWorld()->GetTimerManager().ClearTimer(Timer_Cooldown);
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::Default:
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Paused:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
					{
						InteractableState = NewState;
						OnInteractableStateChanged.Broadcast(InteractableState);
						break;
					}
				case EInteractableStateV2::EIS_Paused:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::Default:
				default: break;
			}
			break;
		case EInteractableStateV2::Default: 
		default:
			StopHighlight();
			break;
	}
	
	ProcessDependencies();
}

void UActorInteractableComponentBase::StartHighlight()
{
	SetHiddenInGame(false, true);
	for (const auto& Itr : HighlightableComponents)
	{
		Itr->SetRenderCustomDepth(bInteractionHighlight);
		Itr->SetCustomDepthStencilValue(StencilID);
	}
}

void UActorInteractableComponentBase::StopHighlight()
{
	SetHiddenInGame(true, true);
	for (const auto& Itr : HighlightableComponents)
	{
		//Itr->SetRenderCustomDepth(false);
		Itr->SetCustomDepthStencilValue(0);
	}
}

TArray<TSoftClassPtr<UObject>> UActorInteractableComponentBase::GetIgnoredClasses() const
{ return IgnoredClasses; }

void UActorInteractableComponentBase::SetIgnoredClasses(const TArray<TSoftClassPtr<UObject>> NewIgnoredClasses)
{
	IgnoredClasses.Empty();

	IgnoredClasses = NewIgnoredClasses;
}

void UActorInteractableComponentBase::AddIgnoredClass(TSoftClassPtr<UObject> AddIgnoredClass)
{
	if (AddIgnoredClass == nullptr) return;

	if (IgnoredClasses.Contains(AddIgnoredClass)) return;

	IgnoredClasses.Add(AddIgnoredClass);

	OnIgnoredInteractorClassAdded.Broadcast(AddIgnoredClass);
}

void UActorInteractableComponentBase::AddIgnoredClasses(TArray<TSoftClassPtr<UObject>> AddIgnoredClasses)
{
	for (const auto& Itr : AddIgnoredClasses)
	{
		AddIgnoredClass(Itr);
	}
}

void UActorInteractableComponentBase::RemoveIgnoredClass(TSoftClassPtr<UObject> RemoveIgnoredClass)
{
	if (RemoveIgnoredClass == nullptr) return;

	if (!IgnoredClasses.Contains(RemoveIgnoredClass)) return;

	IgnoredClasses.Remove(RemoveIgnoredClass);

	OnIgnoredInteractorClassRemoved.Broadcast(RemoveIgnoredClass);
}

void UActorInteractableComponentBase::RemoveIgnoredClasses(TArray<TSoftClassPtr<UObject>> RemoveIgnoredClasses)
{
	for (const auto& Itr : RemoveIgnoredClasses)
	{
		RemoveIgnoredClass(Itr);
	}
}

void UActorInteractableComponentBase::AddInteractionDependency(const TScriptInterface<IActorInteractableInterface> InteractionDependency)
{
	if (InteractionDependency.GetObject() == nullptr) return;
	if (InteractionDependencies.Contains(InteractionDependency)) return;

	OnInteractableDependencyChanged.Broadcast(InteractionDependency);
	
	InteractionDependencies.Add(InteractionDependency);

	InteractionDependency->GetInteractableDependencyStarted().Broadcast(this);
}

void UActorInteractableComponentBase::RemoveInteractionDependency(const TScriptInterface<IActorInteractableInterface> InteractionDependency)
{
	if (InteractionDependency.GetObject() == nullptr) return;
	if (!InteractionDependencies.Contains(InteractionDependency)) return;

	OnInteractableDependencyChanged.Broadcast(InteractionDependency);

	InteractionDependencies.Remove(InteractionDependency);

	InteractionDependency->GetInteractableDependencyStopped().Broadcast(this);
}

TArray<TScriptInterface<IActorInteractableInterface>> UActorInteractableComponentBase::GetInteractionDependencies() const
{ return InteractionDependencies; }

void UActorInteractableComponentBase::ProcessDependencies()
{
	if (InteractionDependencies.Num() == 0) return;

	auto Dependencies = InteractionDependencies;
	for (const auto& Itr : Dependencies)
	{
		switch (InteractableState)
		{
			case EInteractableStateV2::EIS_Active:
			case EInteractableStateV2::EIS_Suppressed:
				Itr->GetInteractableDependencyStarted().Broadcast(this);
				switch (Itr->GetState())
				{
					case EInteractableStateV2::EIS_Active:
					case EInteractableStateV2::EIS_Awake:
					case EInteractableStateV2::EIS_Asleep:
						Itr->SetState(EInteractableStateV2::EIS_Suppressed);
						break;
					case EInteractableStateV2::EIS_Cooldown:
						
						Itr->SetState(EInteractableStateV2::EIS_Suppressed);
						
						break;
					case EInteractableStateV2::EIS_Completed: break;
					case EInteractableStateV2::EIS_Disabled: break;
					case EInteractableStateV2::EIS_Suppressed: break;
					case EInteractableStateV2::Default: break;
					default: break;
				}
				break;
			case EInteractableStateV2::EIS_Cooldown:
			case EInteractableStateV2::EIS_Awake:
			case EInteractableStateV2::EIS_Asleep:
				Itr->GetInteractableDependencyStarted().Broadcast(this);
				switch (Itr->GetState())
				{
					
					case EInteractableStateV2::EIS_Awake:
					case EInteractableStateV2::EIS_Asleep:
					case EInteractableStateV2::EIS_Suppressed: 
						Itr->SetState(Itr->GetDefaultState());
						break;
					case EInteractableStateV2::EIS_Cooldown: break;
					case EInteractableStateV2::EIS_Completed: break;
					case EInteractableStateV2::EIS_Disabled: break;
					case EInteractableStateV2::EIS_Active:
					case EInteractableStateV2::Default: break;
					default: break;
				}
				break;
			case EInteractableStateV2::EIS_Disabled:
			case EInteractableStateV2::EIS_Completed:
				Itr->GetInteractableDependencyStopped().Broadcast(this);
				Itr->SetState(Itr->GetDefaultState());
				RemoveInteractionDependency(Itr);
				break;
			case EInteractableStateV2::Default:
			default:
				break;
		}
	}
}

TScriptInterface<IActorInteractorInterface> UActorInteractableComponentBase::GetInteractor() const
{ return Interactor; }

void UActorInteractableComponentBase::SetInteractor(const TScriptInterface<IActorInteractorInterface> NewInteractor)
{
	const TScriptInterface<IActorInteractorInterface> OldInteractor = Interactor;

	Interactor = NewInteractor;
	
	if (NewInteractor.GetInterface() != nullptr)
	{
		NewInteractor->GetOnInteractableSelectedHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
		NewInteractor->GetOnInteractableFoundHandle().Broadcast(this);
	}
	else
	{
		if (OldInteractor.GetInterface() != nullptr)
		{
			OldInteractor->GetOnInteractableSelectedHandle().RemoveDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
		}

		StopHighlight();
	}

	//Interactor = NewInteractor;
	OnInteractorChanged.Broadcast(Interactor);
}

float UActorInteractableComponentBase::GetInteractionProgress() const
{
	if (!GetWorld()) return -1;

	if (Timer_Interaction.IsValid())
	{
		return GetWorld()->GetTimerManager().GetTimerElapsed(Timer_Interaction) / InteractionPeriod;
	}
	return 0.f;
}

float UActorInteractableComponentBase::GetInteractionPeriod() const
{ return InteractionPeriod; }

void UActorInteractableComponentBase::SetInteractionPeriod(const float NewPeriod)
{
	float TempPeriod = NewPeriod;
	if (TempPeriod > -1.f && TempPeriod < 0.01f)
	{
		TempPeriod = 0.01f;
	}
	if (FMath::IsNearlyZero(TempPeriod, 0.0001f))
	{
		TempPeriod = 0.01f;
	}

	InteractionPeriod = FMath::Max(-1.f, TempPeriod);
}

int32 UActorInteractableComponentBase::GetInteractableWeight() const
{ return InteractionWeight; }

void UActorInteractableComponentBase::SetInteractableWeight(const int32 NewWeight)
{
	InteractionWeight = NewWeight;

	OnInteractableWeightChanged.Broadcast(InteractionWeight);
}

AActor* UActorInteractableComponentBase::GetInteractableOwner() const
{ return InteractionOwner; }

void UActorInteractableComponentBase::SetInteractableOwner(AActor* NewOwner)
{
	if (NewOwner == nullptr) return;
	InteractionOwner = NewOwner;
	
	OnInteractableOwnerChanged.Broadcast(InteractionOwner);
}

ECollisionChannel UActorInteractableComponentBase::GetCollisionChannel() const
{ return CollisionChannel; }

void UActorInteractableComponentBase::SetCollisionChannel(const ECollisionChannel& NewChannel)
{
	CollisionChannel = NewChannel;

	OnInteractableCollisionChannelChanged.Broadcast(CollisionChannel);
}

TArray<UPrimitiveComponent*> UActorInteractableComponentBase::GetCollisionComponents() const
{	return CollisionComponents;}

EInteractableLifecycle UActorInteractableComponentBase::GetLifecycleMode() const
{	return LifecycleMode;}

void UActorInteractableComponentBase::SetLifecycleMode(const EInteractableLifecycle& NewMode)
{
	LifecycleMode = NewMode;

	OnLifecycleModeChanged.Broadcast(LifecycleMode);
}

int32 UActorInteractableComponentBase::GetLifecycleCount() const
{	return LifecycleCount;}

void UActorInteractableComponentBase::SetLifecycleCount(const int32 NewLifecycleCount)
{
	switch (LifecycleMode)
	{
		case EInteractableLifecycle::EIL_Cycled:
			if (NewLifecycleCount < -1)
			{
				LifecycleCount = -1;
				OnLifecycleCountChanged.Broadcast(LifecycleCount);
			}
			else if (NewLifecycleCount < 2)
			{
				LifecycleCount = 2;
				OnLifecycleCountChanged.Broadcast(LifecycleCount);
			}
			else if (NewLifecycleCount > 2)
			{
				LifecycleCount = NewLifecycleCount;
				OnLifecycleCountChanged.Broadcast(LifecycleCount);
			}
			break;
		case EInteractableLifecycle::EIL_OnlyOnce:
		case EInteractableLifecycle::Default:
		default: break;
	}
}

int32 UActorInteractableComponentBase::GetRemainingLifecycleCount() const
{ return RemainingLifecycleCount; }

float UActorInteractableComponentBase::GetCooldownPeriod() const
{ return CooldownPeriod; }

void UActorInteractableComponentBase::SetCooldownPeriod(const float NewCooldownPeriod)
{
	switch (LifecycleMode)
	{
		case EInteractableLifecycle::EIL_Cycled:
			LifecycleCount = FMath::Max(0.1f, NewCooldownPeriod);
			OnLifecycleCountChanged.Broadcast(LifecycleCount);
			break;
		case EInteractableLifecycle::EIL_OnlyOnce:
		case EInteractableLifecycle::Default:
		default: break;
	}
}

FKey UActorInteractableComponentBase::GetInteractionKeyForPlatform(const FString& RequestedPlatform) const
{
	if(const FInteractionKeySetup* KeySet = InteractionKeysPerPlatform.Find(RequestedPlatform))
	{
		if (KeySet->Keys.Num() == 0) return FKey();

		return KeySet->Keys[0];
	}
	
	return FKey();
}

TArray<FKey> UActorInteractableComponentBase::GetInteractionKeysForPlatform(const FString& RequestedPlatform) const
{
	if(const FInteractionKeySetup* KeySet = InteractionKeysPerPlatform.Find(RequestedPlatform))
	{
		if (KeySet->Keys.Num() == 0) return TArray<FKey>();

		return KeySet->Keys;
	}
	
	return TArray<FKey>();
}

void UActorInteractableComponentBase::SetInteractionKey(const FString& Platform, const FKey NewInteractorKey)
{
	if (const auto KeySet = InteractionKeysPerPlatform.Find(Platform))
	{
		if (KeySet->Keys.Contains(NewInteractorKey)) return;

		KeySet->Keys.Add(NewInteractorKey);
	}
}

TMap<FString, FInteractionKeySetup> UActorInteractableComponentBase::GetInteractionKeys() const
{	return InteractionKeysPerPlatform;}

bool UActorInteractableComponentBase::FindKey(const FKey& RequestedKey, const FString& Platform) const
{
	if (const auto KeySet = InteractionKeysPerPlatform.Find(Platform))
	{
		return KeySet->Keys.Contains(RequestedKey);
	}
	
	return false;
}

void UActorInteractableComponentBase::AddCollisionComponent(UPrimitiveComponent* CollisionComp)
{
	if (CollisionComp == nullptr) return;
	if (CollisionComponents.Contains(CollisionComp)) return;
	
	CollisionComponents.Add(CollisionComp);
	
	BindCollisionShape(CollisionComp);
	
	OnCollisionComponentAdded.Broadcast(CollisionComp);
}

void UActorInteractableComponentBase::AddCollisionComponents(const TArray<UPrimitiveComponent*> NewCollisionComponents)
{
	for (UPrimitiveComponent* const Itr : NewCollisionComponents)
	{
		AddCollisionComponent(Itr);
	}
}

void UActorInteractableComponentBase::RemoveCollisionComponent(UPrimitiveComponent* CollisionComp)
{
	if (CollisionComp == nullptr) return;
	if (!CollisionComponents.Contains(CollisionComp)) return;
	
	CollisionComponents.Remove(CollisionComp);

	UnbindCollisionShape(CollisionComp);
	
	OnCollisionComponentRemoved.Broadcast(CollisionComp);
}

void UActorInteractableComponentBase::RemoveCollisionComponents(const TArray<UPrimitiveComponent*> RemoveCollisionComponents)
{
	for (UPrimitiveComponent* const Itr : RemoveCollisionComponents)
	{
		RemoveCollisionComponent(Itr);
	}
}

TArray<UMeshComponent*> UActorInteractableComponentBase::GetHighlightableComponents() const
{	return HighlightableComponents;}

void UActorInteractableComponentBase::AddHighlightableComponent(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr) return;
	if (HighlightableComponents.Contains(MeshComponent)) return;

	HighlightableComponents.Add(MeshComponent);

	BindHighlightableMesh(MeshComponent);

	OnHighlightableComponentAdded.Broadcast(MeshComponent);
}

void UActorInteractableComponentBase::AddHighlightableComponents(const TArray<UMeshComponent*> AddMeshComponents)
{
	for (UMeshComponent* const Itr : HighlightableComponents)
	{
		AddHighlightableComponent(Itr);
	}
}

void UActorInteractableComponentBase::RemoveHighlightableComponent(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr) return;
	if (!HighlightableComponents.Contains(MeshComponent)) return;

	HighlightableComponents.Remove(MeshComponent);

	UnbindHighlightableMesh(MeshComponent);

	OnHighlightableComponentRemoved.Broadcast(MeshComponent);
}

void UActorInteractableComponentBase::RemoveHighlightableComponents(const TArray<UMeshComponent*> RemoveMeshComponents)
{
	for (UMeshComponent* const Itr : RemoveMeshComponents)
	{
		RemoveHighlightableComponent(Itr);
	}
}

UMeshComponent* UActorInteractableComponentBase::FindMeshByName(const FName Name) const
{
	if (!GetOwner()) return nullptr;

	TArray<UMeshComponent*> MeshComponents;
	GetOwner()->GetComponents(MeshComponents);

	for (const auto& Itr : MeshComponents)
	{
		if (Itr && Itr->GetName().Equals(Name.ToString()))
		{
			return Itr;
		}
	}

	return nullptr;
}

UMeshComponent* UActorInteractableComponentBase::FindMeshByTag(const FName Tag) const
{
	if (!GetOwner()) return nullptr;

	TArray<UMeshComponent*> MeshComponents;
	GetOwner()->GetComponents(MeshComponents);

	for (const auto& Itr : MeshComponents)
	{
		if (Itr && Itr->ComponentHasTag(Tag))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

UPrimitiveComponent* UActorInteractableComponentBase::FindPrimitiveByName(const FName Name) const
{
	if (!GetOwner()) return nullptr;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetOwner()->GetComponents(PrimitiveComponents);

	for (const auto& Itr : PrimitiveComponents)
	{
		if (Itr && Itr->GetName().Equals(Name.ToString()))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

UPrimitiveComponent* UActorInteractableComponentBase::FindPrimitiveByTag(const FName Tag) const
{
	if (!GetOwner()) return nullptr;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetOwner()->GetComponents(PrimitiveComponents);

	for (const auto& Itr : PrimitiveComponents)
	{
		if (Itr && Itr->ComponentHasTag(Tag))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

TArray<FName> UActorInteractableComponentBase::GetCollisionOverrides() const
{	return CollisionOverrides;}

TArray<FName> UActorInteractableComponentBase::GetHighlightableOverrides() const
{	return HighlightableOverrides;}

FDataTableRowHandle UActorInteractableComponentBase::GetInteractableData()
{ return InteractableData; }

void UActorInteractableComponentBase::SetInteractableData(FDataTableRowHandle NewData)
{ InteractableData = NewData; }

FText UActorInteractableComponentBase::GetInteractableName() const
{ return InteractableName; }

void UActorInteractableComponentBase::SetInteractableName(const FText& NewName)
{
	if (NewName.IsEmpty()) return;
	InteractableName = NewName;
}

ETimingComparison UActorInteractableComponentBase::GetComparisonMethod() const
{ return ComparisonMethod; }

void UActorInteractableComponentBase::SetComparisonMethod(const ETimingComparison Value)
{ ComparisonMethod = Value; }

void UActorInteractableComponentBase::SetDefaults()
{
	if (const auto DefaultTable = UActorInteractionFunctionLibrary::GetInteractableDefaultDataTable())
	{
		InteractableData.DataTable = DefaultTable;
	}
	
	if (const auto DefaultWidgetClass = UActorInteractionFunctionLibrary::GetInteractableDefaultWidgetClass())
	{
		if (DefaultWidgetClass != nullptr)
		{
			SetWidgetClass(DefaultWidgetClass.Get());
		}
	}
}

void UActorInteractableComponentBase::InteractorFound(const TScriptInterface<IActorInteractorInterface>& FoundInteractor)
{
	if (CanBeTriggered())
	{
		ToggleWidgetVisibility(true);
		
		SetInteractor(FoundInteractor);
		
		Execute_OnInteractorFoundEvent(this, FoundInteractor);
	}
}

void UActorInteractableComponentBase::InteractorLost(const TScriptInterface<IActorInteractorInterface>& LostInteractor)
{
	if (LostInteractor.GetInterface() == nullptr) return;
	
	if (Interactor == LostInteractor)
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer_Interaction);
		GetWorld()->GetTimerManager().ClearTimer(Timer_ProgressExpiration);
		
		ToggleWidgetVisibility(false);

		switch (GetState())
		{
			case EInteractableStateV2::EIS_Cooldown:
				if (GetInteractor().GetObject() == nullptr)
				{
					SetState(DefaultInteractableState);
				}
				break;
			case EInteractableStateV2::EIS_Asleep:
			case EInteractableStateV2::EIS_Suppressed:
				SetState(DefaultInteractableState);
				break;
			case EInteractableStateV2::EIS_Active:
			case EInteractableStateV2::EIS_Awake:
			case EInteractableStateV2::EIS_Paused:
				SetState(DefaultInteractableState);
				break;
			case EInteractableStateV2::EIS_Completed:
			case EInteractableStateV2::EIS_Disabled:
			case EInteractableStateV2::Default:
			default: break;
		}
		
		if (Interactor.GetInterface() != nullptr)
		{
			Interactor->GetOnInteractableSelectedHandle().RemoveDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
			Interactor->GetOnInteractableLostHandle().RemoveDynamic(this, &UActorInteractableComponentBase::InteractableLost);
		}
		
		SetInteractor(nullptr);
		Execute_OnInteractorLostEvent(this, LostInteractor);

		OnInteractionCanceled.Broadcast();
	}
}

void UActorInteractableComponentBase::InteractionCompleted(const float& TimeCompleted, const TScriptInterface<IActorInteractorInterface>& CausingInteractor)
{
	ToggleWidgetVisibility(false);
	
	if (LifecycleMode == EInteractableLifecycle::EIL_Cycled)
	{
		if (TriggerCooldown()) return;
	}
	
	FString ErrorMessage;
	if( CompleteInteractable(ErrorMessage))
	{
		Execute_OnInteractionCompletedEvent(this, TimeCompleted, CausingInteractor);
	}
	else AIntP_LOG(Display, TEXT("%s"), *ErrorMessage);
}

void UActorInteractableComponentBase::InteractionCycleCompleted(const float& CompletedTime, const int32 CyclesRemaining, const TScriptInterface<IActorInteractorInterface>& CausingInteractor)
{
	Execute_OnInteractionCycleCompletedEvent(this, CompletedTime, CyclesRemaining, CausingInteractor);
}

void UActorInteractableComponentBase::InteractionStarted(const float& TimeStarted, const FKey& PressedKey, const TScriptInterface<IActorInteractorInterface>& CausingInteractor)
{
	if (CanInteract())
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer_ProgressExpiration);
		
		SetState(EInteractableStateV2::EIS_Active);
		Execute_OnInteractionStartedEvent(this, TimeStarted, PressedKey, CausingInteractor);
	}
}

void UActorInteractableComponentBase::InteractionStopped(const float& TimeStarted, const FKey& PressedKey, const TScriptInterface<IActorInteractorInterface>& CausingInteractor)
{
	if (!GetWorld()) return;

	PauseInteraction(TimeStarted, PressedKey, CausingInteractor);
}

void UActorInteractableComponentBase::InteractionCanceled()
{
	if (CanInteract())
	{
		ToggleWidgetVisibility(false);
		
		GetWorld()->GetTimerManager().ClearTimer(Timer_Interaction);
		GetWorld()->GetTimerManager().ClearTimer(Timer_ProgressExpiration);
		
		switch (GetState())
		{
			case EInteractableStateV2::EIS_Cooldown:
			case EInteractableStateV2::EIS_Awake:
				if (GetInteractor().GetObject() == nullptr)
				{
					SetState(DefaultInteractableState);
				}
				break;
			case EInteractableStateV2::EIS_Active:
			case EInteractableStateV2::EIS_Asleep:
			case EInteractableStateV2::EIS_Completed:
			case EInteractableStateV2::EIS_Disabled:
			case EInteractableStateV2::EIS_Suppressed:
				SetState(DefaultInteractableState);
				break;
			case EInteractableStateV2::Default:
			default: break;
		}
		
		Execute_OnInteractionCanceledEvent(this);
	}
}

void UActorInteractableComponentBase::InteractionLifecycleCompleted()
{
	SetState(EInteractableStateV2::EIS_Completed);

	Execute_OnLifecycleCompletedEvent(this);
}

void UActorInteractableComponentBase::InteractionCooldownCompleted()
{
	if (Interactor.GetInterface() != nullptr)
	{
		StartHighlight();

		SetState(DefaultInteractableState);
		
		if (Interactor->GetActiveInteractable() == this)
		{
			SetState(EInteractableStateV2::EIS_Active);
		}
		else
		{
			StopHighlight();
			SetState(DefaultInteractableState);
		}
	}
	else
	{
		StopHighlight();
		SetState(DefaultInteractableState);
	}
	
	Execute_OnCooldownCompletedEvent(this);
}

void UActorInteractableComponentBase::OnInteractableBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!CanBeTriggered()) return;
	if (IsInteracting()) return;
	if (!OtherActor) return;
	if (!OtherComp) return;

	if (OtherComp->GetCollisionResponseToChannel(CollisionChannel) == ECollisionResponse::ECR_Ignore) return;

	TArray<UActorComponent*> InteractorComponents = OtherActor->GetComponentsByInterface(UActorInteractorInterface::StaticClass());

	if (InteractorComponents.Num() == 0) return;
	
	for (const auto& Itr : InteractorComponents)
	{
		TScriptInterface<IActorInteractorInterface> FoundInteractor;
		if (IgnoredClasses.Contains(Itr->StaticClass())) continue;
		
		FoundInteractor = Itr;
		FoundInteractor.SetObject(Itr);
		FoundInteractor.SetInterface(Cast<IActorInteractorInterface>(Itr));

		if (FoundInteractor->CanInteract())
		{
			switch (FoundInteractor->GetState())
			{
				case EInteractorStateV2::EIS_Active:
				case EInteractorStateV2::EIS_Awake:
					if (FoundInteractor->GetResponseChannel() != GetCollisionChannel()) continue;
					FoundInteractor->GetOnInteractableLostHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableLost);
					FoundInteractor->GetOnInteractableSelectedHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
					OnInteractorFound.Broadcast(FoundInteractor);
					OnInteractorOverlapped.Broadcast(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
					break;
				case EInteractorStateV2::EIS_Asleep:
				case EInteractorStateV2::EIS_Suppressed:
				case EInteractorStateV2::EIS_Disabled:
				case EInteractorStateV2::Default:
					break;
			}
		}
	}
}

void UActorInteractableComponentBase::OnInteractableStopOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;

	TArray<UActorComponent*> InteractorComponents = OtherActor->GetComponentsByInterface(UActorInteractorInterface::StaticClass());

	if (InteractorComponents.Num() == 0) return;

	for (const auto& Itr : InteractorComponents)
	{
		TScriptInterface<IActorInteractorInterface> LostInteractor;
		if (IgnoredClasses.Contains(Itr->StaticClass())) continue;
		
		LostInteractor = Itr;
		LostInteractor.SetObject(Itr);
		LostInteractor.SetInterface(Cast<IActorInteractorInterface>(Itr));

		if (LostInteractor->CanInteract())
		{
			if (LostInteractor == GetInteractor())
			{
				GetInteractor()->GetOnInteractableLostHandle().RemoveDynamic(this, &UActorInteractableComponentBase::InteractableLost);
				GetInteractor()->GetOnInteractableLostHandle().Broadcast(this);
				
				OnInteractorLost.Broadcast(GetInteractor());
				OnInteractorStopOverlap.Broadcast(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
				
				return;
			}
		}
	}
}

void UActorInteractableComponentBase::OnInteractableTraced(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!CanBeTriggered()) return;
	if (!OtherActor) return;

	TArray<UActorComponent*> InteractorComponents = OtherActor->GetComponentsByInterface(UActorInteractorInterface::StaticClass());

	if (InteractorComponents.Num() == 0) return;
	
	for (const auto& Itr : InteractorComponents)
	{
		TScriptInterface<IActorInteractorInterface> FoundInteractor;
		if (IgnoredClasses.Contains(Itr->StaticClass())) continue;
		
		FoundInteractor = Itr;
		FoundInteractor.SetObject(Itr);
		FoundInteractor.SetInterface(Cast<IActorInteractorInterface>(Itr));

		switch (FoundInteractor->GetState())
		{
			case EInteractorStateV2::EIS_Active:
			case EInteractorStateV2::EIS_Awake:
				if (FoundInteractor->CanInteract() == false) return;
				if (FoundInteractor->GetResponseChannel() != GetCollisionChannel()) continue;
				FoundInteractor->GetOnInteractableLostHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableLost);
				FoundInteractor->GetOnInteractableSelectedHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
				OnInteractorFound.Broadcast(FoundInteractor);
				Execute_OnInteractableTracedEvent(this, HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
				break;
			case EInteractorStateV2::EIS_Asleep:
			case EInteractorStateV2::EIS_Suppressed:
			case EInteractorStateV2::EIS_Disabled:
			case EInteractorStateV2::Default:
				break;
		}
	}
}

void UActorInteractableComponentBase::OnInteractionProgressExpired(const float ExpirationTime, const FKey UsedKey, const TScriptInterface<IActorInteractorInterface>& CausingInteractor)
{
	if (!GetWorld()) return;
	
	switch (GetState())
	{
		case EInteractableStateV2::EIS_Paused:
			{
				GetWorld()->GetTimerManager().ClearTimer(Timer_Interaction);
				GetWorld()->GetTimerManager().ClearTimer(Timer_ProgressExpiration);
				
				if (DoesHaveInteractor() && GetInteractor()->GetActiveInteractable() == this)
				{
					SetState(EInteractableStateV2::EIS_Active);
				}
				else
				{
					Execute_OnInteractionStoppedEvent(this, ExpirationTime, UsedKey, CausingInteractor);
				}
			}
			break;
		case EInteractableStateV2::EIS_Active: break;
		case EInteractableStateV2::EIS_Awake: break;
		case EInteractableStateV2::EIS_Cooldown: break;
		case EInteractableStateV2::EIS_Completed: break;
		case EInteractableStateV2::EIS_Disabled: break;
		case EInteractableStateV2::EIS_Suppressed: break;
		case EInteractableStateV2::EIS_Asleep: break;
		case EInteractableStateV2::Default: break;
	}
}

void UActorInteractableComponentBase::InteractableSelected(const TScriptInterface<IActorInteractableInterface>& Interactable)
{
 	if (Interactable == this)
 	{
 		StartHighlight();
 		
 		SetState(EInteractableStateV2::EIS_Active);
 		OnInteractableSelected.Broadcast(Interactable);
 	}
	else
	{
		OnInteractionCanceled.Broadcast();
		
		SetState(DefaultInteractableState);
		OnInteractorLost.Broadcast(GetInteractor());
	}
}

void UActorInteractableComponentBase::InteractableLost(const TScriptInterface<IActorInteractableInterface>& Interactable)
{
	if (Interactable == this)
	{
		switch (GetState())
		{
			case EInteractableStateV2::EIS_Active:
				SetState(DefaultInteractableState);
				break;
			case EInteractableStateV2::EIS_Cooldown:
			case EInteractableStateV2::EIS_Awake:
				if (GetInteractor().GetObject() == nullptr)
				{
					SetState(DefaultInteractableState);
				}
				break;
			case EInteractableStateV2::EIS_Asleep:
			case EInteractableStateV2::EIS_Completed:
			case EInteractableStateV2::EIS_Disabled:
			case EInteractableStateV2::EIS_Suppressed:
			case EInteractableStateV2::Default:
			default: break;
		}
		
		OnInteractorLost.Broadcast(GetInteractor());
	}
}

void UActorInteractableComponentBase::FindAndAddCollisionShapes()
{
	for (const auto& Itr : CollisionOverrides)
	{
		if (const auto NewCollision = FindPrimitiveByName(Itr))
		{
			AddCollisionComponent(NewCollision);
			BindCollisionShape(NewCollision);
		}
		else
		{
			if (const auto NewCollisionByTag = FindPrimitiveByTag(Itr))
			{
				AddCollisionComponent(NewCollisionByTag);
				BindCollisionShape(NewCollisionByTag);
			}
			else AIntP_LOG(Error, TEXT("[Actor Interactable Component] Primitive Component '%s' not found!"), *Itr.ToString())
		}
	}
}

void UActorInteractableComponentBase::FindAndAddHighlightableMeshes()
{
	for (const auto& Itr : HighlightableOverrides)
	{
		if (const auto NewMesh = FindMeshByName(Itr))
		{
			AddHighlightableComponent(NewMesh);
			BindHighlightableMesh(NewMesh);
		}
		else
		{
			if (const auto NewHighlightByTag = FindMeshByTag(Itr))
			{
				AddHighlightableComponent(NewHighlightByTag);
				BindHighlightableMesh(NewHighlightByTag);
			}
			else AIntP_LOG(Error, TEXT("[Actor Interactable Component] Mesh Component '%s' not found!"), *Itr.ToString())
		}
	}
}

bool UActorInteractableComponentBase::TriggerCooldown()
{
	if (LifecycleCount != -1)
	{
		const int32 TempRemainingLifecycleCount = RemainingLifecycleCount - 1;
		RemainingLifecycleCount = FMath::Max(0, TempRemainingLifecycleCount);
	}
	
	if (GetWorld())
	{
		if (RemainingLifecycleCount == 0) return false;

		SetState(EInteractableStateV2::EIS_Cooldown);

		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UActorInteractableComponentBase::OnCooldownCompletedCallback);
		
		GetWorld()->GetTimerManager().SetTimer
		(
			Timer_Cooldown,
			Delegate,
			CooldownPeriod,
			false
		);

		for (const auto& Itr : CollisionComponents)
		{
			UnbindCollisionShape(Itr);
		}

		OnInteractionCycleCompleted.Broadcast(GetWorld()->GetTimeSeconds(), RemainingLifecycleCount, GetInteractor());
		return true;
	}

	return false;
}

void UActorInteractableComponentBase::ToggleWidgetVisibility(const bool IsVisible)
{
	if (GetWidget())
	{
		UpdateInteractionWidget();
	}
}

void UActorInteractableComponentBase::BindCollisionShape(UPrimitiveComponent* PrimitiveComponent) const
{
	if (!PrimitiveComponent) return;
	
	PrimitiveComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableBeginOverlap);
	PrimitiveComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableStopOverlap);

	FCollisionShapeCache CachedValues;
	CachedValues.bGenerateOverlapEvents = PrimitiveComponent->GetGenerateOverlapEvents();
	CachedValues.CollisionEnabled = PrimitiveComponent->GetCollisionEnabled();
	CachedValues.CollisionResponse = GetCollisionResponseToChannel(CollisionChannel);
	
	CachedCollisionShapesSettings.Add(PrimitiveComponent, CachedValues);
	
	PrimitiveComponent->SetGenerateOverlapEvents(true);
	PrimitiveComponent->SetCollisionResponseToChannel(CollisionChannel, ECollisionResponse::ECR_Overlap);

	switch (PrimitiveComponent->GetCollisionEnabled())
	{
		case ECollisionEnabled::NoCollision:
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			break;
		case ECollisionEnabled::QueryOnly:
		case ECollisionEnabled::PhysicsOnly:
		case ECollisionEnabled::QueryAndPhysics:
		default: break;
	}
}

void UActorInteractableComponentBase::UnbindCollisionShape(UPrimitiveComponent* PrimitiveComponent) const
{
	if(!PrimitiveComponent) return;
	
	PrimitiveComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UActorInteractableComponentBase::OnInteractableBeginOverlap);
	PrimitiveComponent->OnComponentEndOverlap.RemoveDynamic(this, &UActorInteractableComponentBase::OnInteractableStopOverlap);

	if (CachedCollisionShapesSettings.Find(PrimitiveComponent))
	{
		PrimitiveComponent->SetGenerateOverlapEvents(CachedCollisionShapesSettings[PrimitiveComponent].bGenerateOverlapEvents);
		PrimitiveComponent->SetCollisionEnabled(CachedCollisionShapesSettings[PrimitiveComponent].CollisionEnabled);
		PrimitiveComponent->SetCollisionResponseToChannel(CollisionChannel, CachedCollisionShapesSettings[PrimitiveComponent].CollisionResponse);
	}
	else
	{
		PrimitiveComponent->SetGenerateOverlapEvents(true);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PrimitiveComponent->SetCollisionResponseToChannel(CollisionChannel, ECollisionResponse::ECR_Overlap);
	}
}

void UActorInteractableComponentBase::BindHighlightableMesh(UMeshComponent* MeshComponent) const
{
	if (!MeshComponent) return;
	
	MeshComponent->SetRenderCustomDepth(true);
}

void UActorInteractableComponentBase::UnbindHighlightableMesh(UMeshComponent* MeshComponent) const
{
	if (!MeshComponent) return;
	
	MeshComponent->SetRenderCustomDepth(false);
}

void UActorInteractableComponentBase::ToggleDebug()
{
	DebugSettings.DebugMode = !DebugSettings.DebugMode;
}

void UActorInteractableComponentBase::AutoSetup()
{
	switch (SetupType)
	{
		case ESetupType::EST_FullAll:
			{
				if (GetOwner() == nullptr) break;

				TArray<UPrimitiveComponent*> OwnerPrimitives;
				GetOwner()->GetComponents(OwnerPrimitives);

				TArray<UMeshComponent*> OwnerMeshes;
				GetOwner()->GetComponents(OwnerMeshes);

				for (const auto& Itr : OwnerPrimitives)
				{
					if (Itr)
					{
						AddCollisionComponent(Itr);
					}
				}

				for (const auto& Itr : OwnerMeshes)
				{
					if (Itr)
					{
						AddHighlightableComponent(Itr);
					}
				}
			}
			break;
		case ESetupType::EST_AllParent:
			{
				// Get all Parent Components
				TArray<USceneComponent*> ParentComponents;
				GetParentComponents(ParentComponents);

				// Iterate over them and assign them properly
				if (ParentComponents.Num() > 0)
				{
					for (const auto& Itr : ParentComponents)
					{
						if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Itr))
						{
							AddCollisionComponent(PrimitiveComp);

							if (UMeshComponent* MeshComp = Cast<UMeshComponent>(PrimitiveComp))
							{
								AddHighlightableComponent(MeshComp);
							}
						}
					}
				}
			}
			break;
		case ESetupType::EST_Quick:
			{
				if (USceneComponent* ParentComponent = GetAttachParent())
				{
					if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(ParentComponent))
					{
						AddCollisionComponent(PrimitiveComp);

						if (UMeshComponent* MeshComp = Cast<UMeshComponent>(PrimitiveComp))
						{
							AddHighlightableComponent(MeshComp);
						}
					}
				}
			}
			break;
		default:
			break;
	}
	
	FindAndAddCollisionShapes();
	FindAndAddHighlightableMeshes();
}

void UActorInteractableComponentBase::OnCooldownCompletedCallback()
{
	if (!GetWorld())
	{
		AIntP_LOG(Error, TEXT("[TriggerCooldown] Interactable has no World, cannot request OnCooldownCompletedEvent!"))
		return;
	}
	
	for (const auto& Itr : CollisionComponents)
	{
		BindCollisionShape(Itr);
	}
	
	OnCooldownCompleted.Broadcast();
}

bool UActorInteractableComponentBase::ValidateInteractable() const
{
	if (GetWidgetClass().Get() == nullptr)
	{
		AIntP_LOG(Error, TEXT("[%s] Has null Widget Class! Disabled!"), *GetName())
		return false;
	}
	if (GetWidgetClass() != UActorInteractableWidget::StaticClass())
	{
		if (GetWidgetClass()->ImplementsInterface(UActorInteractionWidget::StaticClass()) == false)
		{
			AIntP_LOG(Error, TEXT("[%s] Has invalid Widget Class! Disabled!"), *GetName())
			return false;
		}
	}

	return true;
}

void UActorInteractableComponentBase::UpdateInteractionWidget()
{
	if (UUserWidget* UserWidget = GetWidget() )
	{
		if (UserWidget->Implements<UActorInteractionWidget>())
		{
			TScriptInterface<IActorInteractionWidget> InteractionWidget = UserWidget;
			InteractionWidget.SetObject(UserWidget);
			InteractionWidget.SetInterface(Cast<IActorInteractionWidget>(UserWidget));

			InteractionWidget->Execute_UpdateWidget(UserWidget, this);
		}
		else if (UActorInteractableWidget* InteractableWidget = Cast<UActorInteractableWidget>(UserWidget))
		{
			InteractableWidget->InitializeInteractionWidget(FText::FromString("E"), FText::FromString("Object"), FText::FromString("Use"), nullptr, nullptr);
			InteractableWidget->SetInteractionProgress(GetInteractionProgress());
		}
	}
}

void UActorInteractableComponentBase::InteractableDependencyStartedCallback(const TScriptInterface<IActorInteractableInterface>& NewMaster)
{
	if (NewMaster.GetObject() == nullptr) return;

	// Force lower weight but never higher than it was before.
	const int32 NewWeight = FMath::Min(InteractionWeight, NewMaster->GetInteractableWeight() - 1);
	SetInteractableWeight(NewWeight);
}

void UActorInteractableComponentBase::InteractableDependencyStoppedCallback(const TScriptInterface<IActorInteractableInterface>& FormerMaster)
{
	SetInteractableWeight(CachedInteractionWeight);
}

#if (!UE_BUILD_SHIPPING || WITH_EDITOR)
#if WITH_EDITOR

void UActorInteractableComponentBase::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.GetPropertyName() : NAME_None;

	FString interactableName = GetName();
	// Format Name
	{
		if (interactableName.Contains(TEXT("_GEN_VARIABLE")))
		{
			interactableName.ReplaceInline(TEXT("_GEN_VARIABLE"), TEXT(""));
		}
		if(interactableName.EndsWith(TEXT("_C")) && interactableName.StartsWith(TEXT("Default__")))
		{
		
			interactableName.RightChopInline(9);
			interactableName.LeftChopInline(2);
		}
	}
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UActorInteractableComponentBase, DefaultInteractableState))
	{
		if
		(
			DefaultInteractableState == EInteractableStateV2::EIS_Active ||
			DefaultInteractableState == EInteractableStateV2::EIS_Completed ||
			DefaultInteractableState == EInteractableStateV2::EIS_Cooldown
		)
		{
			const FText ErrorMessage = FText::FromString
			(
				interactableName.Append(TEXT(": DefaultInteractableState cannot be ")).Append(GetEnumValueAsString("EInteractableStateV2", DefaultInteractableState)).Append(TEXT("!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));

			DefaultInteractableState = EInteractableStateV2::EIS_Awake;
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UActorInteractableComponentBase, LifecycleCount))
	{
		if (LifecycleMode == EInteractableLifecycle::EIL_Cycled)
		{
			if (LifecycleCount == 0 || LifecycleCount == 1)
			{
				if (DebugSettings.EditorDebugMode)
				{
					const FText ErrorMessage = FText::FromString
					(
						interactableName.Append(TEXT(": Cycled LifecycleCount cannot be: ")).Append(FString::FromInt(LifecycleCount)).Append(TEXT("!"))
					);
				
					FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));
				}
				
				LifecycleCount = 2.f;
				RemainingLifecycleCount = LifecycleCount;
			}
		}
	}
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UActorInteractableComponentBase, InteractionPeriod))
	{
		if (InteractionPeriod < -1.f)
		{
			const FText ErrorMessage = FText::FromString
			(
				interactableName.Append(TEXT(": InteractionPeriod cannot be less than -1!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));

			InteractionPeriod = -1.f;
		}

		if (InteractionPeriod > -1.f && InteractionPeriod < 0.1f)
		{
			InteractionPeriod = 0.1f;
		}

		if (FMath::IsNearlyZero(InteractionPeriod, 0.001f))
		{
			InteractionPeriod = 0.1f;
		}
	}
		
	if (PropertyName == TEXT("WidgetClass"))
	{
		if (GetWidgetClass() == nullptr)
		{
			const FText ErrorMessage = FText::FromString
			(
				interactableName.Append(TEXT(": Widget Class is NULL!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));
		}
		else
		{
			if (!GetWidgetClass()->IsChildOf(UActorInteractableWidget::StaticClass()))
			{
				if (GetWidgetClass()->ImplementsInterface(UActorInteractionWidget::StaticClass()) == false)
				{
					const FText ErrorMessage = FText::FromString
					(
						interactableName.Append(TEXT(": Widget Class must either implement 'ActorInteractionWidget Interface' or inherit from 'ActorInteractableWidget' class!"))
					);
					
					SetWidgetClass(nullptr);
					FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));
				}
			}
		}
	}

	if (PropertyName == TEXT("Space"))
	{
		if (GetWidgetSpace() == EWidgetSpace::World)
		{
			SetWorldScale3D(FVector(0.01f));
			SetDrawSize(FVector2D(2000));
			bDrawAtDesiredSize = false;
		}
		else
		{
			SetWorldScale3D(FVector(1.f));
			bDrawAtDesiredSize = false;
		}

		const FText ErrorMessage = FText::FromString
		(
			interactableName.Append(TEXT(": UI Space changed! Component Scale has been updated. Update 'DrawSize' to match new Widget Space!"))
		);
		FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("NotificationList.DefaultMessage"));
	}
}

EDataValidationResult UActorInteractableComponentBase::IsDataValid(TArray<FText>& ValidationErrors)
{
	const auto DefaultValue = Super::IsDataValid(ValidationErrors);
	bool bAnyError = false;

	FString interactableName = GetName();
	// Format Name
	{
		if (interactableName.Contains(TEXT("_GEN_VARIABLE")))
		{
			interactableName.ReplaceInline(TEXT("_GEN_VARIABLE"), TEXT(""));
		}
		if(interactableName.EndsWith(TEXT("_C")) && interactableName.StartsWith(TEXT("Default__")))
		{
		
			interactableName.RightChopInline(9);
			interactableName.LeftChopInline(2);
		}
	}
	
	if
	(
		DefaultInteractableState == EInteractableStateV2::EIS_Active ||
		DefaultInteractableState == EInteractableStateV2::EIS_Completed ||
		DefaultInteractableState == EInteractableStateV2::EIS_Cooldown
	)
	{
		const FText ErrorMessage = FText::FromString
		(
			interactableName.Append(TEXT(": DefaultInteractableState cannot be")).Append(GetEnumValueAsString("EInteractableStateV2", DefaultInteractableState)).Append(TEXT("!"))
		);

		DefaultInteractableState = EInteractableStateV2::EIS_Awake;
		
		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}

	if (InteractionPeriod < -1.f)
	{
		const FText ErrorMessage = FText::FromString
		(
			interactableName.Append(TEXT(": DefaultInteractableState cannot be lesser than -1!"))
		);

		InteractionPeriod = -1.f;
		
		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}
	
	if (LifecycleMode == EInteractableLifecycle::EIL_Cycled && (LifecycleCount == 0 || LifecycleCount == 1))
	{
		const FText ErrorMessage = FText::FromString
		(
			interactableName.Append(TEXT(":")).Append(TEXT(" LifecycleCount cannot be %d!"), LifecycleCount)
		);
			
		LifecycleCount = 2.f;
		RemainingLifecycleCount = LifecycleCount;
		
		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}

	if (GetWidgetClass() == nullptr)
	{
		const FText ErrorMessage = FText::FromString
		(
			interactableName.Append(TEXT(": Widget Class is NULL!"))
		);

		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}
	else
	{
		if (!GetWidgetClass()->IsChildOf(UActorInteractableWidget::StaticClass()))
		{
			if (GetWidgetClass()->ImplementsInterface(UActorInteractionWidget::StaticClass()) == false)
			{
				const FText ErrorMessage = FText::FromString
				(
					interactableName.Append(TEXT(" : Widget Class must either implement 'ActorInteractionWidget Interface' or inherit from 'ActorInteractableWidget' class!"))
				);

				SetWidgetClass(nullptr);
				ValidationErrors.Add(ErrorMessage);
				bAnyError = true;
			}
		}
	}
	
	return bAnyError ? EDataValidationResult::Invalid : DefaultValue;
}

bool UActorInteractableComponentBase::Modify(bool bAlwaysMarkDirty)
{
	const bool bResult = Super::Modify(bAlwaysMarkDirty);

	const bool bShouldNotify =
	{
		GetOwner() != nullptr &&
		UActorInteractionFunctionLibrary::IsEditorDebugEnabled() &&
		(
			InteractableData.DataTable == nullptr ||
			GetWidgetClass() == nullptr
		)
	};

	if (bShouldNotify)
	{
		FString interactableName = GetName();
		// Format Name
		{
			if (interactableName.Contains(TEXT("_GEN_VARIABLE")))
			{
				interactableName.ReplaceInline(TEXT("_GEN_VARIABLE"), TEXT(""));
			}
			if (interactableName.Contains(TEXT("SKEL_")))
			{
				interactableName.ReplaceInline(TEXT("SKEL_"), TEXT(""));
			}
			if(interactableName.EndsWith(TEXT("_C")) && interactableName.StartsWith(TEXT("Default__")))
			{
		
				interactableName.RightChopInline(9);
				interactableName.LeftChopInline(2);
			}
		}

		FString ownerName;
		GetOwner()->GetName(ownerName);
		
		const FText ErrorMessage = FText::FromString
		(
			interactableName.Append(" from ").Append(ownerName).Append(TEXT(": Interactable Data or Widget Class are not valid! Use 'SetDefaults' to avoid issues!"))
		);
		FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("NotificationList.DefaultMessage"));

	}
	
	return bResult;
}

#endif

void UActorInteractableComponentBase::DrawDebug()
{
	if (DebugSettings.DebugMode)
	{
		for (const auto& Itr : CollisionComponents)
		{
			Itr->SetHiddenInGame(false);
		}
	}
}

#endif
#pragma endregion

#undef LOCTEXT_NAMESPACE