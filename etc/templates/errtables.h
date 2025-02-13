///
///  @file    errtables.h
///  @brief   TECO error tables
///
/* (INSERT: WARNING NOTICE) */
///
///  @copyright 2019-2022 Franklin P. Johnston / Nowwith Treble Software
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

///  @struct  errlist
///  @brief   Structure of error message

struct errlist
{
    const char *code;           ///< Three-letter error code
    const char *text;           ///< Short description of error
};

///  @struct  errlist
///  @var     errlist
///
///  @brief   TECO error messages

static struct errlist errlist[] =
{
/* (INSERT: ERROR MESSAGES) */
};

///  @var    errhelp
///
///  @brief  Detailed help for TECO errors.

static const char *errhelp[] =
{
/* (INSERT: ERROR DETAILS) */
};
