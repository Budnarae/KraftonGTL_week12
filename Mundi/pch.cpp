#include "pch.h"

TMap<FString, FString> EditorINI;
const FString GDataDir = "Data";                    // Raw 소스 파일 (FBX, OBJ, 텍스처 원본)
const FString GContentDir = "Content";              // 콘텐츠 루트 폴더
const FString GResourceDir = "Content/Resources";   // 처리된 에셋 (.umesh, .uskel, .uanim, .utxt 등)
const FString GPrefabDir = "Content/Prefabs";       // 프리팹 파일 (.prefab)
const FString GScriptDir = "Content/Scripts";       // 스크립트 파일 (.lua)
const FString GAudioDataDir = "Audio";

// Global client area size used by various modules
float CLIENTWIDTH = 1024.0f;
float CLIENTHEIGHT = 1024.0f;

#ifdef _EDITOR
UEditorEngine GEngine;
#endif

#ifdef _GAME
UGameEngine GEngine;
#endif

UWorld* GWorld = nullptr;