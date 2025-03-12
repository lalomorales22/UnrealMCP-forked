#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MCPSettings.generated.h"

UCLASS(config = Editor, defaultconfig)
class UNREALMCP_API UMCPSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(config, EditAnywhere, Category = "MCP", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 Port = 9876; // Default port, matching Blender MCP
}; 