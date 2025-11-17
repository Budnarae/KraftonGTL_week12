#pragma once

#include <set>
#include <functional>

class FSkeleton;
struct FBone;

/**
 * 재사용 가능한 본 계층 구조 트리 위젯
 *
 * 용도:
 * - 스켈레탈 메시 뷰어에서 본 트리 표시
 * - 애니메이션 뷰어에서 본 선택
 * - 기타 본 계층 구조 시각화가 필요한 곳
 *
 * 특징:
 * - ImGui TreeNode 기반 재귀 렌더링
 * - 펼침/접힘 상태 관리
 * - 선택 상태 하이라이트
 * - 선택 변경 콜백 지원
 */
class FBoneHierarchyWidget
{
public:
    FBoneHierarchyWidget();
    ~FBoneHierarchyWidget();

    /**
     * 본 계층 구조 트리를 렌더링합니다.
     *
     * @param Skeleton - 렌더링할 스켈레톤 데이터 (nullptr이면 에러 메시지 표시)
     * @param InOutSelectedBoneIndex - 현재 선택된 본 인덱스 (in/out, -1이면 선택 없음)
     * @param InOutExpandedIndices - 펼쳐진 본 인덱스 집합 (in/out, 상태 유지)
     * @return 선택이 변경되었으면 true
     */
    bool Render(
        const FSkeleton* Skeleton,
        int32& InOutSelectedBoneIndex,
        std::set<int32>& InOutExpandedIndices
    );

    /**
     * 특정 본까지의 경로를 모두 펼칩니다.
     *
     * @param BoneIndex - 타겟 본 인덱스
     * @param Skeleton - 스켈레톤 데이터
     * @param InOutExpandedIndices - 펼쳐진 본 인덱스 집합 (업데이트됨)
     */
    static void ExpandToNode(
        int32 BoneIndex,
        const FSkeleton* Skeleton,
        std::set<int32>& InOutExpandedIndices
    );

    /**
     * 본 선택 변경 시 호출될 콜백을 설정합니다.
     *
     * @param Callback - void(int32 NewBoneIndex) 형태의 콜백 함수
     */
    void SetOnBoneSelectedCallback(std::function<void(int32)> Callback);

    /**
     * 트리뷰 높이를 설정합니다 (0이면 남은 공간 전체 사용)
     *
     * @param Height - 픽셀 단위 높이
     */
    void SetTreeViewHeight(float Height);

private:
    /**
     * 재귀적으로 본 노드를 렌더링합니다.
     *
     * @param BoneIndex - 현재 렌더링할 본 인덱스
     * @param Bones - 전체 본 배열
     * @param Children - 부모-자식 관계 배열
     * @param InOutSelectedBoneIndex - 선택된 본 인덱스 (변경 가능)
     * @param InOutExpandedIndices - 펼쳐진 인덱스 집합 (변경 가능)
     * @param bOutSelectionChanged - 선택 변경 여부 출력
     */
    void RenderBoneNode(
        int32 BoneIndex,
        const TArray<FBone>& Bones,
        const TArray<TArray<int32>>& Children,
        int32& InOutSelectedBoneIndex,
        std::set<int32>& InOutExpandedIndices,
        bool& bOutSelectionChanged
    );

private:
    // 선택 변경 콜백
    std::function<void(int32)> OnBoneSelected;

    // 트리뷰 높이 (0 = 자동)
    float TreeViewHeight;
};
