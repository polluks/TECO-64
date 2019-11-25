///
///  @file    cmd_exec.c
///  @brief   Execute command string.
///
///  @author  Nowwith Treble Software
///
///  @bug     No known bugs.
///
///  @copyright  tbd
///
///  Permission is hereby granted, free of charge, to any person obtaining a copy
///  of this software and associated documentation files (the "Software"), to deal
///  in the Software without restriction, including without limitation the rights
///  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
///  copies of the Software, and to permit persons to whom the Software is
///  furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
///
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "teco.h"
#include "ascii.h"
#include "eflags.h"
#include "errors.h"
#include "exec.h"


struct vars
{
    int b;                              // Beginning of buffer = 0
    int z;                              // End of buffer
    int dot;                            // Current pointer position
    int eof;                            // End of file flag
};

struct vars vars =
{
    .b   = 0,
    .z   = 1000,
    .dot = 0,
    .eof = 0,
};

// The following is used when a command is done and we want to skip to the
// next command.

jmp_buf jump_command;

// Local functions

static const struct cmd_table *scan_caret(struct cmd *cmd);


//
//  Dispatch table, defining functions to call for each character read, as well
//  as the options for each command.
//
//  Note that to avoid duplication, lower-case letters can be omitted, because
//  we translate characters to upper-case before indexing into this table.
//
//  Also, we handle E and F commands specially, as they involve a 2nd character.
//

static const struct cmd_table cmd_table[] =
{
    [NUL]         = { NULL,       NULL,             ""             },
    [CTRL_A]      = { scan_done,  exec_ctrl_a,      "@ 1",         },
    [CTRL_B]      = { scan_expr,  exec_ctrl_b,      ""             },
    [CTRL_C]      = { NULL,       exec_ctrl_c,      ""             },
    [CTRL_D]      = { scan_done,  exec_ctrl_d,      ""             },
    [CTRL_E]      = { scan_expr,  exec_ctrl_e,      ""             },
    [CTRL_F]      = { scan_expr,  exec_ctrl_f,      "n"            },
    [CTRL_G]      = { scan_bad,   NULL,             ""             },
    [BS]          = { scan_expr,  exec_ctrl_h,      ""             },
    [TAB]         = { scan_done,  exec_ctrl_i,      "1"            },
    [LF]          = { NULL,       exec_nul,         ""             },
    [VT]          = { scan_bad,   NULL,             ""             },
    [FF]          = { NULL,       exec_ctrl_l,      ""             },
    [CR]          = { NULL,       NULL,             ""             },
    [CTRL_N]      = { scan_expr,  exec_ctrl_n,      ""             },
    [CTRL_O]      = { scan_done,  exec_ctrl_o,      ""             },
    [CTRL_P]      = { scan_bad,   exec_ctrl_p,      ""             },
    [CTRL_Q]      = { NULL,       exec_ctrl_q,      ""             },
    [CTRL_R]      = { NULL,       exec_ctrl_r,      "n"            },
    [CTRL_S]      = { scan_expr,  exec_ctrl_s,      ""             },
    [CTRL_T]      = { NULL,       exec_ctrl_t,      ":"            },
    [CTRL_U]      = { scan_done,  exec_ctrl_u,      "n : @ q 1"    },
    [CTRL_V]      = { NULL,       exec_ctrl_v,      ""             },
    [CTRL_W]      = { NULL,       exec_ctrl_w,      ""             },
    [CTRL_X]      = { scan_expr,  exec_ctrl_x,      "n"            },
    [CTRL_Y]      = { scan_expr,  exec_ctrl_y,      ""             },
    [CTRL_Z]      = { scan_expr,  exec_ctrl_z,      ""             },
    [ESC]         = { scan_done,  exec_escape,      "m n"          },
    [FS]          = { scan_bad,   NULL,             ""             },
    [GS]          = { scan_bad,   NULL,             ""             },
    [RS]          = { NULL,       exec_ctrl_caret,  ""             },
    [US]          = { scan_expr,  exec_ctrl_ubar,   ""             },
    [SPACE]       = { NULL,       NULL,             ""             },
    ['!']         = { scan_done,  exec_bang,        "@ 1"          },
    ['"']         = { scan_done,  exec_quote,       ""             },
    ['#']         = { scan_expr,  exec_operator,    ""             },
    ['$']         = { scan_done,  exec_escape,      "m n"          },
    ['%']         = { scan_done,  exec_pct,         "n q"          },
    ['&']         = { scan_expr,  exec_operator,    ""             },
    ['\'']        = { scan_done,  exec_apos,        ""             },
    ['(']         = { scan_expr,  exec_operator,    ""             },
    [')']         = { scan_expr,  exec_rparen,      ""             },
    ['*']         = { scan_expr,  exec_operator,    ""             },
    ['+']         = { scan_expr,  exec_operator,    ""             },
    [',']         = { scan_expr,  exec_comma,       ""             },
    ['-']         = { scan_expr,  exec_operator,    ""             },
    ['.']         = { scan_expr,  exec_dot,         ""             },
    ['/']         = { scan_expr,  exec_operator,    ""             },
    ['0']         = { scan_expr,  NULL,             ""             },
    ['1']         = { scan_expr,  NULL,             ""             },
    ['2']         = { scan_expr,  NULL,             ""             },
    ['3']         = { scan_expr,  NULL,             ""             },
    ['4']         = { scan_expr,  NULL,             ""             },
    ['5']         = { scan_expr,  NULL,             ""             },
    ['6']         = { scan_expr,  NULL,             ""             },
    ['7']         = { scan_expr,  NULL,             ""             },
    ['8']         = { scan_expr,  NULL,             ""             },
    ['9']         = { scan_expr,  NULL,             ""             },
    [':']         = { scan_mod,   NULL,             ""             },
    [';']         = { scan_done,  exec_semi,        "n :"          },
    ['<']         = { scan_done,  exec_langle,      "n"            },
    ['=']         = { scan_done,  exec_equals,      "n :"          },
    ['>']         = { scan_done,  exec_rangle,      ""             },
    ['?']         = { scan_done,  exec_question,    ""             },
    ['@']         = { scan_mod,   NULL,             ""             },
    ['A']         = { scan_done,  exec_A,           "n :"          },
    ['B']         = { scan_expr,  exec_B,           ""             },
    ['C']         = { scan_done,  exec_C,           "n :"          },
    ['D']         = { scan_done,  exec_D,           "m n :"        },
    ['E']         = { NULL,       NULL,             ""             },
    ['F']         = { NULL,       NULL,             ""             },
    ['G']         = { scan_done,  exec_G,           ":q"           },
    ['H']         = { scan_expr,  exec_H,           ""             },
    ['I']         = { scan_done,  exec_I,           "n @ 1"        },
    ['J']         = { scan_done,  exec_J,           "n :"          },
    ['K']         = { scan_done,  exec_K,           "m n"          },
    ['L']         = { scan_done,  exec_L,           "n"            },
    ['M']         = { scan_done,  exec_M,           "m n : q"      },
    ['N']         = { scan_done,  exec_N,           "n : @ 1"      },
    ['O']         = { scan_done,  exec_O,           "n @ 1"        },
    ['P']         = { scan_done,  exec_P,           "m n : W"      },
    ['Q']         = { scan_done,  exec_Q,           ": q"          },
    ['R']         = { scan_done,  exec_R,           "n :"          },
    ['S']         = { scan_done,  exec_S,           "m n : :: @ 1" },
    ['T']         = { scan_done,  exec_T,           "m n :"        },
    ['U']         = { scan_done,  exec_U,           "m n q"        },
    ['V']         = { scan_done,  exec_V,           "m n"          },
    ['W']         = { scan_done,  exec_W,           "m n :"        },
    ['X']         = { scan_done,  exec_X,           "m n : q"      },
    ['Y']         = { scan_done,  exec_Y,           "n :"          },
    ['Z']         = { scan_expr,  exec_Z,           ""             },
    ['[']         = { scan_done,  exec_lbracket,    "q"            },
    ['\\']        = { scan_expr,  exec_backslash,   ""             },
    [']']         = { scan_done,  exec_rbracket,    ": q"          },
    ['^']         = { NULL,       NULL,             ""             },
    ['_']         = { scan_expr,  exec_ubar,        "n : @ 1"      },
    ['`']         = { scan_bad,   NULL,             ""             },
    ['a']         = { NULL,       NULL,             ""             },
    ['b']         = { NULL,       NULL,             ""             },
    ['c']         = { NULL,       NULL,             ""             },
    ['d']         = { NULL,       NULL,             ""             },
    ['e']         = { NULL,       NULL,             ""             },
    ['f']         = { NULL,       NULL,             ""             },
    ['g']         = { NULL,       NULL,             ""             },
    ['h']         = { NULL,       NULL,             ""             },
    ['i']         = { NULL,       NULL,             ""             },
    ['j']         = { NULL,       NULL,             ""             },
    ['k']         = { NULL,       NULL,             ""             },
    ['l']         = { NULL,       NULL,             ""             },
    ['m']         = { NULL,       NULL,             ""             },
    ['n']         = { NULL,       NULL,             ""             },
    ['o']         = { NULL,       NULL,             ""             },
    ['p']         = { NULL,       NULL,             ""             },
    ['q']         = { NULL,       NULL,             ""             },
    ['r']         = { NULL,       NULL,             ""             },
    ['s']         = { NULL,       NULL,             ""             },
    ['t']         = { NULL,       NULL,             ""             },
    ['u']         = { NULL,       NULL,             ""             },
    ['v']         = { NULL,       NULL,             ""             },
    ['w']         = { NULL,       NULL,             ""             },
    ['x']         = { NULL,       NULL,             ""             },
    ['y']         = { NULL,       NULL,             ""             },
    ['z']         = { NULL,       NULL,             ""             },
    ['{']         = { scan_bad,   NULL,             ""             },
    ['|']         = { scan_done,  exec_vbar,        ""             },
    ['}']         = { scan_bad,   NULL,             ""             },
    ['~']         = { scan_bad,   NULL,             ""             },
    [DEL]         = { scan_bad,   NULL,             ""             },
};

// Define initial values for command block

const struct cmd null_cmd =
{
    .state      = CMD_NULL,
    .level      = 0,
    .flag       = 0,
    .c1         = NUL,
    .c2         = NUL,
    .c3         = NUL,
    .m          = 0,
    .n          = 0,
    .paren      = 0,
    .delim      = ESC,
    .qreg       = NUL,
    .qlocal     = false,
    .expr       = { .buf = NULL, .len = 0},
    .text1      = { .buf = NULL, .len = 0},
    .text2      = { .buf = NULL, .len = 0},
    .next       = NULL,
    .prev       = NULL,
};


// Local functions

static void exec_expr(struct cmd *cmd);

static void set_opts(struct cmd *cmd, const char *opts);


///
///  @brief    Execute command string.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_cmd(void)
{
    struct cmd cmd = null_cmd;

    // Here to start parsing a command string,  one character at a time. Note
    // that although some commands may contain only a single character, most of
    // them comprise multiple characters, so we have to keep looping until we
    // have found everything we need. As we continue, we store information in
    // the command block defined by 'cmd', for use when we're ready to actually
    // execute the command. This includes such things as m and n arguments,
    // modifiers such as : and @, and any text strings followed the command.

    for (;;)
    {
        // The following is used to note the start of any expression that may
        // precede the actual command. Until we start one, we will keep setting
        // a pointer to our current location in the command string. Once we
        // start an expression, we will increment the count for each character
        // parsed. Once we're done with the expression, we will have a pointer
        // to the start of it, along with the length of the no. of characters.
        //
        // Note that one of the reasons we do it this way is so that we will
        // count will be correct even if the expression contains characters
        // such as spaces which don't affect its evaluation, but which need to
        // be counted anyway.

        if (cmd.expr.len == 0)
        {
            cmd.expr.buf = next_buf();
        }

        if (next_buf() == NULL)
        {
            break;
        }

        int c = fetch_buf();

        // Flag that command execution has started. This is placed after we
        // read the next character, so that any CTRL/C causes return to main
        // loop without an error message.

        // TODO: add check for trace mode

        f.ei.exec = true;

        if ((uint)c >= countof(cmd_table))
        {
            printc_err(E_ILL, c);
        }
        else if (setjmp(jump_command) == 0)
        {
            const struct cmd_table *table;

            cmd.c1 = (char)c;

            if (toupper(c) == 'E')               // E{x} command
            {
                cmd.c2 = (char)fetch_buf();

                table = init_E(&cmd);
            }
            else if (toupper(c) == 'F')          // F{x} command
            {
                cmd.c2 = (char)fetch_buf();

                table = init_F(&cmd);
            }
            else if (toupper(c) == '^')          // ^{x} command
            {
                cmd.c1 = (char)fetch_buf();

                table = scan_caret(&cmd);
            }
            else                                 // Everything else
            {
                cmd.c2 = NUL;

                table = &cmd_table[toupper(c)];
            }

            void (*exec_scan)(struct cmd *cmd) = table->scan;
            void (*exec_func)(struct cmd *cmd) = table->exec;
            
            if (exec_scan == NULL)
            {
                continue;
            }
 
            set_opts(&cmd, table->opts);

            f.ei.dryrun = true;

            (*exec_scan)(&cmd);

            f.ei.dryrun = false;

            if (cmd.state == CMD_EXPR)
            {
                cmd.expr.len = (uint)(next_buf() - cmd.expr.buf);
            }
            else if (cmd.state == CMD_DONE)
            {
                scan_cmd(&cmd);         // Finish scanning command

                if (c != ESC || cmd.expr.len != 0)
                {
                    if (teco_debug)
                    {
                        print_cmd(&cmd);
                    }
                }

//                init_expr();
                exec_expr(&cmd);        // Execute expression

                if (exec_func != NULL)
                {
                    if (operand_expr())
                    {
                        cmd.got_n = true;
                        cmd.n = get_n_arg();
                    }

                    (*exec_func)(&cmd);

                    cmd = null_cmd;
                }
            }
        }
        else
        {
            reset_buf();

            // TODO: check for unclosed loop
            // print_err(E_UTC);
        }

        f.ei.exec = false;              // Suspend this to check CTRL/C

        if (f.ei.ctrl_c)                // If CTRL/C typed, return to main loop
        {
            f.ei.ctrl_c = false;

            print_err(E_XAB);
        }
    }
}


///
///  @brief    Process ESCape which terminates a command string.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_escape(struct cmd *cmd)
{
    assert(cmd != NULL);

    init_expr();
}


///
///  @brief    Do a full scan of the expression. All we've done up to now is to
///            check the expression for form. Now we check it for substance.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void exec_expr(struct cmd *cmd)
{
    assert(cmd != NULL);

    cmd->got_m = false;
    cmd->got_n = false;

//    init_expr();

    if (cmd->c1 == ESC)
    {
        return;
    }

    const char *p = cmd->expr.buf;

    while (p < cmd->expr.buf + cmd->expr.len)
    {
        const struct cmd_table *table;
        int c = *p++;

        if ((uint)c >= countof(cmd_table))
        {
            printc_err(E_ILL, c);
        }

        cmd->c1 = (char)c;

        if (toupper(c) == 'E')          // E{x} command
        {
            cmd->c2 = (char)*p++;

            assert(p <= cmd->expr.buf + cmd->expr.len);

            table = init_E(cmd);
        }
        else if (toupper(c) == 'F')     // F{x} command
        {
            cmd->c2 = (char)*p++;

            assert(p <= cmd->expr.buf + cmd->expr.len);

            table = init_F(cmd);
        }
        else if (toupper(c) == '^')     // ^{x} command
        {
            cmd->c1 = (char)*p++;

            assert(p <= cmd->expr.buf + cmd->expr.len);

            table = scan_caret(cmd);
        }
        else                            // Everything else
        {
            cmd->c2 = NUL;

            table = &cmd_table[toupper(c)];
        }

        void (*exec_func)(struct cmd *cmd) = table->exec;

        if (exec_func != NULL)
        {
            (*exec_func)(cmd);
        }
        else if (isdigit(c))
        {
            --p;

            int maxchrs = cmd->expr.buf + cmd->expr.len - p;
            int ndigits = exec_digit(p, maxchrs);

            p += ndigits;

            assert(p <= cmd->expr.buf + cmd->expr.len);
        }
    }
}


///
///  @brief    Translate command starting with a caret (^). Most TECO commands
///            which are control characters (^A, ^B, etc) can also be entered
///            as a caret and letter combination. For example, control-A can
///            also be entered as caret-A.
///
///  @returns  true if command completed, else false.
///
////////////////////////////////////////////////////////////////////////////////

static const struct cmd_table *scan_caret(struct cmd *cmd)
{
    assert(cmd != NULL);

    int c = cmd->c1;
    int ctrl = (toupper(c) - 'A') + 1;  // Convert to control character

    if (ctrl <= NUL || ctrl >= SPACE)
    {
        printc_err(E_IUC, c);           // Illegal character following ^
    }

    cmd->c1 = (char)ctrl;

    return &cmd_table[ctrl];
}


///
///  @brief    Set command options. The command tables in this file, e_cmd.c,
///            and f_cmd.c all include strings that define the options for each
///            command. The while() and switch() statements below parse these
///            strings to set the appropriate options, as follows:
///
///            n  - Command allows one argument          (e.g., nC).
///            m  - Command allows two arguments         (e.g., m,nT).
///            :  - Command allows colon modifier        (e.g., :ERfile`).
///            :: - Command allows double colon modifier (e.g., ::Stext`).
///            @  - Command allows atsign form           (e.g., @^A/hello/).
///            q  - Command requires Q-register          (e.g., Mq).
///            W  - Command allows W                     (e.g., PW).
///            1  - Command allows one text string       (e.g., Otag`).
///            2  - Command allows two text strings      (e.g., FNfoo`baz`).
///
///            These do not need to be in any particular order, and characters
///            not in the list above will be ignored. This allows the use of
///            spaces to improve readability.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void set_opts(struct cmd *cmd, const char *opts)
{
    assert(cmd != NULL);

    if (opts == NULL)
    {
        return;
    }

    int c;

    while ((c = *opts++) != NUL)
    {
        switch (c)
        {
            case 'm':
                cmd->opt_m = true;
                //lint -fallthrough

            case 'n':
                cmd->opt_n = true;

                break;

            case ':':
                if (*opts == ':')
                {
                    cmd->opt_dcolon = true;

                    ++opts;
                }
                else
                {
                    cmd->opt_colon  = true;
                }

                break;

            case '@':
                cmd->opt_atsign = true;

                break;

            case 'q':
                cmd->opt_qreg = true;

                break;

            case 'W':
                cmd->opt_w  = true;

                break;

            case '2':
                cmd->opt_t2 = true;
                //lint -fallthrough

            case '1':
                cmd->opt_t1 = true;

                break;

            default:
                break;
        }
    }
}
