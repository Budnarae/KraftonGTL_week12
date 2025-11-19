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

-- 블렌드 보간용
local CurrentX = 0.0
local CurrentY = 0.0
local BlendSpeed = 3.0  -- 블렌드 속도 (높을수록 빠르게 전환)

-- 애니메이션 시퀀스
local IdleAnim = nil
local ForwardAnim = nil
local BackwardAnim = nil
local LeftAnim = nil
local RightAnim = nil
local JumpAnim = nil

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
    add_sample(ForwardAnim, 0, 2)   -- Forward-Left
    add_sample(ForwardAnim, 2, 2)   -- Forward-Right
    add_sample(BackwardAnim, 0, 0)  -- Backward-Left
    add_sample(BackwardAnim, 2, 0)  -- Backward-Right

    BlendSpaceNode.EaseFunction = EAnimBlendEaseType.EaseInOut
    return true
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

    -- Cactus 애니메이션 로드
    IdleAnim     = LoadAnimationSequence("CactusPA_Cactus_IdleBattle")
    ForwardAnim  = LoadAnimationSequence("CactusPA_Cactus_WalkFWD")
    BackwardAnim = LoadAnimationSequence("CactusPA_Cactus_WalkBWD")
    LeftAnim     = LoadAnimationSequence("CactusPA_Cactus_WalkLFT")
    RightAnim    = LoadAnimationSequence("CactusPA_Cactus_WalkRGT")
    JumpAnim     = LoadAnimationSequence("CactusPA_Cactus_RunFWD")

    if not (IdleAnim and ForwardAnim and BackwardAnim and LeftAnim and RightAnim) then
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
        local JumpState = ASM:add_state(FName("Jump"))
        if JumpState then
            local JumpNode = JumpState:CreateSequenceNode(JumpAnim, true)
            JumpState:SetEntryNode(JumpNode)

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

    -- 애니메이션 모드 설정 및 초기화
    SkeletalComp:SetAnimationModeInt(1)  -- AnimationBlueprint mode
    SkeletalComp:InitAnimInstance()

    print("[TempCharacter] Animation initialized with BlendSpace2D")
end

local function lerp(a, b, t)
    return a + (b - a) * t
end

function AnimUpdate(deltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    -- 점프 상태 전환 체크
    if MovementComp and ASM.current_state then
        local bIsFalling = MovementComp:IsFalling()

        -- 전환 플래그 리셋
        if TransitionToJump then
            TransitionToJump.CanEnterTransition = false
        end
        if TransitionToLocomotion then
            TransitionToLocomotion.CanEnterTransition = false
        end

        -- 조건에 따라 전환 활성화
        if bIsFalling then
            if TransitionToJump then
                TransitionToJump.CanEnterTransition = true
            end
        else
            if TransitionToLocomotion then
                TransitionToLocomotion.CanEnterTransition = true
            end
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
    TransitionToJump = nil
    TransitionToLocomotion = nil
    CurrentX = 0.0
    CurrentY = 0.0
end
