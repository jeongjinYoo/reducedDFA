#ifndef REGEX_AUTOMATA_H
#define REGEX_AUTOMATA_H

/*
  RegexAutomata.h
  
  정규표현식을 자동기계(NFA/DFA)로 변환하는 라이브러리입니다.
  
  주요 기능:
  1. Infix 표현식 → Postfix 표현식 변환 (Shunting Yard 알고리즘)
  2. Postfix 표현식 → 구문 트리(Syntax Tree) 변환
  3. 구문 트리 → ε-NFA 변환 (Thompson 구성)
  4. ε-NFA → DFA 변환 (부분집합 구성법)
  5. DFA 최소화 (분할 기반 최소화)
  6. Graphviz DOT, D3 JSON 형식으로 내보내기
  
  사용 예시:
    char expr[] = "a+b";
    char* postfix = infix_to_postfix(expr);
    TreeNode* tree = postfix_to_tree(postfix);
    NFA nfa = build_nfa_from_tree(tree);
    DFA dfa = build_dfa_from_nfa(nfa);
    DFA reduced = minimize_dfa(dfa);
*/

#include <iostream>
#include <ctype.h>
#include <cstring>
#include <stack>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

using namespace std;

// ============================================================================
// 구조체 정의
// ============================================================================

/**
  TreeNode: 구문 트리(Syntax Tree)의 노드
  
  정규표현식을 트리 구조로 표현합니다.
  - 리프 노드: 알파벳 문자(a-z, A-Z, 0-9)
  - 내부 노드: 연산자('+' 합집합, '.' 연결, '*' 클린 스타)
  
  예: "a+b"의 트리
      +
     / \
    a   b
*/
struct TreeNode {
    char value;           // 노드 값 (문자 또는 연산자)
    TreeNode* left;       // 왼쪽 자식 노드
    TreeNode* right;      // 오른쪽 자식 노드
    
    TreeNode(char val) : value(val), left(NULL), right(NULL) {}
};

// ============================================================================
// Infix → Postfix 변환 함수
// ============================================================================

/**
  op_precedence(char op)
  
  연산자의 우선순위를 반환합니다.
  - '+' (합집합): 우선순위 1 (가장 낮음)
  - '.' (연결): 우선순위 2
  - '*' (클린 스타): 우선순위 3 (가장 높음)
  
  반환값: 우선순위 (높을수록 먼저 계산), 연산자 아닙면 -1
*/
int op_precedence(char op);

/**
  char* infix_to_postfix(char* expression)
  
  Infix 표현식을 Postfix 표현식으로 변환합니다.
  (Shunting Yard 알고리즘 사용)
  
  입력: "a+b", "(a.b)+c*" 등의 infix 표현식
  반환: 동적 할당된 postfix 문자열
        (사용 후 delete[]로 메모리 해제 필요)
  
  예: "a+b" → "ab+"
*/
char* infix_to_postfix(char* expression);

// ============================================================================
// 트리 구성 및 조작 함수
// ============================================================================

/**
  TreeNode* postfix_to_tree(char* postfix)
  
  Postfix 표현식을 구문 트리로 변환합니다.
  스택을 이용하여 리프부터 트리를 상향식으로 구축합니다.
  
  입력: "ab+" 같은 postfix 표현식
  반환: 트리의 루트 노드 포인터
        (사용 후 delete_tree()로 메모리 해제 필요)
  
  예: "ab+" → 합집합 트리 구성
*/
TreeNode* postfix_to_tree(char* postfix);

/**
  void delete_tree(TreeNode* node)
  
  트리와 모든 노드의 메모리를 해제합니다.
  후위 순회(postorder)로 자식을 먼저 삭제한 후 부모를 삭제합니다.
*/
void delete_tree(TreeNode* node);

/**
  void print_tree_simple(TreeNode* node, int depth)
  
  트리를 인덴테이션으로 간단히 출력합니다.
  depth: 재귀 깊이 (자동으로 증가)
*/
void print_tree_simple(TreeNode* node, int depth = 0);

/**
  int get_tree_height(TreeNode* node)
  
  트리의 높이를 계산합니다.
  반환: 빈 트리는 0, 단일 노드는 1
*/
int get_tree_height(TreeNode* node);

/**
  int tree_size(TreeNode* node)
  
  트리의 총 노드 개수를 반환합니다.
*/
int tree_size(TreeNode* node);

/**
  int tree_leaf_count(TreeNode* node)
  
  트리의 리프 노드 개수를 반환합니다.
  (알파벳 문자 개수)
*/
int tree_leaf_count(TreeNode* node);

// ============================================================================
// NFA 상태 통계 및 JSON 내보내기
// ============================================================================

/**
  NFAStats: NFA의 상태와 호(arc)의 통계
  
  - states: NFA가 생성할 총 상태 개수
  - arcs: 총 호(전환) 개수
  - epsilon_arcs: ε-전환의 개수
*/
struct NFAStats {
    int states;
    int arcs;
    int epsilon_arcs;
};

/**
  NFAStats compute_nfa_stats(TreeNode* node)
  
  주어진 트리로부터 생성될 NFA의 상태와 호 개수를 계산합니다.
  Thompson 구성 규칙을 따릅니다.
*/
NFAStats compute_nfa_stats(TreeNode* node);

/**
  int nfa_state_upper_bound(int size)
  int nfa_arc_upper_bound(int size)
  
  트리 크기로부터 NFA의 상태/호 상한성을 계산합니다.
  (실제 값보다 클 수 있음)
*/
int nfa_state_upper_bound(int size);
int nfa_arc_upper_bound(int size);

/**
  string tree_to_d3_json(TreeNode* node)
  
  트리를 D3 라이브러리 형식의 JSON으로 변환합니다.
  반환: 계층 구조를 나타내는 JSON 문자열
*/
string tree_to_d3_json(TreeNode* node);

/**
  bool write_tree_json(TreeNode* node, const string& filename)
  
  트리를 JSON 파일로 저장합니다.
  생성된 파일은 tree_viz.html에서 시각화 가능합니다.
  반환: 성공 여부
*/
bool write_tree_json(TreeNode* node, const string& filename);

// ============================================================================
// ε-NFA 구조 및 함수
// ============================================================================

/**
  Transition: NFA의 상태 전환
  
  - from: 출발 상태 (상태 ID)
  - to: 도착 상태 (상태 ID)
  - symbol: 전환 기호
           0: ε-전환 (입력 없이 진행)
           'a'-'z', 'A'-'Z', '0'-'9': 입력 문자
  
  예: Transition{0, 1, 'a'} = 상태 0에서 문자 'a'로 상태 1로 전환
      Transition{1, 2, 0} = 상태 1에서 ε-전환으로 상태 2로 전환
*/
struct Transition {
    int from;
    int to;
    char symbol;  // 0은 ε-전환
};

/**
  NFA: 비결정적 유한 자동기계(Non-deterministic Finite Automaton)
  
  - n_states: 상태 개수 (상태 ID는 0 ~ n_states-1)
  - alphabet: 입력 알파벳 집합
  - start: 시작 상태 ID
  - accepts: 수락(인정) 상태 ID들의 목록
  - transitions: 모든 상태 전환 목록
  
  특징: ε-전환을 포함할 수 있음
        같은 입력에 대해 여러 전환 가능
*/
struct NFA {
    int n_states;
    set<char> alphabet;
    int start;
    vector<int> accepts;
    vector<Transition> transitions;
};

/**
  NFA build_nfa_from_tree(TreeNode* root)
  
  구문 트리로부터 Thompson 구성을 사용하여 ε-NFA를 구축합니다.
  
  Thompson 구성 규칙:
  1. 문자 'a': 두 상태 s→t, 'a' 전환
  2. 합집합 e1|e2: 새 시작 상태에서 e1, e2로 각각 ε-전환
  3. 연결 e1.e2: e1의 수락 상태에서 e2의 시작 상태로 ε-전환
  4. 클린 스타 e*: 루프 구조 (0회 이상 반복)
  
  반환: 구축된 NFA 구조체
*/
NFA build_nfa_from_tree(TreeNode* root);

/**
  void print_nfa(const NFA& nfa)
  
  NFA의 상태 정보와 전환을 표준 출력으로 인쇄합니다.
  
  출력 형식:
    NFA states: N
    Start: S
    Accepts: A1 A2 ...
    Alphabet: c1 c2 ...
    Transitions:
    From  To  Symbol
    ...
*/
void print_nfa(const NFA& nfa);

/**
  string nfa_to_dot(const NFA& nfa)
  
  NFA를 Graphviz DOT 형식으로 변환합니다.
  반환: DOT 형식의 그래프 정의 문자열
*/
string nfa_to_dot(const NFA& nfa);

/**
  bool write_nfa_dot(const NFA& nfa, const string& filename)
  
  NFA를 DOT 파일로 저장합니다.
  생성된 파일은 nfa_viz.html에서 시각화 가능합니다.
  반환: 성공 여부
*/
bool write_nfa_dot(const NFA& nfa, const string& filename);

// ============================================================================
// DFA 구조 및 함수
// ============================================================================

/**
  DFATransition: DFA의 상태 전환
  
  - from: 출발 상태
  - to: 도착 상태
  - symbol: 입력 문자 (ε-전환 없음)
  
  DFA 특징: 각 상태에서 각 입력에 대해 최대 하나의 전환만 존재
*/
struct DFATransition {
    int from;
    int to;
    char symbol;
};

/**
  DFA: 결정적 유한 자동기계(Deterministic Finite Automaton)
  
  - n_states: 상태 개수
  - alphabet: 입력 알파벳 집합
  - start: 시작 상태 ID
  - accepts: 수락 상태 ID들의 목록
  - transitions: 모든 상태 전환 목록
  
  특징: ε-전환이 없음
        같은 입력에 대해 정확히 하나의 전환만 존재
*/
struct DFA {
    int n_states;
    set<char> alphabet;
    int start;
    vector<int> accepts;
    vector<DFATransition> transitions;
};

/**
  DFA build_dfa_from_nfa(const NFA& nfa)
  
  ε-NFA를 DFA로 변환합니다. (부분집합 구성법 사용)
  
  알고리즘:
  1. NFA의 시작 상태의 ε-폐포를 DFA의 시작 상태로 설정
  2. 각 DFA 상태에 대해, 모든 입력에 대한 전환 계산
     - NFA 상태 집합의 모든 상태에서 입력 가능한 상태들을 모음
     - 그 상태들의 ε-폐포를 취함
  3. NFA의 수락 상태를 포함하는 DFA 상태를 수락 상태로 표시
  
  반환: DFA 상태 개수는 NFA보다 작거나 같음 (많은 경우 더 작음)
*/
DFA build_dfa_from_nfa(const NFA& nfa);

/**
  DFA minimize_dfa(const DFA& dfa)
  
  DFA를 최소화합니다. (도달 불가능한 상태 제거 & 동치 상태 병합)
  
  알고리즘: Hopcroft 또는 DFA 최소화 표 채우기
  1. 수락 상태와 비수락 상태로 초기 분할
  2. 각 분할에 대해:
     - 같은 입력에서 같은 분할으로 가는 상태들을 같은 그룹으로 유지
     - 다른 분할으로 가는 상태들은 분할
  3. 분할이 더 이상 변하지 않을 때까지 반복
  4. 각 분할을 하나의 상태로 축약
  
  반환: 상태 개수가 원래 DFA 이하인 최소 DFA
*/
DFA minimize_dfa(const DFA& dfa);

/**
  void print_dfa(const DFA& dfa)
  
  DFA의 상태 정보와 전환을 표준 출력으로 인쇄합니다.
  print_nfa()와 유사한 형식
*/
void print_dfa(const DFA& dfa);

/**
  void print_dfa_table(const DFA& dfa)
  
  DFA의 전환을 표 형식으로 인쇄합니다.
  
  출력 형식:
    State  a  b  c
    0      1  -  2
    1      -  3  -
    ...
  
  (-)는 정의되지 않은 전환
*/
void print_dfa_table(const DFA& dfa);

/**
  string dfa_to_dot(const DFA& dfa)
  
  DFA를 Graphviz DOT 형식으로 변환합니다.
  반환: DOT 형식의 그래프 정의 문자열
*/
string dfa_to_dot(const DFA& dfa);

/**
  bool write_dfa_dot(const DFA& dfa, const string& filename)
  
  DFA를 DOT 파일로 저장합니다.
  생성된 파일은 dfa_viz.html에서 시각화 가능합니다.
  반환: 성공 여부
*/
bool write_dfa_dot(const DFA& dfa, const string& filename);

// ============================================================================
// 시각화 관련 함수
// ============================================================================

/**
  void print_tree_pretty(TreeNode* node, string prefix, bool is_tail)
  
  트리를 ASCII 아트로 예쁘게 출력합니다.
  유닉스 tree 명령어와 유사한 형식
  
  예 출력:
    └── +
        ├── a
        └── b
*/
void print_tree_pretty(TreeNode* node, string prefix = "", bool is_tail = true);

/**
  void print_tree_detailed(TreeNode* node)
  
  트리의 상세 구조를 출력합니다.
  전위 순회(preorder)로 모든 노드를 표시
*/
void print_tree_detailed(TreeNode* node);

#endif // REGEX_AUTOMATA_H
