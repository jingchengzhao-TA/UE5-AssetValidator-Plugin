// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetCheckerTypes.h" // 引入刚才写的数据结构
#include "AssetCheckerCore.generated.h"

/**
 * 
 */
UCLASS()
class ASSETCHECKER_API UAssetCheckerCore : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// 执行贴图的规范性检查
	static TArray<FString> CheckTextures(const TArray<UObject*>& SelectedAssets, UDataTable* RuleTable, bool bAutoFix);

	// 执行静态模型（Static Mesh）的规范性检查
	static TArray<FString> CheckStaticMeshes(const TArray<UObject*>& SelectedAssets, UDataTable* RuleTable);

	// 执行场景树对象（Actor）的规范性检查
	static TArray<FString> CheckActorsInWorld(UWorld* World, UDataTable* RuleTable);

	// 一键执行所有的检查流程，并输出TXT与弹窗
	UFUNCTION(BlueprintCallable, Category = "AssetChecker")
	static void RunGlobalAssetCheck(bool bAutoFix);

	// 在 Message Log 输出带有超链接的高级日志
	static void PrintClickableLog(UObject* Asset, const FString& ErrorMessage);

	// 绑定到引擎保存事件的回调函数
	static void OnObjectSaved(UObject* SavedObject);

	// 单独执行场景 Actor 检测
	UFUNCTION(BlueprintCallable, Category = "AssetChecker")
	static void RunSceneActorCheckOnly();
};