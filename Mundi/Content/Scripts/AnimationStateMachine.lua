-- AnimationStateMachine.lua
-- Thin Lua wrapper around the native FAnimNode_StateMachine so gameplay code can
-- construct animation graphs entirely from bound C++ nodes.

local AnimationStateMachine = {}
AnimationStateMachine.__index = AnimationStateMachine

local function convert_to_fname(value)
    if value == nil then
        return nil
    end

    if type(value) == "string" then
        return FName(value)
    end

    return value
end

local function create_native_state_machine()
    if type(CreateAnimNodeStateMachine) == "function" then
        return CreateAnimNodeStateMachine()
    end

    local TypeInfo = FAnimNode_StateMachine
    if TypeInfo ~= nil then
        if type(TypeInfo) == "table" and TypeInfo.new then
            return TypeInfo.new()
        end
        if type(TypeInfo) == "function" then
            return TypeInfo()
        end
    end

    error("[Lua][AnimationStateMachine] Unable to create FAnimNode_StateMachine instance. Binding missing?")
end

function AnimationStateMachine:new()
    local obj = {}
    obj.NativeStateMachine = create_native_state_machine()
    obj.states = {}
    obj.transitions = {}
    obj.current_state = nil
    obj.current_transition = nil
    obj.bIsInTransition = false

    setmetatable(obj, AnimationStateMachine)

    obj:_sync_native_state()

    return obj
end

function AnimationStateMachine:initialize()
end

function AnimationStateMachine:shutdown()
    self.NativeStateMachine = nil
    self.states = {}
    self.transitions = {}
    self.current_state = nil
    self.current_transition = nil
    self.bIsInTransition = false
end

function AnimationStateMachine:_sync_native_state()
    if not self.NativeStateMachine then
        self.current_state = nil
        self.current_transition = nil
        self.bIsInTransition = false
        return
    end

    self.current_state = self.NativeStateMachine.CurrentState
    self.current_transition = self.NativeStateMachine.CurrentTransition
    self.bIsInTransition = self.NativeStateMachine.bIsInTransition == true
end

function AnimationStateMachine:get_transitions()
    return self.transitions
end

function AnimationStateMachine:reset_transition_flags()
    if not self.NativeStateMachine then
        return
    end

    if self.NativeStateMachine.ResetTransitionFlags then
        self.NativeStateMachine:ResetTransitionFlags()
    end

    for _, transition in ipairs(self.transitions) do
        if transition then
            transition.CanEnterTransition = false
        end
    end
end

function AnimationStateMachine:update(context)
    if not self.NativeStateMachine then
        return
    end

    for _, transition in ipairs(self.transitions) do
        if transition and transition.Update then
            transition:Update(context)
        end
    end

    self.NativeStateMachine:Update(context)
    self:_sync_native_state()
end

function AnimationStateMachine:evaluate(output)
    if not self.NativeStateMachine then
        return
    end

    self.NativeStateMachine:Evaluate(output)
end

function AnimationStateMachine:add_state(state_name)
    if not self.NativeStateMachine then
        return nil
    end

    local name = convert_to_fname(state_name)
    if not name then
        return nil
    end

    local state = self.NativeStateMachine:AddState(name)
    if state then
        table.insert(self.states, state)
        self:_sync_native_state()
    end
    return state
end

function AnimationStateMachine:delete_state(target_name)
    if not self.NativeStateMachine then
        return
    end

    local name = convert_to_fname(target_name)
    if not name then
        return
    end

    local target_state = self:find_state_by_name(name)

    self.NativeStateMachine:DeleteState(name)

    if target_state then
        for i = #self.states, 1, -1 do
            if self.states[i] == target_state then
                table.remove(self.states, i)
                break
            end
        end

        for i = #self.transitions, 1, -1 do
            local transition = self.transitions[i]
            if transition and (transition.SourceState == target_state or transition.TargetState == target_state) then
                table.remove(self.transitions, i)
            end
        end
    end

    self:_sync_native_state()
end

function AnimationStateMachine:find_state_by_name(state_name)
    local name = convert_to_fname(state_name)
    if not name then
        return nil
    end

    for _, state in ipairs(self.states) do
        if state and state.Name == name then
            return state
        end
    end

    return nil
end

function AnimationStateMachine:add_transition(source_name, target_name)
    if not self.NativeStateMachine then
        return nil
    end

    local source = convert_to_fname(source_name)
    local target = convert_to_fname(target_name)
    if not source or not target then
        return nil
    end

    local transition = self.NativeStateMachine:AddTransition(source, target)
    if transition then
        table.insert(self.transitions, transition)
    end
    return transition
end

function AnimationStateMachine:add_transition_with_rule(source_name, target_name, transition_rule)
    return self:add_transition(source_name, target_name)
end

function AnimationStateMachine:delete_transition(source_name, target_name)
    if not self.NativeStateMachine then
        return
    end

    local source = convert_to_fname(source_name)
    local target = convert_to_fname(target_name)

    if not source or not target then
        return
    end

    local source_state = self:find_state_by_name(source)
    local target_state = self:find_state_by_name(target)

    self.NativeStateMachine:DeleteTransition(source, target)

    if source_state and target_state then
        for i = #self.transitions, 1, -1 do
            local transition = self.transitions[i]
            if transition and transition.SourceState == source_state and transition.TargetState == target_state then
                table.remove(self.transitions, i)
            end
        end
    end
end

_G.AnimationStateMachine = AnimationStateMachine

return AnimationStateMachine
