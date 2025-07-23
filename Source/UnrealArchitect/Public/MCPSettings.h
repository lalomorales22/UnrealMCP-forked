#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MCPConstants.h"
#include "MCPSettings.generated.h"

UCLASS(config = Editor, defaultconfig)
class UNREALARCHITECT_API UMCPSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(config, EditAnywhere, Category = "MCP", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 Port = MCPConstants::DEFAULT_PORT;
}; 