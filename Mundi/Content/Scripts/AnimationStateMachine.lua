-- AnimationStateMachine (Lua implementation)
-- C++의 FAnimNode_StateMachine 구조를 Lua로 구현

AnimationStateMachine = {};
AnimationStateMachine.__index = AnimationStateMachine;

function AnimationStateMachine:new()
    local obj = {};

    obj.current_state = nil;
    obj.current_transition = nil;

    obj.bIsInTransition = false;

    -- TransitionBlendNode 역할을 할 필드들
    obj.TransitionElapsed = 0.0;
    obj.TransitionDuration = 0.0;
    obj.BlendFrom = nil;  -- Source State의 첫 번째 AnimSequenceNode
    obj.BlendTo = nil;    -- Target State의 첫 번째 AnimSequenceNode
    obj.BlendAlpha = 0.0;

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

    -- 모든 Transition의 조건 평가 (각 Transition이 Lua 함수를 호출)
    for i, transition in ipairs(self.transitions) do
        if transition then
            transition:Update(context)
        end
    end

    if self.bIsInTransition then
        if not self.current_transition then
            return
        end

        -- TransitionBlendNode.Update() 역할: From, To 모두 업데이트
        if self.BlendFrom and self.BlendFrom.Update then
            self.BlendFrom:Update(context)
        end
        if self.BlendTo and self.BlendTo.Update then
            self.BlendTo:Update(context)
        end

        -- Alpha 계산
        self.TransitionElapsed = self.TransitionElapsed + context.DeltaTime
        local Alpha = 1.0
        if self.TransitionDuration > 0.0 then
            Alpha = math.min(self.TransitionElapsed / self.TransitionDuration, 1.0)
            Alpha = math.max(Alpha, 0.0)
        end
        self.BlendAlpha = Alpha

        -- Transition 완료 처리
        if Alpha >= 1.0 then
            if self.current_transition and self.current_transition.TargetState then
                self.current_state = self.current_transition.TargetState
            end

            self.bIsInTransition = false
            self.current_transition = nil
            self.TransitionElapsed = 0.0
            self.TransitionDuration = 0.0
        end
    else
        if not self.current_state or #self.current_state.AnimSequenceNodes == 0 then
            return
        end

        -- 조건을 충족한 Transition이 있으면 State를 변환
        for i, transition in ipairs(self.transitions) do
            if not transition then
                goto continue
            end

            if transition.CanEnterTransition and transition.SourceState == self.current_state then
                -- --- 전이 시작 설정 ---
                self.current_transition = transition
                self.bIsInTransition = true
                self.TransitionElapsed = 0.0
                self.TransitionDuration = transition.BlendTime

                -- Source/Target의 첫 번째 시퀀스 노드
                local SourceNode = nil
                local TargetNode = nil

                if #self.current_state.AnimSequenceNodes > 0 then
                    SourceNode = self.current_state.AnimSequenceNodes[1]
                end

                if transition.TargetState and #transition.TargetState.AnimSequenceNodes > 0 then
                    TargetNode = transition.TargetState.AnimSequenceNodes[1]
                end

                -- Phase 동기화 (SourceNode의 진행률을 TargetNode에 적용)
                if SourceNode and TargetNode and SourceNode.Sequence and TargetNode.Sequence then
                    local SrcLen = SourceNode.Sequence:GetPlayLength()
                    local DstLen = TargetNode.Sequence:GetPlayLength()

                    if SrcLen > 0.0 and DstLen > 0.0 then
                        local Phase = SourceNode.CurrentTime / SrcLen
                        Phase = math.min(math.max(Phase, 0.0), 1.0)
                        TargetNode.CurrentTime = Phase * DstLen
                    end
                end

                -- 블렌드 노드 구성
                self.BlendFrom = SourceNode
                self.BlendTo = TargetNode
                self.BlendAlpha = 0.0

                return  -- 이번 프레임은 전이 세팅까지만
            end

            ::continue::
        end

        -- 전이 없다 -> 현재 상태의 첫 번째 AnimSequence만 Update
        if #self.current_state.AnimSequenceNodes > 0 then
            local SeqNode = self.current_state.AnimSequenceNodes[1]
            if SeqNode and SeqNode.Update then
                SeqNode:Update(context)
            end
        end
    end
end

-- Evaluate: 최종 Pose 계산
function AnimationStateMachine:evaluate(output)
    if not self.current_state then
        return
    end

    -- Transition 중이면 블렌딩된 Pose 계산
    if self.bIsInTransition and self.current_transition then
        -- TransitionBlendNode.Evaluate() 역할: From, To 평가 후 블렌딩
        if not self.BlendFrom or not self.BlendTo then
            return
        end

        -- From, To 각각 평가
        local PoseFrom = FPoseContext()
        local PoseTo = FPoseContext()

        if output.Skeleton then
            PoseFrom.Skeleton = output.Skeleton
            PoseTo.Skeleton = output.Skeleton
            PoseFrom.EvaluatedPoses:SetNum(output.EvaluatedPoses:Num())
            PoseTo.EvaluatedPoses:SetNum(output.EvaluatedPoses:Num())
        end

        if self.BlendFrom.Evaluate then
            self.BlendFrom:Evaluate(PoseFrom)
        end
        if self.BlendTo.Evaluate then
            self.BlendTo:Evaluate(PoseTo)
        end

        -- 블렌딩: Lerp(From, To, Alpha)
        local BoneCnt = math.min(PoseFrom.EvaluatedPoses:Num(), PoseTo.EvaluatedPoses:Num())
        output.EvaluatedPoses:SetNum(BoneCnt)

        for i = 1, BoneCnt do
            local TransformFrom = PoseFrom.EvaluatedPoses:Get(i)
            local TransformTo = PoseTo.EvaluatedPoses:Get(i)

            if TransformFrom and TransformTo then
                local BlendedTransform = FTransform.Lerp(TransformFrom, TransformTo, self.BlendAlpha)
                output.EvaluatedPoses:Set(i, BlendedTransform)
            end
        end
    else
        -- 일반 상태: CurrentState의 첫 번째 AnimSequence 평가
        if not self.current_state or #self.current_state.AnimSequenceNodes == 0 then
            return
        end

        local SequenceToEvaluate = self.current_state.AnimSequenceNodes[1]
        if SequenceToEvaluate and SequenceToEvaluate.Evaluate then
            SequenceToEvaluate:Evaluate(output)
        end
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