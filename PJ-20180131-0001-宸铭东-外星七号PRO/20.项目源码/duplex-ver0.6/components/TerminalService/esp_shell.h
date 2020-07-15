#ifndef _ESP_SHELL_H_
#define _ESP_SHELL_H_

typedef void (*shellcmd_t)(void *self, int argc, char *argv[]);

/**
 * @brief   Custom command entry type.
 */
typedef struct {
    const char      *sc_name;           /**< @brief Command name.       */
    shellcmd_t      sc_function;        /**< @brief Command function.   */
} ShellCommand;


#define _shell_clr_line(stream)   chprintf(stream, "\033[K")

#define SHELL_NEWLINE_STR   "\r\n"
#define SHELL_MAX_ARGUMENTS 10
#define SHELL_PROMPT_STR "\r\n>"
#define SHELL_MAX_LINE_LENGTH 512

void shell_init(const ShellCommand *_scp, void *ref);
void shell_stop();
#endif
