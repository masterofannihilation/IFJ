/**
 * @file scanner.c
 * @author František Holáň (xholan13@stud.fit.vut.cz)
 * @brief Lexikální analyzátor
 * @version 0.1
 * @date 2023-10-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "scanner.h"

#define END_OF_MULTILINE_STRING 3 //Počet uvozovek které jsou třeba k uknčení víceřádkového řetězce

//stavy automatu
typedef enum state {
    INIT_STATE_S, 
    COMMENT_SLASH_S, 
    COMMENT_BLOCK_S,
    COMMENT_BLOCK_END_S,
    COMMENT_LINE_S,
    DASH_MINUS_S,
    EQ_S, 
    GT_S, 
    LT_S, 
    STRING_S,
    EXCL_S,
    ID_S, 
    UNDERSCORE_S,
    INT_NUMBER_S,
    DOUBLE_NUMBER_S,
    EXP_NUMBER_S,
    EXP_NUMBER_SIGN_S,
    PRE_DOUBLE_NUMBER_S,
    SINGLE_LINE_STRING_S,
    MULTI_LINE_STRING_S,
    MULTI_LINE_NEW_LINE_S,
    PRE_MULTI_LINE_STRING_S,
    MULTI_LINE_STRING_END_S,
    EMPTY_STRING_S,
    ESCAPE_SEKV_S,
    QUEST_MARK_S,
    NESTED_COMMENT_S
} state_t;

///< úschovňa pre jeden token
token_T *storage = NULL; 

/**
 * @brief Převede řetězec, který odpovídá klíčovému slovu na odpovídající token id. Tedy funkce slouží jako tabulka klíčových slov
 * 
 * @param string 
 * @return int, 0 pokud string neodpovídá žádnému klíčovému slovu
 */
int keyw_token_num(char * string) {
    if (strcmp(string, "Double") == 0) {
        return DOUBLE_TYPE;
    } else if (strcmp(string, "else") == 0) {
        return ELSE;
    } else if (strcmp(string, "func") == 0) {
        return FUNC;
    } else if (strcmp(string, "if") == 0) {
        return IF;
    } else if (strcmp(string, "Int") == 0) {
        return INT_TYPE;
    } else if (strcmp(string, "let") == 0) {
        return LET;
    } else if (strcmp(string, "var") == 0) {
        return VAR;
    } else if (strcmp(string, "nil") == 0) {
        return NIL;
    } else if (strcmp(string, "return") == 0) {
        return RETURN;
    } else if (strcmp(string, "String") == 0) {
        return STRING_TYPE;
    } else if (strcmp(string, "while") == 0) {
        return WHILE;
    }
    return 0;
}

/**
 * @brief Inicializuje strukturu token
 * 
 * @param token token
 * @return ukazatel na token 
 */
token_T* init_token(token_T **token) {
    //alokace pameti pro token
    *token = malloc(sizeof(token_T));

    //alokace paměti pro token se nepovedla, vrať false
    if (*token == NULL) {
        return NULL;
    } 

    //Inicializace struktury
    (*token)->type = INVALID;
    (*token)->ln = 0;
    (*token)->col = 0;

    return *token;
}

/**
 * @brief Nastaví token strukturu
 * 
 * @param type 
 * @param atr 
 * @param ln 
 * @param col_begin_token 
 * @return token_T*
 */
void set_token(token_T* token, int type, str_T atr, int ln, int col) {
    token->type = type;
    token->atr = atr;
    token->ln = ln;
    token->col = col;
}

/**
 * @brief Funkce funguje jako podautomat. Analyzuje escape sekvence v řetězci
 * 
 * @param c 
 * @return int 
 */
int escape_seq_process(char c) {
    static int state = 0; //Stav -- stavy jsou celkem tři {0,1,2}
    static int num_of_digits = 0; //Zde bude uložen počet hexadecimálních číslic (může jich být maximálně 8)
    int result = 0; //Výsledek procesu (zpracování escape sekvence), 0 = false, 1 = true, -1 = je v procesu
    switch (state)
    {
    case 0: ;//jednoduché (jednoznakové) escape sekvence
        switch (c) {
            case '"': ;
            case '\\': ;
            case 'n': ;
            case 'r': ;
            case 't': ;
                result = 1; //Vše je v pořádku vrací se true
                break;
            case 'u': ;
                //unicode escape sekvence
                state = 1;
                result = -1;
                break;
            default:
                //Na vstupu je špatný znak
                result = 0;
                break;
        }
        break;
    case 1: ;//na vstupu je levá složená závorka
        if (c == '{') {
            state = 2;
            result = -1;
        } else { 
            result = 0;  
        }
        break;
    case 2: ;
        if (isxdigit(c) && num_of_digits < 8) {
        //Na vstupu může být jakékoliv hexadecimální číslo, ale může jich být pouze 8
            state = 2;
            result = -1;
            num_of_digits++;
        } else if (c == '}' && num_of_digits >= 1) {
        //Ukončení této sekvence pomocí levé složené závorky
        //Uvnitř složených závorek musí být alespoň jedna hexadecimální číslice
            num_of_digits = 0;
            state = 0;
            result = 1;
        } else {
        //Na vstupu je špatný znak
            result = 0;
        }
        break;
    }
    return result;
}
/**
 * @brief Funkce vypočítá výsledné odsazení nejbližší trojice uvozovek od místa použití funkce
 *          Funkce navrátí ukazatel do souboru na místo, odkud byla použita
 * 
 * @return odsazení první nalezené """
 */
int get_indentation() {
    int c;
    int indent = 0;
    bool possibility_to_find = true;
    int num_of_quotes = 0;
    int num_of_readed_characters = 0;
    
    // Čtení znaků ze vstupu a hledání prvních trojitých uvozovek
    while ((c = fgetc(stdin)) != EOF) {
        if (c == EOF) {
            indent = 0;
            break;
        }
    num_of_readed_characters++;
      
      if (isblank(c)) {
        if (possibility_to_find == true)
            indent++;

      } else if (c == '\n') {
        possibility_to_find = true;
        indent = 0;
      } else if (c == '"' && possibility_to_find == true) {
            
            num_of_quotes = 1;

            while ((c = fgetc(stdin)) == '"') {
                num_of_quotes++;
            }

            num_of_readed_characters += num_of_quotes;

            if (num_of_quotes == END_OF_MULTILINE_STRING) {
                break;
            } else {
                indent = 0;
                num_of_quotes = 0;
                possibility_to_find = false;
            }
      } else {
        possibility_to_find = true;
        num_of_quotes = 0;
        indent = 0;
      }
    }
    fseek(stdin, -num_of_readed_characters, SEEK_CUR);
    return indent + 1;
}

token_T *getToken()
{
    //stav automatu
    state_t state = 0;
    //čtený znak
    int c;
    //Proměnná reprezentuje konkrétní token, pokud je rovna -1, znamená to, že token ještě nebyl zpracován
    int id_token = -1;

    //Lokální statické proměnné pro řádek a sloupec začínajícího tokenu
    static int ln = 1;          //řádek
    static int col = 0;         //sloupec

    //Pomocné proměnné pro některé stavy
    int col_begin_token = 0;        //speciální proměnná pro uložení pozice, kde začíná víceznakový token (string, identifikátor,...)
    int line_begin_token = 0;       //speciální proměnná pro uložení pozice, kde začíná víceznakový token (string, identifikátor,...)
    int quote_mark_num = 0;         //speciální proměnná, která indikuje počet za sebou jdoucích uvozovek
    bool is_multi_line_string = 0;  //speciální proměnná, která indikuje zda se přešlo do stavu ESCAPE_SEKV_S z víceřádkového řetězce nebo z jednořádkového řetězce
    int nested_comment_cnt = 0;     //speciální proměnná, která zaznamenává počet vnořených komentářů
    bool add_char_to_tkn = true;    //speciální proměnná, která indikuje jestli se má znak přidat do řetězce v tokenu (tkn->atr)
    int indent = 0;                 //speciální proměnná, kde bude uloženo, jaké odsazení má ukončovací """ v multiline řetězci
    
    //zde bude uložený nový token
    token_T *tkn = NULL;

    if (storage != NULL) {
        tkn = storage;
        storage = NULL;
        return tkn;
    }
    
    //inicializace struktury
    if (init_token(&tkn) == NULL) {
        return NULL;
    }
    //inicializace řetězce, kam se budou ukládat víceznakové tokeny
    StrInit(&tkn->atr);
/*===============================================================HLAVNÍ SMYČKA===============================================================*/
    while(true) {
        //načtení znaku ze souboru
        c = getc(stdin);

        //přečtený další znak na jednom řádku
        col++;
        //Znak nového řádku, je třeba inkrementovat ln = line number
        if (c == '\n') {
            ln++;
            col = 0;
        }

        //proměnná indikuje, jestli se má vložit znak zpět do streamu nebo ne
        bool push_to_stream = false;

        switch (state) {
/*=======================================STATE=======================================*/
            case INIT_STATE_S: ;

                col_begin_token = col;
                line_begin_token = ln;
                add_char_to_tkn = true;

                if (isspace(c)) {
                    //Na vstupu jsou bílé znaky, které je třeba přeskočit
                    //Ignorujeme
                    state = INIT_STATE_S;
                    continue;
                } else if (c == '/') {
                    //komentář - bude se ignorovat
                    state = COMMENT_SLASH_S;
                    //Je komentář, nebude se přidávat žádný znak do atributu tokenu
                    add_char_to_tkn = false;
                } else if (isdigit(c)) {
                    //číslo
                    state = INT_NUMBER_S;
                } else if (c == '"') {
                    //Řetězec
                    state = STRING_S;
                    //Nebude se přidávat uvozovka do řetězce
                    add_char_to_tkn = false;
                } else if (isalpha(c)) {
                    //Identifikátor nebo klíčové slovo
                    state = ID_S;
                    
                } else if (c == '-') {
                    //Minus/pomlčka
                    state = DASH_MINUS_S;
                }
                //operátory plus a krát
                else if (c == '+') id_token = OP_PLUS;
                else if (c == '*') id_token = OP_MUL;
                //dvojtečka
                else if (c == ':') id_token = COLON;
                //čárka
                else if (c == ',') id_token = COMMA;
                else if (c == '!') {
                    //vykřičník
                    state = EXCL_S;   
                } else if (c == '_') {
                    //podtržítko
                    state = UNDERSCORE_S;
                } else if (c == '=') {
                    //rovnítko
                    state = EQ_S;
                } else if (c == '>') {
                    //Znak "větší"
                    state = GT_S;
                } else if (c == '<') {
                    //Znak "menší"
                    state = LT_S;
                } else if (c == '?') {
                    //Otazník
                    state = QUEST_MARK_S;
                }
                //Závorky 
                else if (c == '{') id_token = BRT_CUR_L;
                else if (c == '}') id_token = BRT_CUR_R;
                else if (c == '(') id_token = BRT_RND_L;
                else if (c == ')') id_token = BRT_RND_R;
                //Konec souboru
                else if (c == EOF) id_token = EOF_TKN;
                //není přečtený správný znak
                else 
                    id_token = INVALID;
                break;
/*=======================================STATE=======================================*/
            case COMMENT_SLASH_S: ;
                switch(c) {
                    //blokový komentář
                    case '*': ;
                        state = COMMENT_BLOCK_S;
                        break;
                    //řádkový komentář
                    case '/': ;
                        state = COMMENT_LINE_S;
                        break;
                    //na vstupu není ani * ani / => jedná se o operátor děleno
                    default:
                        //potřeba vrátit znak do STREAMu
                        push_to_stream = true;
                        id_token = OP_DIV;
                        break;
                }
                break;
/*=======================================STATE=======================================*/               
            case COMMENT_BLOCK_S: ;
                switch (c)
                {
                    case '*': ;
                        //Komentář může být ukončen
                        state = COMMENT_BLOCK_END_S;
                        break;
                    case '/': ;
                        //Může začít vnořený komentář
                        state = NESTED_COMMENT_S;
                        break;
                    case EOF: ;
                        //Neukončený blokový komentář
                        id_token = INVALID;
                        break;
                    default:
                        //'/*' ==> blokový komentář, zůstaň v tomto stavu
                        state = COMMENT_BLOCK_S;
                        break;
                }

                break;
/*=======================================STATE=======================================*/
            case NESTED_COMMENT_S: ;
                //Vnořený komentář
                if (c == '*') {
                    //Jedná se o vnořený komentář, inkrementuj čítač
                    nested_comment_cnt++;
                } 

                state = COMMENT_BLOCK_S;
                break;
/*=======================================STATE=======================================*/ 
            case COMMENT_BLOCK_END_S: ;
                if (c == '/') {
                    //blokový komentář je ukončen
                    if (nested_comment_cnt == 0) {
                        //Všechny (vnořené) komenáře uvnitř blokového jsou ukončeny
                        state = INIT_STATE_S;
                    } else {
                        nested_comment_cnt--;
                        state = COMMENT_BLOCK_S;
                    }
                } else if (c == EOF) {
                    //Neukončený blokový komentář
                    id_token = INVALID;
                } else {
                    state = COMMENT_BLOCK_S;
                }
                break;
/*=======================================STATE=======================================*/ 
            case COMMENT_LINE_S: ;
                if (c == '\n') {
                    //řádkový komentář zahrnuje vše až do konce řádku
                    state = INIT_STATE_S;
                } else if (c == EOF) {
                    //Přečtený EOF
                    push_to_stream = true;
                    state = INIT_STATE_S;  
                } else {
                    state = COMMENT_LINE_S;
                }
                break;
/*=======================================STATE=======================================*/
            case INT_NUMBER_S: ;
                if (isdigit(c)) {
                    //Na vstupu jsou čísla = zůstává se ve stavu celé číslo
                    state = INT_NUMBER_S;
                    
                } else if (tolower(c) == 'e') {
                    //celé číslo s exponentem
                    state = EXP_NUMBER_S;
                    
                } else if (c == '.') {
                    //desetinné číslo
                    state = PRE_DOUBLE_NUMBER_S;
                    
                } else {
                    push_to_stream = true;
                    id_token = INT_CONST;
                }
                break;
/*=======================================STATE=======================================*/
            case PRE_DOUBLE_NUMBER_S: ;
                if (isdigit(c)) {
                    //desetinné číslo s tečkou
                    state = DOUBLE_NUMBER_S;
                } else {
                    //na vstup nepřišel žádný vhodný znak
                    id_token = INVALID;
                }
                
                break;
/*=======================================STATE=======================================*/
            case DOUBLE_NUMBER_S: ;
                if (isdigit(c)) {
                    //zůstaň v tomto stavu (na vstupu jsou čísla)
                    state = DOUBLE_NUMBER_S;
                    
                } else if (tolower(c) == 'e') {
                    //desetinné číslo s exponentem
                    state = EXP_NUMBER_S;
                    
                } else {
                    push_to_stream = true;
                    id_token = DOUBLE_CONST;
                }
                break;
/*=======================================STATE=======================================*/
            case EXP_NUMBER_S: ;
                if (isdigit(c)) {
                    state = DOUBLE_NUMBER_S;
                } else if (c == '+' || c == '-') {
                    //exponenciální tvar s volitelným znaménkem
                    state = EXP_NUMBER_SIGN_S;
                } else {
                    //špatná hodnota
                    id_token = INVALID;
                }
                
                break;
/*=======================================STATE=======================================*/
            case EXP_NUMBER_SIGN_S: ;
                //Cislo ve tvaru XeY - tedy s "eckem"
                if (isdigit(c)) {
                    state = DOUBLE_NUMBER_S;
                } else {
                    id_token = INVALID;
                }
                
                break;
/*=======================================STATE=======================================*/
            case ID_S: ;
                int keyw;
                //Součástí názvu identifikátoru může být jakýkoliv alfanumerický znak a podtržítko
                if (isalnum(c) || c == '_') {
                    state = ID_S;
                    
                } else {
                    push_to_stream = true;
                    if ((keyw = keyw_token_num(tkn->atr.data))) {
                        //Bylo nalezeno klíčové slovo, vrátí se token konkrétního klíčového slova
                        id_token = keyw;
                    } else {
                        id_token = ID;
                    }
                }
                break;
/*=======================================STATE=======================================*/
            case UNDERSCORE_S: ;
                if (isalnum(c) || c == '_') {
                    //Bude se jednat o identifikátor začínající podtržítkem
                    state = ID_S;
                
                } else {
                    //jedná se o znak podtržítko
                    push_to_stream = true;
                    id_token = UNDERSCORE;
                }
                break;
/*=======================================STATE=======================================*/
            case STRING_S: ;
                add_char_to_tkn = true;
                if (c == '"') {
                    //prázdný řetězec
                    state = EMPTY_STRING_S;
                    add_char_to_tkn = false;
                } else if (c == EOF || c == '\n') {
                    //Otevřený řetězec, za kterým následuje EOF
                    push_to_stream = true;
                    id_token = INVALID;
                } else {
                    push_to_stream = true;
                    state = SINGLE_LINE_STRING_S;
                }
                
                break;
/*=======================================STATE=======================================*/
            case SINGLE_LINE_STRING_S: ;
                add_char_to_tkn = true;

                switch (c)
                {
                    case '"': ;
                        id_token = STRING_CONST;
                        //Nebude se přidávat uvozovka, jakožto konec řetězce, do tokenu
                        add_char_to_tkn = false;
                        break;
                    case '\\': ;
                        //Escape sekvence
                        is_multi_line_string = false;
                        state = ESCAPE_SEKV_S;
                        break;
                    case '\n': ;
                    case EOF: ;
                        //Neukončený řetězec
                        push_to_stream = true;
                        id_token = INVALID;
                        break;
                    default: ;
                        state = SINGLE_LINE_STRING_S;
                        break;
                }
                break;
/*=======================================STATE=======================================*/
            case EMPTY_STRING_S: ;
                //Toto je přechodový stav mezi víceřádkovým řetězcem a jednořádkovým řetězcem
                if (c == '"') {
                    state = PRE_MULTI_LINE_STRING_S;
                } else {
                    push_to_stream = true;
                    id_token = STRING_CONST;
                }
                break;
/*=======================================STATE=======================================*/
            case PRE_MULTI_LINE_STRING_S: ;
                //Tři uvozovky (víceřádkový řetězec) musí být na samostatném řádku
                if (c == '\n') {
                    state = MULTI_LINE_NEW_LINE_S;
                    add_char_to_tkn = false;
                    //Nyni se vypocita, jake odsazeni maji posledni ukoncovaci uvozovky
                    indent = get_indentation();
                    
                } else if (isblank(c)) {
                    //Bílé znaky se ignorují
                    //Nebudou se do řetězce přidávat
                    add_char_to_tkn = false;
                    state = PRE_MULTI_LINE_STRING_S;
                } else {
                    id_token = INVALID;
                }
                break;
/*=======================================STATE=======================================*/
            case MULTI_LINE_NEW_LINE_S: ;
                add_char_to_tkn = true;
                if (c == '"') {
                    //Potenciální ukončení řetězce
                    push_to_stream = true;
                    state = MULTI_LINE_STRING_END_S;
                } else if (c == '\n') {
                    state = MULTI_LINE_NEW_LINE_S;
                } else if (c == EOF) {
                    push_to_stream = true;
                    id_token = INVALID;  
                } else if (isblank(c)) {
                    //přeskakování mezer
                    add_char_to_tkn = false;
                    state = MULTI_LINE_NEW_LINE_S;
                } else {
                    //Dalsi mezery se pridaji
                    int indentation = col - indent;
                    //Kontrola spravneho odsazeni
                    if (indentation < 0) {
                        id_token = INVALID;
                    } else {
                        for (int i = 0; i < indentation; i++) {
                            StrAppend(&tkn->atr, ' '); 
                        }
                    }
                    push_to_stream = true;
                    state = MULTI_LINE_STRING_S;
                }
                
                break;
/*=======================================STATE=======================================*/
            case MULTI_LINE_STRING_S: ;
                add_char_to_tkn = true;
                //Víceřádkový řetězec
                if (c == '\n') {
                    //Potenciální možnost ukončení řetězce
                    push_to_stream = true;
                    state = MULTI_LINE_NEW_LINE_S;
                    //konec zmeny
                } else if (c == EOF) {
                    push_to_stream = true;
                    id_token = INVALID;
                } else if (c == '\\') {
                    //Escape sekvence
                    is_multi_line_string = true;
                    state = ESCAPE_SEKV_S;
                } else {
                    //Na vstup přichází správné znaky, zůstaň v tomto stavu
                    state = MULTI_LINE_STRING_S;
                }
                break;
/*=======================================STATE=======================================*/
            case MULTI_LINE_STRING_END_S: ;
            add_char_to_tkn = true;
                //Řetězec může být ukončen
                if (c == '"') {
                    quote_mark_num++;
                    //Abych nemusel přecházet do dalších dvou stavů, je zde pomocná proměnná, která počítá uvozovky
                    if (quote_mark_num == END_OF_MULTILINE_STRING) {
                            id_token = STRING_CONST;
                    }
                    state = MULTI_LINE_STRING_END_S;
                } else if (c == EOF) {
                    //Neukončený řetězec
                    id_token = INVALID;
                } else if (c == '\n') {
                    //Přeskakují se mezery
                    state = MULTI_LINE_NEW_LINE_S;
                } else {
                    quote_mark_num = 0;
                    push_to_stream = true;
                    state = MULTI_LINE_STRING_S;
                }

                break;
/*=======================================STATE=======================================*/
            case ESCAPE_SEKV_S: ;
                //Zde se zpracovává escape sekvence v řetězci
                int esc_seq_proc = escape_seq_process(c);
                if (esc_seq_proc == -1) {
                    //analýza escape sekvence není u konce, zůstává se v tomto stavu
                    state = ESCAPE_SEKV_S;
                } else if (esc_seq_proc) {
                    //analýza escape sekvence proběhla úspěšně
                    if (is_multi_line_string) {
                        state = MULTI_LINE_STRING_S;
                    } else {
                        state = SINGLE_LINE_STRING_S;
                    }
                } else if (esc_seq_proc == 0) {
                    //Analýza escape sekvence neproběhla úspěšně
                   id_token = INVALID;
                }
                
                break;
/*=======================================STATE=======================================*/
            case DASH_MINUS_S: ;
                
                //Přečtený znak je '>': výsledný token: ->
                if (c == '>') {
                    id_token = ARROW;
                } else {
                    //Jedná se o pomlčku = operátor minus (jednoznakový operátor)
                    push_to_stream = true;
                    id_token = OP_MINUS;
                } 
                break;
/*=======================================STATE=======================================*/
            case EQ_S: ;
                //přiřazení nebo porovnání
                if (c == '=') {
                    id_token = EQ;
                } else {
                    //Operátor přiřazení
                    push_to_stream = true;
                    id_token = ASSIGN;
                }
                break;
/*=======================================STATE=======================================*/
            case GT_S: ;

                //větší nebo větší rovno
                if (c == '=') {
                    id_token = GTEQ;
                } else {
                    push_to_stream = true;
                    id_token = GT;
                }
                break;
/*=======================================STATE=======================================*/
            case LT_S: ;

                //menší nebo menší rovno
                if (c == '=') {
                    id_token = LTEQ;
                } else {
                    //Operátor menší
                    push_to_stream = true;
                    id_token = LT;
                }
                break;
/*=======================================STATE=======================================*/
            case EXCL_S: ;

                if (c == '=') {
                    //Negace porovnání
                    id_token = NEQ;
                } else {
                    push_to_stream = true;
                    id_token = EXCL;
                }
                break;
/*=======================================STATE=======================================*/
            case QUEST_MARK_S: ;

                if (c == '?') {
                    //Operátor ??
                    id_token = TEST_NIL;
                } else {
                    push_to_stream = true;
                    id_token = QUEST_MARK;
                }
                break;
        }
        if (push_to_stream) {
            //Vraceny znak do streamu se nebude pridavat do retezce
            add_char_to_tkn = false;
            //Je třeba vrátit znak do streamu 
            //Pokud znak, který bude vrácen je "new line", je třeba decrementovat i ln, aby se znak \n nepřečetl dvakrát
            if (c == '\n')
                ln--;

            col--;
            ungetc(c, stdin);
        }
        //Přidání znaku do atributu tokenu
        if (add_char_to_tkn) {
            if (id_token == EOF_TKN)
                StrFillWith(&tkn->atr, "EOF");
            else 
                StrAppend(&tkn->atr, c);
        } 

        //Odstraneni uvozovek a noveho radku
        if (id_token == STRING_CONST && state == MULTI_LINE_STRING_END_S) {
            //Odstrani se posledni tri uvozovky z multiline stringu
            tkn->atr.data[strlen(StrRead(&tkn->atr)) - 3] = '\0'; 
            //Odstrani se posledni znak noveho radku
            int multi_line_len = strlen(StrRead(&tkn->atr));
            while (tkn->atr.data[multi_line_len] != '\n') {
                multi_line_len--;
            } 
            tkn->atr.data[multi_line_len] = '\0';
        }

        if (id_token != -1) {
            //Token je zpracován, vrátí se
            set_token(tkn, id_token, tkn->atr, line_begin_token, col_begin_token);
            return tkn;
        }

    }
    return NULL;
}

void storeToken(token_T *tkn)
{
    storage = tkn;
}

void destroyToken(token_T *tkn) {
    StrDestroy(&tkn->atr);
    free(tkn);
}