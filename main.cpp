#include "RegexAutomata.h"
#include <sys/stat.h>

static const string OUTPUT_DIR = "generated/";

static bool is_valid_regex_char(char c){
    return isalnum(c) || c == '(' || c == ')' || c == '+' || c == '.' || c == '*';
}

static bool is_operand_char(char c){
    return isalnum(c);
}

static bool validate_regex(const string& expr, string& error){
    if(expr.empty()){
        error = "표현식이 비어 있습니다.";
        return false;
    }

    bool expect_operand = true;
    int depth = 0;

    for(int i = 0; i < (int)expr.size(); i++){
        char c = expr[i];

        if(!is_valid_regex_char(c)){
            error = "정의되지 않은 문자: '" + string(1, c) + "'";
            return false;
        }

        if(c == '('){
            depth++;
            expect_operand = true;
            continue;
        }

        if(c == ')'){
            if(expect_operand){
                error = "')' 앞에 피연산자가 없습니다.";
                return false;
            }
            if(depth <= 0){
                error = "괄호 불균형: 닫는 괄호가 많습니다.";
                return false;
            }
            depth--;
            expect_operand = false;
            continue;
        }

        if(c == '*'){
            if(expect_operand){
                error = "'*' 앞에 올바른 피연산자가 없습니다.";
                return false;
            }
            expect_operand = false;
            continue;
        }

        if(c == '+' || c == '.'){
            if(expect_operand){
                error = "이항 연산자 '" + string(1, c) + "' 앞에 피연산자가 없습니다.";
                return false;
            }
            expect_operand = true;
            continue;
        }

        if(is_operand_char(c)){
            expect_operand = false;
            continue;
        }

        error = "정의되지 않은 피연산자: '" + string(1, c) + "'";
        return false;
    }

    if(depth != 0){
        error = "괄호 불균형: 닫히지 않은 '('가 있습니다.";
        return false;
    }

    if(expect_operand){
        error = "표현식이 올바른 피연산자로 끝나지 않습니다.";
        return false;
    }

    return true;
}

static bool dfa_accepts(const DFA& dfa, const string& input){
    int state = dfa.start;
    for(char c : input){
        int next = -1;
        for(const DFATransition& tr : dfa.transitions){
            if(tr.from == state && tr.symbol == c){
                next = tr.to;
                break;
            }
        }
        if(next < 0) return false;
        state = next;
    }
    return find(dfa.accepts.begin(), dfa.accepts.end(), state) != dfa.accepts.end();
}

// 정규표현식 하나를 처리하고, 4단계 scanner 실험에서 사용할 수 있도록
// 최종 생성된 Reduced DFA를 반환합니다. (검증/변환 실패 시 빈 DFA 반환)
static DFA report_regex_case(int index, const string& expr){
    DFA empty_dfa{};

    cout << "\n========== 테스트 케이스 " << index << " ==========" << endl;
    cout << "정규표현식: " << expr << endl;

    string error;
    if(!validate_regex(expr, error)){
        cout << "검증 실패: " << error << endl;
        return empty_dfa;
    }

    string expr_copy = expr;
    char* expr_buf = &expr_copy[0];
    char* postfix = infix_to_postfix(expr_buf);
    if(postfix == NULL){
        cout << "후위 표기 변환 중 오류가 발생했습니다." << endl;
        return empty_dfa;
    }

    cout << "후위표현식: " << postfix << endl;
    TreeNode* tree = postfix_to_tree(postfix);
    if(tree == NULL){
        cout << "구문 트리 변환 중 오류가 발생했습니다." << endl;
        delete[] postfix;
        return empty_dfa;
    }

    int size = tree_size(tree);
    int leaf_count = tree_leaf_count(tree);
    int height = get_tree_height(tree);
    cout << "트리 크기: " << size << ", 리프 개수: " << leaf_count << ", 높이: " << height << endl;
    print_tree_pretty(tree);

    NFAStats stats = compute_nfa_stats(tree);
    cout << "NFA 상태 상한: " << nfa_state_upper_bound(size) << ", 실제 상태: " << stats.states << endl;
    cout << "NFA 호 상한: " << nfa_arc_upper_bound(size) << ", 실제 호: " << stats.arcs << " (ε-전환=" << stats.epsilon_arcs << ")" << endl;

    write_tree_json(tree, OUTPUT_DIR + string("tree_") + to_string(index) + ".json");
    NFA nfa = build_nfa_from_tree(tree);
    cout << "\n--- 생성된 ε-NFA ---" << endl;
    print_nfa(nfa);
    write_nfa_dot(nfa, OUTPUT_DIR + string("nfa_") + to_string(index) + ".dot");

    DFA dfa = build_dfa_from_nfa(nfa);
    DFA reduced = minimize_dfa(dfa);
    cout << "\n--- Reduced DFA ---" << endl;
    print_dfa(reduced);
    print_dfa_table(reduced);
    write_dfa_dot(reduced, OUTPUT_DIR + string("dfa_") + to_string(index) + ".dot");

    cout << "\n비교: ε-NFA 상태 수 = " << nfa.n_states
         << ", Reduced DFA 상태 수 = " << reduced.n_states << endl;

    delete_tree(tree);
    delete[] postfix;
    return reduced;
}

// DFA를 구동하면서 각 단계를 출력
static void scanner_drive(const DFA& dfa, const string& input) {
    cout << "\n[Scanner 구동]\n";
    cout << "입력: \"" << input << "\"\n";
    cout << "초기 상태: q" << dfa.start << "\n\n";
    
    int state = dfa.start;
    cout << "단계별 전이:\n";
    for(int i = 0; i < (int)input.size(); i++){
        char c = input[i];
        int next = -1;
        for(const DFATransition& tr : dfa.transitions){
            if(tr.from == state && tr.symbol == c){
                next = tr.to;
                break;
            }
        }
        if(next < 0){
            cout << i+1 << ": q" << state << " --('" << c << "')--> 오류 (전이 없음)\n";
            return;
        }
        cout << i+1 << ": q" << state << " --('" << c << "')--> q" << next << "\n";
        state = next;
    }
    
    bool accepted = find(dfa.accepts.begin(), dfa.accepts.end(), state) != dfa.accepts.end();
    cout << "\n최종 상태: q" << state << " (" << (accepted ? "수락" : "거부") << ")\n";
}

int main(){
    mkdir(OUTPUT_DIR.c_str(), 0755);

    char test_cases[][50] = {
        "(aA)+b+(0(c*))",
        "aA+b+0",
        "aA+b+0ccc",
        "aA++b",
        "(aA+b",
        "aA+b+0d"
    };

    int case_count = sizeof(test_cases) / sizeof(test_cases[0]);

    // [4단계] reduced DFA 실험용 입력: test_cases와 동일한 순서/인덱스로 대응.
    // 각 정규식의 알파벳과 구조(특히 '*' 유무, 정확한 길이 요구 등)에 맞춰
    // 의미 있는 수락/거부 사례가 드러나도록 케이스별로 다르게 구성함.
    vector<vector<string>> scan_inputs_per_case = {
        // 1: (aA)+b+(0(c*))  -- 알파벳 {0,A,a,b,c}, c*로 인해 0회 이상 c 반복 허용
        {
            "aA",       // 수락: 첫 번째 union 분기
            "b",        // 수락: 두 번째 union 분기
            "0",        // 수락: 세 번째 분기, c* 0회 반복
            "0ccc",     // 수락: c* 다회 반복 (case3과 비교 포인트)
            "a",        // 거부(Incomplete): 'A' 없이 끝남
            "aA0ccc",   // 거부(Trap): 이미 수락한 'aA' 뒤에 추가 입력
            "z",        // 거부(Trap): 알파벳에 없는 문자
            ""          // 거부(Incomplete): 빈 문자열
        },
        // 2: aA+b+0  -- 알파벳 {0,A,a,b}, 세 토큰 모두 정확히 일치해야 함
        {
            "aA",       // 수락
            "b",        // 수락
            "0",        // 수락
            "a",        // 거부(Incomplete)
            "aA0",      // 거부(Trap): union 분기끼리는 이어붙일 수 없음
            "x",        // 거부(Trap): 알파벳에 없는 문자
            ""          // 거부(Incomplete)
        },
        // 3: aA+b+0ccc  -- 알파벳 {0,A,a,b,c}, '*' 없이 정확히 0+c+c+c 만 허용
        {
            "aA",       // 수락
            "b",        // 수락
            "0ccc",     // 수락: 정확히 3개의 c (case1의 c*와 대비)
            "0cc",      // 거부(Incomplete): c가 하나 모자람
            "0cccc",    // 거부(Trap): c가 하나 초과 (반복 불가, case1과 대비)
            "a",        // 거부(Incomplete)
            ""          // 거부(Incomplete)
        },
        // 4: aA++b -- 검증 단계에서 실패하여 DFA 자체가 생성되지 않음 (사용 안 함)
        {},
        // 5: (aA+b -- 검증 단계에서 실패하여 DFA 자체가 생성되지 않음 (사용 안 함)
        {},
        // 6: aA+b+0d  -- 알파벳 {0,A,a,b,d}, 세 토큰 모두 정확히 일치해야 함
        {
            "aA",       // 수락
            "b",        // 수락
            "0d",       // 수락
            "0",        // 거부(Incomplete): 'd' 없이 끝남
            "0dd",      // 거부(Trap): 'd' 초과 입력
            "a",        // 거부(Incomplete)
            ""          // 거부(Incomplete)
        }
    };

    for(int i = 0; i < case_count; ++i){
        DFA reduced = report_regex_case(i + 1, string(test_cases[i]));

        if(reduced.n_states == 0){
            cout << "\n[Scanner 구동 생략] 표현식 처리에 실패하여 DFA가 없습니다." << endl;
            continue;
        }

        for(const string& input : scan_inputs_per_case[i]){
            scanner_drive(reduced, input);
        }
    }

    return 0;
}