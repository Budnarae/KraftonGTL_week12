-- TestBlendSpace2D.lua
-- BlendSpace that exposes the new ease functions so we can feel the timing difference at runtime.

local AnimationStateMachine = dofile("Content/Scripts/AnimationStateMachine.lua")

local ASM = nil
local SkeletalComp = nil
local BlendSpaceNode = nil

local InputX = 1.0
local InputY = 1.0
local AutoTimer = 0.0
local bAutoDrive = true
local AutoSpeed = 1.35

local EaseModes = {
    { name = "Linear", type = EAnimBlendEaseType.Linear },
    { name = "EaseIn", type = EAnimBlendEaseType.EaseIn },
    { name = "EaseOut", type = EAnimBlendEaseType.EaseOut },
    { name = "EaseInOut", type = EAnimBlendEaseType.EaseInOut },
}
local CurrentEaseIndex = 1

local IdleAnim = nil
local ForwardAnim = nil
local BackwardAnim = nil
local LeftAnim = nil
local RightAnim = nil

local FallingAnim = nil

local RightForwardAnim = nil
local LeftBackAnim = nil
local RightBackAnim = nil
local LeftForwardAnim = nil

local function apply_ease(index)
    if not BlendSpaceNode then
        return
    end

    local count = #EaseModes
    CurrentEaseIndex = ((index - 1) % count) + 1

    local mode = EaseModes[CurrentEaseIndex]
    BlendSpaceNode.EaseFunction = mode.type
    print(string.format("[TestBlendSpace2D] Ease Mode: %s", mode.name))
end

local function handle_ease_hotkeys()
    if InputManager:IsKeyPressed('1') then
        apply_ease(1)
    elseif InputManager:IsKeyPressed('2') then
        apply_ease(2)
    elseif InputManager:IsKeyPressed('3') then
        apply_ease(3)
    elseif InputManager:IsKeyPressed('4') then
        apply_ease(4)
    end

    if InputManager:IsKeyPressed('Q') then
        apply_ease(CurrentEaseIndex - 1)
    elseif InputManager:IsKeyPressed('E') then
        apply_ease(CurrentEaseIndex + 1)
    end
end

local function clamp_axis(value)
    if value < 0.0 then
        return 0.0
    end
    if value > 2.0 then
        return 2.0
    end
    return value
end

local function update_blend_inputs(deltaTime)
    handle_ease_hotkeys()

    if InputManager:IsKeyPressed('P') then
        bAutoDrive = not bAutoDrive
        local modeName = bAutoDrive and "Auto" or "Keyboard"
        print(string.format("[TestBlendSpace2D] Input Mode: %s", modeName))
    end

    -- if bAutoDrive then
    --     AutoTimer = AutoTimer + deltaTime * AutoSpeed
    --     InputX = math.sin(AutoTimer)
    --     InputY = math.cos(AutoTimer * 0.75)
    -- else
        local delta = deltaTime * 2.0
        if InputManager:IsKeyDown('F') then
            InputX = InputX + delta
        end
        if InputManager:IsKeyDown('H') then
            InputX = InputX - delta
        end
        if InputManager:IsKeyDown('T') then
            InputY = InputY + delta
        end
        if InputManager:IsKeyDown('G') then
            InputY = InputY - delta
        end
        print("X " .. InputX)
        print("Y " .. InputY)
    -- end

    InputX = clamp_axis(InputX)
    InputY = clamp_axis(InputY)

    if BlendSpaceNode then
        BlendSpaceNode:SetBlendInput(InputX, InputY)
    end
end

local function build_blend_space(state)
    BlendSpaceNode = state:CreateBlendSpace2DNode()
    if not BlendSpaceNode then
        return false
    end

    local axis = { 0.0, 1.0, 2.0 }
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

    add_sample(BackwardAnim, 1, 0)
    add_sample(IdleAnim,     1, 1)
    add_sample(ForwardAnim,  1, 2)
    add_sample(LeftAnim,     0, 1)
    add_sample(RightAnim,    2, 1)

    -- Fill diagonal cells with the closest directional clips to keep interpolation stable.
    add_sample(LeftBackAnim, 0, 0)
    add_sample(RightBackAnim, 2, 0)
    add_sample(LeftForwardAnim,  0, 2)
    add_sample(RightForwardAnim,  2, 2)

    apply_ease(CurrentEaseIndex)
    return true
end

function BeginPlay()
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[TestBlendSpace2D] No SkeletalMeshComponent found")
        return
    end

    ASM = AnimationStateMachine:new()
    ASM:initialize()

    IdleAnim     = LoadAnimationSequence("Idle_mixamo.com")
    ForwardAnim  = LoadAnimationSequence("Standard Walk_mixamo.com")
    BackwardAnim = LoadAnimationSequence("Walking Backward_mixamo.com")
    LeftAnim     = LoadAnimationSequence("Left Strafe Walk_mixamo.com")
    RightAnim    = LoadAnimationSequence("Right Strafe Walk_mixamo.com")

    FallingAnim = LoadAnimationSequence("Falling Idle_mixamo.com")

    LeftForwardAnim = LoadAnimationSequence("Jog Forward Diagonal Left_mixamo.com")
    RightForwardAnim = LoadAnimationSequence("Jog Forward Diagonal Right_mixamo.com")
    LeftBackAnim = LoadAnimationSequence("Jog Backward Diagonal Left_mixamo.com")
    RightBackAnim = LoadAnimationSequence("Jog Backward Diagonal Right_mixamo.com")

    if not (IdleAnim and ForwardAnim and BackwardAnim and LeftAnim and RightAnim and FallingAnim
            and LeftForwardAnim and RightForwardAnim and LeftBackAnim and RightBackAnim) then
        print("[TestBlendSpace2D] Failed to load required animations")
        return
    end

    local Locomotion = ASM:add_state(FName("Locomotion"))
    if not Locomotion then
        print("[TestBlendSpace2D] Failed to create state")
        return
    end

    if not build_blend_space(Locomotion) then
        print("[TestBlendSpace2D] Failed to build BlendSpace2D")
        return
    end

    Locomotion:SetEntryNode(BlendSpaceNode)

    SkeletalComp:SetAnimationModeInt(1)
    SkeletalComp:InitAnimInstance()

    print("[TestBlendSpace2D] 1~4 or Q/E change ease. Press P to toggle Auto vs Keyboard input. T/G/H/F move manually.")
end

function AnimUpdate(deltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    update_blend_inputs(deltaTime)

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
    -- Input is processed in AnimUpdate via update_blend_inputs.
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
    AutoTimer = 0.0
end
