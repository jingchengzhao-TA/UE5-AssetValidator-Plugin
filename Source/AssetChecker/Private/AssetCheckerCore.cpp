// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetCheckerCore.h"
#include "Engine/Texture2D.h"
#include "Misc/MessageDialog.h"
#include "Engine/StaticMesh.h"
#include "Internationalization/Regex.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h" // 用于 TActorIterator 遍历场景
// ===== I/O 与编辑器 UI 需要的头文件 =====
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Editor.h"
// =====================================
// ===== 高级日志与系统事件需要的头文件 =====
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "UObject/UObjectGlobals.h"
// =========================================

TArray<FString> UAssetCheckerCore::CheckTextures(const TArray<UObject*>& SelectedAssets, UDataTable* RuleTable, bool bAutoFix)
{
	TArray<FString> ErrorMessages;

	// 如果没有配置表，直接返回空错误
	if (!RuleTable)
	{
		ErrorMessages.Add(TEXT("错误：未提供贴图检查规则配置表！"));
		return ErrorMessages;
	}

	// 从配置表中获取所有的规则行
	TArray<FTextureCheckRule*> Rules;
	RuleTable->GetAllRows<FTextureCheckRule>(TEXT("TextureCheckContext"), Rules);

	// 遍历用户选中的所有资产
	for (UObject* Asset : SelectedAssets)
	{
		// 尝试将资产转换为 Texture2D，如果转换失败说明选中的不是贴图，跳过
		UTexture2D* Texture = Cast<UTexture2D>(Asset);
		if (!Texture) continue;

		// 获取这个贴图在引擎里的路径 (例如: /Game/Textures/T_Hero_D)
		FString AssetPath = Texture->GetPathName();
		FTextureCheckRule* MatchedRule = nullptr;

		// 遍历规则表，看这个贴图属于哪个目录下的规则
		for (FTextureCheckRule* Rule : Rules)
		{
			if (AssetPath.StartsWith(Rule->DirectoryPath))
			{
				MatchedRule = Rule;
				break; // 找到对应目录的规则就停止寻找
			}
		}

		// 如果没有为这个贴图所在的目录配置规则，则跳过不检查
		if (!MatchedRule) continue;

		bool bNeedsFix = false;
		FString ErrorStr = FString::Printf(TEXT("资产: %s -> "), *Asset->GetName());

		// 检查 1：分辨率 (取贴图源文件的最大边长)
		int32 MaxSourceSize = FMath::Max(Texture->Source.GetSizeX(), Texture->Source.GetSizeY());
		// 虚幻中通过 MaxTextureSize 属性来限制最终的烘焙分辨率
		int32 ActualMaxSize = (Texture->MaxTextureSize > 0) ? Texture->MaxTextureSize : MaxSourceSize;

		if (ActualMaxSize > MatchedRule->MaxResolution)
		{
			ErrorStr += FString::Printf(TEXT("[分辨率超标: 当前 %d，限制 %d] "), ActualMaxSize, MatchedRule->MaxResolution);
			bNeedsFix = true;
		}

		// 检查 2：压缩格式
		if (Texture->CompressionSettings != MatchedRule->CompressionSettings)
		{
			ErrorStr += TEXT("[压缩格式不规范] ");
			bNeedsFix = true;
		}

		// 如果查出了问题
		if (bNeedsFix)
		{
			// 如果用户勾选了“自动修复”
			if (bAutoFix)
			{
				// 修改贴图属性
				Texture->MaxTextureSize = MatchedRule->MaxResolution;
				Texture->CompressionSettings = MatchedRule->CompressionSettings;

				// 调用 Modify() 告诉引擎这个对象被动过手脚了（用于撤销历史）
				Texture->Modify();

				// 【核心要求点】给资产添加脏标记，这样资产左下角会出现星号，提示用户需要保存
				Texture->MarkPackageDirty();

				// 更新贴图资源使修改在编辑器里立刻生效
				Texture->UpdateResource();

				ErrorStr += TEXT("【已自动修复并添加脏标记】");
			}

			// 将这一条错误信息加入到最终要打印的列表中
			ErrorMessages.Add(ErrorStr);
		}
	}

	return ErrorMessages;
}

// ==========================================
// 模型检测逻辑 (Requirement b)
// ==========================================
TArray<FString> UAssetCheckerCore::CheckStaticMeshes(const TArray<UObject*>& SelectedAssets, UDataTable* RuleTable)
{
	TArray<FString> ErrorMessages;
	if (!RuleTable) return ErrorMessages;

	TArray<FStaticMeshCheckRule*> Rules;
	RuleTable->GetAllRows<FStaticMeshCheckRule>(TEXT("SMContext"), Rules);

	for (UObject* Asset : SelectedAssets)
	{
		// 转换成 StaticMesh，失败则跳过
		UStaticMesh* SM = Cast<UStaticMesh>(Asset);
		if (!SM) continue;

		FString AssetPath = SM->GetPathName();
		FString AssetName = SM->GetName(); // 资产名字，用于正则匹配
		FStaticMeshCheckRule* MatchedRule = nullptr;

		// 匹配对应目录的规则
		for (FStaticMeshCheckRule* Rule : Rules)
		{
			if (AssetPath.StartsWith(Rule->DirectoryPath))
			{
				MatchedRule = Rule;
				break;
			}
		}

		if (!MatchedRule) continue;

		bool bHasError = false;
		FString ErrorStr = FString::Printf(TEXT("模型: %s -> "), *AssetName);

		// 1. 正则表达式检查名称/路径
		FRegexPattern Pattern(MatchedRule->NameRegexPattern);
		FRegexMatcher Matcher(Pattern, AssetName);
		// 如果 FindNext 返回 false，说明名字不符合正则规则
		if (!Matcher.FindNext())
		{
			ErrorStr += FString::Printf(TEXT("[命名不符合正则规范: %s] "), *MatchedRule->NameRegexPattern);
			bHasError = true;
		}

		// 2. 检查顶点数 (读取 LOD 0 的顶点数)
		if (SM->GetNumLODs() > 0)
		{
			int32 VertexCount = SM->GetNumVertices(0);
			if (VertexCount > MatchedRule->MaxVertexCount)
			{
				ErrorStr += FString::Printf(TEXT("[顶点数超标: 当前 %d，限制 %d] "), VertexCount, MatchedRule->MaxVertexCount);
				bHasError = true;
			}
		}

		if (bHasError)
		{
			ErrorMessages.Add(ErrorStr);
		}
	}
	return ErrorMessages;
}

// ==========================================
// 场景Actor检测逻辑 (Requirement c) - 增强版
// ==========================================
TArray<FString> UAssetCheckerCore::CheckActorsInWorld(UWorld* World, UDataTable* RuleTable)
{
	TArray<FString> ErrorMessages;
	if (!World || !RuleTable) return ErrorMessages;

	TArray<FActorCheckRule*> Rules;
	RuleTable->GetAllRows<FActorCheckRule>(TEXT("ActorContext"), Rules);

	// 创建一个专门用于 Actor 报错的日志分类
	FMessageLog ActorLog("AssetChecker_Scene");

	// 遍历当前场景中的所有 Actor
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		// 获取 Actor 在场景大纲(Outliner)中的文件夹路径
		FString FolderPath = Actor->GetFolderPath().ToString();
		FActorCheckRule* MatchedRule = nullptr;

		// 寻找该 Actor 所在目录的规则
		for (FActorCheckRule* Rule : Rules)
		{
			if (FolderPath.StartsWith(Rule->FolderPath))
			{
				MatchedRule = Rule;
				break;
			}
		}

		if (!MatchedRule) continue; // 如果不在需要检查的目录，跳过

		bool bIsAllowed = false;
		UClass* ActorClass = Actor->GetClass();

		// 检查该 Actor 的类型是否在允许的白名单里
		for (TSubclassOf<AActor> AllowedClass : MatchedRule->AllowedClasses)
		{
			// IsChildOf 可以判断继承关系，比如允许放置"Character"，那玩家继承自Character的蓝图也是合法的
			if (AllowedClass && ActorClass->IsChildOf(AllowedClass))
			{
				bIsAllowed = true;
				break;
			}
		}

		// 如果发现不符合要求的 Actor
		if (!bIsAllowed)
		{
			FString OldLabel = Actor->GetActorLabel();

			// 1. 【满足要求】：在大纲中把错误信息附加上去，引起视觉注意
			if (!OldLabel.Contains(TEXT("[❌非法类型]")))
			{
				Actor->SetActorLabel(TEXT("[❌非法类型] ") + OldLabel);
			}

			// 2. 【满足要求】：输出带超链接的高级日志，点击即可在场景中定位（高亮爆红）
			FString ErrorMsg = FString::Printf(TEXT(" 目录 [%s] 限制蓝图类型，当前非法类型为: %s"), *FolderPath, *ActorClass->GetName());
			TSharedRef<FTokenizedMessage> Msg = ActorLog.Error();
			Msg->AddToken(FUObjectToken::Create(Actor)); // 生成可点击的 Actor 超链接
			Msg->AddToken(FTextToken::Create(FText::FromString(ErrorMsg)));

			ErrorMessages.Add(OldLabel + ErrorMsg);
		}
	}

	// 如果有错误，就自动弹出并聚焦日志窗口
	if (ErrorMessages.Num() > 0)
	{
		ActorLog.Open();
	}

	return ErrorMessages;
}


// ==========================================
// 全局执行与输出 (Requirement 2 & 3)
// ==========================================
void UAssetCheckerCore::RunGlobalAssetCheck(bool bAutoFix)
{
	// 1. 加载在引擎里配置好的数据表
	UDataTable* TexTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AssetCheckerRules/DT_TexRule.DT_TexRule")));
	UDataTable* SMTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AssetCheckerRules/DT_SMRule.DT_SMRule")));
	UDataTable* ActorTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AssetCheckerRules/DT_ActorRule.DT_ActorRule")));

	// 2. 获取当前内容浏览器中，用户右键选中的资产
	FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssetsData;
	CBModule.Get().GetSelectedAssets(SelectedAssetsData);

	TArray<UObject*> SelectedObjects;
	for (const FAssetData& AssetData : SelectedAssetsData)
	{
		if (UObject* Obj = AssetData.GetAsset())
		{
			SelectedObjects.Add(Obj);
		}
	}

	// 3. 执行所有的检查，收集报错信息
	TArray<FString> AllErrors;

	if (TexTable) AllErrors.Append(CheckTextures(SelectedObjects, TexTable, bAutoFix));
	if (SMTable) AllErrors.Append(CheckStaticMeshes(SelectedObjects, SMTable));

	if (GEditor && ActorTable)
	{
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		AllErrors.Append(CheckActorsInWorld(EditorWorld, ActorTable));
	}

	// 4. 处理结果 (弹出提示框 & 写入 TXT)
	if (AllErrors.Num() > 0)
	{
		FString OutputString;
		for (const FString& Err : AllErrors) OutputString += Err + TEXT("\n");

		// 保存到工程的 Saved 目录下
		FString FilePath = FPaths::ProjectSavedDir() / TEXT("AssetCheckerReport.txt");
		FFileHelper::SaveStringToFile(OutputString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);

		// 弹出统计信息提示框
		FString DialogMsg = FString::Printf(TEXT("检查完毕！共发现 %d 个不规范资产。\n详细报错已输出至：\n%s"), AllErrors.Num(), *FilePath);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(DialogMsg));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("检查完毕！太棒了，没有发现任何不规范的资产。")));
	}
}

// ==========================================
// 升级 UX 与自动化
// ==========================================
void UAssetCheckerCore::PrintClickableLog(UObject* Asset, const FString& ErrorMessage)
{
	if (!Asset) return;

	// 创建一个名为 "AssetChecker" 的独立日志分类
	FMessageLog AssetLog("AssetChecker");

	// 创建一条 Error 级别的信息
	TSharedRef<FTokenizedMessage> Msg = AssetLog.Error();

	// 【核心】添加一个 UObjectToken，这样用户在日志里点击它，就能在内容浏览器中定位该资产
	Msg->AddToken(FUObjectToken::Create(Asset));
	Msg->AddToken(FTextToken::Create(FText::FromString(ErrorMessage)));

	// 弹出并聚焦到日志窗口
	AssetLog.Open();
}

void UAssetCheckerCore::OnObjectSaved(UObject* SavedObject)
{
	// 确保保存的是一个真正的实体资产（而不是底层的临时对象）
	if (!SavedObject || !SavedObject->IsAsset()) return;

	// 构造参数
	TArray<UObject*> SavedAssets;
	SavedAssets.Add(SavedObject);

	// 读取配置表
	UDataTable* TexTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AssetCheckerRules/DT_TexRule.DT_TexRule")));
	UDataTable* SMTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AssetCheckerRules/DT_SMRule.DT_SMRule")));

	TArray<FString> Errors;
	// 这里传 false（不自动修复），因为保存时强制改写用户资产容易导致体验不好，所以只做警告
	if (TexTable) Errors.Append(CheckTextures(SavedAssets, TexTable, false));
	if (SMTable)  Errors.Append(CheckStaticMeshes(SavedAssets, SMTable));

	// 如果查出错误，就输出高级日志
	for (const FString& Err : Errors)
	{
		PrintClickableLog(SavedObject, TEXT(" Auto-Check Blocked: ") + Err);
	}
}

void UAssetCheckerCore::RunSceneActorCheckOnly()
{
	UDataTable* ActorTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AssetCheckerRules/DT_ActorRule.DT_ActorRule")));

	if (GEditor && ActorTable)
	{
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		TArray<FString> Errors = CheckActorsInWorld(EditorWorld, ActorTable);

		if (Errors.Num() == 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Scene Check Complete! No illegal actors found.")));
		}
	}
}