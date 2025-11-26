● 완벽합니다! 이제 UPROPERTY 시스템에 대해 종합적으로 설명드리겠습니다.

  ---
  📋 UPROPERTY 시스템 설명

  🎯 1. 용도 (Why?)

  UPROPERTY 시스템은 다음 세 가지 주요 목적으로 사용됩니다:

  1️⃣ 런타임 리플렉션 (Runtime Reflection)

  - C++ 멤버 변수를 런타임에 조회하고 조작할 수 있게 만듭니다
  - 타입 정보, 이름, 메모리 오프셋 등을 저장하여 동적 접근이 가능합니다

  2️⃣ 에디터 편집 가능 (Editor Integration)

  - 에디터의 프로퍼티 패널에서 값을 시각적으로 편집할 수 있게 합니다
  - 슬라이더, 체크박스, 텍스처 선택기 등 타입별 UI를 자동으로 제공합니다

  3️⃣ Lua 스크립트 연동 (Script Binding)

  - Lua 스크립트에서 C++ 객체의 프로퍼티를 읽고 쓸 수 있게 합니다
  - actor:GetProperty("Speed") 같은 동적 접근이 가능해집니다

  ---
  🔧 2. 구현 원리 (How?)

  전체 워크플로우

  ┌─────────────────────────────────────────────────────────────┐
  │  Step 1: 개발자가 C++ 헤더 파일 작성                          │
  └─────────────────────────────────────────────────────────────┘
                                │
                                ▼
      class UStaticMeshComponent : public UMeshComponent {
          UPROPERTY(EditAnywhere, Category="Static Mesh")
          UStaticMesh* StaticMesh = nullptr;
      };
                                │
                                ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  Step 2: Python 코드 생성기 (generate.py) 실행              │
  │  - HeaderParser: 헤더 파일 파싱                              │
  │  - PropertyGenerator: 프로퍼티 등록 코드 생성                │
  └─────────────────────────────────────────────────────────────┘
                                │
                                ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  Step 3: .generated.cpp 파일 자동 생성                       │
  └─────────────────────────────────────────────────────────────┘
                                │
                                ▼
      BEGIN_PROPERTIES(UStaticMeshComponent)
          ADD_PROPERTY_STATICMESH(UStaticMesh*, StaticMesh,
                                 "Static Mesh", true, "...")
      END_PROPERTIES()
                                │
                                ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  Step 4: C++ 컴파일러가 매크로 확장 및 프로퍼티 등록        │
  └─────────────────────────────────────────────────────────────┘
                                │
                                ▼
      void UStaticMeshComponent::StaticRegisterProperties() {
          UClass* Class = StaticClass();
          FProperty Prop;
          Prop.Name = "StaticMesh";
          Prop.Type = EPropertyType::StaticMesh;
          Prop.Offset = offsetof(UStaticMeshComponent, StaticMesh);
          Class->AddProperty(Prop);
      }
                                │
                                ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  Step 5: 런타임에 리플렉션 시스템 사용 가능                  │
  │  - 에디터: 프로퍼티 패널에 자동 표시                          │
  │  - Lua: GetProperty/SetProperty 호출 가능                    │
  └─────────────────────────────────────────────────────────────┘

  ---
  핵심 컴포넌트

  A. Python 파서 (header_parser.py)

  헤더 파일을 스캔하여 UPROPERTY 정보를 추출합니다:

  # UPROPERTY(EditAnywhere, Category="Static Mesh", Tooltip="...")
  # UStaticMesh* StaticMesh = nullptr;

  # 이를 파싱하여 Property 객체 생성:
  Property(
      name="StaticMesh",
      type="UStaticMesh*",
      category="Static Mesh",
      editable=True,  # EditAnywhere 때문
      tooltip="Static mesh asset to render"
  )

  주요 파싱 기능:
  - 괄호 매칭: 중첩된 괄호를 정확하게 파싱 (UPROPERTY(Range="0.0, 100.0"))
  - 주석 제거: 주석 처리된 UPROPERTY는 무시
  - 타입 감지: 포인터, 배열, 범위 등을 자동으로 감지

  B. 코드 생성기 (property_generator.py)

  파싱된 정보로부터 C++ 등록 코드를 생성합니다:

  def get_property_type_macro(self) -> str:
      type_lower = self.type.lower()

      if 'tarray' in type_lower:
          return 'ADD_PROPERTY_ARRAY'
      elif '*' in self.type and 'ustaticmesh' in type_lower:
          return 'ADD_PROPERTY_STATICMESH'
      elif self.has_range:
          return 'ADD_PROPERTY_RANGE'
      else:
          return 'ADD_PROPERTY'

  타입별 매크로 선택 로직:
  - UStaticMesh* → ADD_PROPERTY_STATICMESH
  - TArray<T> → ADD_PROPERTY_ARRAY
  - Range="min, max" → ADD_PROPERTY_RANGE
  - 기본 타입 → ADD_PROPERTY

  C. 매크로 확장 (ObjectMacros.h)

  컴파일 타임에 매크로가 실제 코드로 확장됩니다:

  // 매크로 정의
  #define ADD_PROPERTY_STATICMESH(VarType, VarName, CategoryName, bEditAnywhere, ...) \
  { \
      FProperty Prop; \
      Prop.Name = #VarName; \
      Prop.Type = EPropertyType::StaticMesh; \
      Prop.Offset = offsetof(ThisClass_t, VarName); \
      Prop.Category = CategoryName; \
      Prop.bIsEditAnywhere = bEditAnywhere; \
      Prop.Tooltip = "" __VA_ARGS__; \
      Class->AddProperty(Prop); \
  }

  // 확장된 코드
  {
      FProperty Prop;
      Prop.Name = "StaticMesh";
      Prop.Type = EPropertyType::StaticMesh;
      Prop.Offset = offsetof(UStaticMeshComponent, StaticMesh);
      Prop.Category = "Static Mesh";
      Prop.bIsEditAnywhere = true;
      Prop.Tooltip = "Static mesh asset to render";
      Class->AddProperty(Prop);
  }

  offsetof의 역할:
  - 멤버 변수의 메모리 오프셋을 계산합니다
  - 나중에 객체 주소 + 오프셋으로 실제 값에 접근할 수 있습니다

  D. 리플렉션 데이터 저장 (Property.h)

  각 프로퍼티는 FProperty 구조체에 저장됩니다:

  struct FProperty {
      FName Name;                    // "StaticMesh"
      EPropertyType Type;            // EPropertyType::StaticMesh
      size_t Offset;                 // offsetof(클래스, 변수)
      FString Category;              // "Static Mesh"
      bool bIsEditAnywhere;          // true
      FString Tooltip;               // "Static mesh asset to render"
      float MinValue, MaxValue;      // Range용
      TMap<FName, FString> Metadata; // 추가 메타데이터
  };

  ---
  런타임 동작

  1. 프로퍼티 등록 시점

  // StaticClass() 호출 시 자동으로 프로퍼티 등록
  static UClass* StaticClass() {
      static UClass Cls{ "UStaticMeshComponent", ... };
      static bool bRegistered = (UClass::SignUpClass(&Cls), true);
      return &Cls;
  }

  // 첫 StaticClass() 호출 시 StaticRegisterProperties() 실행
  const bool UStaticMeshComponent::bPropertiesRegistered = []() {
      UStaticMeshComponent::StaticRegisterProperties();
      return true;
  }();

  초기화 순서:
  1. 프로그램 시작 → 전역 변수 초기화
  2. bPropertiesRegistered 초기화 → lambda 실행
  3. StaticRegisterProperties() 호출 → 프로퍼티 등록
  4. UClass에 프로퍼티 정보 저장 완료

  2. 프로퍼티 값 읽기/쓰기

  // 에디터나 Lua에서 프로퍼티 접근
  void* GetPropertyValue(UObject* Object, const FProperty& Prop) {
      // 객체 주소 + 오프셋 = 실제 멤버 변수 주소
      uint8* ObjectPtr = reinterpret_cast<uint8*>(Object);
      return ObjectPtr + Prop.Offset;
  }

  // 예시: StaticMesh 프로퍼티 읽기
  UStaticMeshComponent* Component = ...;
  FProperty* Prop = Component->GetClass()->FindProperty("StaticMesh");
  void* ValuePtr = GetPropertyValue(Component, *Prop);

  // 타입에 따라 캐스팅
  UStaticMesh** MeshPtr = static_cast<UStaticMesh**>(ValuePtr);
  UStaticMesh* Mesh = *MeshPtr;

  ---
  📝 3. 사용법 (Usage)

  기본 사용법

  // 헤더 파일: StaticMeshComponent.h
  #pragma once
  #include "MeshComponent.h"
  #include "UStaticMeshComponent.generated.h"  // ⚠️ 반드시 마지막에 include

  // UCLASS: 클래스 메타데이터
  UCLASS(DisplayName="스태틱 메시 컴포넌트",
         Description="정적 메시를 렌더링하는 컴포넌트입니다")
  class UStaticMeshComponent : public UMeshComponent
  {
  public:
      GENERATED_REFLECTION_BODY()  // ⚠️ public 섹션에 위치

      // UPROPERTY: 프로퍼티 마킹
      UPROPERTY(EditAnywhere, Category="Static Mesh", Tooltip="Static mesh asset")
      UStaticMesh* StaticMesh = nullptr;

      UPROPERTY(EditAnywhere, Category="Rendering", Range="0.0, 1.0")
      float Opacity = 1.0f;
  };

  ---
  UPROPERTY 파라미터

  | 파라미터         | 설명             | 예시                      |
  |--------------|----------------|-------------------------|
  | EditAnywhere | 에디터에서 편집 가능    | UPROPERTY(EditAnywhere) |
  | Category     | 에디터에서 그룹화 표시   | Category="Rendering"    |
  | Range        | 숫자 값 범위 제한     | Range="0.0, 100.0"      |
  | Tooltip      | 마우스 오버 시 설명 표시 | Tooltip="밝기 조절"         |

  ---
  지원하는 타입

  기본 타입

  UPROPERTY(EditAnywhere, Category="Basic")
  bool bIsEnabled = true;

  UPROPERTY(EditAnywhere, Category="Basic")
  int32 Count = 0;

  UPROPERTY(EditAnywhere, Category="Basic")
  float Speed = 100.0f;

  UPROPERTY(EditAnywhere, Category="Basic")
  FString Name = "Default";

  UPROPERTY(EditAnywhere, Category="Basic")
  FVector Position = FVector::ZeroVector;

  UPROPERTY(EditAnywhere, Category="Basic")
  FLinearColor Color = FLinearColor::White;

  포인터 타입 (에셋 참조)

  UPROPERTY(EditAnywhere, Category="Assets")
  UTexture* Texture = nullptr;  // → ADD_PROPERTY_TEXTURE

  UPROPERTY(EditAnywhere, Category="Assets")
  UStaticMesh* Mesh = nullptr;  // → ADD_PROPERTY_STATICMESH

  UPROPERTY(EditAnywhere, Category="Assets")
  UMaterialInterface* Material = nullptr;  // → ADD_PROPERTY_MATERIAL

  UPROPERTY(EditAnywhere, Category="Assets")
  USoundBase* Sound = nullptr;  // → ADD_PROPERTY_AUDIO

  배열 타입

  UPROPERTY(EditAnywhere, Category="Arrays")
  TArray<UMaterialInterface*> Materials;  // → ADD_PROPERTY_ARRAY

  UPROPERTY(EditAnywhere, Category="Arrays")
  TArray<int> Numbers;

  범위 제한 타입

  UPROPERTY(EditAnywhere, Category="Light", Range="0.0, 10.0")
  float Intensity = 1.0f;  // → ADD_PROPERTY_RANGE (슬라이더 UI)

  UPROPERTY(EditAnywhere, Category="Camera", Range="1.0, 179.0")
  float FOV = 90.0f;

  ---
  코드 생성 실행

  # Python 스크립트 실행
  python Tools/CodeGenerator/generate.py \
      --source-dir Source/Runtime \
      --output-dir Generated

  # 출력:
  # ✓ Found reflection class: UStaticMeshComponent in StaticMeshComponent.h
  # ✓ Updated: UStaticMeshComponent.generated.cpp
  #   ├─ Properties: 1
  #   └─ Functions: 0

  ---
  Lua에서 사용

  -- 컴포넌트 가져오기
  local meshComp = actor:GetComponentByClass("UStaticMeshComponent")

  -- 프로퍼티 읽기
  local mesh = meshComp:GetProperty("StaticMesh")

  -- 프로퍼티 쓰기
  meshComp:SetProperty("StaticMesh", "Assets/Models/Cube.fbx")
  meshComp:SetProperty("Opacity", 0.5)

  -- 프로퍼티 목록 조회
  local props = meshComp:GetClass():GetProperties()
  for i, prop in ipairs(props) do
      print(prop.Name, prop.Type, prop.Category)
  end

  ---
  에디터에서 사용

  에디터에서 액터를 선택하면 프로퍼티 패널에 다음과 같이 표시됩니다:

  ┌─────────────────────────────────────────────┐
  │  [Static Mesh]                               │
  │    StaticMesh: [ 없음 ▼ ]  ℹ️ Static mesh... │
  │                                              │
  │  [Rendering]                                 │
  │    Opacity: [━━━━━━━●━━] 1.0                │
  │             (0.0 ~ 1.0)                      │
  └─────────────────────────────────────────────┘

  ---
  ⚙️ 장점과 특징

  ✅ 장점

  1. 보일러플레이트 제거
    - 수동으로 작성해야 할 수백 줄의 등록 코드가 자동 생성됩니다
  2. 타입 안전성
    - offsetof를 사용하여 컴파일 타임에 타입 검증
  3. 일관성 유지
    - 모든 프로퍼티가 동일한 패턴으로 등록됩니다
  4. 개발 속도 향상
    - 새 프로퍼티 추가: 한 줄만 작성하면 끝
  5. 오류 감소
    - Python 파서가 문법 오류를 자동으로 감지

  🔧 특징

  - Unreal Engine 스타일: UE4/UE5의 UPROPERTY와 거의 동일한 사용법
  - 확장 가능: 새로운 타입이나 메타데이터를 쉽게 추가 가능
  - 빌드 시스템 통합: Pre-Build Event로 자동화 가능

  ---
  ⚠️ 주의사항

  // ❌ 잘못된 예시들

  // 1. .generated.h include 순서 잘못됨
  #include "Actor.h"
  #include "MyActor.generated.h"  // ⚠️ 반드시 마지막!

  // 2. GENERATED_REFLECTION_BODY() 위치 잘못됨
  class MyActor {
  private:  // ⚠️ public에 있어야 함!
      GENERATED_REFLECTION_BODY()
  };

  // 3. 포인터 초기화 누락
  UPROPERTY(EditAnywhere)
  UTexture* Texture;  // ⚠️ = nullptr 필수!

  // 4. Range 형식 잘못됨
  UPROPERTY(EditAnywhere, Range="0-100")  // ⚠️ 쉼표 사용!

  ---
  📊 실제 생성 예시

  입력 (헤더 파일):
  UPROPERTY(EditAnywhere, Category="Static Mesh", Tooltip="Static mesh asset")
  UStaticMesh* StaticMesh = nullptr;

  출력 (.generated.cpp):
  BEGIN_PROPERTIES(UStaticMeshComponent)
      MARK_AS_COMPONENT("스태틱 메시 컴포넌트", "정적 메시를 렌더링하는 컴포넌트입니다")
      ADD_PROPERTY_STATICMESH(UStaticMesh*, StaticMesh, "Static Mesh", true, "Static mesh asset to render")
  END_PROPERTIES()

  최종 확장 (컴파일 후):
  void UStaticMeshComponent::StaticRegisterProperties() {
      UClass* Class = StaticClass();
      FProperty Prop;
      Prop.Name = "StaticMesh";
      Prop.Type = EPropertyType::StaticMesh;
      Prop.Offset = offsetof(UStaticMeshComponent, StaticMesh);
      Prop.Category = "Static Mesh";
      Prop.bIsEditAnywhere = true;
      Prop.Tooltip = "Static mesh asset to render";
      Class->AddProperty(Prop);
  }

  ---
  이 시스템은 Unreal Engine의 리플렉션 시스템을 본떠 만들어졌으며, Python 기반 코드 생성 + C++ 매크로
  확장의 조합으로 동작합니다. 개발자는 단순히 UPROPERTY 매크로만 추가하면, 에디터 편집과 Lua 스크립팅이
  모두 자동으로 지원됩니다!