#pragma once

#include "ActorComponent.h"
#include "Vector.h"
#include "LuaCoroutineScheduler.h"
#include "ULuaScriptComponent.generated.h"

namespace sol { class state; }
using state = sol::state;

class USceneComponent;

UCLASS(DisplayName="Lua 스크립트 컴포넌트", Description="Lua 스크립트를 실행하는 컴포넌트입니다")
class ULuaScriptComponent : public UActorComponent
{
public:

	GENERATED_REFLECTION_BODY()

	ULuaScriptComponent();
	~ULuaScriptComponent() override;

public:
	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;       // 매 프레임
	void EndPlay() override;							// 파괴/월드 제거 시

	void OnBeginOverlap(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp);
	void OnEndOverlap(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp);
	void OnHit(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp);

	bool Call(const char* FuncName, sol::variadic_args VarArgs); // 다른 클래스가 날 호출할 때 씀

	// 템플릿 버전 - 일반 C++ 코드에서 호출할 때 사용
	template<typename... Args>
	bool CallFunction(const char* FuncName, Args&&... args)
	{
		if (!Env.valid())
		{
			return false;
		}

		sol::protected_function Func = Env[FuncName];
		if (!Func.valid())
		{
			return false;
		}

		auto Result = Func(std::forward<Args>(args)...);
		if (!Result.valid())
		{
			sol::error Err = Result;
			return false;
		}

		return true;
	}

	// 반환값을 받을 수 있는 템플릿 버전
	template<typename ReturnType, typename... Args>
	bool CallFunctionWithReturn(const char* FuncName, ReturnType& OutReturn, Args&&... args)
	{
		if (!Env.valid())
		{
			return false;
		}

		sol::protected_function Func = Env[FuncName];
		if (!Func.valid())
		{
			return false;
		}

		auto Result = Func(std::forward<Args>(args)...);
		if (!Result.valid())
		{
			sol::error Err = Result;
			return false;
		}

		// 반환값 추출
		OutReturn = Result;
		return true;
	}

	void CleanupLuaResources();
protected:
	// 이 컴포넌트가 실행할 .lua 스크립트 파일의 경로 (에디터에서 설정)
	UPROPERTY(EditAnywhere, Category="Script", Tooltip="Lua Script 파일 경로입니다")
	FString ScriptFilePath{};

	sol::state* Lua = nullptr;
	sol::environment Env{};

	/* 함수 캐시 */
	sol::protected_function FuncBeginPlay{};
	sol::protected_function FuncTick{};
	sol::protected_function FuncOnBeginOverlap{};
	sol::protected_function FuncOnEndOverlap{};
	sol::protected_function FuncOnHit{};
	sol::protected_function FuncEndPlay{};

	FDelegateHandle BeginHandleLua{};
	FDelegateHandle EndHandleLua{};
	
	bool bIsLuaCleanedUp = false;
};