#include "RegexAutomata.h"
#include <sstream>
#include <fstream>

/*
  NFA.cpp

  이 파일은 정규표현식 구문 트리로부터 ε-NFA를 생성하고,
  ε-NFA를 DFA로 변환하며, DFA를 최소화하는 기능을 제공합니다.
  또한 Graphviz DOT 파일을 생성하여 브라우저 시각화에 사용됩니다.
*/

// Thompson 구성에서 사용하는 임시 NFA 조각
struct NFAFragment {
    int start;
    vector<int> accepts;
    vector<Transition> trans;
};

static int new_state_id(int &counter){
    // 상태 번호를 순차적으로 생성합니다.
    return counter++;
}

static void append_trans(vector<Transition> &dst, const vector<Transition> &src){
    // 한 NFA fragment의 전환을 다른 fragment의 전환 뒤에 붙입니다.
    dst.insert(dst.end(), src.begin(), src.end());
}

// 구문 트리의 각 연산자를 Thompson 구성으로 변환하여 NFA 조각을 만듭니다.
static NFAFragment build_fragment(TreeNode* node, int &state_counter){
    // 이 재귀 함수는 구문 트리의 하나의 서브트리에 대응하는
    // ε-NFA 조각(NFAFragment)을 구성합니다.
    // 상태 번호는 state_counter를 사용하여 순서대로 생성됩니다.
    NFAFragment frag;
    if(node == NULL) return frag;
    if(node->left == NULL && node->right == NULL){
        // 문자 리프 노드: 문자 하나에 대해 두 상태와 한 개의 전환을 생성합니다.
        int s = new_state_id(state_counter);
        int t = new_state_id(state_counter);
        Transition tr; tr.from = s; tr.to = t; tr.symbol = node->value;
        frag.start = s;
        frag.accepts = {t};
        frag.trans.push_back(tr);
        return frag;
    }

    if(node->value == '.'){
        // 연결(Concatenation): 왼쪽 NFA의 accept 상태를 오른쪽 NFA 시작 상태와 ε-전환으로 연결
        NFAFragment L = build_fragment(node->left, state_counter);
        NFAFragment R = build_fragment(node->right, state_counter);
        append_trans(L.trans, R.trans);
        for(int a : L.accepts){
            Transition e; e.from = a; e.to = R.start; e.symbol = 0;
            L.trans.push_back(e);
        }
        frag.start = L.start;
        frag.accepts = R.accepts;
        frag.trans = std::move(L.trans);
        return frag;
    }
    else if(node->value == '+'){
        // 합집합(Union): 새로운 시작 상태에서 두 분기를 ε-전환으로 연결하고,
        // 두 분기의 accept 상태를 새로운 공통 accept로 ε-전환
        NFAFragment L = build_fragment(node->left, state_counter);
        NFAFragment R = build_fragment(node->right, state_counter);
        int s = new_state_id(state_counter);
        int t = new_state_id(state_counter);
        Transition e1{ s, L.start, 0 };
        Transition e2{ s, R.start, 0 };
        vector<Transition> trans;
        trans.push_back(e1);
        trans.push_back(e2);
        append_trans(trans, L.trans);
        append_trans(trans, R.trans);
        for(int a : L.accepts){ trans.push_back(Transition{a, t, 0}); }
        for(int a : R.accepts){ trans.push_back(Transition{a, t, 0}); }
        frag.start = s;
        frag.accepts = {t};
        frag.trans = std::move(trans);
        return frag;
    }
    else if(node->value == '*'){
        // 클린 스타(Kleene star): 반복을 허용하는 루프 구조를 생성
        // 새 시작 상태에서 서브트리 시작으로, 그리고 새 accept로 ε 전환
        NFAFragment C = build_fragment(node->left, state_counter);
        int s = new_state_id(state_counter);
        int t = new_state_id(state_counter);
        vector<Transition> trans;
        trans.push_back(Transition{s, C.start, 0});
        trans.push_back(Transition{s, t, 0});
        append_trans(trans, C.trans);
        for(int a : C.accepts){
            trans.push_back(Transition{a, C.start, 0});
            trans.push_back(Transition{a, t, 0});
        }
        frag.start = s;
        frag.accepts = {t};
        frag.trans = std::move(trans);
        return frag;
    }

    // 안전 장치: 예기치 않은 노드가 들어올 경우 빈 fragment를 반환합니다.
    return frag;
}

NFA build_nfa_from_tree(TreeNode* root){
    // 구문 트리에서 전체 ε-NFA를 생성합니다.
    // 상태 번호는 0부터 순차적으로 할당됩니다.
    NFA nfa;
    if(root == NULL){ nfa.n_states = 0; nfa.start = -1; return nfa; }
    int state_counter = 0;
    NFAFragment frag = build_fragment(root, state_counter);
    nfa.n_states = state_counter;
    nfa.start = frag.start;
    nfa.accepts = frag.accepts;
    nfa.transitions = std::move(frag.trans);
    for(const Transition &t : nfa.transitions){ if(t.symbol != 0) nfa.alphabet.insert(t.symbol); }
    return nfa;
}

void print_nfa(const NFA& nfa){
    // NFA의 상태 수, 시작 상태, 수락 상태, 알파벳, 전환을 콘솔에 출력합니다.
    cout << "NFA states: " << nfa.n_states << endl;
    cout << "Start: " << nfa.start << "\nAccepts:";
    for(int a : nfa.accepts) cout << ' ' << a;
    cout << "\nAlphabet:";
    for(char c : nfa.alphabet) cout << ' ' << c;
    cout << "\nTransitions:\n";
    cout << "From\tTo\tSymbol\n";
    for(const Transition &t : nfa.transitions){
        cout << t.from << '\t' << t.to << '\t';
        if(t.symbol == 0) cout << "eps";
        else cout << t.symbol;
        cout << '\n';
    }
}

string nfa_to_dot(const NFA& nfa){
    // NFA를 Graphviz DOT 문자열로 변환합니다.
    // 생성된 문자열은 브라우저에서 Viz.js로 렌더링할 수 있습니다.
    std::ostringstream out;
    out << "digraph NFA {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape = doublecircle];";
    for(int a : nfa.accepts) out << ' ' << "s" << a;
    out << ";\n  node [shape = circle];\n";
    out << "  start [shape=point];\n";
    out << "  start -> s" << nfa.start << ";\n";
    for(const Transition &t : nfa.transitions){
        out << "  s" << t.from << " -> s" << t.to << " [label=\"";
        if(t.symbol == 0) out << "ε";
        else out << t.symbol;
        out << "\"];\n";
    }
    out << "}\n";
    return out.str();
}

bool write_nfa_dot(const NFA& nfa, const string& filename){
    std::ofstream ofs(filename);
    if(!ofs.is_open()) return false;
    ofs << nfa_to_dot(nfa);
    return ofs.good();
}

static vector<int> epsilon_closure(const NFA& nfa, const vector<int>& states){
    // 주어진 상태 집합에서 ε-전환을 따라 도달할 수 있는 모든 상태를 계산합니다.
    // ε-전환만을 따라 이동하기 때문에 실제 입력 없이 상태를 확장합니다.
    vector<bool> visited(nfa.n_states, false);
    vector<int> stack = states;
    vector<int> closure;
    for(int s : states){
        if(s >= 0 && s < nfa.n_states && !visited[s]){
            visited[s] = true;
            closure.push_back(s);
        }
    }
    for(size_t i = 0; i < stack.size(); i++){
        int cur = stack[i];
        for(const Transition &t : nfa.transitions){
            if(t.from == cur && t.symbol == 0 && !visited[t.to]){
                visited[t.to] = true;
                closure.push_back(t.to);
                stack.push_back(t.to);
            }
        }
    }
    sort(closure.begin(), closure.end());
    return closure;
}

static vector<int> move_nfa(const NFA& nfa, const vector<int>& states, char symbol){
    // 주어진 상태 집합에서 특정 입력 기호(symbol)로 이동 가능한 상태를 모두 찾습니다.
    // ε-전환은 고려하지 않고, 오직 symbol과 일치하는 전환만 검사합니다.
    vector<bool> seen(nfa.n_states, false);
    vector<int> result;
    for(int s : states){
        for(const Transition &t : nfa.transitions){
            if(t.from == s && t.symbol == symbol && !seen[t.to]){
                seen[t.to] = true;
                result.push_back(t.to);
            }
        }
    }
    sort(result.begin(), result.end());
    return result;
}

static int find_or_add_state(vector<vector<int>>& dfa_states, const vector<int>& state_set){
    // 부분집합 구성에서 나온 NFA 상태 집합이 이미 존재하면 그 상태 ID를 반환합니다.
    // 새로 생긴 집합이라면 목록에 추가하고 새로운 상태 ID를 할당합니다.
    for(size_t i = 0; i < dfa_states.size(); i++){
        if(dfa_states[i] == state_set) return (int)i;
    }
    dfa_states.push_back(state_set);
    return (int)dfa_states.size() - 1;
}

DFA build_dfa_from_nfa(const NFA& nfa){
    // 부분집합 구성법을 사용하여 ε-NFA를 DFA로 변환합니다.
    DFA dfa;
    dfa.alphabet = nfa.alphabet;
    if(nfa.n_states == 0){
        dfa.n_states = 0;
        dfa.start = -1;
        return dfa;
    }

    vector<vector<int>> dfa_states;
    vector<bool> dfa_accept;
    vector<bool> visited;
    vector<DFATransition> transitions;
    vector<int> queue;

    vector<int> start_closure = epsilon_closure(nfa, vector<int>{nfa.start});
    int start_id = find_or_add_state(dfa_states, start_closure);
    queue.push_back(start_id);
    visited.push_back(true);
    dfa_accept.push_back(false);
    if(!start_closure.empty()){
        for(int s : start_closure){
            if(find(nfa.accepts.begin(), nfa.accepts.end(), s) != nfa.accepts.end()){
                dfa_accept[start_id] = true;
                break;
            }
        }
    }

    for(size_t qi = 0; qi < queue.size(); qi++){
        int current_id = queue[qi];
        vector<int> current_set = dfa_states[current_id];

        for(char symbol : dfa.alphabet){
            // 현재 DFA 상태(상태 집합)에서 symbol을 읽었을 때 도달하는 NFA 상태를 계산합니다.
            vector<int> moved = move_nfa(nfa, current_set, symbol);
            if(moved.empty()) continue;
            // symbol 이동 후에도 ε-전환을 따라 확장해야 최종 DFA 상태를 얻습니다.
            vector<int> target_set = epsilon_closure(nfa, moved);
            int target_id = find_or_add_state(dfa_states, target_set);
            if(target_id >= (int)visited.size()){
                visited.push_back(false);
                dfa_accept.push_back(false);
                queue.push_back(target_id);
                if(!target_set.empty()){
                    for(int s : target_set){
                        if(find(nfa.accepts.begin(), nfa.accepts.end(), s) != nfa.accepts.end()){
                            dfa_accept[target_id] = true;
                            break;
                        }
                    }
                }
            }
            transitions.push_back(DFATransition{current_id, target_id, symbol});
        }
    }

    dfa.n_states = (int)dfa_states.size();
    dfa.start = start_id;
    for(int i = 0; i < (int)dfa_states.size(); i++){
        if(dfa_accept[i]) dfa.accepts.push_back(i);
    }
    dfa.transitions = std::move(transitions);
    return dfa;
}

static vector<vector<int>> partition_by_accept(const DFA& dfa){
    // 초기 분할은 수락 상태 그룹과 비수락 상태 그룹으로 합니다.
    // 이 두 그룹은 DFA 최소화의 시작점이 됩니다.
    vector<int> accept_mark(dfa.n_states, 0);
    for(int a : dfa.accepts){
        if(a >= 0 && a < dfa.n_states) accept_mark[a] = 1;
    }
    vector<vector<int>> classes(2);
    for(int i = 0; i < dfa.n_states; i++){
        classes[accept_mark[i]].push_back(i);
    }
    vector<vector<int>> partition;
    for(auto &cls : classes){
        if(!cls.empty()) partition.push_back(cls);
    }
    return partition;
}

static int find_class(const vector<vector<int>>& partition, int state){
    for(int i = 0; i < (int)partition.size(); i++){
        if(find(partition[i].begin(), partition[i].end(), state) != partition[i].end()){
            return i;
        }
    }
    return -1;
}

static bool equivalent_in_partition(const DFA& dfa, int p1, int p2, const vector<int>& class_id){
    // 두 상태가 현재 분할(partition) 안에서 동작이 같은지 확인합니다.
    // 같은 입력 기호에 대해 같은 분할 클래스(target class)로 이동하면 동등합니다.
    for(char symbol : dfa.alphabet){
        int t1 = -1, t2 = -1;
        for(const DFATransition &tr : dfa.transitions){
            if(tr.from == p1 && tr.symbol == symbol){ t1 = tr.to; break; }
        }
        for(const DFATransition &tr : dfa.transitions){
            if(tr.from == p2 && tr.symbol == symbol){ t2 = tr.to; break; }
        }
        int c1 = (t1 >= 0 ? class_id[t1] : -1);
        int c2 = (t2 >= 0 ? class_id[t2] : -1);
        if(c1 != c2) return false;
    }
    return true;
}

DFA minimize_dfa(const DFA& dfa){
    // DFA 최소화: 동일하게 동작하는 상태들을 병합합니다.
    // 이 알고리즘은 수락 상태/비수락 상태로 초기 분할을 만들고,
    // 각 입력 심볼에 대해 상태 그룹을 더 세분화합니다.
    DFA result;
    if(dfa.n_states == 0){
        result.n_states = 0;
        result.start = -1;
        return result;
    }

    vector<vector<int>> partition = partition_by_accept(dfa);
    bool changed = true;
    while(changed){
        changed = false;
        vector<vector<int>> next_partition;
        for(const auto& cls : partition){
            if(cls.size() <= 1){
                next_partition.push_back(cls);
                continue;
            }
            vector<int> class_id(dfa.n_states, -1);
            for(int i = 0; i < (int)partition.size(); i++){
                for(int s : partition[i]) class_id[s] = i;
            }
            vector<int> exemplar_group;
            exemplar_group.push_back(cls[0]);
            for(int i = 1; i < (int)cls.size(); i++){
                if(equivalent_in_partition(dfa, cls[0], cls[i], class_id)){
                    exemplar_group.push_back(cls[i]);
                } else {
                    next_partition.push_back(vector<int>{cls[i]});
                }
            }
            next_partition.push_back(exemplar_group);
            if(exemplar_group.size() != cls.size()) changed = true;
        }
        partition = std::move(next_partition);
    }

    vector<int> state_map(dfa.n_states, -1);
    for(int i = 0; i < (int)partition.size(); i++){
        for(int s : partition[i]) state_map[s] = i;
    }

    // 최소화된 DFA 상태 수는 최종 partition 크기와 같습니다.
    result.n_states = (int)partition.size();
    result.alphabet = dfa.alphabet;
    result.start = state_map[dfa.start];
    for(int a : dfa.accepts){
        int mapped = state_map[a];
        if(find(result.accepts.begin(), result.accepts.end(), mapped) == result.accepts.end()){
            result.accepts.push_back(mapped);
        }
    }

    for(const DFATransition &tr : dfa.transitions){
        int from = state_map[tr.from];
        int to = state_map[tr.to];
        DFATransition mapped{from, to, tr.symbol};
        bool duplicate = false;
        for(const DFATransition &existing : result.transitions){
            if(existing.from == mapped.from && existing.to == mapped.to && existing.symbol == mapped.symbol){ duplicate = true; break; }
        }
        if(!duplicate) result.transitions.push_back(mapped);
    }
    return result;
}

void print_dfa(const DFA& dfa){
    // DFA의 상태 정보와 전환을 콘솔에 출력합니다.
    cout << "DFA states: " << dfa.n_states << endl;
    cout << "Start: " << dfa.start << "\nAccepts:";
    for(int a : dfa.accepts) cout << ' ' << a;
    cout << "\nAlphabet:";
    for(char c : dfa.alphabet) cout << ' ' << c;
    cout << "\nTransitions:\n";
    cout << "From\tTo\tSymbol\n";
    for(const DFATransition &t : dfa.transitions){
        cout << t.from << '\t' << t.to << '\t' << t.symbol << '\n';
    }
}

void print_dfa_table(const DFA& dfa){
    cout << "\nDFA 전이 표:\n";
    vector<int> state_ids(dfa.n_states);
    for(int i = 0; i < dfa.n_states; i++) state_ids[i] = i;
    cout << "State";
    for(char c : dfa.alphabet) cout << '\t' << c;
    cout << '\n';
    for(int s : state_ids){
        cout << s;
        for(char c : dfa.alphabet){
            int dest = -1;
            for(const DFATransition &t : dfa.transitions){
                if(t.from == s && t.symbol == c){ dest = t.to; break; }
            }
            if(dest >= 0) cout << '\t' << dest;
            else cout << '\t' << '-';
        }
        cout << '\n';
    }
}

string dfa_to_dot(const DFA& dfa){
    std::ostringstream out;
    out << "digraph DFA {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape = doublecircle];";
    for(int a : dfa.accepts) out << ' ' << "s" << a;
    out << ";\n";
    out << "  node [shape = circle];\n";
    out << "  start [shape=point];\n";
    out << "  start -> s" << dfa.start << ";\n";
    for(const DFATransition &t : dfa.transitions){
        out << "  s" << t.from << " -> s" << t.to << " [label=\"" << t.symbol << "\"];\n";
    }
    out << "}\n";
    return out.str();
}

bool write_dfa_dot(const DFA& dfa, const string& filename){
    std::ofstream ofs(filename);
    if(!ofs.is_open()) return false;
    ofs << dfa_to_dot(dfa);
    return ofs.good();
}
