/*
  RegexAutomata.cpp

  이 파일은 정규표현식 파싱의 첫 번째 단계를 담당합니다.
  Infix 표기법을 Postfix 표기법으로 변환하여
  후속 단계(트리 생성, NFA/DFA 변환)에 사용할 수 있게 합니다.
*/

#include "RegexAutomata.h"

// ============================================================================
// 연산자 우선순위 계산
// ============================================================================

/**
  op_precedence(char op)

  정규표현식의 연산자 우선순위를 반환합니다.

  - '+': 합집합(union) 연산자, 가장 낮은 우선순위
  - '.': 연결(concatenation) 연산자, 중간 우선순위
  - '*': 클린 스타(Kleene star), 가장 높은 우선순위
*/
int op_precedence(char op){
    switch(op){
        case '+': return 1;
        case '.': return 2;
        case '*': return 3;
    }
    return -1;  // 연산자가 아닌 경우
}

// ============================================================================
// Infix → Postfix 변환
// ============================================================================

/**
  infix_to_postfix(char* expression)

  Infix 형식의 정규표현식을 Postfix 형식으로 변환합니다.

  1. 피연산자(문자 또는 숫자)는 즉시 출력으로 보냅니다.
  2. 연산자는 스택에 저장합니다.
  3. 현재 연산자의 우선순위가 스택 top보다 작거나 같으면
     스택에 있는 연산자를 출력으로 보냅니다.
  4. 여는 괄호 '('는 무조건 스택에 push합니다.
  5. 닫는 괄호 ')'를 만나면 대응하는 '('까지 전부 pop하여 출력합니다.
  6. 입력이 끝나면 스택에 남은 연산자들을 모두 출력합니다.

  추가로, 이 함수는 묵시적 연결을 처리합니다.
  예: "ab"는 "a.b"로 해석되어야 합니다.

  반환값:
    변환된 postfix 문자열. 호출자는 delete[]로 메모리 해제합니다.
    변환 중 오류가 나면 NULL을 반환합니다.
*/
char* infix_to_postfix(char* expression){
    int len = strlen(expression);
    char* postfix = new char[len + 1];
    int pos = 0;
    stack<char> stk;
    bool need_concat = false;  // 앞선 토큰이 연결 조건인지 추적

    for(int i = 0; i < len; i++){
        char token = expression[i];

        // ============================================================
        // 묵시적 연결 처리
        // ============================================================
        // 이전 토큰이 피연산자 또는 ')'였고,
        // 현재 토큰이 피연산자 또는 '('라면 연결 연산자 '.'를 넣어야 합니다.
        if(need_concat && (isalnum(token) || token == '(')){
            while(!stk.empty() && stk.top() != '(' &&
                  op_precedence(stk.top()) >= op_precedence('.')){
                postfix[pos++] = stk.top();
                stk.pop();
            }
            stk.push('.');
            need_concat = false;
        }

        // ============================================================
        // 피연산자 처리
        // ============================================================
        if(isalnum(token)){
            postfix[pos++] = token;
            need_concat = true;  // 다음 문자가 오면 연결이 가능함
        }
        else if(token == '('){
            stk.push('(');
            need_concat = false;
        }
        else if(token == ')'){
            // '('가 나올 때까지 스택 내용을 pop
            while(!stk.empty() && stk.top() != '('){
                postfix[pos++] = stk.top();
                stk.pop();
            }
            if(stk.empty()){
                cerr << "Error: 괄호 짝이 맞지 않습니다" << endl;
                delete[] postfix;
                return NULL;
            }
            stk.pop();
            need_concat = true;
        }
        else if(token == '+' || token == '.' || token == '*'){
            // 연산자 우선순위에 따라 스택에서 pop
            while(!stk.empty() && stk.top() != '(' &&
                  ((token == '*' && op_precedence(stk.top()) > op_precedence(token)) ||
                   (token != '*' && op_precedence(stk.top()) >= op_precedence(token)))){
                postfix[pos++] = stk.top();
                stk.pop();
            }
            stk.push(token);
            need_concat = (token == '*');
        }
    }

    // 스택에 남은 연산자 처리
    while(!stk.empty()){
        if(stk.top() == '(' || stk.top() == ')'){
            cerr << "Error: 괄호 짝이 맞지 않습니다" << endl;
            delete[] postfix;
            return NULL;
        }
        postfix[pos++] = stk.top();
        stk.pop();
    }

    postfix[pos] = '\0';
    return postfix;
}
