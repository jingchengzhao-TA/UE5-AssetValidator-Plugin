// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetCheckerCore.h"
#include "AssetChecker.h"
#include "AssetCheckerStyle.h"
#include "AssetCheckerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "UObject/UObjectGlobals.h"

static const FName AssetCheckerTabName("AssetChecker");

#define LOCTEXT_NAMESPACE "FAssetCheckerModule"

void FAssetCheckerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FAssetCheckerStyle::Initialize();
	FAssetCheckerStyle::ReloadTextures();

	FAssetCheckerCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAssetCheckerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FAssetCheckerModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetCheckerModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AssetCheckerTabName, FOnSpawnTab::CreateRaw(this, &FAssetCheckerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FAssetCheckerTabTitle", "AssetChecker"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// 注册全局保存事件，当有资产被保存时，自动调用 OnObjectSaved 函数
	FCoreUObjectDelegates::OnObjectSaved.AddStatic(&UAssetCheckerCore::OnObjectSaved);
}

void FAssetCheckerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FAssetCheckerStyle::Shutdown();

	FAssetCheckerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AssetCheckerTabName);
}

TSharedRef<SDockTab> FAssetCheckerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FAssetCheckerModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("AssetChecker.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FAssetCheckerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(AssetCheckerTabName);
}

void FAssetCheckerModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FAssetCheckerCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FAssetCheckerCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}

	// ========== 向内容浏览器的右键菜单添加工具 ==========
	{
		// 获取内容浏览器的右键菜单
		UToolMenu* AssetMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
		// 添加一个我自己的 Section (板块)
		FToolMenuSection& Section = AssetMenu->AddSection("AssetCheckerSection", FText::FromString(TEXT("TA 工具箱")));

		// 菜单项 1：仅检查
		Section.AddMenuEntry(
			"RunAssetCheck",
			FText::FromString(TEXT("执行规范检查 (不修复)")),
			FText::FromString(TEXT("检查选中的资产并生成TXT报告")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { UAssetCheckerCore::RunGlobalAssetCheck(false); }))
		);

		// 菜单项 2：检查并修复
		Section.AddMenuEntry(
			"RunAssetCheckFix",
			FText::FromString(TEXT("执行规范检查 (并自动修复)")),
			FText::FromString(TEXT("检查选中资产并尝试自动修正贴图")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { UAssetCheckerCore::RunGlobalAssetCheck(true); }))
		);

		// 菜单项 3：专属场景检测
		Section.AddMenuEntry(
			"RunSceneCheck",
			FText::FromString(TEXT("执行规范检查 (仅场景 Actor)")),
			FText::FromString(TEXT("仅检查当前场景大纲中的 Actor 对象。")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { UAssetCheckerCore::RunSceneActorCheckOnly(); }))
		);
	}
	// =======================================================
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetCheckerModule, AssetChecker)