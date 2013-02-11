#ifndef REGEXBITS_H
#define REGEXBITS_H

/**
 * \file regexbits.h
 * Defines some macros to conveniently build regular expressions.
 */

#define RX_DEC_LITERAL "[0-9]+"
#define RX_HEX_LITERAL "(?:0x)?[0-9a-fA-F]+"
#define RX_IDENTIFIER  "[_a-zA-Z][_a-zA-Z0-9]*"
#define RX_WS          "\\s+"
#define RX_OPT_WS      "\\s*"
#define RX_CAP(s)      "(" s ")"
#define RX_GROUP(s)    "(?:" s ")"

#endif // REGEXBITS_H
