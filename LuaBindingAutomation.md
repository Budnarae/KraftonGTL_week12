# Lua 바인딩 자동화 시스템

## 개요

이 프로젝트는 C++ 헤더 파일에서 **UPROPERTY**와 **UFUNCTION** 매크로를 자동으로 파싱하여 Lua 바인딩 코드를 생성하는 Python 기반 자동화 시스템입니다. Unreal Engine 스타일의 리플렉션 시스템을 구현하며, C++ 클래스를 Lua 스크립트에서 사용할 수 있도록 바인딩 코드를 자동 생성합니다.

## 시스템 아키텍처

```
C++ 헤더 파일 (.h)
    ↓
[header_parser.py] → C++ 파일 파싱
    ↓
[macro_parser.py] → ObjectMacros.h 분석
    ↓
[ClassInfo 생성] → 클래스/프로퍼티/함수 정보
    ↓
[property_generator.py] → BEGIN_PROPERTIES 블록 생성
    ↓
[lua_generator.py] → LUA_BIND_BEGIN 블록 생성
    ↓
.generated.h / .generated.cpp 파일 생성
```

## 구성 요소

### 1. generate.py (메인 진입점)

전체 코드 생성 프로세스를 조율하는 메인 스크립트입니다.

**주요 기능:**
- 소스 디렉토리 스캔
- 각 파서/제네레이터 초기화
- .generated.h 및 .generated.cpp 파일 생성
- Visual Studio 프로젝트 파일 자동 업데이트

**실행 방법:**

```bash
python generate.py --source-dir Source/Runtime --output-dir Build/Generated --vcxproj Mundi.vcxproj
```

**생성되는 파일 구조:**

```cpp
// ClassName.generated.h
#pragma once

#define CURRENT_CLASS_GENERATED_BODY \
public: \
    using Super = ParentClass; \
    using ThisClass_t = ClassName; \
    static UClass* StaticClass() { ... } \
    virtual UClass* GetClass() const override { ... } \
    ClassName* Duplicate() const override { ... }
```

```cpp
// ClassName.generated.cpp
#include "pch.h"
#include "ClassName.h"

// Class Factory Registration
namespace {
    struct ClassNameFactoryRegister { ... };
    static ClassNameFactoryRegister GRegister_ClassName;
}

// Property Reflection
BEGIN_PROPERTIES(ClassName)
    MARK_AS_SPAWNABLE("DisplayName", "Description")
    ADD_PROPERTY(float, Speed, "Movement", true, "이동 속도")
END_PROPERTIES()

// Lua Binding
LUA_BIND_BEGIN(ClassName)
{
    AddMethodR<void, ClassName>(T, "Move", &ClassName::Move);
}
LUA_BIND_END()
```

### 2. header_parser.py (C++ 헤더 파서)

C++ 헤더 파일을 파싱하여 클래스, 프로퍼티, 함수 정보를 추출합니다.

**파싱 대상:**

```cpp
// 예제 C++ 헤더 파일
UCLASS(DisplayName="플레이어", Description="플레이어 캐릭터")
class APlayer : public AActor
{
    GENERATED_REFLECTION_BODY()

    UPROPERTY(Category="이동", EditAnywhere, Tooltip="이동 속도")
    float MovementSpeed = 10.0f;

    UPROPERTY(Category="렌더링", EditAnywhere)
    UTexture* PlayerTexture = nullptr;

    UFUNCTION(LuaBind, DisplayName="점프")
    void Jump(float Height);
};
```

**파싱 결과 (ClassInfo 객체):**

```python
ClassInfo(
    name="APlayer",
    parent="AActor",
    display_name="플레이어",
    description="플레이어 캐릭터",
    properties=[
        Property(
            name="MovementSpeed",
            type="float",
            category="이동",
            editable=True,
            tooltip="이동 속도"
        ),
        Property(
            name="PlayerTexture",
            type="UTexture*",
            category="렌더링",
            editable=True
        )
    ],
    functions=[
        Function(
            name="Jump",
            display_name="점프",
            return_type="void",
            parameters=[Parameter(name="Height", type="float")],
            metadata={'lua_bind': True}
        )
    ]
)
```

**주요 기능:**
- 괄호 매칭을 통한 정확한 메타데이터 추출
- 주석 처리된 매크로 자동 제외
- GENERATED_REFLECTION_BODY() 마커 감지
- TArray<T> 같은 복잡한 타입 지원

### 3. macro_parser.py (ObjectMacros.h 파서)

`ObjectMacros.h` 파일에서 ADD_PROPERTY_* 매크로를 파싱하여 타입-매크로 매핑을 자동 생성합니다.

**파싱 예제:**

```cpp
// ObjectMacros.h에서 파싱
#define ADD_PROPERTY_TEXTURE(Type, PropName, Category, bEdit, Tooltip) \
    { \
        FProperty Prop; \
        Prop.Type = EPropertyType::Texture; \
        // ...
    }

#define ADD_PROPERTY_STATICMESH(Type, PropName, Category, bEdit, Tooltip) \
    { \
        FProperty Prop; \
        Prop.Type = EPropertyType::StaticMesh; \
        // ...
    }
```

**자동 타입 매칭:**

```python
# macro_parser가 자동으로 감지
"UTexture*" → "ADD_PROPERTY_TEXTURE"
"UStaticMesh*" → "ADD_PROPERTY_STATICMESH"
"UMaterialInterface*" → "ADD_PROPERTY_MATERIAL"
"USoundBase*" → "ADD_PROPERTY_AUDIO"
"TArray<UTexture*>" → "ADD_PROPERTY_ARRAY" (inner_type=EPropertyType::Texture)
```

**매칭 알고리즘:**

```python
class MacroInfo:
    def get_cpp_type_patterns(self) -> List[str]:
        """
        EPropertyType::Sound → ['sound', 'usound', 'usoundbase', 'soundbase']
        EPropertyType::Texture → ['texture', 'utexture', 'utexturebase', 'texturebase']
        """
        enum_value = self.property_type.split('::')[-1].lower()
        patterns = [
            enum_value,                    # 'sound'
            'u' + enum_value,              # 'usound'
            'u' + enum_value + 'base',     # 'usoundbase'
            enum_value + 'base'            # 'soundbase'
        ]
        return patterns
```

### 4. property_generator.py (프로퍼티 코드 생성기)

리플렉션 시스템을 위한 BEGIN_PROPERTIES 블록을 생성합니다.

**생성 예제:**

```cpp
BEGIN_PROPERTIES(APlayer)
    MARK_AS_SPAWNABLE("플레이어", "플레이어 캐릭터")
    ADD_PROPERTY(float, MovementSpeed, "이동", true, "이동 속도")
    ADD_PROPERTY_RANGE(float, JumpHeight, "이동", 0.0f, 10.0f, true, "점프 높이")
    ADD_PROPERTY_TEXTURE(UTexture*, PlayerTexture, "렌더링", true)
    ADD_PROPERTY_ARRAY(EPropertyType::Material, Materials, "렌더링", true)
END_PROPERTIES()
```

**템플릿 엔진 (Jinja2):**

```python
PROPERTY_TEMPLATE = """
BEGIN_PROPERTIES({{ class_name }})
{%- if mark_type == 'SPAWNABLE' %}
    MARK_AS_SPAWNABLE("{{ display_name }}", "{{ description }}")
{%- elif mark_type == 'COMPONENT' %}
    MARK_AS_COMPONENT("{{ display_name }}", "{{ description }}")
{%- endif %}
{%- for prop in properties %}
    {{ prop.get_property_type_macro() }}({{ prop.type }}, {{ prop.name }}, "{{ prop.category }}", {{ 'true' if prop.editable else 'false' }})
{%- endfor %}
END_PROPERTIES()
"""
```

### 5. lua_generator.py (Lua 바인딩 생성기)

C++ 함수를 Lua에서 호출 가능하도록 바인딩 코드를 생성합니다.

**생성 예제:**

```cpp
extern "C" void LuaBind_Anchor_APlayer() {}

LUA_BIND_BEGIN(APlayer)
{
    // void 반환 타입
    AddAlias<APlayer>(T, "Jump", &APlayer::Jump);
    AddAlias<APlayer, float>(T, "SetSpeed", &APlayer::SetSpeed);

    // 반환 값이 있는 함수
    AddMethodR<float, APlayer>(T, "GetSpeed", &APlayer::GetSpeed);
    AddMethodR<Vector, APlayer>(T, "GetLocation", &APlayer::GetLocation);
}
LUA_BIND_END()
```

**템플릿 로직:**

```python
LUA_BIND_TEMPLATE = """
LUA_BIND_BEGIN({{ class_name }})
{
{%- for func in functions %}
    {%- if func.return_type == 'void' %}
    AddAlias<{{ class_name }}{{ func.get_parameter_types_string() }}>(
        T, "{{ func.display_name }}", &{{ class_name }}::{{ func.name }});
    {%- else %}
    AddMethodR<{{ func.return_type }}, {{ class_name }}{{ func.get_parameter_types_string() }}>(
        T, "{{ func.display_name }}", &{{ class_name }}::{{ func.name }});
    {%- endif %}
{%- endfor %}
}
LUA_BIND_END()
"""
```

### 6. vcxproj_updater.py (Visual Studio 프로젝트 업데이터)

생성된 .generated.cpp 파일을 자동으로 Visual Studio 프로젝트에 추가합니다.

### 7. RunCodeGen.bat (실행 스크립트)

Visual Studio Pre-Build Event에서 호출되는 배치 스크립트입니다.

```batch
@echo off
REM 임베디드 Python 사용
if exist "%~dp0..\Python\python.exe" (
    "%~dp0..\Python\python.exe" "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

REM 시스템 Python 사용
py --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    py "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

echo [ERROR] Python not found!
exit /b 1
```

## Lua 스크립트 예제

### template.lua (기본 템플릿)

```lua
function BeginPlay()
    print("[BeginPlay] " .. Obj.UUID)
end

function EndPlay()
    print("[EndPlay] " .. Obj.UUID)
end

function OnBeginOverlap(OtherActor)
    -- 충돌 처리
end

function OnEndOverlap(OtherActor)
    -- 충돌 종료 처리
end

function Tick(dt)
    -- C++에서 바인딩된 프로퍼티 접근
    Obj.Location = Obj.Location + Obj.Velocity * dt
end
```

### Typings.lua (타입 힌트)

Lua LSP를 위한 타입 힌트를 제공합니다:

```lua
---@class Vector
---@field X number
---@field Y number
---@field Z number

---@class GameObject
---@field UUID string
---@field Location Vector
---@field Velocity Vector
---@field PrintLocation fun(self: GameObject)

---@type GameObject
Obj = Obj  -- C++에서 주입되지만, LSP에 타입 힌트 제공
```

### Player.lua (실제 사용 예제)

```lua
-- 전역 변수
local MovementDelta = 10
local JumpSpeed = 6

-- C++에서 바인딩된 함수 호출
function BeginPlay()
    local Camera = GetCamera()
    if Camera then
        Camera:SetCameraForward(ForwardVector)
    end
end

-- C++에서 바인딩된 프로퍼티 접근
function Tick(Delta)
    if InputManager:IsKeyDown('W') then
        MoveForward(MovementDelta * Delta)
    end

    if InputManager:IsKeyPressed(' ') then
        Jump()
    end

    -- 위치 업데이트 (C++ 바인딩)
    Obj.Location = Obj.Location + Obj.Velocity * Delta
end

-- Prefab 생성 (C++ 바인딩)
function ShootProjectile()
    local projectile = SpawnPrefab("Data/Prefabs/Apple.prefab")
    projectile.Location = Obj.Location + ForwardOffset
    projectile.Velocity = ForwardVector * 30.0
end
```

## Python 내부 동작 원리

이 섹션에서는 Python 코드 생성기가 내부적으로 어떻게 동작하는지 상세히 설명합니다.

### 1. header_parser.py - 정규표현식과 괄호 매칭

#### 1.1 UPROPERTY 파싱 알고리즘

**문제:** C++ 매크로는 중첩된 괄호를 가질 수 있습니다.

```cpp
UPROPERTY(Category="Test", Meta=(Min=0, Max=100, DisplayName="복잡한(괄호)"))
float ComplexProperty;
```

**해결책:** 괄호 깊이를 추적하여 매칭합니다.

```python
@staticmethod
def _extract_balanced_parens(text: str, start_pos: int) -> Tuple[str, int]:
    """
    괄호 매칭하여 내용 추출

    Args:
        text: 전체 텍스트
        start_pos: '(' 다음 위치

    Returns:
        (괄호 안의 내용, 닫는 괄호 위치)
    """
    depth = 1  # 시작 괄호를 이미 발견했으므로 1부터 시작
    i = start_pos

    while i < len(text) and depth > 0:
        if text[i] == '(':
            depth += 1  # 여는 괄호 발견 - 깊이 증가
        elif text[i] == ')':
            depth -= 1  # 닫는 괄호 발견 - 깊이 감소
        i += 1

    # depth가 0이 되면 매칭 완료
    return text[start_pos:i-1], i
```

**실행 예제:**

```python
text = "UPROPERTY(Category=\"Test\", Meta=(Min=0, Max=100)) float X;"
pos = 10  # '(' 다음 위치

# 실행:
# i=10: 'C', depth=1
# i=11: 'a', depth=1
# ...
# i=30: '(', depth=2  ← Meta의 여는 괄호
# i=31: 'M', depth=2
# ...
# i=47: ')', depth=1  ← Meta의 닫는 괄호
# i=48: ')', depth=0  ← UPROPERTY의 닫는 괄호 (종료)

content, end = _extract_balanced_parens(text, pos)
# content = 'Category="Test", Meta=(Min=0, Max=100)'
# end = 49
```

#### 1.2 주석 제거 알고리즘

주석 처리된 UPROPERTY는 파싱에서 제외해야 합니다.

```python
@staticmethod
def _remove_comments(content: str) -> str:
    """C++ 주석 제거"""
    # 1. 여러 줄 주석 /* */ 제거
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)

    # 2. 한 줄 주석 // 제거
    content = re.sub(r'//.*?$', '', content, flags=re.MULTILINE)

    return content
```

**적용:**

```python
def parse_header(self, header_path: Path) -> Optional[ClassInfo]:
    content = header_path.read_text(encoding='utf-8')

    # 주석 제거
    content_no_comments = self._remove_comments(content)

    # GENERATED_REFLECTION_BODY() 체크 (주석 제거된 버전)
    if not self.GENERATED_REFLECTION_PATTERN.search(content_no_comments):
        return None

    # 파싱 진행...
```

#### 1.3 UPROPERTY 선언 파싱 전체 흐름

```python
@staticmethod
def _parse_uproperty_declarations(content: str):
    """UPROPERTY 선언 찾기"""
    results = []
    pos = 0

    # 정규표현식으로 'UPROPERTY(' 찾기
    UPROPERTY_START = re.compile(r'UPROPERTY\s*\(')

    while True:
        match = UPROPERTY_START.search(content, pos)
        if not match:
            break  # 더 이상 UPROPERTY 없음

        # 주석 체크
        line_start = content.rfind('\n', 0, match.start()) + 1
        line_before = content[line_start:match.start()]

        # // 주석이면 건너뛰기
        if '//' in line_before:
            pos = match.end()
            continue

        # /* */ 주석 체크
        last_comment_start = content.rfind('/*', 0, match.start())
        last_comment_end = content.rfind('*/', 0, match.start())
        if last_comment_start > last_comment_end:
            pos = match.end()
            continue

        # 괄호 안 메타데이터 추출
        metadata_start = match.end()
        metadata, metadata_end = HeaderParser._extract_balanced_parens(
            content, metadata_start
        )

        # 메타데이터 다음에 타입과 변수명 찾기
        remaining = content[metadata_end:metadata_end+200]

        # 정규표현식으로 타입과 변수명 추출
        # 형식: Type VarName = ...; 또는 Type VarName;
        var_match = re.match(
            r'\s*([\w<>*:,\s]+?)\s+(\w+)\s*(?:[;=]|{[^}]*};?)',
            remaining
        )

        if var_match:
            var_type = var_match.group(1).strip()  # "float" 또는 "UTexture*"
            var_name = var_match.group(2)           # "Speed" 또는 "MyTexture"
            results.append((metadata, var_type, var_name))

        pos = metadata_end

    return results
```

**실행 예제:**

```python
# 입력 C++ 코드:
"""
UPROPERTY(Category="Movement", EditAnywhere)
float Speed = 10.0f;

UPROPERTY(Category="Rendering")
UTexture* MyTexture = nullptr;
"""

# 결과:
[
    ('Category="Movement", EditAnywhere', 'float', 'Speed'),
    ('Category="Rendering"', 'UTexture*', 'MyTexture')
]
```

#### 1.4 메타데이터 파싱

메타데이터 문자열을 딕셔너리로 변환합니다.

```python
def _parse_metadata(self, metadata: str) -> Dict[str, str]:
    """Key="Value" 패턴을 찾아서 딕셔너리로 변환"""
    result = {}

    # 정규표현식: (\w+)\s*=\s*"([^"]+)"
    # (\w+): 키 (단어)
    # \s*=\s*: 등호 (공백 허용)
    # "([^"]+)": 큰따옴표로 감싼 값

    for match in re.finditer(r'(\w+)\s*=\s*"([^"]+)"', metadata):
        key = match.group(1)    # "Category", "Tooltip", ...
        value = match.group(2)  # "Movement", "이동 속도", ...
        result[key] = value

    return result
```

**실행 예제:**

```python
metadata = 'Category="Movement", EditAnywhere, Tooltip="이동 속도", Range="0,100"'
result = _parse_metadata(metadata)

# 결과:
{
    'Category': 'Movement',
    'Tooltip': '이동 속도',
    'Range': '0,100'
}

# EditAnywhere는 Key="Value" 형식이 아니므로 별도 체크
editable = 'EditAnywhere' in metadata  # True
```

---

### 2. macro_parser.py - ObjectMacros.h 파싱

#### 2.1 전처리: 백슬래시-줄바꿈 제거

C++ 매크로는 `\`로 여러 줄에 걸쳐 작성됩니다.

```cpp
#define ADD_PROPERTY_TEXTURE(Type, PropName, Category, bEdit, Tooltip) \
    { \
        FProperty Prop; \
        Prop.Type = EPropertyType::Texture; \
        Prop.Name = #PropName; \
    }
```

**전처리:**

```python
def parse(self) -> Dict[str, MacroInfo]:
    content = self.macros_header_path.read_text(encoding='utf-8')

    # 백슬래시-줄바꿈을 공백으로 치환
    # "\ \n" 또는 "\\\n"을 ' '로 변환
    content = re.sub(r'\\\s*\n', ' ', content)

    # 이제 매크로가 한 줄로 변환됨:
    # #define ADD_PROPERTY_TEXTURE(...) { FProperty Prop; Prop.Type = EPropertyType::Texture; ... }
```

#### 2.2 매크로 정규표현식 파싱

```python
# 정규표현식 패턴
MACRO_PATTERN = re.compile(
    r'#define\s+(ADD_PROPERTY_\w+)\s*\([^)]*\)\s*\{[^}]*?Prop\.Type\s*=\s*(EPropertyType::\w+);',
    re.DOTALL
)

# 설명:
# #define\s+                      : #define 키워드
# (ADD_PROPERTY_\w+)              : 매크로 이름 (캡처 그룹 1)
# \s*\([^)]*\)                    : 매개변수 괄호
# \s*\{[^}]*?                     : 중괄호 시작
# Prop\.Type\s*=\s*               : Prop.Type =
# (EPropertyType::\w+);           : EPropertyType::XXX (캡처 그룹 2)
```

**매칭 실행:**

```python
for match in self.MACRO_PATTERN.finditer(content):
    macro_name = match.group(1)    # "ADD_PROPERTY_TEXTURE"
    property_type = match.group(2) # "EPropertyType::Texture"

    # 타입 suffix 추출
    type_suffix = macro_name[len('ADD_PROPERTY_'):]  # "TEXTURE"

    macro_info = MacroInfo(
        macro_name=macro_name,
        property_type=property_type,
        type_suffix=type_suffix
    )

    self.macros[macro_name] = macro_info
```

#### 2.3 C++ 타입 패턴 자동 생성

EPropertyType enum 값으로부터 매칭 패턴을 자동 생성합니다.

```python
def get_cpp_type_patterns(self) -> List[str]:
    """
    EPropertyType::Sound → ['sound', 'usound', 'usoundbase', 'soundbase']
    """
    # EPropertyType::XXX에서 XXX 추출
    if '::' in self.property_type:
        enum_value = self.property_type.split('::')[-1].lower()
    else:
        enum_value = self.type_suffix.lower()

    # 특수 케이스
    if enum_value in ['array', 'script', 'scriptfile']:
        return []

    if enum_value == 'srv':
        return ['srv', 'shaderresourceview']

    # 일반 패턴 생성
    patterns = [
        enum_value,                 # 'sound'
        'u' + enum_value,           # 'usound'
        'u' + enum_value + 'base',  # 'usoundbase'
        enum_value + 'base'         # 'soundbase'
    ]

    return patterns
```

#### 2.4 C++ 타입 매칭

```python
def get_macro_for_type(self, cpp_type: str) -> Optional[str]:
    """
    C++ 타입 → 매크로 이름

    예:
        "USoundBase*" → "ADD_PROPERTY_AUDIO"
        "UTexture*" → "ADD_PROPERTY_TEXTURE"
    """
    type_lower = cpp_type.lower()

    # 포인터 체크
    if '*' not in cpp_type:
        return None

    # 각 매크로의 패턴과 매칭 시도
    for macro_name, macro_info in self.macros.items():
        patterns = macro_info.get_cpp_type_patterns()

        for pattern in patterns:
            if pattern and pattern in type_lower:
                return macro_name  # 매칭 성공!

    return None
```

**실행 예제:**

```python
# ADD_PROPERTY_AUDIO → EPropertyType::Sound
# patterns = ['sound', 'usound', 'usoundbase', 'soundbase']

cpp_type = "USoundBase*"
type_lower = "usoundbase*"

# 'usoundbase'가 'usoundbase*'에 포함됨
# → return "ADD_PROPERTY_AUDIO"
```

---

### 3. property_generator.py - Jinja2 템플릿 렌더링

#### 3.1 Jinja2 템플릿 문법

```python
PROPERTY_TEMPLATE = """
BEGIN_PROPERTIES({{ class_name }})
{%- if mark_type == 'SPAWNABLE' %}
    MARK_AS_SPAWNABLE("{{ display_name }}", "{{ description }}")
{%- elif mark_type == 'COMPONENT' %}
    MARK_AS_COMPONENT("{{ display_name }}", "{{ description }}")
{%- endif %}
{%- for prop in properties %}
    {%- if prop.get_property_type_macro() == 'ADD_PROPERTY_RANGE' %}
    ADD_PROPERTY_RANGE({{ prop.type }}, {{ prop.name }}, "{{ prop.category }}", {{ prop.min_value }}f, {{ prop.max_value }}f, {{ 'true' if prop.editable else 'false' }})
    {%- else %}
    {{ prop.get_property_type_macro() }}({{ prop.type }}, {{ prop.name }}, "{{ prop.category }}", {{ 'true' if prop.editable else 'false' }})
    {%- endif %}
{%- endfor %}
END_PROPERTIES()
"""
```

**문법 설명:**
- `{{ variable }}`: 변수 출력
- `{% if condition %}`: 조건문
- `{%- for item in list %}`: 반복문 (`-`는 공백 제거)
- `{{ 'true' if condition else 'false' }}`: 삼항 연산자

#### 3.2 템플릿 렌더링 과정

```python
from jinja2 import Template

class PropertyGenerator:
    def __init__(self):
        self.template = Template(PROPERTY_TEMPLATE)

    def generate(self, class_info: ClassInfo) -> str:
        # mark_type 결정
        if class_info.name == 'AActor':
            mark_type = None
        elif class_info.parent == 'AActor':
            mark_type = 'SPAWNABLE'
        else:
            mark_type = 'COMPONENT'

        # 템플릿 렌더링
        return self.template.render(
            class_name=class_info.name,
            mark_type=mark_type,
            display_name=class_info.display_name or class_info.name,
            description=class_info.description or f"Auto-generated {class_info.name}",
            properties=class_info.properties
        )
```

**실행 예제:**

```python
# 입력
class_info = ClassInfo(
    name="APlayer",
    parent="AActor",
    display_name="플레이어",
    description="플레이어 액터",
    properties=[
        Property(name="Speed", type="float", category="Movement",
                 editable=True, has_range=True, min_value=0.0, max_value=100.0),
        Property(name="MyTexture", type="UTexture*", category="Rendering",
                 editable=True)
    ]
)

# 렌더링
result = generator.generate(class_info)

# 출력
"""
BEGIN_PROPERTIES(APlayer)
    MARK_AS_SPAWNABLE("플레이어", "플레이어 액터")
    ADD_PROPERTY_RANGE(float, Speed, "Movement", 0.0f, 100.0f, true)
    ADD_PROPERTY_TEXTURE(UTexture*, MyTexture, "Rendering", true)
END_PROPERTIES()
"""
```

---

### 4. lua_generator.py - 함수 시그니처 분석

#### 4.1 파라미터 타입 문자열 생성

```python
class Function:
    def get_parameter_types_string(self) -> str:
        """
        템플릿 파라미터 문자열 생성

        예:
            [] → ""
            [Parameter("Height", "float")] → ", float"
            [Parameter("X", "int"), Parameter("Y", "int")] → ", int, int"
        """
        if not self.parameters:
            return ""

        param_types = ", ".join([p.type for p in self.parameters])
        return f", {param_types}"
```

**실행 예제:**

```python
# 함수 1: void Jump()
func1 = Function(name="Jump", return_type="void", parameters=[])
func1.get_parameter_types_string()  # ""

# 함수 2: void Jump(float Height)
func2 = Function(
    name="Jump",
    return_type="void",
    parameters=[Parameter(name="Height", type="float")]
)
func2.get_parameter_types_string()  # ", float"

# 함수 3: void Move(float X, float Y)
func3 = Function(
    name="Move",
    return_type="void",
    parameters=[
        Parameter(name="X", type="float"),
        Parameter(name="Y", type="float")
    ]
)
func3.get_parameter_types_string()  # ", float, float"
```

#### 4.2 Lua 바인딩 템플릿 렌더링

```python
LUA_BIND_TEMPLATE = """
extern "C" void LuaBind_Anchor_{{ class_name }}() {}

LUA_BIND_BEGIN({{ class_name }})
{
{%- for func in functions %}
    {%- if func.return_type == 'void' %}
    AddAlias<{{ class_name }}{{ func.get_parameter_types_string() }}>(
        T, "{{ func.display_name }}", &{{ class_name }}::{{ func.name }});
    {%- else %}
    AddMethodR<{{ func.return_type }}, {{ class_name }}{{ func.get_parameter_types_string() }}>(
        T, "{{ func.display_name }}", &{{ class_name }}::{{ func.name }});
    {%- endif %}
{%- endfor %}
}
LUA_BIND_END()
"""
```

**실행 예제:**

```python
# 입력
functions = [
    Function(name="Jump", display_name="점프", return_type="void",
             parameters=[Parameter("Height", "float")],
             metadata={'lua_bind': True}),
    Function(name="GetSpeed", display_name="GetSpeed", return_type="float",
             parameters=[],
             metadata={'lua_bind': True})
]

# 렌더링
result = lua_gen.generate(ClassInfo(name="APlayer", functions=functions))

# 출력
"""
extern "C" void LuaBind_Anchor_APlayer() {}

LUA_BIND_BEGIN(APlayer)
{
    AddAlias<APlayer, float>(T, "점프", &APlayer::Jump);
    AddMethodR<float, APlayer>(T, "GetSpeed", &APlayer::GetSpeed);
}
LUA_BIND_END()
"""
```

---

### 5. generate.py - 메인 루프

#### 5.1 전체 실행 흐름

```python
def main():
    # 1. 인자 파싱
    args = parser.parse_args()

    # 2. 파서/제네레이터 초기화
    header_parser = HeaderParser()
    prop_gen = PropertyGenerator()
    lua_gen = LuaBindingGenerator()

    # 3. 헤더 파일 스캔
    classes = header_parser.find_reflection_classes(args.source_dir)

    # 4. 각 클래스에 대해 코드 생성
    for class_info in classes:
        # .generated.h 생성
        header_code = generate_header_file(class_info)
        header_output = args.output_dir / f"{class_info.name}.generated.h"
        write_file_if_different(header_output, header_code)

        # .generated.cpp 생성
        cpp_code = generate_cpp_file(class_info, prop_gen, lua_gen)
        cpp_output = args.output_dir / f"{class_info.name}.generated.cpp"
        write_file_if_different(cpp_output, cpp_code)

    # 5. vcxproj 업데이트 (선택)
    if args.vcxproj:
        update_vcxproj(args.vcxproj, generated_files)
```

#### 5.2 파일 스캔 알고리즘

```python
def find_reflection_classes(self, source_dir: Path) -> List[ClassInfo]:
    """
    소스 디렉토리를 재귀적으로 탐색하여
    GENERATED_REFLECTION_BODY가 있는 모든 클래스 찾기
    """
    classes = []

    # rglob: 재귀적 glob (모든 하위 디렉토리 검색)
    for header in source_dir.rglob("*.h"):
        try:
            class_info = self.parse_header(header)
            if class_info:
                classes.append(class_info)
                print(f"[OK] Found: {class_info.name} in {header.name}")
        except Exception as e:
            print(f"[ERROR] {header}: {e}")

    return classes
```

**실행 예제:**

```
Source/Runtime/
├── Core/
│   └── Object/
│       ├── Object.h           ✓ GENERATED_REFLECTION_BODY 있음
│       └── Property.h         ✗ 없음
├── Engine/
│   ├── Actor/
│   │   ├── Actor.h            ✓ 있음
│   │   └── Pawn.h             ✓ 있음
│   └── Component/
│       └── ActorComponent.h   ✓ 있음

# 결과:
[OK] Found: UObject in Object.h
[OK] Found: AActor in Actor.h
[OK] Found: APawn in Pawn.h
[OK] Found: UActorComponent in ActorComponent.h
```

#### 5.3 증분 빌드 최적화

```python
def write_file_if_different(file_path: Path, new_content: str) -> bool:
    """
    파일 내용이 실제로 다를 때만 파일 작성
    → 타임스탬프 유지로 불필요한 재컴파일 방지
    """
    if file_path.exists():
        existing_content = file_path.read_text(encoding='utf-8')

        if existing_content == new_content:
            # 내용이 같으면 파일을 건드리지 않음
            # → 타임스탬프 변경 없음
            # → Visual Studio가 재컴파일하지 않음
            return False

    # 내용이 다르거나 파일이 없으면 쓰기
    file_path.write_text(new_content, encoding='utf-8')
    return True
```

**효과:**

```
첫 빌드:
  Updated: AActor.generated.cpp
  Updated: APlayer.generated.cpp
  Updated: UActorComponent.generated.cpp
  → 3개 파일 컴파일

두 번째 빌드 (변경 없음):
  Skipped: AActor.generated.cpp (no changes)
  Skipped: APlayer.generated.cpp (no changes)
  Skipped: UActorComponent.generated.cpp (no changes)
  → 0개 파일 컴파일 (빠름!)

세 번째 빌드 (APlayer.h만 수정):
  Skipped: AActor.generated.cpp (no changes)
  Updated: APlayer.generated.cpp
  Skipped: UActorComponent.generated.cpp (no changes)
  → 1개 파일만 컴파일
```

---

### 6. 전체 실행 시퀀스 다이어그램

```
[generate.py 실행]
    │
    ├─ HeaderParser 초기화
    │   └─ MacroParser 생성 및 ObjectMacros.h 파싱
    │       ├─ 백슬래시-줄바꿈 제거
    │       ├─ 정규표현식으로 ADD_PROPERTY_* 찾기
    │       └─ 타입 패턴 자동 생성
    │
    ├─ PropertyGenerator 초기화
    │   └─ Jinja2 템플릿 로드
    │
    ├─ LuaBindingGenerator 초기화
    │   └─ Jinja2 템플릿 로드
    │
    ├─ 소스 디렉토리 스캔 (rglob *.h)
    │   │
    │   ├─ 각 헤더 파일에 대해:
    │   │   │
    │   │   ├─ 주석 제거
    │   │   ├─ GENERATED_REFLECTION_BODY() 찾기
    │   │   │   └─ 없으면 건너뛰기
    │   │   │
    │   │   ├─ 클래스 이름/부모 추출 (정규표현식)
    │   │   │
    │   │   ├─ UCLASS 메타데이터 추출
    │   │   │   ├─ 'UCLASS(' 찾기
    │   │   │   ├─ 괄호 매칭으로 메타데이터 추출
    │   │   │   └─ DisplayName, Description 파싱
    │   │   │
    │   │   ├─ UPROPERTY 파싱
    │   │   │   ├─ 'UPROPERTY(' 찾기
    │   │   │   ├─ 주석 체크 (건너뛰기)
    │   │   │   ├─ 괄호 매칭으로 메타데이터 추출
    │   │   │   ├─ 정규표현식으로 타입/변수명 추출
    │   │   │   ├─ Category, Range, Tooltip 파싱
    │   │   │   └─ Property 객체 생성
    │   │   │
    │   │   ├─ UFUNCTION 파싱
    │   │   │   ├─ 'UFUNCTION(' 찾기
    │   │   │   ├─ 정규표현식으로 반환타입/함수명/파라미터 추출
    │   │   │   ├─ LuaBind 메타데이터 체크
    │   │   │   └─ Function 객체 생성
    │   │   │
    │   │   └─ ClassInfo 객체 생성
    │   │
    │   └─ 모든 ClassInfo를 리스트로 반환
    │
    ├─ 각 ClassInfo에 대해:
    │   │
    │   ├─ .generated.h 생성
    │   │   ├─ GENERATED_HEADER_TEMPLATE 포맷
    │   │   └─ write_file_if_different() 호출
    │   │
    │   ├─ .generated.cpp 생성
    │   │   ├─ IMPLEMENT_CLASS 코드 생성
    │   │   ├─ PropertyGenerator.generate() 호출
    │   │   │   ├─ mark_type 결정
    │   │   │   ├─ 각 Property에 대해 get_property_type_macro() 호출
    │   │   │   │   ├─ TArray 체크 → ADD_PROPERTY_ARRAY
    │   │   │   │   ├─ Range 체크 → ADD_PROPERTY_RANGE
    │   │   │   │   ├─ 포인터 체크 → MacroParser로 매크로 찾기
    │   │   │   │   └─ 기본 → ADD_PROPERTY
    │   │   │   └─ Jinja2 템플릿 렌더링
    │   │   ├─ LuaBindingGenerator.generate() 호출
    │   │   │   ├─ LuaBind=true인 함수만 필터링
    │   │   │   ├─ 각 함수에 대해 파라미터 문자열 생성
    │   │   │   └─ Jinja2 템플릿 렌더링
    │   │   ├─ GENERATED_CPP_TEMPLATE 포맷
    │   │   └─ write_file_if_different() 호출
    │   │
    │   └─ 상태 출력 (Updated/Skipped)
    │
    ├─ vcxproj 업데이트 (선택)
    │   └─ vcxproj_updater.py 실행
    │
    └─ 완료 통계 출력
```

---

## 동작 원리

### 1. 빌드 시점

```
[Visual Studio Pre-Build Event]
    ↓
RunCodeGen.bat 실행
    ↓
generate.py 실행
    ↓
Source/Runtime 디렉토리 스캔
    ↓
GENERATED_REFLECTION_BODY() 마커 감지
    ↓
헤더 파일 파싱
    ↓
.generated.h/.generated.cpp 생성
    ↓
vcxproj 파일 업데이트
    ↓
[C++ 컴파일]
```

### 2. 런타임

```
[C++ 프로그램 시작]
    ↓
Static Initializer 실행
    ↓
ObjectFactory에 클래스 등록
    ↓
리플렉션 시스템 초기화
    ↓
Lua 바인딩 등록 (LUA_BIND_BEGIN 실행)
    ↓
[Lua 스크립트에서 C++ 객체 사용 가능]
```

### 3. Lua에서 C++ 호출

```lua
-- Lua 스크립트
Obj.Location = Vector(10, 20, 30)  -- C++ 프로퍼티 설정
Obj:Jump(5.0)                       -- C++ 함수 호출
```

```cpp
// C++에서 바인딩된 코드
LUA_BIND_BEGIN(APlayer)
{
    // Location 프로퍼티 바인딩 (리플렉션 시스템)
    // Jump 함수 바인딩
    AddAlias<APlayer, float>(T, "Jump", &APlayer::Jump);
}
LUA_BIND_END()
```

## 사용 방법

### 1. C++ 클래스 작성

```cpp
// Player.h
#pragma once
#include "Actor.h"
#include "Player.generated.h"  // 자동 생성될 파일

UCLASS(DisplayName="플레이어", Description="플레이어 액터")
class APlayer : public AActor
{
    GENERATED_REFLECTION_BODY()  // 이 매크로가 있어야 파싱됨

public:
    UPROPERTY(Category="이동", EditAnywhere, Tooltip="이동 속도", Range="0,100")
    float Speed = 10.0f;

    UPROPERTY(Category="렌더링", EditAnywhere)
    UTexture* CharacterTexture = nullptr;

    UFUNCTION(LuaBind, DisplayName="점프")
    void Jump(float Height);

    UFUNCTION(LuaBind)
    float GetSpeed() const;
};
```

### 2. 코드 생성 실행

**방법 1: Visual Studio에서 자동 실행**

- Pre-Build Event에 등록되어 있으면 빌드 시 자동 실행

**방법 2: 수동 실행**

```bash
cd Mundi/Tools/CodeGenerator
RunCodeGen.bat --source-dir ../../Source/Runtime --output-dir ../../Build/Generated --vcxproj ../../Mundi.vcxproj
```

### 3. 생성된 파일 확인

```
Build/Generated/
    ├── APlayer.generated.h    # 매크로 정의
    └── APlayer.generated.cpp  # 리플렉션 + Lua 바인딩
```

### 4. Lua 스크립트 작성

```lua
-- Player.lua
function BeginPlay()
    print("Player Speed: " .. Obj.Speed)  -- C++에서 바인딩된 프로퍼티
end

function Tick(dt)
    if InputManager:IsKeyPressed(' ') then
        Obj:Jump(5.0)  -- C++에서 바인딩된 함수
    end
end
```

## 최적화 기능

### 1. 증분 빌드 지원

변경되지 않은 파일은 다시 생성하지 않습니다:

```python
def write_file_if_different(file_path: Path, new_content: str) -> bool:
    """
    파일 내용이 실제로 다를 때만 파일을 씁니다.
    타임스탬프를 유지하여 불필요한 재컴파일을 방지합니다.
    """
    if file_path.exists():
        existing_content = file_path.read_text(encoding='utf-8')
        if existing_content == new_content:
            return False  # 변경 없음 - 파일 건드리지 않음

    file_path.write_text(new_content, encoding='utf-8')
    return True
```

### 2. 출력 예제

```
============================================================
 Mundi Engine - Code Generator
============================================================
 Source: C:\Project\Source\Runtime
 Output: C:\Project\Build\Generated

 Scanning for reflection classes...

 Found 15 reflection class(es)

  Updated: AActor
  ├─ AActor.generated.h (modified)
  ├─ AActor.generated.cpp (modified)
  ├─ Properties: 3
  └─ Functions:  2

  Skipped (no changes): APlayer
  ├─ Properties: 5
  └─ Functions:  8

============================================================
 Code generation complete!
   Total files: 30
   Updated: 4
   Skipped: 26 (unchanged)
============================================================
```

## 의존성

```txt
# requirements.txt
jinja2>=3.1.0
```

## 특징

### 1. 자동 타입 감지

- 헤더 파일에서 ObjectMacros.h 파싱
- ADD_PROPERTY_* 매크로 자동 매핑
- 새로운 타입 추가 시 재컴파일 없이 자동 인식

### 2. 유연한 메타데이터

```cpp
UPROPERTY(
    Category="이동",           // 에디터 카테고리
    EditAnywhere,              // 편집 가능 여부
    Tooltip="이동 속도입니다", // 툴팁
    Range="0,100"              // 범위 제한
)
float Speed;
```

### 3. 복잡한 타입 지원

```cpp
UPROPERTY(Category="Materials")
TArray<UMaterialInterface*> Materials;  // 자동으로 ADD_PROPERTY_ARRAY 생성
```

### 4. 주석 처리 감지

```cpp
// UPROPERTY()  // 주석 처리됨 - 파싱 제외
// float OldProperty;

/*
UPROPERTY()  // 여러 줄 주석도 처리
float DeprecatedProperty;
*/
```

### 5. 괄호 매칭

```cpp
UPROPERTY(Category="Test", Meta=(Min=0, Max=100, DisplayName="복잡한(괄호)"))
float ComplexMetadata;  // 올바르게 파싱
```

## 확장 가능성

### 새로운 프로퍼티 타입 추가

1. `ObjectMacros.h`에 매크로 추가:

```cpp
#define ADD_PROPERTY_ANIMATION(Type, PropName, Category, bEdit, Tooltip) \
    { \
        FProperty Prop; \
        Prop.Type = EPropertyType::Animation; \
        // ...
    }
```

2. 코드 생성기가 자동으로 인식하여 사용

```cpp
UPROPERTY(Category="Animation")
UAnimSequence* IdleAnimation;  // 자동으로 ADD_PROPERTY_ANIMATION 사용
```

## 요약

이 자동화 시스템은:

1. **C++ 헤더 파일 파싱** - UPROPERTY/UFUNCTION 매크로 추출
2. **리플렉션 코드 생성** - BEGIN_PROPERTIES 블록 자동 생성
3. **Lua 바인딩 생성** - LUA_BIND_BEGIN 블록 자동 생성
4. **빌드 시스템 통합** - Visual Studio Pre-Build Event로 자동 실행
5. **증분 빌드 지원** - 변경된 파일만 재생성하여 빌드 시간 단축

개발자는 C++ 헤더 파일에 매크로만 추가하면, 나머지 모든 바인딩 코드가 자동으로 생성되어 Lua 스크립트에서 즉시 사용할 수 있습니다.
