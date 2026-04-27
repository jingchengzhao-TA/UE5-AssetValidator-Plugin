#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/TextureDefines.h"
#include "AssetCheckerTypes.generated.h"

// ==========================================
// 规则 A：贴图检查配置表 (Texture Check Rule)
// ==========================================
USTRUCT(BlueprintType)
struct FTextureCheckRule : public FTableRowBase
{
    GENERATED_BODY()

public:
    // 要检查的目录路径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    FString DirectoryPath;

    // 允许的最大分辨率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    int32 MaxResolution = 2048;

    // 规定的压缩格式
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_Default;
};

// ==========================================
// 规则 B：静态模型检查配置表 (Static Mesh Check Rule)
// ==========================================
USTRUCT(BlueprintType)
struct FStaticMeshCheckRule : public FTableRowBase
{
    GENERATED_BODY()

public:
    // 要检查的目录路径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    FString DirectoryPath;

    // 正则表达式匹配规则
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    FString NameRegexPattern = TEXT("^SM_.*");

    // 允许的最大顶点数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    int32 MaxVertexCount = 10000;
};

// ==========================================
// 规则 C：场景对象检查配置表 (Actor Check Rule)
// ==========================================
USTRUCT(BlueprintType)
struct FActorCheckRule : public FTableRowBase
{
    GENERATED_BODY()

public:
    // 场景大纲中的特定层级目录
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    FString FolderPath;

    // 该目录下允许存在的蓝图类型 (白名单)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    TArray<TSubclassOf<AActor>> AllowedClasses;
};