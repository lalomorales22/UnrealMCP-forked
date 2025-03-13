#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "UnrealMCP.h"

/**
 * Simple file logger for MCP operations
 * Writes logs to a file in the plugin directory
 */
class FMCPFileLogger
{
public:
    static FMCPFileLogger& Get()
    {
        static FMCPFileLogger Instance;
        return Instance;
    }

    void Initialize(const FString& InLogFilePath)
    {
        LogFilePath = InLogFilePath;
        
        // Create or clear the log file
        FString LogDirectory = FPaths::GetPath(LogFilePath);
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
        if (!PlatformFile.DirectoryExists(*LogDirectory))
        {
            PlatformFile.CreateDirectoryTree(*LogDirectory);
        }
        
        // Clear the file and write a header
        FString Header = FString::Printf(TEXT("MCP Server Log - Started at %s\n"), *FDateTime::Now().ToString());
        FFileHelper::SaveStringToFile(Header, *LogFilePath);
        
        bInitialized = true;
        UE_LOG(LogMCP, Log, TEXT("MCP File Logger initialized at %s"), *LogFilePath);
    }

    // Log with verbosity level
    void Log(ELogVerbosity::Type Verbosity, const FString& Message)
    {
        if (!bInitialized) return;
        
        // Log to Unreal's logging system - need to handle each verbosity level separately
        switch (Verbosity)
        {
            case ELogVerbosity::Fatal:
                UE_LOG(LogMCP, Fatal, TEXT("%s"), *Message);
                break;
            case ELogVerbosity::Error:
                UE_LOG(LogMCP, Error, TEXT("%s"), *Message);
                break;
            case ELogVerbosity::Warning:
                UE_LOG(LogMCP, Warning, TEXT("%s"), *Message);
                break;
            case ELogVerbosity::Display:
                UE_LOG(LogMCP, Display, TEXT("%s"), *Message);
                break;
            case ELogVerbosity::Log:
                UE_LOG(LogMCP, Log, TEXT("%s"), *Message);
                break;
            case ELogVerbosity::Verbose:
                UE_LOG(LogMCP, Verbose, TEXT("%s"), *Message);
                break;
            case ELogVerbosity::VeryVerbose:
                UE_LOG(LogMCP, VeryVerbose, TEXT("%s"), *Message);
                break;
            default:
                UE_LOG(LogMCP, Log, TEXT("%s"), *Message);
                break;
        }
        
        // Also log to file
        FString TimeStamp = FDateTime::Now().ToString();
        FString VerbosityStr;
        
        switch (Verbosity)
        {
            case ELogVerbosity::Fatal:   VerbosityStr = TEXT("Fatal"); break;
            case ELogVerbosity::Error:   VerbosityStr = TEXT("Error"); break;
            case ELogVerbosity::Warning: VerbosityStr = TEXT("Warning"); break;
            case ELogVerbosity::Display: VerbosityStr = TEXT("Display"); break;
            case ELogVerbosity::Log:     VerbosityStr = TEXT("Log"); break;
            case ELogVerbosity::Verbose: VerbosityStr = TEXT("Verbose"); break;
            case ELogVerbosity::VeryVerbose: VerbosityStr = TEXT("VeryVerbose"); break;
            default: VerbosityStr = TEXT("Unknown"); break;
        }
        
        FString LogEntry = FString::Printf(TEXT("[%s][%s] %s\n"), *TimeStamp, *VerbosityStr, *Message);
        FFileHelper::SaveStringToFile(LogEntry, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
    }
    
    // Convenience methods for different verbosity levels
    void Error(const FString& Message) { Log(ELogVerbosity::Error, Message); }
    void Warning(const FString& Message) { Log(ELogVerbosity::Warning, Message); }
    void Info(const FString& Message) { Log(ELogVerbosity::Log, Message); }
    void Verbose(const FString& Message) { Log(ELogVerbosity::Verbose, Message); }
    
    // For backward compatibility
    void Log(const FString& Message) { Info(Message); }

private:
    FMCPFileLogger() : bInitialized(false) {}
    ~FMCPFileLogger() {}
    
    // Make non-copyable
    FMCPFileLogger(const FMCPFileLogger&) = delete;
    FMCPFileLogger& operator=(const FMCPFileLogger&) = delete;
    
    bool bInitialized;
    FString LogFilePath;
}; 