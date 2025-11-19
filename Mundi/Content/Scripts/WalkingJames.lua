-- WalkingJames.lua
-- 걷는 애니메이션만 재생하는 간단한 Skeletal Mesh Actor
-- SkeletalMeshComponent를 가진 Actor에 붙여서 사용

-- AnimationStateMachine 라이브러리 로드
dofile("Content/Scripts/AnimationStateMachine.lua")

-- 전역 변수
local ASM = nil  -- Lua AnimationStateMachine 인스턴스
local SkeletalComp = nil

-- State 정보
local WalkState = nil

-- Animation 정보 (GC 방지를 위해 전역 스코프에 저장)
local WalkAnim = nil

function BeginPlay()
    -- SkeletalMeshComponent 가져오기
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[WalkingJames] Error: No SkeletalMeshComponent found")
        return
    end

    -- Lua AnimationStateMachine 생성
    ASM = AnimationStateMachine:new()
    if not ASM then
        print("[WalkingJames] Error: Failed to construct AnimationStateMachine")
        return
    end

    ASM:initialize()

    -- 걷는 애니메이션 로드
    WalkAnim = LoadAnimationSequence("Standard Walk_mixamo.com")
    if not WalkAnim then
        print("[WalkingJames] Error: Failed to load walking animation")
        return
    end

    -- PIE 재실행 시 Notify 중복 방지: 기존 Notify 모두 제거
    WalkAnim:ClearAnimNotifies()
    WalkAnim:ClearAnimNotifyStates()

    -- 발걸음 사운드 로드
    local FootWalkSound = LoadSound("Data/Audio/FootWalk.wav")
    if not FootWalkSound then
        print("[WalkingJames] Warning: Failed to load FootWalk.wav")
    end

    -- Walking State 생성
    WalkState = ASM:add_state(FName("Walking"))
    if WalkState then
        local SeqNode = WalkState:CreateSequenceNode(WalkAnim, true)  -- true = looping
        WalkState:SetEntryNode(SeqNode)
    else
        print("[WalkingJames] Error: Failed to create Walking state")
        return
    end

    -- AnimationMode 설정 및 AnimInstance 생성
    SkeletalComp:SetAnimationModeInt(1)  -- AnimationBlueprint = 1

    -- AnimInstance 수동 생성
    SkeletalComp:InitAnimInstance()

    -- AnimInstance 가져오기
    local AnimInstance = SkeletalComp:GetAnimInstance()
    if not AnimInstance then
        print("[WalkingJames] Error: No AnimInstance found")
        return
    end

    -- Sound AnimNotify 생성 및 설정
    local OwnerActor = Obj:GetOwner()

    -- 발걸음 소리 Notify 추가 (왼발)
    if WalkAnim and FootWalkSound then
        local FootNotify1 = NewSoundAnimNotify()
        if FootNotify1 then
            FootNotify1:SetSound(FootWalkSound)
            FootNotify1:SetTimeToNotify(0.1)  -- 애니메이션 시작 후 0.3초
            FootNotify1:SetVolume(0.8)
            FootNotify1:SetPitch(1.0)
            FootNotify1:SetOwner(OwnerActor)
            WalkAnim:AddAnimNotify(FootNotify1)
        end
    end

    -- 발걸음 소리 Notify 추가 (오른발)
    if WalkAnim and FootWalkSound then
        local FootNotify2 = NewSoundAnimNotify()
        if FootNotify2 then
            FootNotify2:SetSound(FootWalkSound)
            FootNotify2:SetTimeToNotify(0.65)  -- 애니메이션 시작 후 1.0초
            FootNotify2:SetVolume(0.8)
            FootNotify2:SetPitch(1.0)
            FootNotify2:SetOwner(OwnerActor)
            WalkAnim:AddAnimNotify(FootNotify2)
        end
    end

    print("[WalkingJames] Successfully initialized walking animation")
end

-- AnimUpdate: UAnimInstance::NativeUpdateAnimation에서 호출
function AnimUpdate(DeltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    -- Lua ASM Update 호출
    local Context = FAnimationUpdateContext()
    Context.DeltaTime = DeltaTime
    ASM:update(Context)

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

function Tick(DeltaTime)
    -- AnimUpdate는 UAnimInstance::NativeUpdateAnimation에서 호출됨
    -- 여기서는 특별한 작업 없음
end

function EndPlay()
    if ASM then
        ASM:shutdown()
    end

    -- 모든 전역 변수 정리
    ASM = nil
    SkeletalComp = nil
    WalkState = nil
    WalkAnim = nil
end
