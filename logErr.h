/** Projekt IFJ2023
 * @file logErr.h
 * @brief Funkcie pre hlásenie chýb prekladu kódu či samotného prekladača
 * @author Michal Krulich
 * @date 12.11.2023
 */

#ifndef _LOGERR_H_
#define _LOGERR_H_ 1

#include "scanner.h"

/**
 * @brief Vypíše do stderr chybu prekladača s priloženou správou
*/
void logErrCompiler(const char *msg);

/**
 * @brief Vypíše do stderr chybu alokácie pamäte
*/
void logErrCompilerMemAlloc();

/**
 * @brief Vypíše do stderr chybu prekladu kódu, funguje podobne ako printf
*/
void logErrCodeAnalysis(const int err_code, const int ln, const int c, const char *format, ...);

/**
 * @brief Vypíše do stderr informáciu o syntaktickej chybe
*/
void logErrSyntax(const token_T *t, const char *expected);

/**
 * @brief Vypíše do stderr informáciu o sémantickej chybe
*/
void logErrSemantic(const token_T *t, const char *format, ...);

/**
 * @brief Vypíše do stderr informáciu o sémantickej chybe v definícií funkcie.
*/
void logErrSemanticFn(const char *fn, const char *format, ...);

/**
 * @brief Aktualizuje informácie o pozícií posledného tokenu
*/
void logErrUpdateTokenInfo(const token_T *t);

#endif

/* Koniec súboru logErr.h */
