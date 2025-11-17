-- AnimationStateMachine (Lua implementation)
-- C++의 FAnimNode_StateMachine 구조를 Lua로 구현

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

-- Update: 내부 상태 업데이트 (StateMachine 전이, 시퀀스 재생 시간 증가 등)
function AnimationStateMachine:update(context)
    if not self.current_state then
        return
    end

    -- 모든 Transition의 조건을 평가
    for i, transition in ipairs(self.transitions) do
        if transition then
            transition:Update(context)
        end
    end

    if self.bIsInTransition then
        if not self.current_transition then
            return
        end

        -- Transition 중에도 TargetState 애니메이션 업데이트
        if self.current_transition.TargetState then
            self.current_transition.TargetState:Update(context)
        end

        -- Transition 완료 처리
        if not self.current_transition.bIsBlending then
            self.current_state = self.current_transition.TargetState
            self.bIsInTransition = false
            self.current_transition.CanEnterTransition = false
            self.current_transition = nil
        end
    else
        -- 조건을 충족한 Transition이 있으면 State를 변환
        for i, transition in ipairs(self.transitions) do
            if transition and transition.CanEnterTransition and transition.SourceState == self.current_state then
                self.current_transition = transition
                self.bIsInTransition = true
                transition:StartBlending()
                return
            end
        end

        -- CurrentState의 애니메이션 노드 업데이트
        self.current_state:Update(context)
    end
end

-- Evaluate: 최종 Pose 계산
function AnimationStateMachine:evaluate(output)
    if not self.current_state then
        return
    end

    -- Transition 중이면 TargetState를 평가, 아니면 CurrentState를 평가
    if self.bIsInTransition then
        -- Transition 중: TargetState의 첫 번째 AnimSequence 평가
        if self.current_transition and self.current_transition.TargetState then
            self.current_transition.TargetState:Evaluate(output)
        end
    else
        -- 일반 상태: CurrentState의 첫 번째 AnimSequence 평가
        self.current_state:Evaluate(output)
    end
end

-- state 추가
function AnimationStateMachine:add_state(state_name)
    -- 같은 이름을 가진 State가 있으면 거부
    for i, state in ipairs(self.states) do
        if state.Name == state_name then
            print("[Lua][AnimationStateMachine:add_state][Warning] A state with the same name already exists in the Animation State Machine.");
            return nil;
        end
    end

    -- 내부에서 state 생성
    local state = FAnimState();
    state.Name = state_name;
    state.Index = #self.states;
    table.insert(self.states, state);

    if self.current_state == nil then
        self.current_state = state;
    end

    return state;
end

function AnimationStateMachine:delete_state(target_name)
    local target_state = self:find_state_by_name(target_name);
    if target_state == nil then
        print("[Lua][AnimationStateMachine:find_state_by_name][Warning] The state you are trying to delete does not exist in the state machine.");
        return;
    end
    
    -- 해당 state를 참조하는 transition들을 먼저 제거
    for i = #self.transitions, 1, -1 do
        local transition = self.transitions[i];
        if transition.SourceState == target_state or transition.TargetState == target_state then
            table.remove(self.transitions, i);
        end
    end
    
    -- State 제거
    for i = #self.states, 1, -1 do
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
    for i, state in ipairs(self.states) do
        if state.Name == state_name then
            return state
        end
    end

    return nil;
end

function AnimationStateMachine:add_transition(source_name, target_name)
    local source_state = self:find_state_by_name(source_name);
    local target_state = self:find_state_by_name(target_name);

    if source_state == nil or target_state == nil then
        print("[Lua][AnimationStateMachine:add_transition][Warning] The target state you are trying to connect with this transition does not exist in the state machine.");
        return nil;
    end
    
    -- 이미 같은 노드에 대한 연결이 있으면 거부
    for i, transition in ipairs(self.transitions) do
        if (transition.SourceState == source_state and transition.TargetState == target_state) then
            print("[Lua][AnimationStateMachine:add_transition][Warning] A transition to the same target state already exists in the state machine.");
            return nil;
        end
    end
    
    -- 내부에서 transition 생성
    local transition = FAnimStateTransition();
    transition.SourceState = source_state;
    transition.TargetState = target_state;
    transition.Index = #self.transitions;  -- 대문자 I
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
    if transition_rule == nil then
        print("[Lua][AnimationStateMachine:add_transition_with_rule][Warning] TransitionRule is not valid.");
        return transition;
    end

    -- Rule 방식은 폐기됨 - sol::function 방식 사용
    -- Lua에서 SetTransitionCondition()으로 조건 함수 설정 필요
    -- transition:SetTransitionCondition(function() return true end)

    return transition;
end

function AnimationStateMachine:delete_transition(source_name, target_name)
    -- 입력된 이름과 일치하는 state 찾기
    local source_state = self:find_state_by_name(source_name);
    local target_state = self:find_state_by_name(target_name);

    if (source_state == nil or target_state == nil) then
        print("[Lua][AnimationStateMachine:delete_transition][Warning] The transition you are trying to delete does not exist in the state machine.");
        return;
    end

    for i = #self.transitions, 1, -1 do
        local transition = self.transitions[i];
        if transition.SourceState == source_state and transition.TargetState == target_state then
            table.remove(self.transitions, i);
            return;
        end
    end

    print("[Lua][AnimationStateMachine:delete_transition][Warning] The transition you are trying to delete does not exist in the state machine.");
end

return AnimationStateMachine;