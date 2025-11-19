-- TestBlendSpace1D.lua
-- Simple 1D blend space that mirrors the ease-mode controls from the 2D sample.

local AnimationStateMachine = dofile("Content/Scripts/AnimationStateMachine.lua")

local ASM = nil
local SkeletalComp = nil
local BlendSpaceNode = nil

local InputValue = 0.0
local AutoTimer = 0.0
local bAutoDrive = true
local AutoSpeed = 1.25

local EaseModes = {
    { name = "Linear", type = EAnimBlendEaseType.Linear },
    { name = "EaseIn", type = EAnimBlendEaseType.EaseIn },
    { name = "EaseOut", type = EAnimBlendEaseType.EaseOut },
    { name = "EaseInOut", type = EAnimBlendEaseType.EaseInOut },
}
local CurrentEaseIndex = 1

local IdleAnim = nil
local WalkAnim = nil
local RunAnim = nil

local function apply_ease(index)
    if not BlendSpaceNode then
        return
    end

    local count = #EaseModes
    CurrentEaseIndex = ((index - 1) % count) + 1

    local mode = EaseModes[CurrentEaseIndex]
    BlendSpaceNode.EaseFunction = mode.type
    print(string.format("[TestBlendSpace1D] Ease Mode: %s", mode.name))
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

local function update_blend_input(deltaTime)
    handle_ease_hotkeys()

    if InputManager:IsKeyPressed('P') then
        bAutoDrive = not bAutoDrive
        local modeName = bAutoDrive and "Auto" or "Keyboard"
        print(string.format("[TestBlendSpace1D] Input Mode: %s", modeName))
    end

    if bAutoDrive then
        AutoTimer = AutoTimer + deltaTime * AutoSpeed
        InputValue = 0.5 + 0.5 * math.sin(AutoTimer)
    else
        local delta = deltaTime * 1.5
        if InputManager:IsKeyDown('L') then
            InputValue = InputValue + delta
        end
        if InputManager:IsKeyDown('J') then
            InputValue = InputValue - delta
        end
    end

    if InputValue < 0.0 then
        InputValue = 0.0
    elseif InputValue > 1.0 then
        InputValue = 1.0
    end

    if BlendSpaceNode then
        BlendSpaceNode:SetBlendInput(InputValue)
    end
end

local function build_blend_space(state)
    BlendSpaceNode = state:CreateBlendSpace1DNode()
    if not BlendSpaceNode then
        return false
    end

    -- BlendSpaceNode:SetMinimumPosition(BlendSpaceNode, 0.0)
    -- BlendSpaceNode:SetMaximumPosition(BlendSpaceNode, 1.0)

    local function add_sample(anim, position)
        if not anim then
            return
        end
        local node = state:CreateSequenceNode(anim, true)
        if node then
            BlendSpaceNode:AddSample(node, position)
        end
    end

    add_sample(IdleAnim, 0.0)
    add_sample(WalkAnim, 0.5)
    add_sample(RunAnim, 1.0)

    apply_ease(CurrentEaseIndex)
    return true
end

function BeginPlay()
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[TestBlendSpace1D] No SkeletalMeshComponent found")
        return
    end

    ASM = AnimationStateMachine:new()
    ASM:initialize()

    IdleAnim = LoadAnimationSequence("Standard Idle_mixamo.com")
    WalkAnim = LoadAnimationSequence("Standard Walk_mixamo.com")
    RunAnim  = LoadAnimationSequence("Standard Run_mixamo.com")

    if not (IdleAnim and WalkAnim and RunAnim) then
        print("[TestBlendSpace1D] Failed to load required animations")
        return
    end

    local Locomotion = ASM:add_state(FName("SpeedBlend"))
    if not Locomotion then
        print("[TestBlendSpace1D] Failed to create state")
        return
    end

    build_blend_space(Locomotion)

    Locomotion:SetEntryNode(BlendSpaceNode)

    SkeletalComp:SetAnimationModeInt(1)
    SkeletalComp:InitAnimInstance()

    print("[TestBlendSpace1D] L/J scrub the speed. 1~4 or Q/E change the ease. Press P to toggle Auto vs Keyboard mode.")
end

function AnimUpdate(deltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    update_blend_input(deltaTime)

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
    -- Input is processed during AnimUpdate.
end

function EndPlay()
    if ASM then
        ASM:shutdown()
    end

    ASM = nil
    SkeletalComp = nil
    BlendSpaceNode = nil
    IdleAnim = nil
    WalkAnim = nil
    RunAnim = nil
    AutoTimer = 0.0
end
