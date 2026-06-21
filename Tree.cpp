#include "RegexAutomata.h"
#include <functional>
#include <fstream>
#include <sstream>

/*
  Tree.cpp

  이 파일은 정규표현식의 Postfix 표현식으로부터 구문 트리를 생성하고,
  트리 구조를 읽기 쉬운 형식으로 출력하며, D3.js 시각화용 JSON으로 변환합니다.
*/

// Postfix 표현식을 구문 트리로 변환합니다.
// 이 함수는 스택 기반 알고리즘을 사용하여 피연산자와 연산자를
// 적절한 위치에 놓아 트리 구조를 만듭니다.
TreeNode* postfix_to_tree(char* postfix){
    stack<TreeNode*> stk;
    
    for(int i = 0; postfix[i] != '\0'; i++){
        char token = postfix[i];
        
        if(isalnum(token)){  // 알파벳 또는 숫자
            TreeNode* node = new TreeNode(token);
            stk.push(node);
        }
        else if(token == '+' || token == '.' || token == '*'){
            TreeNode* node = new TreeNode(token);
            
            if(token == '*'){
                // 단항 연산자
                if(!stk.empty()){
                    node->left = stk.top();
                    stk.pop();
                }
                else {
                    cerr << "Error: 부족한 operand" << endl;
                    delete node;
                    return NULL;
                }
            }
            else {
                // 이항 연산자
                if(stk.size() >= 2){
                    node->right = stk.top();
                    stk.pop();
                    node->left = stk.top();
                    stk.pop();
                }
                else {
                    cerr << "Error: 부족한 operand" << endl;
                    delete node;
                    return NULL;
                }
            }
            stk.push(node);
        }
    }
    
    if(stk.size() != 1){
        cerr << "Error: 잘못된 postfix 표현식" << endl;
        return NULL;
    }
    
    return stk.top();
}

// 트리 크기 계산
int tree_size(TreeNode* node){
    // 트리의 전체 노드 개수를 재귀적으로 계산합니다.
    if(node == NULL) return 0;
    return 1 + tree_size(node->left) + tree_size(node->right);
}

int tree_leaf_count(TreeNode* node){
    // 트리의 리프 노드(피연산자)의 개수를 계산합니다.
    if(node == NULL) return 0;
    if(node->left == NULL && node->right == NULL) return 1;
    return tree_leaf_count(node->left) + tree_leaf_count(node->right);
}

NFAStats compute_nfa_stats(TreeNode* node){
    // Thompson 구성으로 NFA를 생성할 때 필요한 상태와 전환 개수를 계산합니다.
    // 이 함수는 구문 트리를 재귀적으로 순회하며, 각 연산자가 생성하는
    // 상태 수와 ε-전환 수를 누적합니다.
    // leaf: 2 상태, 1 일반 전환
    // concatenation: 상태 합 + 1 ε-전환
    // union: 상태 합 + 2 새 상태 + 4 ε-전환
    // Kleene star: 상태 합 + 2 새 상태 + 4 ε-전환
    NFAStats stats = {0, 0, 0};
    if(node == NULL) return stats;

    if(node->left == NULL && node->right == NULL){
        stats.states = 2;
        stats.arcs = 1;
        stats.epsilon_arcs = 0;
        return stats;
    }

    NFAStats left_stats = compute_nfa_stats(node->left);
    NFAStats right_stats = compute_nfa_stats(node->right);

    if(node->value == '.'){
        stats.states = left_stats.states + right_stats.states;
        stats.arcs = left_stats.arcs + right_stats.arcs + 1;
        stats.epsilon_arcs = left_stats.epsilon_arcs + right_stats.epsilon_arcs + 1;
    }
    else if(node->value == '+'){
        stats.states = left_stats.states + right_stats.states + 2;
        stats.arcs = left_stats.arcs + right_stats.arcs + 4;
        stats.epsilon_arcs = left_stats.epsilon_arcs + right_stats.epsilon_arcs + 4;
    }
    else if(node->value == '*'){
        stats.states = left_stats.states + 2;
        stats.arcs = left_stats.arcs + 4;
        stats.epsilon_arcs = left_stats.epsilon_arcs + 4;
    }
    return stats;
}

int nfa_state_upper_bound(int size){
    return 2 * size;
}

int nfa_arc_upper_bound(int size){
    return 4 * size;
}

/**
  node_to_json(TreeNode* node)

  트리를 D3.js 시각화에 맞는 JSON 형식으로 변환합니다.
  각 노드는 "name"과 하위 노드 리스트 "children"을 가집니다.
*/
static string node_to_json(TreeNode* node){
    // D3.js 트리 시각화를 위한 JSON 문자열 생성
    if(node == NULL) return "null";
    std::ostringstream out;
    out << "{\"name\": \"" << node->value << "\"";
    if(node->left != NULL || node->right != NULL){
        out << ", \"children\": [";
        bool first = true;
        if(node->left != NULL){
            out << node_to_json(node->left);
            first = false;
        }
        if(node->right != NULL){
            if(!first) out << ", ";
            out << node_to_json(node->right);
        }
        out << "]";
    }
    out << "}";
    return out.str();
}

string tree_to_d3_json(TreeNode* node){
    // 구문 트리를 D3.js용 JSON 문자열로 변환합니다.
    return node_to_json(node);
}

bool write_tree_json(TreeNode* node, const string& filename){
    // 변환한 JSON을 파일로 저장합니다.
    ofstream ofs(filename);
    if(!ofs.is_open()) return false;
    ofs << tree_to_d3_json(node);
    return ofs.good();
}

// 트리 노드를 재귀적으로 삭제하여 할당된 메모리를 해제합니다.
void delete_tree(TreeNode* node){
    if(node == NULL) return;
    delete_tree(node->left);
    delete_tree(node->right);
    delete node;
}

// 간단한 트리 출력 (인덴테이션)
void print_tree_simple(TreeNode* node, int depth){
    if(node == NULL) return;
    
    for(int i = 0; i < depth; i++){
        cout << "  ";
    }
    cout << node->value << endl;
    
    print_tree_simple(node->left, depth + 1);
    print_tree_simple(node->right, depth + 1);
}

// 트리 높이 계산
int get_tree_height(TreeNode* node){
    if(node == NULL) return 0;
    return 1 + max(get_tree_height(node->left), get_tree_height(node->right));
}

// 트리를 ASCII 아트 형식으로 예쁘게 출력합니다.
void print_tree_pretty(TreeNode* node, string prefix, bool is_tail){
    if(node == NULL) return;
    
    cout << prefix;
    cout << (is_tail ? "└── " : "├── ");
    cout << node->value << endl;
    
    vector<TreeNode*> children;
    if(node->left != NULL) children.push_back(node->left);
    if(node->right != NULL) children.push_back(node->right);
    
    for(int i = 0; i < children.size(); i++){
        bool is_last = (i == children.size() - 1);
        string new_prefix = prefix + (is_tail ? "    " : "│   ");
        print_tree_pretty(children[i], new_prefix, is_last);
    }
}

// 트리 구조 출력 (상세 버전)
void print_tree_detailed(TreeNode* node){
    if(node == NULL){
        cout << "빈 트리입니다." << endl;
        return;
    }
    
    cout << "\n=== 트리 구조 ===" << endl;
    cout << "루트: " << node->value << endl;
    cout << "\n(Preorder 순회):" << endl;
    
    function<void(TreeNode*, int)> preorder = [&](TreeNode* n, int depth){
        if(n == NULL) return;
        for(int i = 0; i < depth; i++) cout << "  ";
        cout << n->value << endl;
        preorder(n->left, depth + 1);
        preorder(n->right, depth + 1);
    };
    preorder(node, 0);
}