AnimationStateMachine = {};
AnimationStateMachine.__index = AnimationStateMachine;

function AnimationStateMachine:new()
    local obj = {};
    
    obj.current_state = nil;
    obj.current_transition = nil;

    obj.bIsInTransition = false;

    obj.states = {};
    obj.transitions = {};
    
    setmetatable(obj, AnimationStateMachine);
    
    return obj
end

-- constructor
function AnimationStateMachine:initialize() end

-- destructor
function AnimationStateMachine:shutdown()
    -- state들을 전부 제거
    self.states = {};
    self.transitions = {};
end

-- state 추가
function AnimationStateMachine:add_state(state_name)
    -- type 검사
    if type(state_name) ~= "FName" then
        print("[Lua][AnimationStateMachine:add_state][Warning] Type of parameter should be FName.");
        return nil;
    end

    -- 같은 이름을 가진 State가 있으면 거부
    for state in ipairs(self.states) do
        if state.Name == state_name then
            print("[Lua][AnimationStateMachine:add_state][Warning] A state with the same name already exists in the Animation State Machine.");
            return nil;
        end
    end
    
    -- 내부에서 state 생성
    local state = FAnimState.new();
    state.Name = state_Name;
    state.Index = #self.states;
    table.insert(self.states, state);

    if self.current_state == nil then
        current_state = state;
    end
    
    return state;
end

function AnimationStateMachine:delete_state(target_name)
    if type(target_name) ~= "FName" then
        print("[Lua][AnimationStateMachine:delete_state][Warning] Type of parameter should be FName.");
        return;
    end

    local target_state = self:find_state_by_name(target_name);
    if target_state == nil then
        print("[Lua][AnimationStateMachine:find_state_by_name][Warning] The state you are trying to delete does not exist in the state machine.");
        return;
    end
    
    -- 해당 state를 참조하는 transition들을 먼저 제거
    for i = 1, #self.transitions do
        local transition = self.transitions[i];
        if transition.SourceState == target_state or transition.TargetState == target_state then
            transition:CleanupDelegete();
            table.remove(self.transitions, i);
        end
    end
    
    -- State 제거
    for i = 1, #self.states do
        local state = self.states[i];
        if state == target_state then
            table.remove(self.states, i);
        end
    end
    
    -- CurrentState가 삭제된 경우 
    -- state가 1개 이상이면 처음 state를 가리킨다.
    -- 그렇지 않다면 nil을 입력한다.
    if (self.current_state == target_state) then
        if #self.states > 0 then
            self.current_state = self.states[1];
        else
            self.current_state = nil;
        end
    end
end

-- 보유하고 있는 state 중 이름이 일치하는 state를 반환
function AnimationStateMachine:find_state_by_name(state_name)
    if type(state_name) ~= "FName" then
        print("[Lua][AnimationStateMachine:find_state_by_name][Warning] Type of parameter should be FName.");
        return nil;
    end

    for state in ipairs(self.states) do
        if state.Name == state_name then
            return state
        end
    end

    return nil;
end

function AnimationStateMachine:add_transition(source_name, target_name)
    if type(source_name) ~= "FName" or type(target_name) ~= "FName" then
        print("[Lua][AnimationStateMachine:add_transition][Warning] Type of parameter should be FName.");
        return nil;
    end
    
    local source_state = self:find_state_by_name(source_name);
    local target_state = self:find_state_by_name(target_name);

    if source_state == nil or target_state == nil then
        print("[Lua][AnimationStateMachine:add_transition][Warning] The target state you are trying to connect with this transition does not exist in the state machine.");
        return nil;
    end
    
    -- 이미 같은 노드에 대한 연결이 있으면 거부
    for transition in ipairs(self.transitions) do
        if (transition.SourceState == source_state and transition.TargetState == target_state) then
            print("[Lua][AnimationStateMachine:add_transition][Warning] A transition to the same target state already exists in the state machine.");
            return nil;
        end
    end
    
    -- 내부에서 transition 생성
    local transition = FAnimStateTransition.new();
    transition.SourceState = source_state;
    transition.TargetState = target_state;
    transition.index = #self.transitions;
    table.insert(self.transitions, transition);
    
    return transition;
end

function AnimationStateMachine:add_transition_with_rule(
        source_name,
        target_name,
        transition_rule
)
    -- transition 생성 및 처리가 정상적으로 이루어지지 않았다면 조기 반환
    local transition = self:add_transition(source_name, target_name);
    if transition == nil then
        return nil;
    end

    -- transition_rule이 유효하지 않다면 조기 반환
    if transition_rule == nil or type(transition_rule) ~= "UAnimNodeTransitionRule" then
        print("[Lua][AnimationStateMachine:add_transition_with_rule][Warning] TransitionRule is not valid.");
        return transition;
    end
    
    -- rule 연결 및 delegate 바인딩
    transition.AssociatedRule = transition_rule;
    -- to claude : 이 부분을 해결해주기 바람
    transition.DelegateHandle = transition_rule:GetTransitionDelegate():AddDynamic(
            transition, transition.TriggerTransition
    );
    
    return transition;
end

function AnimationStateMachine:delete_transition(source_name, target_name)
    -- 입력된 이름과 일치하는 state 찾기
    source_state = self:find_state_by_name(source_name);
    target_state = self:find_state_by_name(target_name);

    if (source_state == nil or target_state == nil) then
        print("[Lua][AnimationStateMachine:delete_transition][Warning] The transition you are trying to delete does not exist in the state machine.");
        return;
    end

    for i = 0, #self.transitions do
        local transition = self.transitions[i];
        if transition.SourceState == source_state and transition.TargetState == target_state then
            transition:CleanupDelegete();
            table.remove(self.transitions, transition);
            return;
        end
    end

    print("[Lua][AnimationStateMachine:delete_transition][Warning] The transition you are trying to delete does not exist in the state machine.");
end

return AnimationStateMachine;