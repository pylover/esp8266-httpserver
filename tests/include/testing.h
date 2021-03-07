
#ifndef _TESTING_H
#define _TESTING_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


void equalchr(const char given, const char expected);
void equalstr(const char *given, const char *expected);
void equalnstr(const char *given, const char *expected, uint32_t len);
void equalbin(const unsigned char *given, const unsigned char *, uint32_t len);
void equalint(int given, int expected);


#define RED     "\e[0;31m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\e[0;33m"
#define BLUE    "\e[0;34m"
#define MAGENTA "\e[0;35m"
#define CYAN    "\e[0;36m"
#define WHITE   "\e[0;37m"

#define CLR     "\33[0m"


#define pcolor(c_, f_, ...) \
    printf(c_); \
    printf((f_), ##__VA_ARGS__); \
    printf(CLR)

#define pok(f_, ...) pcolor(GREEN, f_, ##__VA_ARGS__)
#define perr(f_, ...) pcolor(RED, f_, ##__VA_ARGS__)

#define pokln(f_, ...) pok(f_, ##__VA_ARGS__); printf("\r\n")
#define perrln(f_, ...) perr(f_, ##__VA_ARGS__); printf("\r\n")
#define pcolorln(c_, f_, ...) pcolor(c_, f_, ##__VA_ARGS__); printf("\r\n")

#define pdataln(f_, ...) pcolorln(WHITE, f_, ##__VA_ARGS__);



#define SUCCESS(c) if (c) {pokln("%s Ok", __func__); return; }
#define FAILED() perrln("%s Failed", __func__)
#define EXPECTED() pcolor(BLUE, "Expected: ")
#define GIVEN() pcolor(YELLOW, "Given: ")


static void printbinary(const unsigned char *buf, int buflen) {
    int i;
    for (i = 0; i < buflen; i++){
        printf("\\%02X", buf[i]);
    }
    printf("\n");
}


void equalbin(const unsigned char *given, const unsigned char *expected, 
        uint32_t len) {
    SUCCESS(memcmp(given, expected, len) == 0);

    /* Error */
    FAILED();
    EXPECTED();
    printbinary(expected, len);

    GIVEN();
    printbinary(given, len);

    exit(EXIT_FAILURE);
}


void equalchr(const char given, const char expected) {
    SUCCESS(given == expected);

    /* Error */
    FAILED();
    EXPECTED();
    pdataln("%c", expected);

    GIVEN();
    pdataln("%c", given);

    exit(EXIT_FAILURE);
}


void equalstr(const char *given, const char *expected) {
    SUCCESS(strcmp(given, expected) == 0);

    /* Error */
    FAILED();
    EXPECTED();
    pdataln("%s", expected);

    GIVEN();
    pdataln("%s", given);

    exit(EXIT_FAILURE);
}


void equalnstr(const char *given, const char *expected, u_int32_t len) {
    SUCCESS(strncmp(given, expected, len) == 0);

    /* Error */
    FAILED();
    EXPECTED();
    pdataln("%.*s", len, expected);

    GIVEN();
    pdataln("%.*s", len, given);

    exit(EXIT_FAILURE);
}


void equalint(int given, int expected) {
    SUCCESS(given == expected);

    /* Error */
    FAILED();
    EXPECTED();
    pdataln("%d", expected);

    GIVEN();
    pdataln("%d", given);

    exit(EXIT_FAILURE);
}


/* Asserts */

#define assert(f, ...) \
    pcolor(CYAN, "%s:%d", __FILE__, __LINE__); \
    pcolor(MAGENTA, " [%s] ", __func__); \
    f(__VA_ARGS__)

#define eqchr(g, e) assert(equalchr, g, e)
#define eqstr(g, e) assert(equalstr, g, e)
#define eqnstr(g, e, n) assert(equalnstr, g, e, n)
#define eqint(g, e) assert(equalint, g, e)
#define eqbin(g, e, l) assert(equalbin, (unsigned char*)g, (unsigned char*)e, l)


#endif
