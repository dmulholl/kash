// -------------------------------------------------------------------------
//  Kash: a simple command line shell.
// -------------------------------------------------------------------------

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// -------------------------------------------------------------------------
//  Built in commands.
// -------------------------------------------------------------------------

// Exit the shell.
void kash_exit(char **args) {
    exit(0);
}

// Change the working directory.
void kash_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "kash: cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("kash: cd");
        }
    }
}

// Print the shell's help text.
void kash_help(char **args) {
    char *helptext =
        "Kash - the Kinda Aimless Shell. "
        "The following commands are built in:\n"
        "  cd       Change the working directory.\n"
        "  exit     Exit the shell.\n"
        "  help     Print this help text.\n"
        ;
    printf("%s", helptext);
}

// A builtin instance associates a command name with a handler function.
struct builtin {
    char *name;
    void (*func)(char **args);
};

// Array of built in commands.
struct builtin builtins[] = {
    {"help", kash_help},
    {"exit", kash_exit},
    {"cd", kash_cd},
};

// Returns the number of registered commands.
int kash_num_builtins() {
    return sizeof(builtins) / sizeof(struct builtin);
}

// -------------------------------------------------------------------------
//  Process/command launcher.
// -------------------------------------------------------------------------

void kash_exec(char **args) {
    for (int i = 0; i < kash_num_builtins(); i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            builtins[i].func(args);
            return;
        }
    }

    pid_t child_pid = fork();

    if (child_pid == 0) {
        execvp(args[0], args);
        perror("kash");
        exit(1);
    } else if (child_pid > 0) {
        int status;
        do {
            waitpid(child_pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    } else {
        perror("kash");
    }
}

// -------------------------------------------------------------------------
//  Input parser.
// -------------------------------------------------------------------------

// Tokenize a string, splitting on whitespace characters. Leading and
// trailing whitespace is ignored. Consecutive whitespace characters are
// treated as a single delimiter. The return value is a NULL terminated
// array of string pointers which needs to be freed once we're finished
// with it.
char** kash_split_line(char *line) {
    int length = 0;
    int capacity = 32;

    char **tokens = malloc(capacity * sizeof(char*));
    if (!tokens) {
        perror("kash");
        exit(1);
    }

    char *delimiters = " \t\r\n";
    char *token = strtok(line, delimiters);

    while (token != NULL) {
        tokens[length] = token;
        length++;

        if (length >= capacity) {
            capacity = (int) (capacity * 1.5);
            tokens = realloc(tokens, capacity * sizeof(char*));
            if (!tokens) {
                perror("kash");
                exit(1);
            }
        }

        token = strtok(NULL, delimiters);
    }

    tokens[length] = NULL;
    return tokens;
}

// Read a single line of input from stdin. The return value is a string
// pointer which needs to be freed once we're finished with it.
char* kash_read_line() {
    char *line = NULL;
    size_t buflen = 0;
    errno = 0;
    ssize_t strlen = getline(&line, &buflen, stdin);
    if (strlen < 0) {
        if (errno) {
            perror("kash");
        }
        exit(1);
    }
    return line;
}

// -------------------------------------------------------------------------
//  Entry point.
// -------------------------------------------------------------------------

int main() {
    while (true) {
        printf("> ");
        char *line = kash_read_line();
        char **args = kash_split_line(line);

        if (args[0] != NULL) {
            kash_exec(args);
        }

        free(line);
        free(args);
    }
}
