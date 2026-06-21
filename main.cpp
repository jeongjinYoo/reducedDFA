#include "RegexAutomata.h"
#include <sys/stat.h>

static const string OUTPUT_DIR = "generated/";

int main(){
    // generated/ 폴더를 미리 만듭니다. 그래프와 JSON 출력은 이 폴더에 저장됩니다.
    mkdir(OUTPUT_DIR.c_str(), 0755);
    // 프로그램 실행 흐름:
    // 1) 정규표현식을 Postfix로 변환
    // 2) Postfix를 구문 트리로 변환
    // 3) 구문 트리로부터 ε-NFA 생성
    // 4) ε-NFA를 DFA로 변환
    // 5) DFA를 최소화하여 Reduced DFA 생성
    // 6) generated/ 폴더에 DOT/JSON 출력

    // 테스트 1: 간단한 표현식
    char expr1[] = "a+b";  // a 또는 b
    cout << "\n========== 테스트 1: a+b ==========" << endl;
    cout << "중위표현식: " << expr1 << endl;

    char* postfix1 = infix_to_postfix(expr1);
    cout << "후위표현식: " << postfix1 << endl;
    
    TreeNode* tree1 = postfix_to_tree(postfix1);
    print_tree_pretty(tree1);
    delete_tree(tree1);
    delete[] postfix1;

    // 테스트 2: 더 복잡한 표현식
    char expr2[] = "(a.A)+b0c*";  // (a 연결 A) 또는 (b 연결 0 연결 c*)
    cout << "\n========== 테스트 2: (a.A)+b0c* ==========" << endl;
    cout << "중위표현식: " << expr2 << endl;
    
    char* postfix2 = infix_to_postfix(expr2);
    cout << "후위표현식: " << postfix2 << endl;
    
    TreeNode* tree2 = postfix_to_tree(postfix2);
    print_tree_pretty(tree2);
    NFAStats stats2 = compute_nfa_stats(tree2);
    cout << "NFA 상태 상한: " << nfa_state_upper_bound(tree_size(tree2)) << ", 실제: " << stats2.states << endl;
    cout << "NFA 호 상한: " << nfa_arc_upper_bound(tree_size(tree2)) << ", 실제: " << stats2.arcs << " (ε-전환=" << stats2.epsilon_arcs << ")" << endl;
    write_tree_json(tree2, OUTPUT_DIR + string("tree2.json"));
    // 구문 트리로부터 Thompson 구성법을 사용하여 ε-NFA를 생성합니다.
    NFA nfa2 = build_nfa_from_tree(tree2);
    print_nfa(nfa2);
    write_nfa_dot(nfa2, OUTPUT_DIR + string("nfa2.dot"));
    DFA dfa2 = build_dfa_from_nfa(nfa2);
    DFA red2 = minimize_dfa(dfa2);
    cout << "\n=== Reduced DFA 2 ===\n";
    print_dfa(red2);
    print_dfa_table(red2);
    write_dfa_dot(red2, OUTPUT_DIR + string("dfa2.dot"));
    delete_tree(tree2);
    delete[] postfix2;
    
    // 테스트 3: 사용자의 예제와 비슷한 형식
    char expr3[] = "((aA)+b)+(0(c*))";
    cout << "\n========== 테스트 3: ((aA)+b)+(0(c*)) ==========" << endl;
    cout << "중위표현식: " << expr3 << endl;
    
    char* postfix3 = infix_to_postfix(expr3);
    cout << "후위표현식: " << postfix3 << endl;
    
    TreeNode* tree3 = postfix_to_tree(postfix3);
    print_tree_pretty(tree3);
    NFAStats stats3 = compute_nfa_stats(tree3);
    cout << "NFA 상태 상한: " << nfa_state_upper_bound(tree_size(tree3)) << ", 실제: " << stats3.states << endl;
    cout << "NFA 호 상한: " << nfa_arc_upper_bound(tree_size(tree3)) << ", 실제: " << stats3.arcs << " (ε-전환=" << stats3.epsilon_arcs << ")" << endl;
    write_tree_json(tree3, OUTPUT_DIR + string("tree3.json"));
    // 구문 트리로부터 ε-NFA를 생성하고 출력합니다.
    NFA nfa3 = build_nfa_from_tree(tree3);
    print_nfa(nfa3);
    write_nfa_dot(nfa3, OUTPUT_DIR + string("nfa3.dot"));
    DFA dfa3 = build_dfa_from_nfa(nfa3);
    DFA red3 = minimize_dfa(dfa3);
    cout << "\n=== Reduced DFA 3 ===\n";
    print_dfa(red3);
    print_dfa_table(red3);
    write_dfa_dot(red3, OUTPUT_DIR + string("dfa3.dot"));
    delete_tree(tree3);
    delete[] postfix3;
    
    return 0;
}
