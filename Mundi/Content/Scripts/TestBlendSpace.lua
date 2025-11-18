-- SkeletalMeshActor.lua
-- BlendSpace1D 테스트 코드
-- 'U' 키: 속도 증가, 'J' 키: 속도 감소

-- AnimationStateMachine 라이브러리 로드
dofile("Content/Scripts/AnimationStateMachine.lua")

-- 전역 변수
local ASM = nil                 -- Lua AnimationStateMachine 인스턴스
local SkeletalComp = nil
local BlendSpaceNode = nil      -- BlendSpace 노드 참조
local CurrentSpeed = 200.0      -- 현재 속도 (걷기 속도로 시작)

-- 걷기/달리기 애니메이션 (GC 방지를 위해 전역 스코프에 저장)
local WalkAnim = nil
local RunAnim = nil


function BeginPlay()
    -- SkeletalMeshComponent 가져오기
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[SkeletalMeshActor] Error: No SkeletalMeshComponent found")
        return
    end

    -- Lua AnimationStateMachine 생성 및 초기화
    ASM = AnimationStateMachine:new()
    ASM:initialize()

    -- 사용할 애니메이션 로드
    WalkAnim = LoadAnimationSequence("Standard Walk_mixamo.com")
    RunAnim = LoadAnimationSequence("Standard Run_mixamo.com")

    if not WalkAnim or not RunAnim then
        print("[SkeletalMeshActor] Error: Failed to load animations")
        return
    end

    -- 'Locomotion'이라는 새로운 State를 생성
    local LocomotionState = ASM:add_state(FName("Locomotion"))
    if LocomotionState then

        -- 1. BlendSpace1D 노드 생성
        BlendSpaceNode = LocomotionState:CreateBlendSpace1DNode()

        -- 2. BlendSpace에 사용할 시퀀스 노드들(샘플) 생성
        local WalkNode = LocomotionState:CreateSequenceNode(WalkAnim, true)
        local RunNode  = LocomotionState:CreateSequenceNode(RunAnim, true)

        -- 3. BlendSpace에 샘플 추가
        BlendSpaceNode:AddSample(WalkNode, 150.0)  -- 속도 150일 때 걷기
        BlendSpaceNode:AddSample(RunNode, 350.0)   -- 속도 350일 때 달리기

        -- 4. 이 State의 시작 노드를 BlendSpace 노드로 설정
        LocomotionState:SetEntryNode(BlendSpaceNode)
    else
        print("[SkeletalMeshActor] ERROR: Failed to create LocomotionState")
    end

    -- AnimationMode 설정 및 AnimInstance 생성/가져오기
    SkeletalComp:SetAnimationModeInt(1)  -- AnimationBlueprint
    SkeletalComp:InitAnimInstance()

    local AnimInstance = SkeletalComp:GetAnimInstance()
    if not AnimInstance then
        print("[SkeletalMeshActor] Error: No AnimInstance found")
        return
    end
end


-- AnimUpdate: UAnimInstance::NativeUpdateAnimation에서 호출
function AnimUpdate(DeltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    if BlendSpaceNode then
        print(CurrentSpeed)
        BlendSpaceNode:SetBlendInput(CurrentSpeed)
    end

    -- Lua ASM Update 호출
    local Context = FAnimationUpdateContext()
    Context.DeltaTime = DeltaTime
    ASM:update(Context)

    -- 현재 State 반환
    return ASM.current_state
end


-- AnimEvaluate: UAnimInstance::EvaluateAnimation에서 호출
function AnimEvaluate(PoseContext)
    if not ASM or not ASM.current_state then
        return
    end

    -- Lua ASM Evaluate 호출
    ASM:evaluate(PoseContext)
end


-- Tick: 매 프레임 호출되는 함수
function Tick(DeltaTime)
    -- 'K' 키를 누르면 속도 증가
    if InputManager:IsKeyDown('K') then
        CurrentSpeed = CurrentSpeed + 100.0 * DeltaTime
        print('K')
        print(CurrentSpeed)
    end

    -- 'J' 키를 누르면 속도 감소
    if InputManager:IsKeyDown('J') then
        CurrentSpeed = CurrentSpeed - 100.0 * DeltaTime
        print('J')
        print(CurrentSpeed)
    end

    -- 속도를 150 ~ 350 사이로 제한
    CurrentSpeed = math.max(150.0, math.min(CurrentSpeed, 350.0))
end


function EndPlay()
    if ASM then
        ASM:shutdown()
    end

    -- 전역 변수 정리
    ASM = nil
    SkeletalComp = nil
    BlendSpaceNode = nil
    WalkAnim = nil
    RunAnim = nil
end
