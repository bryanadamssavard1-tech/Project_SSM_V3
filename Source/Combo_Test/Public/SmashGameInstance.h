#pragma once

// ============================================================
// INCLUDES
// ============================================================

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"

#include "SmashGameInstance.generated.h"


// ============================================================
// FORWARD DECLARATIONS
// ============================================================

class UTexture2D;
class UWorld;
struct FSlateBrush;


// ============================================================
// LOADING PLAYER DATA
// ============================================================

USTRUCT(BlueprintType)
struct FLoadingPlayerData
{
	GENERATED_BODY()

	// 0 = P1, 1 = P2, 2 = P3, 3 = P4
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
	int32 PlayerIndex = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
	FString CharacterName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
	UTexture2D* Portrait = nullptr;
};


// ============================================================
// LOCAL PLAYER CONNECTION DATA
//
// P1 = clavier
// P2/P3/P4 = manettes physiques
// ============================================================

USTRUCT(BlueprintType)
struct FSmashLocalPlayerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Local Multiplayer")
	int32 PlayerIndex = -1;

	// Identifiant physique de la manette.
	// Pas de UPROPERTY ici.
	FInputDeviceId InputDeviceId;

	UPROPERTY(BlueprintReadOnly, Category = "Local Multiplayer")
	bool bIsConnected = false;
};


// ============================================================
// SMASH GAME INSTANCE
//
// Gère :
// - les connexions/déconnexions de manettes
// - la création de P2/P3/P4
// - le routage Device -> PlatformUser -> LocalPlayer
// - les données du loading screen
// ============================================================

UCLASS()
class COMBO_TEST_API USmashGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	// ========================================================
	// INITIALIZATION
	// ========================================================

	virtual void Init() override;


	// ========================================================
	// BLUEPRINT EVENT - LOCAL PLAYER CRÉÉ
	// ========================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Local Multiplayer")
	void OnSmashLocalPlayerCreated(int32 PlayerIndex);

	// ========================================================
// BLUEPRINT EVENT - CONNECTION STATE CHANGED
//
// Appelé lorsqu'une manette déjà associée à un joueur
// est déconnectée ou reconnectée.
//
// PlayerIndex :
// 1 = P2
// 2 = P3
// 3 = P4
//
// bIsConnected :
// true  = connecté
// false = déconnecté
// ========================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Local Multiplayer")
	void OnSmashLocalPlayerConnectionChanged(
		int32 PlayerIndex,
		bool bIsConnected
	);
	// ========================================================
	// LOCAL MULTIPLAYER DATA
	// ========================================================

	TArray<FSmashLocalPlayerData> ConnectedPlayers;


	// ========================================================
	// INPUT DEVICE CONNECTION EVENT
	// ========================================================

	void HandleInputDeviceConnectionChange(
		EInputDeviceConnectionState NewConnectionState,
		FPlatformUserId PlatformUserId,
		FInputDeviceId InputDeviceId
	);


	// ========================================================
	// CREATE LOCAL PLAYER
	//
	// 1 = P2
	// 2 = P3
	// 3 = P4
	// ========================================================

	void CreateSmashLocalPlayer(
		int32 PlayerIndex,
		FInputDeviceId InputDeviceId
	);


	// ========================================================
	// ROUTE PHYSICAL DEVICE TO LOCAL PLAYER
	//
	// Corrige le propriétaire du FInputDeviceId sans modifier
	// le PlatformUser du LocalPlayer.
	// ========================================================

	void RouteInputDeviceToLocalPlayer(
		int32 PlayerIndex,
		FInputDeviceId InputDeviceId
	);


	// ========================================================
	// LOADING SCREEN DATA
	// ========================================================

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
	TArray<FLoadingPlayerData> LoadingPlayers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
	UTexture2D* LoadingBackgroundTexture = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loading Screen")
	UTexture2D* CenterSlashTexture = nullptr;


private:

	// ========================================================
	// LOADING SCREEN FUNCTIONS
	// ========================================================

	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* LoadedWorld);


	// ========================================================
	// SLATE BRUSHES
	// ========================================================

	TSharedPtr<FSlateBrush> LoadingBackgroundBrush;
	TSharedPtr<FSlateBrush> CenterSlashBrush;
	TArray<TSharedPtr<FSlateBrush>> PlayerPortraitBrushes;
};
