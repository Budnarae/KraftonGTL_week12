-- TestAdditiveBlend.lua
-- Demonstrates how to layer an additive animation on top of a base pose from Lua.

local AnimationStateMachine = dofile("Content/Scripts/AnimationStateMachine.lua")

local ASM = nil
local SkeletalComp = nil
local AdditiveBlendNode = nil
local BasePoseNode = nil
local AdditivePoseNode = nil

local BaseAnimation = nil
local OverlayAnimation = nil

local AlphaTarget = 0.0
local CurrentAlpha = 0.0
local AlphaAdjustSpeed = 1.5
local AlphaLerpSpeed = 6.0

local function build_graph(state)
    BasePoseNode = state:CreateSequenceNode(BaseAnimation, true)
    AdditivePoseNode = state:CreateSequenceNode(OverlayAnimation, true)
    if not BasePoseNode or not AdditivePoseNode then
        return false
    end

    AdditiveBlendNode = state:CreateAdditiveBlendNode()
    if not AdditiveBlendNode then
        return false
    end

    AdditiveBlendNode:SetBasePose(BasePoseNode)
    AdditiveBlendNode:SetAdditivePose(AdditivePoseNode)
    AdditiveBlendNode.Alpha = 0.0

    state:SetEntryNode(AdditiveBlendNode)
    return true
end

local function update_alpha(deltaTime)
    if not AdditiveBlendNode then
        return
    end

    local delta = deltaTime * AlphaAdjustSpeed
    if InputManager:IsKeyDown('J') then
        AlphaTarget = math.max(0.0, AlphaTarget - delta)
    end
    if InputManager:IsKeyDown('L') then
        AlphaTarget = math.min(1.0, AlphaTarget + delta)
    end

    if InputManager:IsKeyPressed('K') then
        AlphaTarget = 0.0
    elseif InputManager:IsKeyPressed('O') then
        AlphaTarget = 1.0
    end

    local lerp = math.min(deltaTime * AlphaLerpSpeed, 1.0)
    CurrentAlpha = CurrentAlpha + (AlphaTarget - CurrentAlpha) * lerp
    AdditiveBlendNode.Alpha = CurrentAlpha
end

function BeginPlay()
    SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalComp then
        print("[TestAdditiveBlend] No SkeletalMeshComponent found")
        return
    end

    ASM = AnimationStateMachine:new()
    ASM:initialize()

    -- Base pose uses a relaxed idle while the overlay loops a more energetic clip.
    BaseAnimation = LoadAnimationSequence("Standard Run_mixamo.com")
    OverlayAnimation = LoadAnimationSequence("Standard Walk_mixamo.com")

    if not BaseAnimation or not OverlayAnimation then
        print("[TestAdditiveBlend] Failed to load animation clips")
        return
    end

    local state = ASM:add_state(FName("AdditiveDemo"))
    if not state then
        print("[TestAdditiveBlend] Failed to create animation state")
        return
    end

    build_graph(state)

    SkeletalComp:SetAnimationModeInt(1)
    SkeletalComp:InitAnimInstance()

    print("[TestAdditiveBlend] Hold L/J to raise/lower additive weight, press O/K for max/min")
end

function AnimUpdate(deltaTime)
    if not ASM or not ASM.current_state then
        return nil
    end

    update_alpha(deltaTime)

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
    -- Alpha adjustments are handled inside AnimUpdate via update_alpha.
end

function EndPlay()
    if ASM then
        ASM:shutdown()
    end

    ASM = nil
    SkeletalComp = nil
    AdditiveBlendNode = nil
    BasePoseNode = nil
    AdditivePoseNode = nil
    BaseAnimation = nil
    OverlayAnimation = nil
    AlphaTarget = 0.0
    CurrentAlpha = 0.0
end
