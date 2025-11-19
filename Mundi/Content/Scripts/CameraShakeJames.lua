-- WalkingJames.lua
-- 걷는 애니메이션만 재생하는 간단한 Skeletal Mesh Actor
-- SkeletalMeshComponent를 가진 Actor에 붙여서 사용

-- AnimationStateMachine 라이브러리 로드
dofile("Content/Scripts/AnimationStateMachine.lua")

-- 전역 변수
local ASM = nil  -- Lua AnimationStateMachine 인스턴스
local SkeletalComp = nil

-- State 정보
local CameraShakeState = nil

-- Animation 정보 (GC 방지를 위해 전역 스코프에 저장)
local CameraShakeAnim = nil

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
    CameraShakeAnim = LoadAnimationSequence("Standard Walk_mixamo.com")
    if not CameraShakeAnim then
        print("[WalkingJames] Error: Failed to load walking animation")
        return
    end

    -- PIE 재실행 시 Notify 중복 방지: 기존 Notify 모두 제거
    CameraShakeAnim:ClearAnimNotifies()
    CameraShakeAnim:ClearAnimNotifyStates()

    -- Walking State 생성
    CameraShakeState = ASM:add_state(FName("CameraShake"))
    if CameraShakeState then
        local SeqNode = CameraShakeState:CreateSequenceNode(CameraShakeAnim, true)  -- true = looping
        CameraShakeState:SetEntryNode(SeqNode)
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

    -- 발걸음 소리 Notify 추가 (왼발)
    if CameraShakeAnim then
        local CMNotify = NewCameraShakeAnimNotify()
        if CMNotify then
            CMNotify:SetDuration(0.6)
            CMNotify:SetAmplitudeLocation(1.0)
            CMNotify:SetAmplitudeRotationDeg(1.0)
            CMNotify:SetFrequency(6.0)
            CMNotify:SetPriority(0)
            CMNotify:SetTimeToNotify(1)
            if EShakeNoise then
                CMNotify:SetNoiseMode(EShakeNoise.Perlin)
            end
            CameraShakeAnim:AddAnimNotify(CMNotify)
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
    CameraShakeState = nil
    CameraShakeAnim = nil
end
