/*
 * JavaNumberToken.cpp
 *
 *  Created on: Sep 12, 2017
 *      Author: Nick
 */

#include <string>
#include <map>
#include <math.h>
#include "JavaNumberToken.h"
#include "../JavaError.h"

namespace wci { namespace frontend { namespace java { namespace tokens {

using namespace std;
using namespace wci::frontend;
using namespace wci::frontend::java;

int JavaNumberToken::MAX_EXPONENT = 37;

JavaNumberToken::JavaNumberToken(Source *source) throw (string)
		: JavaToken(source)
{
	extract();
}

//Extract a Java number token from the source.
//throws exceJTion if an error occured

void JavaNumberToken::extract() throw (string)
{
    string whole_digits = "";     // digits before the decimal point
    string fraction_digits = "";  // digits after the decimal point
    string exponent_digits = "";  // exponent digits
    char exponent_sign = '+';     // exponent sign '+' or '-'
    bool saw_dot_dot = false;     // true if saw .. token
    char current_ch;              // current character

    // Assume INTEGER token type for now.
    type = (TokenType) JT_INTEGER;

    // Extract the digits of the whole part of the number.
    whole_digits = unsigned_integer_digits();

    if (type == (TokenType) JT_ERROR) return;

    // Is there a . ?
    // It could be a decimal point or the start of a .. token.
    current_ch = current_char();
    if (current_ch == '.')
    {
        if (peek_char() == '.')
        {
            saw_dot_dot = true;  // it's a ".." token, so don't consume it
        }
        else
        {
            // Decimal point, so token type is DOUBLE.
            type = (TokenType) JT_DOUBLE;
            text += current_ch;
            current_ch = next_char();  // consume decimal point

            // Collect the digits of the fraction part of the number.
            fraction_digits = unsigned_integer_digits();
            if (type == (TokenType) JT_ERROR)
            {
                return;
            }
        }
    }

    // Is there an exponent part?
    // There cannot be an exponent if we already saw a ".." token.
    current_ch = current_char();
    if (!saw_dot_dot && ((current_ch == 'E') || (current_ch == 'e')))
    {
        // Exponent, so token type is DOUBLE.
        type = (TokenType) JT_DOUBLE;
        text += current_ch;
        current_ch = next_char();  // consume 'E' or 'e'

        // Exponent sign?
        if ((current_ch == '+') || (current_ch == '-'))
        {
            text += current_ch;
            exponent_sign = current_ch;
            current_ch = next_char();  // consume '+' or '-'
        }

        // Extract the digits of the exponent.
        exponent_digits = unsigned_integer_digits();
    }

    // Compute the value of an integer number token.
    if (type == (TokenType) JT_INTEGER)
    {
        int integer_value = compute_integer_value(whole_digits);

        if (type != (TokenType) JT_ERROR)
        {
            value = new DataValue(integer_value);
        }
    }

    // Compute the value of a DOUBLE number token.
    else if (type == (TokenType) JT_DOUBLE)
    {
        float float_value = compute_float_value(whole_digits,
                                                fraction_digits,
                                                exponent_digits,
                                                exponent_sign);

        if (type != (TokenType) JT_ERROR)
        {
            value = new DataValue(float_value);
        }
    }
}

/**
 * Extract and return the digits of an unsigned integer.
 * @param textBuffer the buffer to append the token's characters.
 * @return the string of digits.
 * @throws ExceJTion if an error occurred.
 */
string JavaNumberToken::unsigned_integer_digits() throw (string)
{
    char current_ch = current_char();

    // Must have at least one digit.
    if (!isdigit(current_ch))
    {
        type = (TokenType) JT_ERROR;
        value = new DataValue((int) INVALID_NUMBER);
        return "";
    }

    // Extract the digits.
    string digits = "";
    while (isdigit(current_ch))
    {
        text += current_ch;
        digits += current_ch;
        current_ch = next_char();  // consume digit
    }

    return digits;
}

/**
 * Compute and return the integer value of a string of digits.
 * Check for overflow.
 * @param digits the string of digits.
 * @return the integer value.
 */
int JavaNumberToken::compute_integer_value(string digits)
{
    // Return 0 if no digits.
    if (digits == "") return 0;

    int integer_value = 0;
    int prev_value = -1;    // overflow occurred if prevValue > integerValue
    int index = 0;

    // Loop over the digits to compute the integer value
    // as long as there is no overflow.
    while ((index < digits.length()) && (integer_value >= prev_value))
    {
        prev_value = integer_value;
        integer_value = 10*integer_value + (digits[index] - '0');
        index++;
    }

    // No overflow:  Return the integer value.
    if (integer_value >= prev_value) return integer_value;

    // Overflow:  Set the integer out of range error.
    else {
        type = (TokenType) JT_ERROR;
        value = new DataValue((int) RANGE_INTEGER);
        return 0;
    }
}

/**
 * Compute and return the float value of a DOUBLE number.
 * @param whole_digits the string of digits before the decimal point.
 * @param fraction_digits the string of digits after the decimal point.
 * @param exponent_digits the string of exponent digits.
 * @param exponent_sign the exponent sign.
 * @return the float value.
 */
float JavaNumberToken::compute_float_value(string whole_digits,
                                             string fraction_digits,
                                             string exponent_digits,
                                             char exponent_sign)
{
    double float_value = 0.0;
    int exponent_value = compute_integer_value(exponent_digits);
    string digits = whole_digits;  // whole and fraction digits

    // Negate the exponent if the exponent sign is '-'.
    if (exponent_sign == '-') exponent_value = -exponent_value;

    // If there are any fraction digits, adjust the exponent value
    // and append the fraction digits.
    if (fraction_digits != "")
    {
        exponent_value -= fraction_digits.length();
        digits += fraction_digits;
    }

    // Check for a DOUBLE number out of range error.
    int whole_length = whole_digits.length();
    if (abs(exponent_value + whole_length) > MAX_EXPONENT)
    {
        type = (TokenType) JT_ERROR;
        value = new DataValue((int) RANGE_REAL);
        return 0.0f;
    }

    // Loop over the digits to compute the float value.
    int index = 0;
    while (index < digits.length()) {
        float_value = 10*float_value + (digits[index] - '0');
        index++;
    }

    // Adjust the float value based on the exponent value.
    if (exponent_value != 0) {
        float_value *= pow(10, exponent_value);
    }

    return float_value;
}

}

}}}
