///
///  @file    errors.h
///  @brief   TECO error messages
///
///  @bug     No known bugs.
///
///  @copyright  2019-2020 Franklin P. Johnston
///
///  Permission is hereby granted, free of charge, to any person obtaining a
///  copy of this software and associated documentation files (the "Software"),
///  to deal in the Software without restriction, including without limitation
///  the rights to use, copy, modify, merge, publish, distribute, sublicense,
///  and/or sell copies of the Software, and to permit persons to whom the
///  Software is furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIA-
///  BILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///  THE SOFTWARE.
///
////////////////////////////////////////////////////////////////////////////////

#if     !defined(_ERRORS_H)

#define _ERRORS_H

#if     defined(__DECC)

#define noreturn

#else

#include <stdnoreturn.h>

#endif

extern int last_error;

///  @enum   errors
///  @brief  Definitions of TECO error messages.

enum errors
{
    E_NUL,          ///< No error
    E_ARG,          ///< Improper arguments
    E_BOA,          ///< O argument is out of range
    E_CHR,          ///< Invalid character for command
    E_DIV,          ///< Division by zero
    E_DTB,          ///< Delete too big
    E_DUP,          ///< Duplicate tag "!x!"
    E_EGC,          ///< EG command is too long
    E_ICE,          ///< Illegal ^E command in search argument
    E_IEC,          ///< Illegal character "x" after E
    E_IFC,          ///< Illegal character "x" after F
    E_IFE,          ///< Ill-formed numeric expression
    E_IFN,          ///< Illegal character "x" in filename
    E_IIA,          ///< Illegal insert arg
    E_ILL,          ///< Illegal command "x"
    E_ILN,          ///< Illegal number
    E_INI,          ///< Initialization error
    E_INP,          ///< Error opening input file '%s'
    E_IQC,          ///< Illegal quote character
    E_IQN,          ///< Illegal Q-register name "x"
    E_IRA,          ///< Illegal radix argument to ^R
    E_ISA,          ///< Illegal search argument
    E_ISS,          ///< Illegal search string
    E_IUC,          ///< Illegal character "x" following ^
    E_MAT,          ///< No matching files
    E_MEM,          ///< Memory overflow
    E_MIX,          ///< Maximum insert string exceeded
    E_MLA,          ///< Missing left angle bracket
    E_MLP,          ///< Missing left parenthesis
    E_MMX,          ///< Maximum macro depth exceeded
    E_MOD,          ///< Invalid modifier
    E_MQX,          ///< Maximum Q-register depth exceeded
    E_MRP,          ///< Missing right parenthesis
    E_MSC,          ///< Missing start of conditional
    E_NAB,          ///< No argument before 1's complement operator
    E_NAC,          ///< No argument before comma
    E_NAE,          ///< No argument before equals sign
    E_NAP,          ///< No argument before right parenthesis
    E_NAQ,          ///< No argument before quote
    E_NAS,          ///< No argument before semi-colon
    E_NAU,          ///< No argument before U command
    E_NCA,          ///< Negative argument to comma
    E_NFI,          ///< No file for input
    E_NFO,          ///< No file for output
    E_NOA,          ///< O argument is non-positive
    E_NON,          ///< Missing n argument after m argument
    E_NOT,          ///< O command has no tag
    E_NOW,          ///< Window support not enabled
    E_NPA,          ///< P or PW argument is negative
    E_NYA,          ///< Numeric argument with Y
    E_NYI,          ///< Not yet implemented
    E_OFO,          ///< Output file already open
    E_OUT,          ///< Error opening output file '%s'
    E_PDO,          ///< Push-down list overflow
    E_PES,          ///< Attempt to pop empty stack
    E_POP,          ///< Attempt to move pointer off page with "x"
    E_SNI,          ///< Semi-colon not in iteration
    E_SRH,          ///< Search failure "text"
    E_SYS,          ///< System error message
    E_TAG,          ///< Missing tag "!x!"
    E_T10,          ///< TECO-10 command not implemented
    E_UMA,          ///< Unused m argument
    E_UNA,          ///< Unused n argument
    E_UTC,          ///< Unterminated command string
    E_UTL,          ///< Unterminated loop
    E_UTM,          ///< Unterminated macro
    E_UTQ,          ///< Unterminated quote
    E_WIN,          ///< Window initialization error
    E_XAB,          ///< Execution aborted
    E_YCA,          ///< Y command aborted
    E_ZPA           ///< P or PW argument is zero
};

extern void help_err(int err_teco);

extern noreturn void print_err(int err_teco);

extern noreturn void printc_err(int err_teco, int c);

extern noreturn void prints_err(int err_teco, const char *str);

#endif  // !defined(_ERRORS_H)
