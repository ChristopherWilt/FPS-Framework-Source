// WeaponBuyMenuWidget.cpp

#include "SnD_FPS/UI/BuySystem/WeaponBuyMenuWidget.h"
#include "SnD_FPS/UI/BuySystem/WeaponBuyEntryWidget.h"
#include "Components/UniformGridPanel.h"
#include "SnD_FPS/Weapon/Weapon.h" // Include the full definition of AWeapon
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "SnD_FPS/Player/PlayerCharacter.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"


void UWeaponBuyMenuWidget::NativeConstruct()
{
	// --- Populate Weapon Columns ---
    Super::NativeConstruct();

    if (!WeaponEntryWidgetClass) return;

    APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
    if (!PlayerCharacter) return;

    const int32 PlayerMoney = PlayerCharacter->GetCurrentMoney();


    // Clear all columns to be safe
    Pistols_Column->ClearChildren();
    SMGs_Column->ClearChildren();
    Shotguns_Column->ClearChildren();
    ARs_Column->ClearChildren();
    Snipers_Column->ClearChildren();

    for (TSubclassOf<AWeapon> WeaponClass : WeaponsForSale)
    {
        if (WeaponClass)
        {
            UWeaponBuyEntryWidget* EntryWidget = CreateWidget<UWeaponBuyEntryWidget>(this, WeaponEntryWidgetClass);
            if (EntryWidget)
            {
                // Bind our local function (OnWeaponHovered) to the event from the entry widget
                EntryWidget->OnWeaponEntryHovered.AddDynamic(this, &UWeaponBuyMenuWidget::OnWeaponHovered);

                AWeapon* DefaultWeapon = WeaponClass->GetDefaultObject<AWeapon>();
                if (!DefaultWeapon) continue;

                // Pass data to the entry widget like before
                // EntryWidget->SetWeaponData(...);
                EntryWidget->SetWeaponData(WeaponClass, PlayerMoney);


                // Sort the widget into the correct column
                switch (DefaultWeapon->WeaponType)
                {
                case EWeaponType::Sidearm:
                    Pistols_Column->AddChildToVerticalBox(EntryWidget);
                    break;
                case EWeaponType::SMG:
                    SMGs_Column->AddChildToVerticalBox(EntryWidget);
                    break;
                case EWeaponType::Shotgun:
                    Shotguns_Column->AddChildToVerticalBox(EntryWidget);
                    break;
                case EWeaponType::Rifle:
                    ARs_Column->AddChildToVerticalBox(EntryWidget);
                    break;
                case EWeaponType::Sniper:
                    Snipers_Column->AddChildToVerticalBox(EntryWidget);
                    break;
                }
            }
        }
    }

    // --- Populate Player Info Panel ---

    if (PlayerInfo_Panel && PlayerInfoEntryWidgetClass)
    {
        PlayerInfo_Panel->ClearChildren(); // Clear any old entries

        AGameStateBase* GameState = GetWorld()->GetGameState();
        if (GameState)
        {
            // Loop through all players connected to the game
            for (APlayerState* PlayerState : GameState->PlayerArray)
            {
                if (PlayerState)
                {
                    // Create an instance of our WBP_PlayerInfoEntry widget
                    UUserWidget* EntryWidget = CreateWidget(this, PlayerInfoEntryWidgetClass);
                    if (EntryWidget)
                    {
                        // Find the text blocks within the created widget
                        UTextBlock* PlayerNameText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("PlayerName_Text")));
                        UTextBlock* PlayerMoneyText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("PlayerMoney_Text")));

                        // Update the widget with this player's data
                        if (PlayerNameText)
                        {
                            PlayerNameText->SetText(FText::FromString(PlayerState->GetPlayerName()));
                        }
                        if (PlayerMoneyText)
                        {
                            // NOTE: Money is usually stored on the PlayerState.
                            // We're using Score as a placeholder here.
                            FString MoneyString = FString::Printf(TEXT("$%d"), (int32)PlayerState->GetScore());
                            PlayerMoneyText->SetText(FText::FromString(MoneyString));
                        }

                        // Add the newly created and populated card to our list
                        PlayerInfo_Panel->AddChildToVerticalBox(EntryWidget);
                    }
                }
            }
        }
    }
}

void UWeaponBuyMenuWidget::OnWeaponHovered(TSubclassOf<AWeapon> WeaponClass)
{
    if (!WeaponClass || !WeaponSpecs_Panel) return;

    AWeapon* DefaultWeapon = WeaponClass->GetDefaultObject<AWeapon>();
    if (!DefaultWeapon) return;

    // Get all the UTextBlock pointers for the specs panel (make sure they are BindWidget properties)
    // UTextBlock* SpecName = ...;
    // UTextBlock* SpecDamage = ...;

    // Update the text with the hovered weapon's data
    // SpecName->SetText(DefaultWeapon->WeaponName);
    // SpecDamage->SetText(FText::AsNumber(DefaultWeapon->WeaponDamage));
    // ... and so on for all your stats ...

    // Finally, make the panel visible
    WeaponSpecs_Panel->SetVisibility(ESlateVisibility::Visible);
}
