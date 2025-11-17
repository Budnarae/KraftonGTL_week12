-- SkeletalMeshActor.lua
-- Animation State Machine 테스트 코드
-- SkeletalMeshComponent를 가진 Actor에 붙여서 사용

-- AnimationStateMachine 라이브러리 로드 (dofile 사용 - 현재 환경에서 실행됨)
dofile("Content/Scripts/AnimationStateMachine.lua")

-- 전역 변수
local TransitionTimer = 0.0
local TransitionInterval = 3.0  -- 3초마다 State 전환
local PreviousState = nil
local ASM = nil  -- Lua AnimationStateMachine 인스턴스
local SkeletalComp = nil

-- State 정보
local StateA = nil
local StateB = nil
local StateC = nil

-- Transition 조건 함수들
local function ShouldTransitionAtoB()
    return TransitionTimer >= TransitionInterval
end

local function ShouldTransitionBtoC()
    return TransitionTimer >= TransitionInterval
end

local function ShouldTransitionCtoA()
    return TransitionTimer >= TransitionInterval
end

function BeginPlay()
    print("[SkeletalMeshActor] BeginPlay")

    -- SkeletalMeshComponent 가져오기
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[SkeletalMeshActor] Error: No SkeletalMeshComponent found")
        return
    end

    -- Lua AnimationStateMachine 생성
    ASM = AnimationStateMachine:new()
    ASM:initialize()

    -- 애니메이션 로드
    local AnimA = LoadAnimationSequence("Standard Run_mixamo.com")
    local AnimB = LoadAnimationSequence("Standard Walk_mixamo.com")
    local AnimC = LoadAnimationSequence("James_mixamo.com")

    if not AnimA or not AnimB or not AnimC then
        print("[SkeletalMeshActor] Error: Failed to load animations")
        return
    end

    -- State 생성 및 애니메이션 추가
    StateA = ASM:add_state(FName("StateA"))
    if StateA then
        StateA:AddAnimSequence(AnimA, true)
        print("[SkeletalMeshActor] StateA created")
    end

    StateB = ASM:add_state(FName("StateB"))
    if StateB then
        StateB:AddAnimSequence(AnimB, true)
        print("[SkeletalMeshActor] StateB created")
    end

    StateC = ASM:add_state(FName("StateC"))
    if StateC then
        StateC:AddAnimSequence(AnimC, true)
        print("[SkeletalMeshActor] StateC created")
    end

    -- Transition 생성 및 조건 함수 설정
    local TransitionAtoB = ASM:add_transition(FName("StateA"), FName("StateB"))
    if TransitionAtoB then
        TransitionAtoB:SetBlendTime(0.3)
        TransitionAtoB:SetTransitionCondition(ShouldTransitionAtoB)
        print("[SkeletalMeshActor] Transition A->B created")
    end

    local TransitionBtoC = ASM:add_transition(FName("StateB"), FName("StateC"))
    if TransitionBtoC then
        TransitionBtoC:SetBlendTime(0.3)
        TransitionBtoC:SetTransitionCondition(ShouldTransitionBtoC)
        print("[SkeletalMeshActor] Transition B->C created")
    end

    local TransitionCtoA = ASM:add_transition(FName("StateC"), FName("StateA"))
    if TransitionCtoA then
        TransitionCtoA:SetBlendTime(0.3)
        TransitionCtoA:SetTransitionCondition(ShouldTransitionCtoA)
        print("[SkeletalMeshActor] Transition C->A created")
    end

    -- AnimationMode 설정
    SkeletalComp:SetAnimationModeInt(1)  -- AnimationBlueprint = 1

    -- 초기 State 저장
    PreviousState = ASM.current_state

    print("[SkeletalMeshActor] ASM initialized successfully")
end

-- AnimUpdate: UAnimInstance::NativeUpdateAnimation에서 호출
function AnimUpdate(DeltaTime)
    if not ASM then
        return
    end

    -- 타이머 업데이트
    TransitionTimer = TransitionTimer + DeltaTime

    -- Lua ASM Update 호출
    local Context = FAnimationUpdateContext()
    Context.DeltaTime = DeltaTime
    ASM:update(Context)

    -- State 변경 감지
    local CurrentState = ASM.current_state
    if CurrentState ~= PreviousState then
        -- State가 변경되었으므로 타이머 리셋
        TransitionTimer = 0.0
        PreviousState = CurrentState

        -- 모든 Transition의 CanEnterTransition 리셋
        for i = 1, #ASM.transitions do
            local Transition = ASM.transitions[i]
            if Transition then
                Transition.CanEnterTransition = false
            end
        end

        -- State 변경 로그
        if CurrentState then
            print("[SkeletalMeshActor] State changed to: " .. CurrentState.Name:ToString())
        end
    end
end

-- AnimEvaluate: UAnimInstance::EvaluateAnimation에서 호출
function AnimEvaluate(PoseContext)
    if not ASM then
        return
    end

    -- Lua ASM Evaluate 호출
    ASM:evaluate(PoseContext)
end

function Tick(DeltaTime)
    -- AnimUpdate는 UAnimInstance::NativeUpdateAnimation에서 호출됨
    -- 여기서는 특별한 작업 없음
end

function EndPlay()
    print("[SkeletalMeshActor] EndPlay")

    if ASM then
        ASM:shutdown()
    end

    -- 모든 전역 변수 정리
    TransitionTimer = 0.0
    PreviousState = nil
    ASM = nil
    SkeletalComp = nil
    StateA = nil
    StateB = nil
    StateC = nil
end
