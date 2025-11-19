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

-- Animation 정보 (GC 방지를 위해 전역 스코프에 저장)
local AnimA = nil
local AnimB = nil
local AnimC = nil

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
    -- SkeletalMeshComponent 가져오기
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[SkeletalMeshActor] Error: No SkeletalMeshComponent found")
        return
    end

    -- Lua AnimationStateMachine 생성
    ASM = AnimationStateMachine:new()
    if not ASM then
        print("[SkeletalMeshActor] Error: Failed to construct AnimationStateMachine")
        return
    end

    ASM:initialize()

    -- 애니메이션 로드 (GC 방지를 위해 스크립트 스코프 변수에 저장)
    AnimA = LoadAnimationSequence("Standard Run_mixamo.com")
    AnimB = LoadAnimationSequence("Standard Walk_mixamo.com")
    AnimC = LoadAnimationSequence("James_mixamo.com")

    if not AnimA or not AnimB or not AnimC then
        print("[SkeletalMeshActor] Error: Failed to load animations")
        return
    end

    -- Sound 로드 (애니메이션별로 다른 사운드 사용)
    local SoundA = LoadSound("Data/Audio/Shot.wav")
    -- local SoundB = LoadSound("Data/Audio/HitFireball.wav")
    local SoundC = LoadSound("Data/Audio/HitFireball.wav")

    -- State 생성 및 애니메이션 추가
    StateA = ASM:add_state(FName("StateA"))
    if StateA then
        local SeqNode = StateA:CreateSequenceNode(AnimA, true)
        SeqNode:SetReversePlay();
        StateA:SetEntryNode(SeqNode);
    else
        print("[SkeletalMeshActor] ERROR: Failed to create StateA")
    end

    StateB = ASM:add_state(FName("StateB"))
    if StateB then
        local SeqNode = StateB:CreateSequenceNode(AnimB, true)
        StateB:SetEntryNode(SeqNode)
    else
        print("[SkeletalMeshActor] ERROR: Failed to create StateB")
    end

    StateC = ASM:add_state(FName("StateC"))
    if StateC then
        local SeqNode = StateC:CreateSequenceNode(AnimC, true)
        StateC:SetEntryNode(SeqNode)
    else
        print("[SkeletalMeshActor] ERROR: Failed to create StateC")
    end
    -- Transition 생성 및 조건 함수 설정
    local TransitionAtoB = ASM:add_transition(FName("StateA"), FName("StateB"))
    if TransitionAtoB then
        TransitionAtoB:SetBlendTime(0.3)
        TransitionAtoB:SetTransitionCondition(ShouldTransitionAtoB)
    end

    local TransitionBtoC = ASM:add_transition(FName("StateB"), FName("StateC"))
    if TransitionBtoC then
        TransitionBtoC:SetBlendTime(0.3)
        TransitionBtoC:SetTransitionCondition(ShouldTransitionBtoC)
    end

    local TransitionCtoA = ASM:add_transition(FName("StateC"), FName("StateA"))
    if TransitionCtoA then
        TransitionCtoA:SetBlendTime(0.3)
        TransitionCtoA:SetTransitionCondition(ShouldTransitionCtoA)
    end

    if ASM.reset_transition_flags then
        ASM:reset_transition_flags()
    end

    -- AnimationMode 설정 및 AnimInstance 생성
    SkeletalComp:SetAnimationModeInt(1)  -- AnimationBlueprint = 1

    -- AnimInstance 수동 생성 (OnRegister가 이미 끝났으므로)
    SkeletalComp:InitAnimInstance()

    -- AnimInstance 가져오기 (InitAnimInstance 호출 후)
    local AnimInstance = SkeletalComp:GetAnimInstance()
    if not AnimInstance then
        print("[SkeletalMeshActor] Error: No AnimInstance found")
        return
    end

    -- Sound AnimNotify 생성 및 설정
    -- AnimNotify는 이제 UAnimationSequence에서 관리됩니다
    local OwnerActor = Obj:GetOwner()

    -- StateA: Sound Notify 추가
    if AnimA and SoundA then
        local NotifyA = NewSoundAnimNotify()
        if NotifyA then
            NotifyA:SetSound(SoundA)
            NotifyA:SetTimeToNotify(0.1)
            NotifyA:SetVolume(1.0)
            NotifyA:SetPitch(1.0)
            NotifyA:SetOwner(OwnerActor)
            AnimA:AddAnimNotify(NotifyA)
        end
    end

    -- StateA: Rotating NotifyState 추가
    if AnimA then
        local RotatingNotify = NewRotatingAnimNotifyState()
        if RotatingNotify then
            RotatingNotify:SetOwner(OwnerActor)
            RotatingNotify:SetRotationPerTick(Vector(0, 0, 0.1))  -- 매 틱마다 Yaw 10도 회전 (Roll, Pitch, Yaw)
            RotatingNotify:SetRollbackOnEnd(true)  -- 종료 시 원래 회전으로 복원
            RotatingNotify:SetStartTime(0.5)
            RotatingNotify:SetDurationTime(1.0)
            AnimA:AddAnimNotifyState(RotatingNotify)
        end
    end

    -- StateB: CameraShake Notify 추가
    if AnimB then
        local NotifyB = NewCameraShakeAnimNotify()
        if NotifyB then
            NotifyB:SetDuration(0.5)
            NotifyB:SetAmplitudeLocation(1.0)
            NotifyB:SetAmplitudeRotationDeg(1.0)
            NotifyB:SetFrequency(15.0)
            NotifyB:SetPriority(0)
            NotifyB:SetTimeToNotify(1)
            if EShakeNoise then
                NotifyB:SetNoiseMode(EShakeNoise.Perlin)
            end
            AnimB:AddAnimNotify(NotifyB)
        end
    end

    -- StateB: Scaling NotifyState 추가
    if AnimB then
        local ScalingNotify = NewScalingAnimNotifyState()
        if ScalingNotify then
            ScalingNotify:SetOwner(OwnerActor)
            ScalingNotify:SetScalePerTick(Vector(0.001, 0.001, 0.001))  -- 매 틱마다 스케일 0.01 증가
            ScalingNotify:SetRollbackOnEnd(true)  -- 종료 시 원래 스케일로 복원
            ScalingNotify:SetStartTime(0.5)
            ScalingNotify:SetDurationTime(1.5)
            AnimB:AddAnimNotifyState(ScalingNotify)
        end
    end

    -- StateB: Sound Notify (주석 처리)
    -- if AnimB and SoundB then
    --     local NotifyB = NewSoundAnimNotify()
    --     if NotifyB then
    --         NotifyB:SetSound(SoundB)
    --         NotifyB:SetTimeToNotify(0.1)
    --         NotifyB:SetVolume(1.0)
    --         NotifyB:SetPitch(1.0)
    --         NotifyB:SetOwner(OwnerActor)
    --         AnimB:AddAnimNotify(NotifyB)
    --     end
    -- end

    -- StateC: Sound Notify 추가
    if AnimC and SoundC then
        local NotifyC = NewSoundAnimNotify()
        if NotifyC then
            NotifyC:SetSound(SoundC)
            NotifyC:SetTimeToNotify(0.1)
            NotifyC:SetVolume(1.0)
            NotifyC:SetPitch(1.0)
            NotifyC:SetOwner(OwnerActor)
            AnimC:AddAnimNotify(NotifyC)
        end
    end

    -- 초기 State 저장
    PreviousState = ASM.current_state
end

-- AnimUpdate: UAnimInstance::NativeUpdateAnimation에서 호출
-- 반환값: 현재 재생 중인 AnimSequence (UAnimationSequence*)
function AnimUpdate(DeltaTime)
    if not ASM or not ASM.current_state then
        return nil
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
        if ASM.reset_transition_flags then
            ASM:reset_transition_flags()
        end
    end

    ---- 현재 재생 중인 AnimNode_Sequence 반환
    --local CurrentNode = nil
    --
    --if ASM.bIsInTransition and ASM.BlendTo then
    --    -- Transition 중이면 BlendTo (목표 State의 Node) 반환
    --    CurrentNode = ASM.BlendTo
    --elseif ASM.current_state then
    --    -- 일반 상태면 현재 State의 EntryNode 반환 (새로운 구조)
    --    CurrentNode = ASM.current_state:GetEntryNode()
    --end
    --
    --if CurrentNode.CurrentTime == nil then
    --    print("Current Time is nil");
    --end
    --
    --return CurrentNode.CurrentTime;
    
    return ASM.current_state;
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
    TransitionTimer = 0.0
    PreviousState = nil
    ASM = nil
    SkeletalComp = nil
    StateA = nil
    StateB = nil
    StateC = nil
end