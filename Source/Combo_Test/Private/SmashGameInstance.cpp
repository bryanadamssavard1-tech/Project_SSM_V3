// ============================================================================
// SMASH GAME INSTANCE - LOADING SCREEN FINAL
//
// IMPORTANT :
// - Ce fichier contient la PARTIE LOADING SCREEN de SmashGameInstance.cpp.
// - Garde tes fonctions multiplayer actuelles EN DESSOUS :
//     HandleInputDeviceConnectionChange
//     CreateSmashLocalPlayer
//     RouteInputDeviceToLocalPlayer
// - Ne remets PAS GetMoviePlayer()->PlayMovie() dans BeginLoadingScreen().
// ============================================================================

#include "SmashGameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"



#include "Input/Events.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "MoviePlayer.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Texture2D.h"
#include "Engine/LocalPlayer.h"
#include "Misc/Paths.h"

// ============================================================================
// SLATE
// ============================================================================

#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/SlateBrush.h"
#include "Styling/CoreStyle.h"


// ============================================================================
// JOIN INPUT PREPROCESSOR - DETECTE A / CROIX AVANT L'ATTRIBUTION
// ============================================================================
class FSmashJoinInputProcessor final : public IInputProcessor
{
public:

    // --------------------------------------------------------
    // 01.1 - CONSTRUCTEUR
    // Conserve une référence au GameInstance.
    // --------------------------------------------------------

    explicit FSmashJoinInputProcessor(
        USmashGameInstance* InGameInstance
    )
        : GameInstance(InGameInstance)
    {}

    // --------------------------------------------------------
    // 01.2 - TICK OBLIGATOIRE
    // Nécessaire pour IInputProcessor dans UE 5.6.
    // --------------------------------------------------------

    virtual void Tick(
        const float DeltaTime,
        FSlateApplication& SlateApp,
        TSharedRef<ICursor> Cursor
    ) override
    {
        // Aucun traitement nécessaire.
    }

    // --------------------------------------------------------
    // 01.3 - DÉTECTION DU BOUTON A / CROIX
    // Identifie la manette qui demande à rejoindre.
    // --------------------------------------------------------

    virtual bool HandleKeyDownEvent(
        FSlateApplication& SlateApp,
        const FKeyEvent& InKeyEvent
    ) override
    {
        if (
            InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom
            && !InKeyEvent.IsRepeat()
            )
        {
            if (GameInstance.IsValid())
            {
                const FInputDeviceId InputDeviceId =
                    InKeyEvent.GetInputDeviceId();

                GameInstance->HandleJoinButtonPressed(InputDeviceId);
            }
        }

        // Laisser les autres systèmes recevoir le bouton.
        return false;
    }

private:

    // --------------------------------------------------------
    // 01.4 - RÉFÉRENCE SÉCURISÉE AU GAME INSTANCE
    // --------------------------------------------------------

    TWeakObjectPtr<USmashGameInstance> GameInstance;
};

// ============================================================================
// 01 - GAME INSTANCE INITIALIZATION
// ============================================================================

void USmashGameInstance::Init()
{
    Super::Init();

    // 00 - CAPTER A/CROIX DES MANETTES NON ATTRIBUEES
    if (FSlateApplication::IsInitialized())
    {
        JoinInputProcessor = MakeShared<FSmashJoinInputProcessor>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(JoinInputProcessor);
    }


    // ========================================================================
    // LOCAL MULTIPLAYER - INPUT DEVICE CONNECTION EVENT
    // ========================================================================

    IPlatformInputDeviceMapper::Get()
        .GetOnInputDeviceConnectionChange()
        .AddUObject(
            this,
            &USmashGameInstance::HandleInputDeviceConnectionChange
        );


    // ========================================================================
    // LOADING SCREEN - BACKGROUND
    // ========================================================================

    LoadingBackgroundTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/Project/UI/LoadingScreen/lightning-mcqueen--26021.lightning-mcqueen--26021")
    );

    if (LoadingBackgroundTexture)
    {
        LoadingBackgroundTexture->UpdateResource();

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Loading Screen: Background loaded")
        );
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Loading Screen: Background NOT found")
        );
    }


    // ========================================================================
    // LOADING SCREEN - CENTER SLASH
    // ========================================================================

    CenterSlashTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/Project/UI/LoadingScreen/FX/T_UI_CenterSlash.T_UI_CenterSlash")
    );

    if (CenterSlashTexture)
    {
        CenterSlashTexture->UpdateResource();

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Loading Screen: Center slash loaded")
        );
    }


    // ========================================================================
    // MAP EVENTS
    // ========================================================================

    FCoreUObjectDelegates::PreLoadMap.AddUObject(
        this,
        &USmashGameInstance::BeginLoadingScreen
    );

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this,
        &USmashGameInstance::EndLoadingScreen
    );
}


// ============================================================================
// 02 - BEGIN LOADING SCREEN
// ============================================================================

void USmashGameInstance::BeginLoadingScreen(const FString& MapName)
{
    // ========================================================================
    // 01 - MAP FILTER
    // ========================================================================

    if (!MapName.Contains(TEXT("Final_Arena")))
    {
        if (GetMoviePlayer() && GetMoviePlayer()->IsMovieCurrentlyPlaying())
        {
            GetMoviePlayer()->StopMovie();
        }

        return;
    }


    // ========================================================================
    // 02 - MOVIE PLAYER CHECK
    // ========================================================================

    IGameMoviePlayer* MoviePlayer = GetMoviePlayer();

    if (!MoviePlayer)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Loading Screen: MoviePlayer unavailable")
        );

        return;
    }


    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Loading Screen starting for: %s"),
        *MapName
    );


    // ========================================================================
    // 03 - LOADING SCREEN SETTINGS
    // ========================================================================

    FLoadingScreenAttributes LoadingScreen;

    // Unreal ferme automatiquement le loading screen lorsque la map est prête,
    // mais respecte le temps minimum ci-dessous en Standalone / build packagé.
    LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
    LoadingScreen.bWaitForManualStop = false;
    LoadingScreen.bAllowEngineTick = false;
    LoadingScreen.bMoviesAreSkippable = false;
    LoadingScreen.MinimumLoadingScreenDisplayTime = 5.0f;


    // ========================================================================
    // 04 - STAGE NAME
    // ========================================================================

    FString StageName = FPaths::GetBaseFilename(MapName);
    StageName.ReplaceInline(TEXT("_"), TEXT(" "));
    StageName = StageName.ToUpper();


    // ========================================================================
    // 05 - BACKGROUND BRUSH
    // ========================================================================

    LoadingBackgroundBrush = MakeShared<FSlateBrush>();

    if (LoadingBackgroundTexture)
    {
        LoadingBackgroundBrush->SetResourceObject(LoadingBackgroundTexture);
    }

    LoadingBackgroundBrush->ImageSize = FVector2D(1920.0f, 1080.0f);


    // ========================================================================
    // 06 - CENTER SLASH BRUSH
    // ========================================================================

    CenterSlashBrush.Reset();

    if (CenterSlashTexture)
    {
        CenterSlashBrush = MakeShared<FSlateBrush>();
        CenterSlashBrush->SetResourceObject(CenterSlashTexture);
        CenterSlashBrush->ImageSize = FVector2D(500.0f, 800.0f);
    }


    // ========================================================================
    // 07 - PLAYER DATA
    // ========================================================================

    const int32 PlayerCount = FMath::Clamp(
        LoadingPlayers.Num(),
        0,
        4
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("LOADING PLAYERS COUNT = %d"),
        LoadingPlayers.Num()
    );


    // ========================================================================
    // 08 - PLAYER PORTRAIT BRUSHES
    // ========================================================================

    PlayerPortraitBrushes.Empty();
    PlayerPortraitBrushes.SetNum(PlayerCount);

    for (int32 Index = 0; Index < PlayerCount; ++Index)
    {
        if (LoadingPlayers[Index].Portrait)
        {
            PlayerPortraitBrushes[Index] = MakeShared<FSlateBrush>();
            PlayerPortraitBrushes[Index]->SetResourceObject(
                LoadingPlayers[Index].Portrait
            );
            PlayerPortraitBrushes[Index]->ImageSize = FVector2D(900.0f, 900.0f);
        }
    }


    // ========================================================================
    // 09 - PLAYER COLORS
    //
    // La couleur dépend du VRAI PlayerIndex stocké dans LoadingPlayerData.
    // Donc même si P2 est absent, P3 garde sa couleur de P3.
    // ========================================================================

    const FLinearColor PlayerColors[4] =
    {
        FLinearColor(1.00f, 0.12f, 0.04f, 1.00f), // P1
        FLinearColor(0.10f, 0.55f, 1.00f, 1.00f), // P2
        FLinearColor(0.20f, 1.00f, 0.48f, 1.00f), // P3
        FLinearColor(0.95f, 0.30f, 1.00f, 1.00f)  // P4
    };


    // ========================================================================
    // 10 - HELPER : CREATE PLAYER CARD
    // ========================================================================

    auto CreatePlayerCard =
        [this, &PlayerColors](
            int32 DataIndex,
            float CardWidth,
            float CardHeight
            ) -> TSharedRef<SWidget>
        {
            // ----------------------------------------------------------------
            // SAFETY
            // ----------------------------------------------------------------

            if (!LoadingPlayers.IsValidIndex(DataIndex))
            {
                return SNew(SBox);
            }


            // ----------------------------------------------------------------
            // PLAYER DATA
            // ----------------------------------------------------------------

            const int32 LogicalPlayerIndex = FMath::Clamp(
                LoadingPlayers[DataIndex].PlayerIndex,
                0,
                3
            );

            const FString PlayerLabel = FString::Printf(
                TEXT("PLAYER %d"),
                LogicalPlayerIndex + 1
            );

            FString CharacterName = LoadingPlayers[DataIndex].CharacterName;

            if (CharacterName.IsEmpty())
            {
                CharacterName = TEXT("FIGHTER");
            }

            CharacterName = CharacterName.ToUpper();

            const FLinearColor AccentColor = PlayerColors[LogicalPlayerIndex];


            // ----------------------------------------------------------------
            // PORTRAIT BRUSH
            // ----------------------------------------------------------------

            const FSlateBrush* PortraitBrush = nullptr;

            if (
                PlayerPortraitBrushes.IsValidIndex(DataIndex)
                && PlayerPortraitBrushes[DataIndex].IsValid()
                )
            {
                PortraitBrush = PlayerPortraitBrushes[DataIndex].Get();
            }


            // ----------------------------------------------------------------
            // RESPONSIVE TEXT SIZES
            // ----------------------------------------------------------------

            const int32 PlayerFontSize = CardWidth <= 420.0f ? 11 : 13;
            const int32 NameFontSize = CardWidth <= 420.0f ? 25 : 32;
            const float BottomReservedSpace = CardWidth <= 420.0f ? 92.0f : 108.0f;


            // ----------------------------------------------------------------
            // CARD
            // ----------------------------------------------------------------

            return
                SNew(SBox)
                .WidthOverride(CardWidth)
                .HeightOverride(CardHeight)
                [
                    SNew(SOverlay)


                        // =========================================================
                        // CARD BACKGROUND
                        // =========================================================

                        +SOverlay::Slot()
                        [
                            SNew(SBorder)
                                .Padding(0.0f)
                                .BorderBackgroundColor(
                                    FLinearColor(
                                        0.005f,
                                        0.005f,
                                        0.008f,
                                        0.88f
                                    )
                                )
                        ]


                    // =========================================================
                    // SOFT PLAYER COLOR GLOW
                    // =========================================================

                    +SOverlay::Slot()
                        [
                            SNew(SBorder)
                                .Padding(0.0f)
                                .BorderBackgroundColor(
                                    FLinearColor(
                                        AccentColor.R,
                                        AccentColor.G,
                                        AccentColor.B,
                                        0.09f
                                    )
                                )
                        ]


                    // =========================================================
                    // PORTRAIT
                    // =========================================================

                    +SOverlay::Slot()
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Fill)
                        .Padding(
                            3.0f,
                            8.0f,
                            3.0f,
                            BottomReservedSpace
                        )
                        [
                            SNew(SBox)
                                .Clipping(EWidgetClipping::ClipToBounds)
                                [
                                    SNew(SScaleBox)
                                        .Stretch(EStretch::ScaleToFill)
                                        .StretchDirection(EStretchDirection::Both)
                                        [
                                            SNew(SImage)
                                                .Image(PortraitBrush)
                                        ]
                                ]
                        ]


                    // =========================================================
                    // TOP PLAYER COLOR STRIP
                    // =========================================================

                    +SOverlay::Slot()
                        .VAlign(VAlign_Top)
                        [
                            SNew(SBox)
                                .HeightOverride(6.0f)
                                [
                                    SNew(SColorBlock)
                                        .Color(AccentColor)
                                ]
                        ]


                    // =========================================================
                    // BOTTOM NAME PLATE
                    // =========================================================

                    +SOverlay::Slot()
                        .VAlign(VAlign_Bottom)
                        [
                            SNew(SBorder)
                                .Padding(
                                    FMargin(
                                        14.0f,
                                        9.0f
                                    )
                                )
                                .BorderBackgroundColor(
                                    FLinearColor(
                                        0.0f,
                                        0.0f,
                                        0.0f,
                                        0.92f
                                    )
                                )
                                [
                                    SNew(SVerticalBox)


                                        // -------------------------------------------------
                                        // ACCENT LINE
                                        // -------------------------------------------------

                                        +SVerticalBox::Slot()
                                        .AutoHeight()
                                        [
                                            SNew(SBox)
                                                .HeightOverride(3.0f)
                                                [
                                                    SNew(SColorBlock)
                                                        .Color(AccentColor)
                                                ]
                                        ]


                                    // -------------------------------------------------
                                    // PLAYER NUMBER
                                    // -------------------------------------------------

                                    +SVerticalBox::Slot()
                                        .AutoHeight()
                                        .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(PlayerLabel))
                                                .ColorAndOpacity(AccentColor)
                                                .Font(
                                                    FCoreStyle::GetDefaultFontStyle(
                                                        "Bold",
                                                        PlayerFontSize
                                                    )
                                                )
                                        ]


                                    // -------------------------------------------------
                                    // CHARACTER NAME
                                    // -------------------------------------------------

                                    +SVerticalBox::Slot()
                                        .AutoHeight()
                                        .Padding(0.0f, 1.0f, 0.0f, 0.0f)
                                        [
                                            SNew(SBox)
                                                .HeightOverride(CardWidth <= 420.0f ? 34.0f : 42.0f)
                                                [
                                                    SNew(SScaleBox)
                                                        .Stretch(EStretch::ScaleToFill)
                                                        .StretchDirection(EStretchDirection::DownOnly)
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(FText::FromString(CharacterName))
                                                                .ColorAndOpacity(FLinearColor::White)
                                                                .ShadowOffset(FVector2D(2.0f, 2.0f))
                                                                .ShadowColorAndOpacity(
                                                                    FLinearColor(
                                                                        0.0f,
                                                                        0.0f,
                                                                        0.0f,
                                                                        0.95f
                                                                    )
                                                                )
                                                                .Font(
                                                                    FCoreStyle::GetDefaultFontStyle(
                                                                        "Bold",
                                                                        NameFontSize
                                                                    )
                                                                )
                                                        ]
                                                ]
                                        ]
                                ]
                        ]
                ];
        };


    // ========================================================================
    // 11 - HELPER : CREATE VS PANEL
    // ========================================================================

    auto CreateVSPanel =
        [this](float PanelWidth, float PanelHeight) -> TSharedRef<SWidget>
        {
            return
                SNew(SBox)
                .WidthOverride(PanelWidth)
                .HeightOverride(PanelHeight)
                [
                    SNew(SOverlay)


                        // ---------------------------------------------------------
                        // DARK CENTER COLUMN
                        // ---------------------------------------------------------

                        +SOverlay::Slot()
                        [
                            SNew(SColorBlock)
                                .Color(
                                    FLinearColor(
                                        0.0f,
                                        0.0f,
                                        0.0f,
                                        0.18f
                                    )
                                )
                        ]


                    // ---------------------------------------------------------
                    // ENERGY SLASH
                    // ---------------------------------------------------------

                    +SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SBox)
                                .WidthOverride(PanelWidth * 0.90f)
                                .HeightOverride(PanelHeight * 0.94f)
                                [
                                    SNew(SImage)
                                        .Image(
                                            CenterSlashBrush.IsValid()
                                            ? CenterSlashBrush.Get()
                                            : nullptr
                                        )
                                        .ColorAndOpacity(
                                            FLinearColor(
                                                1.0f,
                                                1.0f,
                                                1.0f,
                                                0.92f
                                            )
                                        )
                                ]
                        ]


                    // ---------------------------------------------------------
                    // LARGE GHOST VS
                    // ---------------------------------------------------------

                    +SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(TEXT("VS")))
                                .ColorAndOpacity(
                                    FLinearColor(
                                        1.0f,
                                        1.0f,
                                        1.0f,
                                        0.11f
                                    )
                                )
                                .Font(
                                    FCoreStyle::GetDefaultFontStyle(
                                        "Bold",
                                        120
                                    )
                                )
                        ]


                    // ---------------------------------------------------------
                    // MAIN VS
                    // ---------------------------------------------------------

                    +SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SBorder)
                                .Padding(FMargin(20.0f, 10.0f))
                                .BorderBackgroundColor(
                                    FLinearColor(
                                        0.0f,
                                        0.0f,
                                        0.0f,
                                        0.84f
                                    )
                                )
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(TEXT("VS")))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .ShadowOffset(FVector2D(4.0f, 4.0f))
                                        .ShadowColorAndOpacity(
                                            FLinearColor(
                                                0.0f,
                                                0.0f,
                                                0.0f,
                                                0.95f
                                            )
                                        )
                                        .Font(
                                            FCoreStyle::GetDefaultFontStyle(
                                                "Bold",
                                                80
                                            )
                                        )
                                ]
                        ]
                ];
        };


    // ========================================================================
    // 12 - PLAYER LAYOUT
    // ========================================================================

    TSharedRef<SOverlay> PlayersOverlay = SNew(SOverlay);


    // ========================================================================
    // 12A - ONE PLAYER
    // ========================================================================

    if (PlayerCount == 1)
    {
        PlayersOverlay->AddSlot()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(0.0f, 112.0f, 0.0f, 115.0f)
            [
                CreatePlayerCard(
                    0,
                    600.0f,
                    690.0f
                )
            ];
    }


    // ========================================================================
    // 12B - TWO PLAYERS
    // ========================================================================

    else if (PlayerCount == 2)
    {
        TSharedRef<SHorizontalBox> TwoPlayerBox = SNew(SHorizontalBox);

        TwoPlayerBox->AddSlot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(22.0f)
            [
                CreatePlayerCard(0, 590.0f, 680.0f)
            ];

        TwoPlayerBox->AddSlot()
            .AutoWidth()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(10.0f, 0.0f)
            [
                CreateVSPanel(190.0f, 680.0f)
            ];

        TwoPlayerBox->AddSlot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(22.0f)
            [
                CreatePlayerCard(1, 590.0f, 680.0f)
            ];

        PlayersOverlay->AddSlot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Center)
            .Padding(45.0f, 110.0f, 45.0f, 115.0f)
            [
                TwoPlayerBox
            ];
    }


    // ========================================================================
    // 12C - THREE PLAYERS
    // ========================================================================

    else if (PlayerCount == 3)
    {
        TSharedRef<SHorizontalBox> ThreePlayerBox = SNew(SHorizontalBox);

        for (int32 Index = 0; Index < 3; ++Index)
        {
            ThreePlayerBox->AddSlot()
                .FillWidth(1.0f)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Padding(13.0f)
                [
                    CreatePlayerCard(
                        Index,
                        500.0f,
                        660.0f
                    )
                ];
        }

        PlayersOverlay->AddSlot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Center)
            .Padding(40.0f, 112.0f, 40.0f, 115.0f)
            [
                ThreePlayerBox
            ];
    }


    // ========================================================================
    // 12D - FOUR PLAYERS
    //
    // P1 | P2 | VS | P3 | P4
    // ========================================================================

    else if (PlayerCount >= 4)
    {
        TSharedRef<SHorizontalBox> FourPlayerBox = SNew(SHorizontalBox);

        FourPlayerBox->AddSlot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(7.0f)
            [
                CreatePlayerCard(0, 380.0f, 640.0f)
            ];

        FourPlayerBox->AddSlot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(7.0f)
            [
                CreatePlayerCard(1, 380.0f, 640.0f)
            ];

        FourPlayerBox->AddSlot()
            .AutoWidth()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f)
            [
                CreateVSPanel(155.0f, 640.0f)
            ];

        FourPlayerBox->AddSlot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(7.0f)
            [
                CreatePlayerCard(2, 380.0f, 640.0f)
            ];

        FourPlayerBox->AddSlot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(7.0f)
            [
                CreatePlayerCard(3, 380.0f, 640.0f)
            ];

        PlayersOverlay->AddSlot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Center)
            .Padding(18.0f, 112.0f, 18.0f, 115.0f)
            [
                FourPlayerBox
            ];
    }


    // ========================================================================
    // 13 - FULL SCREEN
    // ========================================================================

    LoadingScreen.WidgetLoadingScreen =
        SNew(SOverlay)


        // ====================================================================
        // BACKGROUND IMAGE
        // ====================================================================

        +SOverlay::Slot()
        [
            SNew(SImage)
                .Image(LoadingBackgroundBrush.Get())
        ]


        // ====================================================================
        // CINEMATIC DARK FILTER
        // ====================================================================

        +SOverlay::Slot()
        [
            SNew(SColorBlock)
                .Color(
                    FLinearColor(
                        0.0f,
                        0.0f,
                        0.0f,
                        0.44f
                    )
                )
        ]


    // ====================================================================
    // TOP CINEMATIC BAR
    // ====================================================================

    +SOverlay::Slot()
        .VAlign(VAlign_Top)
        [
            SNew(SBox)
                .HeightOverride(92.0f)
                [
                    SNew(SColorBlock)
                        .Color(
                            FLinearColor(
                                0.004f,
                                0.004f,
                                0.008f,
                                0.96f
                            )
                        )
                ]
        ]


    // ====================================================================
    // BOTTOM CINEMATIC BAR
    // ====================================================================

    +SOverlay::Slot()
        .VAlign(VAlign_Bottom)
        [
            SNew(SBox)
                .HeightOverride(92.0f)
                [
                    SNew(SColorBlock)
                        .Color(
                            FLinearColor(
                                0.004f,
                                0.004f,
                                0.008f,
                                0.97f
                            )
                        )
                ]
        ]


    // ====================================================================
    // TOP STAGE INFORMATION
    // ====================================================================

    +SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Top)
        .Padding(0.0f, 14.0f, 0.0f, 0.0f)
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(TEXT("NEXT BATTLE")))
                        .ColorAndOpacity(
                            FLinearColor(
                                0.62f,
                                0.62f,
                                0.68f,
                                1.0f
                            )
                        )
                        .Font(
                            FCoreStyle::GetDefaultFontStyle(
                                "Bold",
                                11
                            )
                        )
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(0.0f, 1.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(StageName))
                        .ColorAndOpacity(FLinearColor::White)
                        .ShadowOffset(FVector2D(2.0f, 2.0f))
                        .ShadowColorAndOpacity(
                            FLinearColor(
                                0.0f,
                                0.0f,
                                0.0f,
                                0.90f
                            )
                        )
                        .Font(
                            FCoreStyle::GetDefaultFontStyle(
                                "Bold",
                                36
                            )
                        )
                ]
        ]


    // ====================================================================
    // PLAYER AREA
    // ====================================================================

    +SOverlay::Slot()
        [
            PlayersOverlay
        ]


        // ====================================================================
        // BOTTOM LOADING INFORMATION
        // ====================================================================

        +SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Bottom)
        .Padding(0.0f, 0.0f, 0.0f, 18.0f)
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(TEXT("PREPARING BATTLE")))
                        .ColorAndOpacity(
                            FLinearColor(
                                0.74f,
                                0.74f,
                                0.78f,
                                1.0f
                            )
                        )
                        .Font(
                            FCoreStyle::GetDefaultFontStyle(
                                "Bold",
                                11
                            )
                        )
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(0.0f, 7.0f, 0.0f, 0.0f)
                [
                    SNew(SBox)
                        .WidthOverride(500.0f)
                        .HeightOverride(5.0f)
                        [
                            SNew(SProgressBar)
                                .Percent(TOptional<float>())
                        ]
                ]
        ];


    // ========================================================================
    // 14 - CONFIGURE MOVIE PLAYER
    // ========================================================================

    MoviePlayer->SetupLoadingScreen(LoadingScreen);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Loading Screen configured | Map=%s | Players=%d | Minimum=5.0s"),
        *StageName,
        PlayerCount
    );
}


// ============================================================================
// 15 - END LOADING SCREEN
// ============================================================================

void USmashGameInstance::EndLoadingScreen(UWorld* LoadedWorld)
{
    if (!LoadedWorld)
    {
        return;
    }

    const FString LoadedMapName = LoadedWorld->GetName();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Map finished loading: %s"),
        *LoadedMapName
    );

    // Si on retourne vers une map qui n'est pas la map de combat,
    // on libère tout MoviePlayer restant.
    if (!LoadedMapName.Contains(TEXT("Final_Arena")))
    {
        if (GetMoviePlayer() && GetMoviePlayer()->IsMovieCurrentlyPlaying())
        {
            GetMoviePlayer()->StopMovie();
        }
    }
}











// manette est connectée ou déconnectée.
//
// Pour l'instant, on affiche seulement les IDs pour vérifier
// exactement ce qu'Unreal détecte.
// ============================================================
// ============================================================================
// 16 - DECONNEXION / RECONNEXION : NE PAS CREER UN NOUVEAU JOUEUR
// ============================================================================
void USmashGameInstance::HandleInputDeviceConnectionChange(
    EInputDeviceConnectionState NewConnectionState,
    FPlatformUserId PlatformUserId,
    FInputDeviceId InputDeviceId)
{
    for (FSmashLocalPlayerData& PlayerData : ConnectedPlayers)
    {
        if (PlayerData.InputDeviceId != InputDeviceId)
        {
            continue;
        }

        if (NewConnectionState == EInputDeviceConnectionState::Disconnected)
        {
            if (PlayerData.bIsConnected)
            {
                PlayerData.bIsConnected = false;
                OnSmashLocalPlayerConnectionChanged(PlayerData.PlayerIndex, false);
            }
        }
        else if (NewConnectionState == EInputDeviceConnectionState::Connected)
        {
            if (!PlayerData.bIsConnected)
            {
                // On restaure l'ancien emplacement, sans creer de LocalPlayer.
                RouteInputDeviceToLocalPlayer(PlayerData.PlayerIndex, InputDeviceId);
                PlayerData.bIsConnected = true;
                OnSmashLocalPlayerConnectionChanged(PlayerData.PlayerIndex, true);
            }
        }
        return;
    }

    // Manette inconnue : on attend A/Croix, meme si deja branchee.
    UE_LOG(LogTemp, Log, TEXT("DEVICE SANS JOUEUR | Device=%d | State=%d"),
        InputDeviceId.GetId(), static_cast<int32>(NewConnectionState));
}

// ============================================================================
// 17 - APPUYER SUR A / CROIX POUR REJOINDRE
// ============================================================================
void USmashGameInstance::HandleJoinButtonPressed(FInputDeviceId InputDeviceId)
{
    if (!InputDeviceId.IsValid())
    {
        return;
    }

    // Une manette connue ne doit jamais creer un second joueur.
    for (const FSmashLocalPlayerData& PlayerData : ConnectedPlayers)
    {
        if (PlayerData.InputDeviceId == InputDeviceId)
        {
            return;
        }
    }

    // P1-P4 utilisent tous une manette. P1 est le LocalPlayer deja cree par UE.
    const int32 NewPlayerIndex = ConnectedPlayers.Num();
    if (NewPlayerIndex > 3)
    {
        return;
    }

    // Enregistrer AVANT la creation : les events Blueprint voient le joueur.
    FSmashLocalPlayerData NewPlayer;
    NewPlayer.PlayerIndex = NewPlayerIndex;
    NewPlayer.InputDeviceId = InputDeviceId;
    NewPlayer.bIsConnected = true;
    ConnectedPlayers.Add(NewPlayer);

    // P1 : reutiliser le LocalPlayer 0. P2-P4 : creer leur LocalPlayer.
    // IMPORTANT : le chemin ExistingLocalPlayer retourne sans declencher
    // OnSmashLocalPlayerCreated, donc on le declenche explicitement pour P1.
    if (NewPlayerIndex == 0)
    {
        if (!GetLocalPlayerByIndex(0))
        {
            ConnectedPlayers.RemoveAt(ConnectedPlayers.Num() - 1);
            UE_LOG(LogTemp, Error, TEXT("JOIN P1 ECHEC | LocalPlayer 0 absent"));
            return;
        }
        RouteInputDeviceToLocalPlayer(0, InputDeviceId);
        OnSmashLocalPlayerCreated(0);
    }
    else
    {
        CreateSmashLocalPlayer(NewPlayerIndex, InputDeviceId);
    }

    // Annuler l'inscription si Unreal n'a pas cree le joueur.
    if (!GetLocalPlayerByIndex(NewPlayerIndex))
    {
        ConnectedPlayers.RemoveAt(ConnectedPlayers.Num() - 1);
        UE_LOG(LogTemp, Error, TEXT("JOIN ECHEC | Device=%d"), InputDeviceId.GetId());
        return;
    }

    OnSmashLocalPlayerConnectionChanged(NewPlayerIndex, true);
    UE_LOG(LogTemp, Warning, TEXT("JOIN OK | Device=%d -> P%d"),
        InputDeviceId.GetId(), NewPlayerIndex + 1);
}

// ============================================================================
// 18 - NETTOYAGE DU PREPROCESSOR
// ============================================================================
void USmashGameInstance::Shutdown()
{
    if (JoinInputProcessor.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(JoinInputProcessor);
        JoinInputProcessor.Reset();
    }
    Super::Shutdown();
}

// ============================================================
// LOCAL MULTIPLAYER - CREATE LOCAL PLAYER
//
// Crée P2/P3/P4 avec leur ControllerId normal, puis associe
// la manette PHYSIQUE au PlatformUser du LocalPlayer créé.
//
// Contrairement à SetPlatformUserId(), on ne modifie pas le
// LocalPlayer. On déplace uniquement la manette vers le bon
// PlatformUser.
// ============================================================

void USmashGameInstance::CreateSmashLocalPlayer(
    int32 PlayerIndex,
    FInputDeviceId InputDeviceId
)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("CREATE LOCAL PLAYER REQUEST | PlayerIndex=%d | Device=%d"),
        PlayerIndex,
        InputDeviceId.GetId()
    );

    // ============================================================
    // SÉCURITÉ - SI LE LOCAL PLAYER EXISTE DÉJÀ
    // ============================================================

    if (ULocalPlayer* ExistingLocalPlayer = GetLocalPlayerByIndex(PlayerIndex))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("LOCAL PLAYER DEJA EXISTANT | PlayerIndex=%d | ControllerId=%d | PlatformUser=%d"),
            PlayerIndex,
            ExistingLocalPlayer->GetControllerId(),
            ExistingLocalPlayer->GetPlatformUserId().GetInternalId()
        );

        RouteInputDeviceToLocalPlayer(
            PlayerIndex,
            InputDeviceId
        );

        return;
    }

    // ============================================================
    // DEBUG - P1
    // ============================================================

    if (ULocalPlayer* Player1 = GetLocalPlayerByIndex(0))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("P1 INFO | ControllerId=%d | PlatformUser=%d"),
            Player1->GetControllerId(),
            Player1->GetPlatformUserId().GetInternalId()
        );
    }

    // ============================================================
    // DEBUG - PROPRIÉTAIRE ACTUEL DE LA MANETTE
    // ============================================================

    IPlatformInputDeviceMapper& DeviceMapper =
        IPlatformInputDeviceMapper::Get();

    const FPlatformUserId DevicePlatformUser =
        DeviceMapper.GetUserForInputDevice(InputDeviceId);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("DEVICE BEFORE CREATE | Device=%d -> PlatformUser=%d"),
        InputDeviceId.GetId(),
        DevicePlatformUser.GetInternalId()
    );

    // ============================================================
    // CRÉER LE LOCAL PLAYER PAR INDEX
    //
    // On conserve la version stable qui fonctionnait déjà :
    // P2 = ControllerId 1
    // P3 = ControllerId 2
    // P4 = ControllerId 3
    // ============================================================

    FString Error;

    ULocalPlayer* NewLocalPlayer = CreateLocalPlayer(
        PlayerIndex,
        Error,
        true
    );

    if (!NewLocalPlayer)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("ECHEC LOCAL PLAYER | PlayerIndex=%d | Error=%s"),
            PlayerIndex,
            *Error
        );

        return;
    }

    // ============================================================
    // IMPORTANT - NE PAS FAIRE SetPlatformUserId()
    //
    // Le LocalPlayer garde le PlatformUser créé normalement par UE.
    // On va plutôt associer la vraie manette à ce PlatformUser.
    // ============================================================

    RouteInputDeviceToLocalPlayer(
        PlayerIndex,
        InputDeviceId
    );

    // ============================================================
    // DEBUG - LOCAL PLAYER FINAL
    // ============================================================

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("LOCAL PLAYER CREE | PlayerIndex=%d | Device=%d | ControllerId=%d | PlatformUser=%d"),
        PlayerIndex,
        InputDeviceId.GetId(),
        NewLocalPlayer->GetControllerId(),
        NewLocalPlayer->GetPlatformUserId().GetInternalId()
    );

    // ============================================================
    // DEBUG - TOUS LES LOCAL PLAYERS
    // ============================================================

    for (int32 Index = 0; Index < GetNumLocalPlayers(); ++Index)
    {
        if (ULocalPlayer* LocalPlayer = GetLocalPlayerByIndex(Index))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("PLAYER STATE | Index=%d | ControllerId=%d | PlatformUser=%d"),
                Index,
                LocalPlayer->GetControllerId(),
                LocalPlayer->GetPlatformUserId().GetInternalId()
            );
        }
    }

    // ============================================================
    // PRÉVENIR LE BLUEPRINT
    // ============================================================

    OnSmashLocalPlayerCreated(PlayerIndex);
}


// ============================================================
// LOCAL MULTIPLAYER - ROUTE PHYSICAL INPUT DEVICE
//
// C'est la correction importante pour :
//
// P1 clavier
// P2 première manette
// P3 deuxième manette
// P4 troisième manette
//
// La première manette arrive normalement sur PlatformUser 0,
// comme le clavier. On la déplace vers le PlatformUser de P2.
// P3/P4 sont également vérifiés et corrigés si nécessaire.
//
// IMPORTANT :
// Dans Project Settings, "Skip Assigning Gamepad to Player 1"
// doit être DÉCOCHÉ avec cette architecture. Sinon le
// GameViewportClient applique encore son propre +1/remapping.
// ============================================================

void USmashGameInstance::RouteInputDeviceToLocalPlayer(
    int32 PlayerIndex,
    FInputDeviceId InputDeviceId
)
{
    ULocalPlayer* TargetLocalPlayer =
        GetLocalPlayerByIndex(PlayerIndex);

    if (!TargetLocalPlayer)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("ROUTING FAILED | LocalPlayer invalide | PlayerIndex=%d | Device=%d"),
            PlayerIndex,
            InputDeviceId.GetId()
        );

        return;
    }

    IPlatformInputDeviceMapper& DeviceMapper =
        IPlatformInputDeviceMapper::Get();

    const FPlatformUserId CurrentUser =
        DeviceMapper.GetUserForInputDevice(InputDeviceId);

    const FPlatformUserId TargetUser =
        TargetLocalPlayer->GetPlatformUserId();

    // Déjà parfaitement routé.
    if (CurrentUser == TargetUser)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("ROUTING OK | Device=%d -> PlayerIndex=%d | PlatformUser=%d"),
            InputDeviceId.GetId(),
            PlayerIndex,
            TargetUser.GetInternalId()
        );

        return;
    }

    // ============================================================
    // CHANGER LE PROPRIÉTAIRE DE LA MANETTE
    //
    // Cette fonction déclenche un PairingChange et non un nouvel
    // événement de connexion, donc elle ne doit pas créer P3/P4
    // accidentellement dans HandleInputDeviceConnectionChange().
    // ============================================================

    const bool bChanged =
        DeviceMapper.Internal_ChangeInputDeviceUserMapping(
            InputDeviceId,
            TargetUser,
            CurrentUser
        );

    if (bChanged)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("ROUTING CHANGED | Device=%d | User %d -> %d | PlayerIndex=%d"),
            InputDeviceId.GetId(),
            CurrentUser.GetInternalId(),
            TargetUser.GetInternalId(),
            PlayerIndex
        );
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("ROUTING CHANGE FAILED | Device=%d | User %d -> %d | PlayerIndex=%d"),
            InputDeviceId.GetId(),
            CurrentUser.GetInternalId(),
            TargetUser.GetInternalId(),
            PlayerIndex
        );
    }
}


// ========================================================
// SELECTION - RECUPERER LES JOUEURS DEJA CONNECTES
// ========================================================

TArray<int32> USmashGameInstance::GetConnectedPlayerIndices() const
{
    TArray<int32> Result;

    for (const FSmashLocalPlayerData& Player : ConnectedPlayers)
    {
        if (Player.bIsConnected)
        {
            Result.Add(Player.PlayerIndex);
        }
    }

    return Result;
}















