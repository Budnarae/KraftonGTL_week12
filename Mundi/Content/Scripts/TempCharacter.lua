-- TempCharacter.lua
-- BlendSpace2D를 사용한 4방향 걷기 애니메이션

local AnimationStateMachine = dofile("Content/Scripts/AnimationStateMachine.lua")

local ASM = nil
local SkeletalComp = nil
local MovementComp = nil
local BlendSpaceNode = nil

-- 상태 전환
local TransitionToJump = nil
local TransitionToLocomotion = nil
local JumpState = nil
local JumpSequenceNode = nil
local bJumpStateActive = false

-- 블렌드 보간용
local CurrentX = 0.0
local CurrentY = 0.0
local BlendSpeed = 5.0  -- 블렌드 속도 (높을수록 빠르게 전환)

-- 애니메이션 시퀀스
local IdleAnim = nil
local ForwardAnim = nil
local BackwardAnim = nil
local LeftAnim = nil
local RightAnim = nil
local JumpAnim = nil
local DizzyAnim = nil
local DizzyAnim2 = nil

-- Hit State 관련
local HitState = nil
local bHitByObstacle = false
local HitTimer = 0.0
--local HitDuration = 1.33  -- Hit State 지속 시간 (초)
local HitDuration = 0.33  -- Hit State 지속 시간 (초)
local PreviousState = nil

local function build_blend_space(state)
    BlendSpaceNode = state:CreateBlendSpace2DNode()
    if not BlendSpaceNode then
        return false
    end

    -- 3x3 그리드 설정 (-1, 0, 1)
    local axis = { -1.0, 0.0, 1.0 }
    BlendSpaceNode:SetGridAxes(axis, axis)

    local function add_sample(anim, xIndex, yIndex)
        if not anim then
            return
        end
        local node = state:CreateSequenceNode(anim, true)
        if node then
            BlendSpaceNode:AddSample(node, xIndex, yIndex)
        end
    end

    -- 중앙 (Idle)
    add_sample(IdleAnim, 1, 1)

    -- 4방향
    add_sample(ForwardAnim, 1, 2)   -- Forward (Y = 1)
    add_sample(BackwardAnim, 1, 0)  -- Backward (Y = -1)
    add_sample(LeftAnim, 0, 1)      -- Left (X = -1)
    add_sample(RightAnim, 2, 1)     -- Right (X = 1)

    -- 대각선 (가장 가까운 방향 애니메이션으로 채움)
    add_sample(LeftForwardAnim, 2, 2)   -- Forward-Left
    add_sample(RightForwardAnim, 0, 2)   -- Forward-Right
    add_sample(LeftBackAnim, 2, 0)  -- Backward-Left
    add_sample(RightBackAnim, 0, 0)  -- Backward-Right

    BlendSpaceNode.EaseFunction = EAnimBlendEaseType.EaseInOut
    return true
end

local function build_hit_state(state)
    local BasePoseNode = state:CreateSequenceNode(DizzyAnim, true)
    local AdditivePoseNode = state:CreateSequenceNode(DizzyAnim2, true)

    if not BasePoseNode or not AdditivePoseNode then
        return false
    end

    local AdditiveBlendNode = state:CreateAdditiveBlendNode()
    if not AdditiveBlendNode then
        return false
    end

    AdditiveBlendNode:SetBasePose(BasePoseNode)
    AdditiveBlendNode:SetAdditivePose(AdditivePoseNode)
    AdditiveBlendNode.Alpha = 0.5  -- Additive 효과 최대

    state:SetEntryNode(AdditiveBlendNode)
    return true
end

-- Transition 조건 함수들
local function ShouldTransitionToHit()
    return bHitByObstacle
end

local function ShouldTransitionToLocomotion()
    return HitTimer >= HitDuration
end

function BeginPlay()
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[TempCharacter] No SkeletalMeshComponent found")
        return
    end

    MovementComp = GetComponent(Obj, "UCharacterMovementComponent")
    if not MovementComp then
        print("[TempCharacter] No CharacterMovementComponent found")
    end

    ASM = AnimationStateMachine:new()
    ASM:initialize()
    
    -- James Animation 로드
    FallingAnim = LoadAnimationSequence("Falling Idle_mixamo.com")

    IdleAnim     = LoadAnimationSequence("Idle_mixamo.com")
    ForwardAnim  = LoadAnimationSequence("Standard Walk_mixamo.com")
    BackwardAnim = LoadAnimationSequence("Walking Backward_mixamo.com")
    LeftAnim     = LoadAnimationSequence("Left Strafe Walk_mixamo.com")
    RightAnim    = LoadAnimationSequence("Right Strafe Walk_mixamo.com")

    LeftForwardAnim = LoadAnimationSequence("Jog Forward Diagonal Left_mixamo.com")
    RightForwardAnim = LoadAnimationSequence("Jog Forward Diagonal Right_mixamo.com")
    LeftBackAnim = LoadAnimationSequence("Jog Backward Diagonal Left_mixamo.com")
    RightBackAnim = LoadAnimationSequence("Jog Backward Diagonal Right_mixamo.com")

    JumpAnim     = LoadAnimationSequence("Jumping_mixamo.com")
    DizzyAnim    = LoadAnimationSequence("Flying Back Death")
    DizzyAnim2 = LoadAnimationSequence("Flying Back Death")

    if not (IdleAnim and ForwardAnim and BackwardAnim and LeftAnim and RightAnim and DizzyAnim and DizzyAnim2 and JumpAnim) then
        print("[TempCharacter] Failed to load Cactus animations")
        return
    end

    -- Locomotion 상태
    local Locomotion = ASM:add_state(FName("Locomotion"))
    if not Locomotion then
        print("[TempCharacter] Failed to create Locomotion state")
        return
    end

    if not build_blend_space(Locomotion) then
        print("[TempCharacter] Failed to build BlendSpace2D")
        return
    end

    Locomotion:SetEntryNode(BlendSpaceNode)

    -- Jump 상태
    if JumpAnim then
        JumpState = ASM:add_state(FName("Jump"))
        if JumpState then
            JumpSequenceNode = JumpState:CreateSequenceNode(JumpAnim, false)
            if JumpSequenceNode and JumpSequenceNode.SetLooping then
                JumpSequenceNode:SetLooping(false)
            end
            JumpState:SetEntryNode(JumpSequenceNode)

            -- 상태 전환 추가
            TransitionToJump = ASM:add_transition(FName("Locomotion"), FName("Jump"))
            TransitionToLocomotion = ASM:add_transition(FName("Jump"), FName("Locomotion"))

            if TransitionToJump then
                TransitionToJump.BlendTime = 0.2
            end
            if TransitionToLocomotion then
                TransitionToLocomotion.BlendTime = 0.2
            end
        end
    end
    -- Hit State 생성
    HitState = ASM:add_state(FName("Hit"))
    if not HitState then
        print("[TempCharacter] Failed to create Hit state")
        return
    end

    if not build_hit_state(HitState) then
        print("[TempCharacter] Failed to build Hit state")
        return
    end

    -- Transition 추가: Locomotion → Hit
    local TransitionToHit = ASM:add_transition(FName("Locomotion"), FName("Hit"))
    if TransitionToHit then
        TransitionToHit:SetBlendTime(0.2)
        TransitionToHit:SetTransitionCondition(ShouldTransitionToHit)
    end

    -- Transition 추가: Hit → Locomotion
    local TransitionToLocomotion = ASM:add_transition(FName("Hit"), FName("Locomotion"))
    if TransitionToLocomotion then
        TransitionToLocomotion:SetBlendTime(0.3)
        TransitionToLocomotion:SetTransitionCondition(ShouldTransitionToLocomotion)
    end

    -- Transition 플래그 리셋
    if ASM.reset_transition_flags then
        ASM:reset_transition_flags()
    end

    -- 애니메이션 모드 설정 및 초기화
    SkeletalComp:SetAnimationModeInt(1)  -- AnimationBlueprint mode
    SkeletalComp:InitAnimInstance()

    -- 초기 State 저장
    PreviousState = ASM.current_state

    print("[TempCharacter] Animation initialized with BlendSpace2D and Hit state")
end

local function lerp(a, b, t)
    return a + (b - a) * t
end

CurVisibilty = true
function AnimUpdate(deltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    if InputManager:IsKeyPressed("C") then
        SetCursorVisible(not CurVisibilty)
    end
    -- 점프 상태 전환 및 입력 처리
    local CurrentState = ASM.current_state
    local bIsInJumpState = (JumpState ~= nil and CurrentState == JumpState)
    local bIsInHitState = (HitState ~= nil and CurrentState == HitState)

    if TransitionToJump then
        TransitionToJump.CanEnterTransition = false
    end
    if TransitionToLocomotion then
        TransitionToLocomotion.CanEnterTransition = false
    end

    if bIsInJumpState then
        bJumpStateActive = true
    elseif bJumpStateActive then
        bJumpStateActive = false
    end

    local bWantsJump = InputManager and InputManager.IsKeyPressed and InputManager:IsKeyPressed(' ')
    if bWantsJump and not bIsInJumpState and not bIsInHitState and JumpSequenceNode then
        if JumpSequenceNode.SetNormalizedTime then
            JumpSequenceNode:SetNormalizedTime(0.0)
        end
        bJumpStateActive = true
        if TransitionToJump then
            TransitionToJump.CanEnterTransition = true
        end
    end

    if bIsInJumpState and JumpSequenceNode and JumpSequenceNode.GetNormalizedTime then
        local NormalizedTime = JumpSequenceNode:GetNormalizedTime()
        if NormalizedTime >= 0.98 and TransitionToLocomotion then
            TransitionToLocomotion.CanEnterTransition = true
        end
    end

    -- 속도 기반 블렌드 입력 계산
    local TargetX = 0.0
    local TargetY = 0.0

    if MovementComp then
        local Velocity = MovementComp:GetVelocity()
        local MaxSpeed = MovementComp:GetMaxWalkSpeed()

        if MaxSpeed > 0.0 then
            local Forward = Obj:GetForward()
            local Right = Obj:GetRight()

            -- 월드 속도를 로컬 공간으로 변환
            local LocalY = Velocity.X * Forward.X + Velocity.Y * Forward.Y
            local LocalX = Velocity.X * Right.X + Velocity.Y * Right.Y

            -- 최대 속도로 정규화
            TargetX = math.max(-1.0, math.min(1.0, LocalX / MaxSpeed))
            TargetY = math.max(-1.0, math.min(1.0, LocalY / MaxSpeed))
        end
    end

    -- 부드러운 보간
    local t = math.min(1.0, deltaTime * BlendSpeed)
    CurrentX = lerp(CurrentX, TargetX, t)
    CurrentY = lerp(CurrentY, TargetY, t)

    -- BlendInput 설정
    if BlendSpaceNode then
        BlendSpaceNode:SetBlendInput(CurrentX, CurrentY)
    end

    -- Hit State에 있을 때 타이머 증가
    local CurrentState = ASM.current_state
    if CurrentState == HitState then
        HitTimer = HitTimer + deltaTime
    end

    -- State 변경 감지
    if CurrentState ~= PreviousState then
        -- State가 변경되었으므로 타이머 및 플래그 리셋
        HitTimer = 0.0
        bHitByObstacle = false
        PreviousState = CurrentState

        -- 모든 Transition의 CanEnterTransition 리셋
        if ASM.reset_transition_flags then
            ASM:reset_transition_flags()
        end
    end

    local Context = FAnimationUpdateContext()
    Context.DeltaTime = deltaTime
    ASM:update(Context)

    return ASM.current_state
end

function AnimEvaluate(PoseContext)
    if not ASM or not ASM.current_state then
        return
    end

    ASM:evaluate(PoseContext)
end

function Tick(deltaTime)
    -- 입력 처리는 AnimUpdate에서 수행
end

function OnBeginOverlap(OtherActor)
    if OtherActor and OtherActor.Tag == "CarObstacle" then
        -- Hit State로 전환하기 위한 플래그 설정
        bHitByObstacle = true
        print("[TempCharacter] Hit by CarObstacle! Transitioning to Hit state...")
    end
end

function EndPlay()
    if ASM then
        ASM:shutdown()
    end

    ASM = nil
    SkeletalComp = nil
    MovementComp = nil
    BlendSpaceNode = nil
    IdleAnim = nil
    ForwardAnim = nil
    BackwardAnim = nil
    LeftAnim = nil
    RightAnim = nil
    JumpAnim = nil
    JumpState = nil
    JumpSequenceNode = nil
    bJumpStateActive = false
    TransitionToJump = nil
    TransitionToLocomotion = nil
    DizzyAnim = nil
    DizzyAnim2 = nil
    HitState = nil
    bHitByObstacle = false
    HitTimer = 0.0
    PreviousState = nil
    CurrentX = 0.0
    CurrentY = 0.0
    FootstepSound = nil
end

function SetCursorVisible(Show)
    InputManager:SetCursorVisible(Show)
    CurVisibilty = Show
end