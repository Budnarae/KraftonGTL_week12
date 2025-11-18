-- TestBlendSpace.lua
-- BlendSpace2D: X는 좌/우(-1~1), Y는 전/후(-1~1)
-- T: 앞(+Y), G: 뒤(-Y), H: 오른(+X), F: 왼(-X)

local AnimationStateMachine = dofile("Content/Scripts/AnimationStateMachine.lua")

local ASM = nil
local SkeletalComp = nil
local BlendSpaceNode = nil

local InputX = 0.0
local InputY = 0.0

local IdleAnim = nil
local ForwardAnim = nil
local BackwardAnim = nil
local LeftAnim = nil
local RightAnim = nil

local function BuildBlendSpace(state)
    BlendSpaceNode = state:CreateBlendSpace2DNode()
    if not BlendSpaceNode then
        return false
    end

    local AxisX = {-1.0, 0.0, 1.0}
    local AxisY = {-1.0, 0.0, 1.0}
    BlendSpaceNode:SetGridAxes(AxisX, AxisY)

    local function add(anim, xIndex, yIndex)
        local node = state:CreateSequenceNode(anim, true)
        if node then
            BlendSpaceNode:AddSample(node, xIndex, yIndex)
        end
    end

    -- GridYIndex: 0 = -1(back), 1 = 0, 2 = +1(front)
    -- GridXIndex: 0 = -1(left), 1 = 0, 2 = +1(right)
    add(BackwardAnim, 1, 0)  -- back
    add(IdleAnim,     1, 1)  -- idle center
    add(ForwardAnim,  1, 2)  -- front
    add(LeftAnim,     0, 1)  -- left
    add(RightAnim,    2, 1)  -- right

    -- 채우지 않은 셀들은 가장 가까운 방향 값으로 자동 보간
    return true
end

function BeginPlay()
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[TestBlendSpace] Error: No SkeletalMeshComponent")
        return
    end

    ASM = AnimationStateMachine:new()
    ASM:initialize()

    IdleAnim     = LoadAnimationSequence("Standard Run_mixamo.com")
    ForwardAnim  = LoadAnimationSequence("Standard Walk_mixamo.com")
    BackwardAnim = LoadAnimationSequence("Standard Run_mixamo.com")
    LeftAnim     = LoadAnimationSequence("Standard Walk_mixamo.com")
    RightAnim    = LoadAnimationSequence("Standard Run_mixamo.com")

    --IdleAnim     = LoadAnimationSequence("Standard Idle_mixamo.com")
    --ForwardAnim  = LoadAnimationSequence("Standard Walk_mixamo.com")
    --BackwardAnim = LoadAnimationSequence("Unarmed Walk Back_mixamo.com")
    --LeftAnim     = LoadAnimationSequence("Left Strafe Walk_mixamo.com")
    --RightAnim    = LoadAnimationSequence("Right Strafe Walk_mixamo.com")

    if not (IdleAnim and ForwardAnim and BackwardAnim and LeftAnim and RightAnim) then
        print("[TestBlendSpace] Error: Failed to load required animations")
        return
    end

    local Locomotion = ASM:add_state(FName("Locomotion"))
    if not Locomotion then
        print("[TestBlendSpace] Error: Failed to create state")
        return
    end

    if not BuildBlendSpace(Locomotion) then
        print("[TestBlendSpace] Error: Failed to build BlendSpace2D")
        return
    end

    Locomotion:SetEntryNode(BlendSpaceNode)

    SkeletalComp:SetAnimationModeInt(1)
    SkeletalComp:InitAnimInstance()
end

local function UpdateBlendInputs()
    InputX = 0.0
    InputY = 0.0

    if InputManager:IsKeyDown('T') then
        InputY = InputY + 1.0
    end
    if InputManager:IsKeyDown('G') then
        InputY = InputY - 1.0
    end
    if InputManager:IsKeyDown('H') then
        InputX = InputX + 1.0
    end
    if InputManager:IsKeyDown('F') then
        InputX = InputX - 1.0
    end

    InputX = math.max(-1.0, math.min(InputX, 1.0))
    InputY = math.max(-1.0, math.min(InputY, 1.0))

    if BlendSpaceNode then
        BlendSpaceNode:SetBlendInput(InputX, InputY)
    end
end

function AnimUpdate(deltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    UpdateBlendInputs()

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
    BlendSpaceNode = nil
    IdleAnim = nil
    ForwardAnim = nil
    BackwardAnim = nil
    LeftAnim = nil
    RightAnim = nil
end
