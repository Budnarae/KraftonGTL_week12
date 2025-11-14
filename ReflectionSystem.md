# 리플렉션 시스템 전체 아키텍처

## 목차
1. [시스템 개요](#시스템-개요)
2. [핵심 구성 요소](#핵심-구성-요소)
3. [동작 흐름](#동작-흐름)
4. [사용 가이드](#사용-가이드)
5. [실전 예제](#실전-예제)

---

## 시스템 개요

이 리플렉션 시스템은 **Unreal Engine 스타일**의 런타임 타입 정보(RTTI)를 제공하여, C++ 코드의 클래스, 프로퍼티, 함수 정보를 런타임에 조회하고 조작할 수 있게 합니다.

### 주요 기능

1. **타입 정보 조회** - 클래스 이름, 크기, 상속 관계
2. **프로퍼티 리플렉션** - 멤버 변수의 타입, 오프셋, 메타데이터
3. **동적 객체 생성** - 클래스 이름으로 인스턴스 생성
4. **직렬화/역직렬화** - 자동 JSON 변환
5. **에디터 통합** - 프로퍼티 패널 자동 생성
6. **Lua 스크립팅** - C++ 객체를 Lua에서 조작

### 시스템 구조

```
┌─────────────────────────────────────────────────────────────┐
│                    리플렉션 시스템                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │   UObject    │───▶│    UClass    │◀───│   Property   │  │
│  │  (기본 객체)  │    │ (타입 정보)   │    │(프로퍼티 정보)│  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│         │                    │                    │          │
│         │                    │                    │          │
│         ▼                    ▼                    ▼          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │ObjectFactory │    │ObjectMacros.h│    │LuaBindHelpers│  │
│  │ (객체 생성)   │    │  (매크로)     │    │(Lua 바인딩)  │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│         │                    │                    │          │
│         └────────────────────┴────────────────────┘          │
│                              │                                │
│                              ▼                                │
│                   ┌──────────────────┐                       │
│                   │  Code Generator   │                       │
│                   │  (Python 자동화)   │                       │
│                   └──────────────────┘                       │
└─────────────────────────────────────────────────────────────┘
```

---

## 핵심 구성 요소

### 1. UObject (Object.h)

모든 리플렉션 가능한 객체의 베이스 클래스입니다.

#### 핵심 멤버

```cpp
class UObject
{
public:
    uint32_t UUID;           // 고유 식별자
    uint32_t InternalIndex;  // GUObjectArray 인덱스
    FName    ObjectName;     // 이름 (에디터 표시용)

    // 타입 정보 조회
    static UClass* StaticClass();           // 정적 타입 정보
    virtual UClass* GetClass() const;       // 동적 타입 정보

    // 타입 검사
    bool IsA(const UClass* C) const;        // 상속 관계 확인
    template<class T> bool IsA() const;     // 템플릿 버전

    // 직렬화
    virtual void Serialize(bool bIsLoading, JSON& Handle);

    // 복사
    virtual UObject* Duplicate() const;     // 깊은 복사
    virtual void DuplicateSubObjects();     // 서브객체 복사
    virtual void PostDuplicate();           // 복사 후 처리

    // UUID 관리
    static uint32 GenerateUUID();           // 새 UUID 생성
    static uint32 PeekNextUUID();           // 다음 UUID 확인
    static void SetNextUUID(uint32 Next);   // UUID 카운터 설정
};
```

#### Cast 헬퍼

안전한 타입 변환을 제공합니다:

```cpp
// Unreal Engine과 동일한 Cast<> 문법
AActor* Actor = Cast<AActor>(Obj);
if (Actor)
{
    // Actor로 사용
}
```

**구현:**

```cpp
template<class T>
T* Cast(UObject* Obj) noexcept
{
    return (Obj && Obj->IsA<T>()) ? static_cast<T*>(Obj) : nullptr;
}
```

---

### 2. UClass (Object.h)

클래스의 메타데이터를 저장하는 구조체입니다.

#### 핵심 멤버

```cpp
struct UClass
{
    const char* Name;              // 클래스 이름
    const UClass* Super;           // 부모 클래스
    SIZE_T Size;                   // 클래스 크기

    // 프로퍼티 리플렉션
    TArray<FProperty> Properties;             // 이 클래스의 프로퍼티
    TArray<FProperty> CachedAllProperties;    // 부모 포함 전체 (캐시)
    bool bAllPropertiesCached;                // 캐시 유효성

    // 메타데이터
    bool bIsSpawnable;             // 에디터에서 생성 가능?
    bool bIsComponent;             // 컴포넌트인가?
    const char* DisplayName;       // UI 표시 이름
    const char* Description;       // 툴팁 설명

    // 상속 관계 확인
    bool IsChildOf(const UClass* Base) const;

    // 프로퍼티 관리
    void AddProperty(const FProperty& Property);
    const TArray<FProperty>& GetProperties() const;
    const TArray<FProperty>& GetAllProperties() const;  // 부모 포함

    // 전역 클래스 관리
    static TArray<UClass*>& GetAllClasses();
    static void SignUpClass(UClass* InClass);
    static UClass* FindClass(const FName& InClassName);
    static TArray<UClass*> GetAllSpawnableActors();
    static TArray<UClass*> GetAllComponents();
};
```

#### 프로퍼티 캐싱 메커니즘

부모 클래스의 프로퍼티를 포함한 전체 프로퍼티 목록을 캐싱하여 성능을 최적화합니다:

```cpp
const TArray<FProperty>& UClass::GetAllProperties() const
{
    if (!bAllPropertiesCached)
    {
        CachedAllProperties.clear();

        // 부모 클래스의 프로퍼티 먼저 추가
        if (Super)
        {
            const TArray<FProperty>& ParentProps = Super->GetAllProperties();
            for (const FProperty& Prop : ParentProps)
                CachedAllProperties.Add(Prop);
        }

        // 이 클래스의 프로퍼티 추가
        for (const FProperty& Prop : Properties)
            CachedAllProperties.Add(Prop);

        bAllPropertiesCached = true;
    }
    return CachedAllProperties;
}
```

---

### 3. FProperty (Property.h)

프로퍼티의 메타데이터를 저장하는 구조체입니다.

#### EPropertyType (지원하는 타입)

```cpp
enum class EPropertyType : uint8
{
    Unknown,
    Bool,                 // bool
    Int32,                // int32
    Float,                // float
    FVector,              // FVector
    FLinearColor,         // FLinearColor
    FString,              // FString
    FName,                // FName
    ObjectPtr,            // UObject* 및 파생 타입
    Struct,               // 구조체
    Texture,              // UTexture* (리소스 선택 UI)
    SkeletalMesh,         // USkeletalMesh*
    StaticMesh,           // UStaticMesh*
    Material,             // UMaterialInterface*
    Array,                // TArray<T>
    SRV,                  // Shader Resource View
    ScriptFile,           // Lua 스크립트 파일
    Sound,                // USoundBase*
    Curve,                // UCurveFloat*
    Count                 // 타입 개수
};
```

#### FProperty 구조체

```cpp
struct FProperty
{
    const char* Name;              // 프로퍼티 이름
    EPropertyType Type;            // 프로퍼티 타입
    EPropertyType InnerType;       // TArray<T>의 T 타입
    size_t Offset;                 // 클래스 인스턴스 내 메모리 오프셋

    // UI 메타데이터
    const char* Category;          // 에디터 카테고리
    float MinValue;                // 범위 최소값
    float MaxValue;                // 범위 최대값
    bool bIsEditAnywhere;          // 에디터에서 편집 가능?
    const char* Tooltip;           // 툴팁 설명

    TMap<FName, FString> Metadata; // 추가 메타데이터

    // 타입 안전 포인터 접근
    template<typename T>
    T* GetValuePtr(void* ObjectInstance) const
    {
        return reinterpret_cast<T*>(
            reinterpret_cast<char*>(ObjectInstance) + Offset
        );
    }

    template<typename T>
    const T* GetValuePtr(const void* ObjectInstance) const
    {
        return reinterpret_cast<const T*>(
            reinterpret_cast<const char*>(ObjectInstance) + Offset
        );
    }
};
```

#### 사용 예제

```cpp
// 런타임에 프로퍼티 값 읽기/쓰기
UClass* Class = Obj->GetClass();
for (const FProperty& Prop : Class->GetAllProperties())
{
    if (Prop.Type == EPropertyType::Float)
    {
        float* ValuePtr = Prop.GetValuePtr<float>(Obj);
        *ValuePtr = 10.0f;  // 값 설정
    }
}
```

---

### 4. ObjectFactory (ObjectFactory.h)

객체의 생성과 삭제를 관리하는 팩토리 시스템입니다.

#### 핵심 기능

```cpp
namespace ObjectFactory
{
    using ConstructFunc = std::function<UObject*()>;

    // 클래스 등록 (Static 초기화 시점에 자동 호출)
    void RegisterClassType(UClass* Class, ConstructFunc Func);

    // 1. 순수 생성 (GUObjectArray 등록 X)
    UObject* ConstructObject(UClass* Class);

    // 2. 생성 + GUObjectArray 자동 등록
    UObject* NewObject(UClass* Class);

    // 3. 템플릿 버전 (타입 안전)
    template<class T>
    T* NewObject()
    {
        return static_cast<T*>(NewObject(T::StaticClass()));
    }

    // 4. 복사 생성 + GUObjectArray 등록
    template<class T>
    T* DuplicateObject(const UObject* Source)
    {
        UObject* Dest = new T(*static_cast<const T*>(Source));
        return static_cast<T*>(AddToGUObjectArray(T::StaticClass(), Dest));
    }

    // 삭제
    void DeleteObject(UObject* Obj);
    void DeleteAll(bool bCallBeginDestroy = true);
    void CompactNullSlots();  // Null 슬롯 압축
}
```

#### GUObjectArray

모든 생성된 UObject를 추적하는 전역 배열:

```cpp
extern TArray<UObject*> GUObjectArray;
```

#### 자동 클래스 등록 매크로

```cpp
#define IMPLEMENT_CLASS(ThisClass) \
    namespace { \
        struct ThisClass##FactoryRegister \
        { \
            ThisClass##FactoryRegister() \
            { \
                ObjectFactory::RegisterClassType( \
                    ThisClass::StaticClass(), \
                    []() -> UObject* { return new ThisClass(); } \
                ); \
            } \
        }; \
        static ThisClass##FactoryRegister GRegister_##ThisClass; \
        static bool bIsRegistered_##ThisClass = []() { \
            ThisClass::StaticClass(); \
            return true; \
        }(); \
    }
```

**Static 초기화 순서:**
1. `ThisClass##FactoryRegister` 정적 인스턴스 생성
2. 생성자에서 `RegisterClassType()` 호출
3. 클래스가 팩토리에 등록됨
4. `bIsRegistered_##ThisClass` 람다 실행으로 `StaticClass()` 호출
5. UClass가 `GetAllClasses()`에 등록됨

---

### 5. ObjectMacros.h

리플렉션 시스템을 위한 매크로 정의 파일입니다.

#### 타입 자동 감지 템플릿

컴파일 타임에 타입을 자동으로 EPropertyType으로 변환합니다:

```cpp
template<typename T>
struct TPropertyTypeTraits
{
    static constexpr EPropertyType GetType()
    {
        if constexpr (std::is_same_v<T, bool>)
            return EPropertyType::Bool;
        else if constexpr (std::is_same_v<T, int32>)
            return EPropertyType::Int32;
        else if constexpr (std::is_same_v<T, float>)
            return EPropertyType::Float;
        else if constexpr (std::is_same_v<T, FVector>)
            return EPropertyType::FVector;
        else if constexpr (std::is_same_v<T, FLinearColor>)
            return EPropertyType::FLinearColor;
        else if constexpr (std::is_same_v<T, FString>)
            return EPropertyType::FString;
        else if constexpr (std::is_same_v<T, FName>)
            return EPropertyType::FName;
        else if constexpr (std::is_pointer_v<T>)
            return EPropertyType::ObjectPtr;
        else
            return EPropertyType::Struct;
    }
};
```

#### 코드 생성 마커 매크로

컴파일 시에는 빈 매크로이지만, Python 코드 생성기가 파싱합니다:

```cpp
// 프로퍼티 마커
#define UPROPERTY(...)

// 함수 마커
#define UFUNCTION(...)

// 클래스 마커
#define UCLASS(...)

// 리플렉션 바디 (코드 생성기가 .generated.h에 정의)
#define GENERATED_REFLECTION_BODY() \
    CURRENT_CLASS_GENERATED_BODY
```

#### 프로퍼티 등록 매크로

```cpp
// StaticRegisterProperties 함수 시작
#define BEGIN_PROPERTIES(ClassName) \
    void ClassName::StaticRegisterProperties() \
    { \
        UClass* Class = StaticClass();

// StaticRegisterProperties 함수 종료
#define END_PROPERTIES() \
    }

// 기본 프로퍼티 추가
#define ADD_PROPERTY(VarType, VarName, CategoryName, bEditAnywhere, ...) \
    { \
        FProperty Prop; \
        Prop.Name = #VarName; \
        Prop.Type = TPropertyTypeTraits<VarType>::GetType(); \
        Prop.Offset = offsetof(ThisClass_t, VarName); \
        Prop.Category = CategoryName; \
        Prop.bIsEditAnywhere = bEditAnywhere; \
        Prop.Tooltip = "" __VA_ARGS__; \
        Class->AddProperty(Prop); \
    }

// 범위 제한 프로퍼티 추가
#define ADD_PROPERTY_RANGE(VarType, VarName, CategoryName, MinVal, MaxVal, bEditAnywhere, ...) \
    { \
        FProperty Prop; \
        Prop.Name = #VarName; \
        Prop.Type = TPropertyTypeTraits<VarType>::GetType(); \
        Prop.Offset = offsetof(ThisClass_t, VarName); \
        Prop.Category = CategoryName; \
        Prop.MinValue = MinVal; \
        Prop.MaxValue = MaxVal; \
        Prop.bIsEditAnywhere = bEditAnywhere; \
        Prop.Tooltip = "" __VA_ARGS__; \
        Class->AddProperty(Prop); \
    }

// 리소스 타입 전용 매크로
#define ADD_PROPERTY_TEXTURE(VarType, VarName, CategoryName, bEditAnywhere, ...) \
    { \
        FProperty Prop; \
        Prop.Name = #VarName; \
        Prop.Type = EPropertyType::Texture; \
        Prop.Offset = offsetof(ThisClass_t, VarName); \
        Prop.Category = CategoryName; \
        Prop.bIsEditAnywhere = bEditAnywhere; \
        Prop.Tooltip = "" __VA_ARGS__; \
        Class->AddProperty(Prop); \
    }

// 마찬가지로:
// ADD_PROPERTY_STATICMESH
// ADD_PROPERTY_SKELETALMESH
// ADD_PROPERTY_MATERIAL
// ADD_PROPERTY_AUDIO
// ADD_PROPERTY_CURVE
// ADD_PROPERTY_ARRAY
// ADD_PROPERTY_SRV
```

#### 메타데이터 마커

```cpp
// Actor를 에디터에서 생성 가능하게 마킹
#define MARK_AS_SPAWNABLE(DisplayName, Description) \
    Class->bIsSpawnable = true; \
    Class->DisplayName = DisplayName; \
    Class->Description = Description;

// 컴포넌트로 마킹
#define MARK_AS_COMPONENT(DisplayName, Description) \
    Class->bIsComponent = true; \
    Class->DisplayName = DisplayName; \
    Class->Description = Description;
```

---

### 6. LuaBindHelpers (LuaBindHelpers.h)

C++ 함수를 Lua에 바인딩하는 헬퍼 함수들입니다.

#### Lua 바인딩 매크로

```cpp
#define LUA_BIND_BEGIN(ClassType) \
static void BuildLua_##ClassType(sol::state_view L, sol::table& T); \
struct FLuaBinder_##ClassType { \
    FLuaBinder_##ClassType() { \
        FLuaBindRegistry::Get().Register( \
            ClassType::StaticClass(), \
            &BuildLua_##ClassType \
        ); \
    } \
}; \
static FLuaBinder_##ClassType G_LuaBinder_##ClassType; \
static void BuildLua_##ClassType(sol::state_view L, sol::table& T)

#define LUA_BIND_END() /* nothing */
```

#### 함수 바인딩 헬퍼

**void 반환 타입:**

```cpp
template<typename C, typename... P>
static void AddMethod(sol::table& T, const char* Name, void(C::*Method)(P...))
{
    T.set_function(Name, [Method](LuaComponentProxy& Proxy, P... Args)
    {
        if (!Proxy.Instance || Proxy.Class != C::StaticClass()) return;
        (static_cast<C*>(Proxy.Instance)->*Method)(std::forward<P>(Args)...);
    });
}
```

**반환값이 있는 타입:**

```cpp
template<typename R, typename C, typename... P>
static void AddMethodR(sol::table& T, const char* Name, R(C::*Method)(P...))
{
    T.set_function(Name, [Method](LuaComponentProxy& Proxy, P... Args) -> R
    {
        if (!Proxy.Instance || Proxy.Class != C::StaticClass())
        {
            if constexpr (!std::is_void_v<R>) return R{};
        }
        return (static_cast<C*>(Proxy.Instance)->*Method)(std::forward<P>(Args)...);
    });
}
```

**const 멤버 함수 오버로드:**

```cpp
template<typename R, typename C, typename... P>
static void AddMethodR(sol::table& T, const char* Name, R(C::*Method)(P...) const)
{
    T.set_function(Name, [Method](LuaComponentProxy& Proxy, P... Args) -> R
    {
        if (!Proxy.Instance || Proxy.Class != C::StaticClass())
        {
            if constexpr (!std::is_void_v<R>) return R{};
        }
        return (static_cast<const C*>(Proxy.Instance)->*Method)(std::forward<P>(Args)...);
    });
}
```

**별칭 설정:**

```cpp
template<typename C, typename... P>
static void AddAlias(sol::table& T, const char* Alias, void(C::*Method)(P...))
{
    AddMethod<C, P...>(T, Alias, Method);
}
```

---

### 7. 코드 생성기 (Python)

Python 스크립트가 C++ 헤더 파일을 파싱하여 .generated.h/.generated.cpp 파일을 자동 생성합니다.

#### generate.py의 역할

1. **헤더 스캔** - `GENERATED_REFLECTION_BODY()` 마커 찾기
2. **매크로 파싱** - UPROPERTY/UFUNCTION/UCLASS 추출
3. **코드 생성** - .generated.h/.generated.cpp 생성
4. **프로젝트 업데이트** - .vcxproj 파일에 자동 추가

자세한 내용은 `LuaBindingAutomation.md` 참조.

---

## 동작 흐름

### 1. Static 초기화 단계 (프로그램 시작)

```
[프로그램 시작]
    │
    ├─ Static 초기화자 실행 (각 .cpp 파일)
    │   │
    │   ├─ IMPLEMENT_CLASS 매크로 확장
    │   │   └─ ObjectFactory::RegisterClassType() 호출
    │   │       └─ 클래스 생성 함수 등록
    │   │
    │   ├─ GENERATED_REFLECTION_BODY 확장
    │   │   └─ StaticClass() 호출
    │   │       ├─ static UClass Cls 생성
    │   │       └─ UClass::SignUpClass(&Cls) 호출
    │   │
    │   ├─ BEGIN_PROPERTIES/END_PROPERTIES 확장
    │   │   └─ StaticRegisterProperties() 호출
    │   │       └─ Class->AddProperty(...) 호출 (각 프로퍼티)
    │   │
    │   └─ LUA_BIND_BEGIN/END 확장
    │       └─ FLuaBindRegistry::Register() 호출
    │           └─ Lua 바인더 함수 등록
    │
    ▼
[초기화 완료 - 모든 클래스/프로퍼티/바인더 등록됨]
```

### 2. 런타임 - 객체 생성

```
[에디터에서 Actor 생성 버튼 클릭]
    │
    ├─ UClass* Class = UClass::FindClass("AMyActor");
    │   └─ GetAllClasses()를 순회하여 이름으로 찾기
    │
    ├─ UObject* Obj = ObjectFactory::NewObject(Class);
    │   ├─ Registry에서 ConstructFunc 조회
    │   ├─ ConstructFunc() 호출 → new AMyActor()
    │   ├─ GUObjectArray에 추가
    │   └─ InternalIndex 설정
    │
    └─ return static_cast<AMyActor*>(Obj);
```

### 3. 런타임 - 프로퍼티 접근

```
[에디터 프로퍼티 패널 표시]
    │
    ├─ UClass* Class = Obj->GetClass();
    │
    ├─ for (const FProperty& Prop : Class->GetAllProperties())
    │   │
    │   ├─ if (Prop.bIsEditAnywhere)  // 편집 가능한 것만
    │   │   │
    │   │   ├─ switch (Prop.Type)
    │   │   │   ├─ case Float:
    │   │   │   │   ├─ float* Value = Prop.GetValuePtr<float>(Obj);
    │   │   │   │   └─ ImGui::DragFloat(Prop.Name, Value, ...);
    │   │   │   │
    │   │   │   ├─ case Texture:
    │   │   │   │   ├─ UTexture** Value = Prop.GetValuePtr<UTexture*>(Obj);
    │   │   │   │   └─ ImGui::ResourcePicker(Prop.Name, Value, ...);
    │   │   │   │
    │   │   │   └─ ...
    │   │   │
    │   └─ [다음 프로퍼티]
    │
    └─ [패널 표시 완료]
```

### 4. 런타임 - Lua에서 C++ 호출

```
[Lua 스크립트 실행: Obj:Jump(5.0)]
    │
    ├─ LuaComponentProxy& Proxy = ...  // C++에서 주입된 Proxy
    │   ├─ Proxy.Instance = AMyActor 인스턴스 포인터
    │   └─ Proxy.Class = AMyActor::StaticClass()
    │
    ├─ Lambda 실행 (AddMethodR에서 등록)
    │   ├─ if (Proxy.Instance && Proxy.Class == AMyActor::StaticClass())
    │   └─ (static_cast<AMyActor*>(Proxy.Instance)->*Method)(5.0f);
    │       └─ AMyActor::Jump(5.0f) 호출
    │
    └─ [C++ 함수 실행 완료]
```

### 5. 런타임 - 직렬화 (씬 저장)

```
[씬 저장]
    │
    ├─ for (UObject* Obj : GUObjectArray)
    │   │
    │   ├─ JSON ObjJson;
    │   ├─ ObjJson["UUID"] = Obj->UUID;
    │   ├─ ObjJson["ClassName"] = Obj->GetClass()->Name;
    │   │
    │   ├─ Obj->Serialize(false, ObjJson);  // Saving
    │   │   │
    │   │   └─ UClass* Class = GetClass();
    │   │       for (const FProperty& Prop : Class->GetAllProperties())
    │   │       {
    │   │           switch (Prop.Type)
    │   │           {
    │   │               case Float:
    │   │                   float* Value = Prop.GetValuePtr<float>(this);
    │   │                   ObjJson[Prop.Name] = *Value;
    │   │                   break;
    │   │               case ObjectPtr:
    │   │                   UObject** Ptr = Prop.GetValuePtr<UObject*>(this);
    │   │                   ObjJson[Prop.Name] = (*Ptr) ? (*Ptr)->UUID : 0;
    │   │                   break;
    │   │               // ...
    │   │           }
    │   │       }
    │   │
    │   └─ SceneJson.append(ObjJson);
    │
    └─ SceneJson.save("Scene.json");
```

### 6. 런타임 - 역직렬화 (씬 로드)

```
[씬 로드]
    │
    ├─ JSON SceneJson = JSON::Load("Scene.json");
    │
    ├─ for (uint32 i = 0; i < SceneJson.size(); ++i)
    │   │
    │   ├─ JSON& ObjJson = SceneJson[i];
    │   ├─ FString ClassName = ObjJson["ClassName"].ToString();
    │   ├─ UClass* Class = UClass::FindClass(ClassName);
    │   │
    │   ├─ UObject* Obj = ObjectFactory::NewObject(Class);
    │   ├─ Obj->UUID = ObjJson["UUID"].ToInt();
    │   │
    │   ├─ Obj->Serialize(true, ObjJson);  // Loading
    │   │   │
    │   │   └─ for (const FProperty& Prop : Class->GetAllProperties())
    │   │       {
    │   │           switch (Prop.Type)
    │   │           {
    │   │               case Float:
    │   │                   float* Value = Prop.GetValuePtr<float>(this);
    │   │                   *Value = ObjJson[Prop.Name].ToFloat();
    │   │                   break;
    │   │               // ...
    │   │           }
    │   │       }
    │   │
    │   └─ [다음 객체]
    │
    └─ [씬 로드 완료]
```

---

## 사용 가이드

### 단계 1: C++ 클래스 작성

```cpp
// MyActor.h
#pragma once
#include "Actor.h"
#include "MyActor.generated.h"  // 자동 생성될 파일

UCLASS(DisplayName="내 액터", Description="테스트용 액터입니다")
class AMyActor : public AActor
{
    GENERATED_REFLECTION_BODY()  // 필수!

public:
    // 프로퍼티
    UPROPERTY(Category="Movement", EditAnywhere, Tooltip="이동 속도", Range="0,100")
    float Speed = 10.0f;

    UPROPERTY(Category="Rendering", EditAnywhere)
    UTexture* MyTexture = nullptr;

    UPROPERTY(Category="Materials", EditAnywhere)
    TArray<UMaterialInterface*> Materials;

    // 함수
    UFUNCTION(LuaBind, DisplayName="점프")
    void Jump(float Height);

    UFUNCTION(LuaBind)
    float GetSpeed() const { return Speed; }
};
```

### 단계 2: 코드 생성 실행

```bash
# Visual Studio Pre-Build Event에 등록하거나 수동 실행
cd Mundi/Tools/CodeGenerator
RunCodeGen.bat --source-dir ../../Source/Runtime --output-dir ../../Build/Generated --vcxproj ../../Mundi.vcxproj
```

### 단계 3: 생성된 파일 확인

**MyActor.generated.h:**

```cpp
// Auto-generated file - DO NOT EDIT!
#pragma once

#define CURRENT_CLASS_GENERATED_BODY \
public: \
    using Super = AActor; \
    using ThisClass_t = AMyActor; \
    static UClass* StaticClass() \
    { \
        static UClass Cls{ "AMyActor", AActor::StaticClass(), sizeof(AMyActor) }; \
        static bool bRegistered = (UClass::SignUpClass(&Cls), true); \
        return &Cls; \
    } \
    virtual UClass* GetClass() const override { return AMyActor::StaticClass(); } \
    AMyActor(const AMyActor&) = default; \
    AMyActor* Duplicate() const override \
    { \
        AMyActor* NewObject = ObjectFactory::DuplicateObject<AMyActor>(this); \
        NewObject->DuplicateSubObjects(); \
        NewObject->PostDuplicate(); \
        return NewObject; \
    } \
private: \
    static void StaticRegisterProperties(); \
    static const bool bPropertiesRegistered; \
public:
```

**MyActor.generated.cpp:**

```cpp
// Auto-generated file - DO NOT EDIT!
#include "pch.h"
#include "MyActor.h"
#include "Source/Runtime/Core/Object/ObjectMacros.h"
#include "Source/Runtime/Engine/Scripting/LuaBindHelpers.h"

// ===== Class Factory Registration =====
namespace {
    struct AMyActorFactoryRegister
    {
        AMyActorFactoryRegister()
        {
            ObjectFactory::RegisterClassType(
                AMyActor::StaticClass(),
                []() -> UObject* { return new AMyActor(); }
            );
        }
    };
    static AMyActorFactoryRegister GRegister_AMyActor;
    static bool bIsRegistered_AMyActor = []() {
        AMyActor::StaticClass();
        return true;
    }();
}

const bool AMyActor::bPropertiesRegistered = []() {
    AMyActor::StaticRegisterProperties();
    return true;
}();

// ===== Property Reflection =====
BEGIN_PROPERTIES(AMyActor)
    MARK_AS_SPAWNABLE("내 액터", "테스트용 액터입니다")
    ADD_PROPERTY_RANGE(float, Speed, "Movement", 0.0f, 100.0f, true, "이동 속도")
    ADD_PROPERTY_TEXTURE(UTexture*, MyTexture, "Rendering", true)
    ADD_PROPERTY_ARRAY(EPropertyType::Material, Materials, "Materials", true)
END_PROPERTIES()

// ===== Lua Binding =====
extern "C" void LuaBind_Anchor_AMyActor() {}

LUA_BIND_BEGIN(AMyActor)
{
    AddAlias<AMyActor, float>(T, "점프", &AMyActor::Jump);
    AddMethodR<float, AMyActor>(T, "GetSpeed", &AMyActor::GetSpeed);
}
LUA_BIND_END()
```

### 단계 4: C++ 구현

```cpp
// MyActor.cpp
#include "MyActor.h"

void AMyActor::Jump(float Height)
{
    // 점프 로직
    Location.Z += Height;
}
```

### 단계 5: Lua에서 사용

```lua
-- MyActor.lua
function BeginPlay()
    print("Speed: " .. Obj:GetSpeed())  -- C++에서 바인딩된 함수
end

function Tick(dt)
    if InputManager:IsKeyPressed(' ') then
        Obj:Jump(5.0)  -- C++ 함수 호출
    end

    -- C++에서 바인딩된 프로퍼티
    Obj.Location = Obj.Location + Obj.Velocity * dt
end
```

---

## 실전 예제

### 예제 1: 에디터에서 동적 생성

```cpp
// 에디터 코드
void SpawnActorButton()
{
    // 모든 생성 가능한 Actor 목록 가져오기
    TArray<UClass*> SpawnableActors = UClass::GetAllSpawnableActors();

    for (UClass* ActorClass : SpawnableActors)
    {
        if (ImGui::Button(ActorClass->DisplayName))
        {
            // 동적으로 Actor 생성
            AActor* NewActor = ObjectFactory::NewObject<AActor>(ActorClass);
            NewActor->Location = GetMouseWorldPosition();
            NewActor->BeginPlay();
        }
    }
}
```

### 예제 2: 프로퍼티 패널 자동 생성

```cpp
void DrawPropertyPanel(UObject* Obj)
{
    if (!Obj) return;

    UClass* Class = Obj->GetClass();
    const TArray<FProperty>& Props = Class->GetAllProperties();

    // 카테고리별로 그룹화
    TMap<FString, TArray<const FProperty*>> CategorizedProps;
    for (const FProperty& Prop : Props)
    {
        if (Prop.bIsEditAnywhere)
        {
            CategorizedProps[Prop.Category].Add(&Prop);
        }
    }

    // 카테고리별로 UI 표시
    for (const auto& [Category, PropList] : CategorizedProps)
    {
        if (ImGui::CollapsingHeader(Category.c_str()))
        {
            for (const FProperty* Prop : PropList)
            {
                DrawPropertyWidget(Obj, *Prop);
            }
        }
    }
}

void DrawPropertyWidget(UObject* Obj, const FProperty& Prop)
{
    switch (Prop.Type)
    {
        case EPropertyType::Float:
        {
            float* Value = Prop.GetValuePtr<float>(Obj);
            if (Prop.MinValue != Prop.MaxValue)
                ImGui::SliderFloat(Prop.Name, Value, Prop.MinValue, Prop.MaxValue);
            else
                ImGui::DragFloat(Prop.Name, Value);
            break;
        }

        case EPropertyType::Bool:
        {
            bool* Value = Prop.GetValuePtr<bool>(Obj);
            ImGui::Checkbox(Prop.Name, Value);
            break;
        }

        case EPropertyType::Texture:
        {
            UTexture** Value = Prop.GetValuePtr<UTexture*>(Obj);
            ImGui::ResourcePicker(Prop.Name, Value, "Texture");
            break;
        }

        case EPropertyType::Array:
        {
            ImGui::Text("%s (Array)", Prop.Name);
            // TArray 편집 UI
            break;
        }

        // ... 다른 타입들
    }

    // 툴팁 표시
    if (Prop.Tooltip && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", Prop.Tooltip);
    }
}
```

### 예제 3: 자동 직렬화

```cpp
// UObject::Serialize 구현 (Object.cpp)
void UObject::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UClass* Class = GetClass();

    for (const FProperty& Prop : Class->GetAllProperties())
    {
        const char* PropName = Prop.Name;

        if (bInIsLoading)  // 로드
        {
            if (!InOutHandle.hasKey(PropName)) continue;

            switch (Prop.Type)
            {
                case EPropertyType::Float:
                {
                    float* Value = Prop.GetValuePtr<float>(this);
                    *Value = InOutHandle[PropName].ToFloat();
                    break;
                }

                case EPropertyType::Int32:
                {
                    int32* Value = Prop.GetValuePtr<int32>(this);
                    *Value = InOutHandle[PropName].ToInt();
                    break;
                }

                case EPropertyType::FVector:
                {
                    FVector* Value = Prop.GetValuePtr<FVector>(this);
                    Value->X = InOutHandle[PropName]["X"].ToFloat();
                    Value->Y = InOutHandle[PropName]["Y"].ToFloat();
                    Value->Z = InOutHandle[PropName]["Z"].ToFloat();
                    break;
                }

                case EPropertyType::FString:
                {
                    FString* Value = Prop.GetValuePtr<FString>(this);
                    *Value = InOutHandle[PropName].ToString();
                    break;
                }

                case EPropertyType::ObjectPtr:
                {
                    // UUID로 저장 (나중에 링크)
                    uint32 UUID = InOutHandle[PropName].ToInt();
                    PendingObjectReferences[PropName] = UUID;
                    break;
                }

                // ... 다른 타입들
            }
        }
        else  // 저장
        {
            switch (Prop.Type)
            {
                case EPropertyType::Float:
                {
                    float* Value = Prop.GetValuePtr<float>(this);
                    InOutHandle[PropName] = *Value;
                    break;
                }

                case EPropertyType::FVector:
                {
                    FVector* Value = Prop.GetValuePtr<FVector>(this);
                    InOutHandle[PropName]["X"] = Value->X;
                    InOutHandle[PropName]["Y"] = Value->Y;
                    InOutHandle[PropName]["Z"] = Value->Z;
                    break;
                }

                case EPropertyType::ObjectPtr:
                {
                    UObject** Ptr = Prop.GetValuePtr<UObject*>(this);
                    InOutHandle[PropName] = (*Ptr) ? (*Ptr)->UUID : 0;
                    break;
                }

                // ... 다른 타입들
            }
        }
    }
}
```

### 예제 4: 복사 생성 (Prefab)

```cpp
// AActor::DuplicateSubObjects 구현
void AActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // 컴포넌트 깊은 복사
    for (UActorComponent*& Comp : Components)
    {
        if (Comp)
        {
            Comp = Comp->Duplicate();  // 재귀적으로 복사
            Comp->Owner = this;        // Owner 재설정
        }
    }
}

// 사용 예제
AActor* Original = ...;
AActor* Copy = Original->Duplicate();  // 깊은 복사
```

### 예제 5: 타입 검사와 Cast

```cpp
void ProcessActor(UObject* Obj)
{
    // 타입 검사
    if (Obj->IsA<AActor>())
    {
        AActor* Actor = Cast<AActor>(Obj);
        if (Actor)
        {
            Actor->Tick(DeltaTime);
        }
    }

    // 더 구체적인 타입 검사
    if (Obj->IsA<APointLight>())
    {
        APointLight* Light = Cast<APointLight>(Obj);
        Light->SetIntensity(1.0f);
    }
}
```

---

## 시스템 장점

### 1. 자동화
- Python 코드 생성기가 모든 boilerplate 코드 생성
- 개발자는 매크로만 추가하면 끝

### 2. 타입 안전성
- 템플릿 기반 타입 추론
- 컴파일 타임 타입 검사

### 3. 확장성
- 새로운 프로퍼티 타입 추가 용이
- 커스텀 메타데이터 지원

### 4. 성능
- 프로퍼티 캐싱으로 조회 성능 최적화
- Static 초기화 시점에 1회만 등록

### 5. 에디터 통합
- 자동 프로퍼티 패널 생성
- 리소스 선택 UI 자동 제공

### 6. 스크립팅 지원
- Lua에서 C++ 객체 완전 제어
- 타입 안전 바인딩

---

## 정리

이 리플렉션 시스템은 Unreal Engine의 RTTI 시스템에서 영감을 받아 구현되었으며, 다음과 같은 핵심 기능을 제공합니다:

1. **UObject/UClass** - 객체와 타입의 기본 구조
2. **FProperty** - 프로퍼티 메타데이터와 런타임 접근
3. **ObjectFactory** - 동적 객체 생성과 관리
4. **ObjectMacros** - 편리한 매크로 시스템
5. **LuaBindHelpers** - C++/Lua 상호운용성
6. **코드 생성기** - Python 기반 자동화

모든 구성 요소가 유기적으로 연결되어, 개발자가 간단한 매크로만 추가하면 에디터, 직렬화, 스크립팅이 자동으로 동작하는 강력한 시스템을 제공합니다.
