#pragma once

// ============================================================================
// 01 - INCLUDES
// ============================================================================
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "SmashGameInstance.generated.h"

class UTexture2D;
class UWorld;
class IInputProcessor;
struct FSlateBrush;

// ============================================================================
// 02 - LOADING PLAYER DATA (INCHANGE)
// ============================================================================
USTRUCT(BlueprintType)
struct FLoadingPlayerData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
    int32 PlayerIndex = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
    FString CharacterName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
    UTexture2D* Portrait = nullptr;
};

// ============================================================================
// 03 - LOCAL PLAYER DATA (P1-P4 MANETTES (APPUYER SUR A/CROIX))
// ============================================================================
USTRUCT(BlueprintType)
struct FSmashLocalPlayerData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Local Multiplayer")
    int32 PlayerIndex = -1;

    FInputDeviceId InputDeviceId;

    UPROPERTY(BlueprintReadOnly, Category = "Local Multiplayer")
    bool bIsConnected = false;
};

// ============================================================================
// 04 - SMASH GAME INSTANCE
// ============================================================================
UCLASS()
class COMBO_TEST_API USmashGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // 04A - CYCLE DE VIE
    virtual void Init() override;
    virtual void Shutdown() override;

    // 04B - EVENTS BLUEPRINT EXISTANTS
    UFUNCTION(BlueprintImplementableEvent, Category = "Local Multiplayer")
    void OnSmashLocalPlayerCreated(int32 PlayerIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Local Multiplayer")
    void OnSmashLocalPlayerConnectionChanged(int32 PlayerIndex, bool bIsConnected);

    // 04C - JOUEURS ET CONNEXIONS
    TArray<FSmashLocalPlayerData> ConnectedPlayers;
    UFUNCTION(BlueprintPure, Category = "Smash|Players")
    TArray<int32> GetConnectedPlayerIndices() const;
    void HandleInputDeviceConnectionChange(
        EInputDeviceConnectionState NewConnectionState,
        FPlatformUserId PlatformUserId,
        FInputDeviceId InputDeviceId);

    // Appele quand une manette non attribuee presse A / Croix.
    void HandleJoinButtonPressed(FInputDeviceId InputDeviceId);

    void CreateSmashLocalPlayer(int32 PlayerIndex, FInputDeviceId InputDeviceId);
    void RouteInputDeviceToLocalPlayer(int32 PlayerIndex, FInputDeviceId InputDeviceId);

    // 04D - LOADING SCREEN DATA (INCHANGE)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
    TArray<FLoadingPlayerData> LoadingPlayers;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
    UTexture2D* LoadingBackgroundTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
    UTexture2D* CenterSlashTexture = nullptr;

private:
    // 04E - LOADING SCREEN
    void BeginLoadingScreen(const FString& MapName);
    void EndLoadingScreen(UWorld* LoadedWorld);

    TSharedPtr<FSlateBrush> LoadingBackgroundBrush;
    TSharedPtr<FSlateBrush> CenterSlashBrush;
    TArray<TSharedPtr<FSlateBrush>> PlayerPortraitBrushes;

    // 04F - DETECTEUR DE BOUTON DES MANETTES NON ATTRIBUEES
    TSharedPtr<IInputProcessor> JoinInputProcessor;
};
