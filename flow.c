#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 32768
#define MAX_COMPONENTS 100
#define MAX_NAME 256
#define MAX_COMMAND 32768
#define MAX_PARTS 50

// Component types
typedef enum {
    TYPE_NODE,
    TYPE_PIPE,
    TYPE_CONCATENATE,
    TYPE_STDERR,
    TYPE_FILE
} ComponentType;

// Component structure
typedef struct {
    ComponentType type;
    char name[MAX_NAME];
    char command[MAX_COMMAND];  // For nodes
    char from[MAX_NAME];         // For pipes, stderr
    char to[MAX_NAME];           // For pipes
    int parts_count;             // For concatenate
    char parts[MAX_PARTS][MAX_NAME]; // For concatenate
    char filename[MAX_NAME];     // For files
    int visited;                 // For cycle detection
    int in_progress;            // For cycle detection
} Component;

// Global storage
Component components[MAX_COMPONENTS];
int component_count = 0;

// Function prototypes
Component* find_component(const char* name);
int parse_flow_file(const char* filename);
int execute_component(const char* name, int input_fd, int output_fd);
int execute_node(Component* node, int input_fd, int output_fd);
int execute_pipe(Component* pipe_comp, int input_fd, int output_fd);
int execute_concatenate(Component* concat, int input_fd, int output_fd);
int execute_stderr(Component* stderr_comp, int input_fd, int output_fd);
int execute_file(Component* file, int input_fd, int output_fd);
void parse_command(const char* cmd_str, char** argv);
int detect_cycle(const char* name);

// Find component by name
Component* find_component(const char* name) {
    for (int i = 0; i < component_count; i++) {
        if (strcmp(components[i].name, name) == 0) {
            return &components[i];
        }
    }
    return NULL;
}

// Parse the .flow file
int parse_flow_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return -1;
    }

    char line[MAX_LINE];
    Component* current = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) continue;

        // Skip comment lines (lines starting with #)
        // Also skip lines that start with whitespace followed by #
        char* trimmed = line;
        while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
        if (*trimmed == '#' || *trimmed == '\0') continue;
        
        // Parse component definitions
        if (strncmp(line, "node=", 5) == 0) {
            current = &components[component_count++];
            current->type = TYPE_NODE;
            strcpy(current->name, line + 5);
            current->visited = 0;
            current->in_progress = 0;
        }
        else if (strncmp(line, "pipe=", 5) == 0) {
            current = &components[component_count++];
            current->type = TYPE_PIPE;
            strcpy(current->name, line + 5);
            current->visited = 0;
            current->in_progress = 0;
        }
        else if (strncmp(line, "concatenate=", 12) == 0) {
            current = &components[component_count++];
            current->type = TYPE_CONCATENATE;
            strcpy(current->name, line + 12);
            current->parts_count = 0;
            current->visited = 0;
            current->in_progress = 0;
        }
        else if (strncmp(line, "stderr=", 7) == 0) {
            current = &components[component_count++];
            current->type = TYPE_STDERR;
            strcpy(current->name, line + 7);
            current->visited = 0;
            current->in_progress = 0;
        }
        else if (strncmp(line, "file=", 5) == 0) {
            current = &components[component_count++];
            current->type = TYPE_FILE;
            strcpy(current->name, line + 5);
            current->visited = 0;
            current->in_progress = 0;
        }
        // Parse attributes
        else if (current && strncmp(line, "command=", 8) == 0) {
            strcpy(current->command, line + 8);
        }
        else if (current && strncmp(line, "from=", 5) == 0) {
            strcpy(current->from, line + 5);
        }
        else if (current && strncmp(line, "to=", 3) == 0) {
            strcpy(current->to, line + 3);
        }
        else if (current && strncmp(line, "parts=", 6) == 0) {
            current->parts_count = atoi(line + 6);
        }
        else if (current && strncmp(line, "part_", 5) == 0) {
            int part_idx = atoi(line + 5);
            char* equals = strchr(line, '=');
            if (equals) {
                strcpy(current->parts[part_idx], equals + 1);
            }
        }
        else if (current && strncmp(line, "name=", 5) == 0) {
            strcpy(current->filename, line + 5);
        }
    }
    
    fclose(file);
    return 0;
}

// Parse command string into argv array
// Replace the existing parse_command function with this version:
void parse_command(const char* cmd_str, char** argv) {
    char* cmd_copy = strdup(cmd_str);
    char* ptr = cmd_copy;
    int i = 0;
    
    while (*ptr && i < 99) {
        // Skip whitespace
        while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
        if (!*ptr) break;
        
        char* start;
        if (*ptr == '\'') {
            // Single-quoted argument
            ptr++; // Skip opening quote
            start = ptr;
            while (*ptr && *ptr != '\'') ptr++;
            if (*ptr == '\'') {
                *ptr = '\0';
                ptr++;
            }
            argv[i++] = strdup(start);
        } else if (*ptr == '"') {
            // Double-quoted argument  
            ptr++; // Skip opening quote
            start = ptr;
            while (*ptr && *ptr != '"') ptr++;
            if (*ptr == '"') {
                *ptr = '\0';
                ptr++;
            }
            argv[i++] = strdup(start);
        } else {
            // Regular argument
            start = ptr;
            while (*ptr && *ptr != ' ' && *ptr != '\t') ptr++;
            if (*ptr) {
                *ptr = '\0';
                ptr++;
            }
            argv[i++] = strdup(start);
        }
    }
    argv[i] = NULL;
    free(cmd_copy);
}

// Execute a node (process)
int execute_node(Component* node, int input_fd, int output_fd) {
    char* argv[100];
    parse_command(node->command, argv);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        execvp(argv[0], argv);
        fprintf(stderr, "Error executing command: %s\n", node->command);
        exit(1);
    }
    
    // Parent - cleanup argv
    for (int i = 0; argv[i]; i++) {
        free(argv[i]);
    }
    
    // Close file descriptors in parent
    if (input_fd != STDIN_FILENO) close(input_fd);
    if (output_fd != STDOUT_FILENO) close(output_fd);
    
    // Wait for child
    int status;
    waitpid(pid, &status, 0);
    
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// Execute a pipe
int execute_pipe(Component* pipe_comp, int input_fd, int output_fd) {
    Component* from = find_component(pipe_comp->from);
    Component* to = find_component(pipe_comp->to);
    
    if (!from || !to) {
        fprintf(stderr, "Error: Invalid pipe configuration\n");
        return -1;
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // First child - execute 'from' component
        close(pipefd[0]);  // Close read end
        
        // Redirect stdout to pipe write end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Redirect stdin if needed
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        // Execute the 'from' component
        execute_component(from->name, STDIN_FILENO, STDOUT_FILENO);
        exit(0);
    }
    
    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Second child - execute 'to' component
        close(pipefd[1]);  // Close write end
        
        // Redirect stdin to pipe read end
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        // Redirect stdout if needed
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // Execute the 'to' component
        execute_component(to->name, STDIN_FILENO, STDOUT_FILENO);
        exit(0);
    }
    
    // Parent
    close(pipefd[0]);
    close(pipefd[1]);
    if (input_fd != STDIN_FILENO) close(input_fd);
    if (output_fd != STDOUT_FILENO) close(output_fd);
    
    // Wait for both children
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    return 0;
}

// Execute concatenate
int execute_concatenate(Component* concat, int input_fd, int output_fd) {
    for (int i = 0; i < concat->parts_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child - execute part
            execute_component(concat->parts[i], STDIN_FILENO, output_fd);
            exit(0);
        }
        // Parent - wait for child
        waitpid(pid, NULL, 0);
    }
    
    if (output_fd != STDOUT_FILENO) close(output_fd);
    return 0;
}

// Execute stderr redirection
int execute_stderr(Component* stderr_comp, int input_fd, int output_fd) {
    Component* from = find_component(stderr_comp->from);
    if (!from || from->type != TYPE_NODE) {
        fprintf(stderr, "Error: stderr must redirect from a node\n");
        return -1;
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    char* argv[100];
    parse_command(from->command, argv);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child - redirect stderr to pipe
        close(pipefd[0]);
        dup2(pipefd[1], STDERR_FILENO);
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
        }
        close(pipefd[1]);
        
        execvp(argv[0], argv);
        fprintf(stderr, "Error executing command: %s\n", from->command);
        exit(1);
    }
    
    // Parent
    close(pipefd[1]);
    if (input_fd != STDIN_FILENO) close(input_fd);
    
    // Read from stderr pipe and write to output
    char buffer[1024];
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        write(output_fd, buffer, n);
    }
    
    close(pipefd[0]);
    if (output_fd != STDOUT_FILENO) close(output_fd);
    
    // Cleanup argv
    for (int i = 0; argv[i]; i++) {
        free(argv[i]);
    }
    
    waitpid(pid, NULL, 0);
    return 0;
}

// Execute file (extra credit)
int execute_file(Component* file, int input_fd, int output_fd) {
    int fd = open(file->filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error: %s: No such file or directory\n", file->filename);
        return -1;
    }
    
    char buffer[32768];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        write(output_fd, buffer, n);
    }
    
    close(fd);
    if (output_fd != STDOUT_FILENO) close(output_fd);
    return 0;
}

// Detect cycles using DFS
int detect_cycle_helper(Component* comp, int* visited_stack) {
    if (!comp) return 0;
    
    // Mark this component as being processed in current path
    int idx = comp - components;  // Get index of component
    
    // If already in current path, cycle detected
    if (visited_stack[idx] == 1) {
        return 1;  // Cycle found
    }
    
    // If already fully processed in a previous path, skip
    if (visited_stack[idx] == 2) {
        return 0;  // Already checked, no cycle from here
    }
    
    // Mark as in current path
    visited_stack[idx] = 1;
    
    // Check dependencies based on component type
    int has_cycle = 0;
    
    if (comp->type == TYPE_PIPE) {
        Component* from = find_component(comp->from);
        Component* to = find_component(comp->to);
        
        if (from && detect_cycle_helper(from, visited_stack)) {
            has_cycle = 1;
        }
        if (!has_cycle && to && detect_cycle_helper(to, visited_stack)) {
            has_cycle = 1;
        }
    } 
    else if (comp->type == TYPE_CONCATENATE) {
        for (int i = 0; i < comp->parts_count && !has_cycle; i++) {
            Component* part = find_component(comp->parts[i]);
            if (part && detect_cycle_helper(part, visited_stack)) {
                has_cycle = 1;
            }
        }
    } 
    else if (comp->type == TYPE_STDERR) {
        Component* from = find_component(comp->from);
        if (from && detect_cycle_helper(from, visited_stack)) {
            has_cycle = 1;
        }
    }
    
    // Mark as fully processed
    visited_stack[idx] = 2;
    
    return has_cycle;
}

int detect_cycle(const char* name) {
    Component* comp = find_component(name);
    if (!comp) return 0;
    
    // Create visited stack array
    // 0 = not visited, 1 = in current path, 2 = fully processed
    int* visited_stack = calloc(component_count, sizeof(int));
    if (!visited_stack) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;  // Err on side of caution
    }
    
    int has_cycle = detect_cycle_helper(comp, visited_stack);
    
    free(visited_stack);
    return has_cycle;
}

// Main execution function
int execute_component(const char* name, int input_fd, int output_fd) {
    Component* comp = find_component(name);
    if (!comp) {
        fprintf(stderr, "Error: Component '%s' not found\n", name);
        return -1;
    }
    
    switch (comp->type) {
        case TYPE_NODE:
            return execute_node(comp, input_fd, output_fd);
        case TYPE_PIPE:
            return execute_pipe(comp, input_fd, output_fd);
        case TYPE_CONCATENATE:
            return execute_concatenate(comp, input_fd, output_fd);
        case TYPE_STDERR:
            return execute_stderr(comp, input_fd, output_fd);
        case TYPE_FILE:
            return execute_file(comp, input_fd, output_fd);
        default:
            fprintf(stderr, "Error: Unknown component type\n");
            return -1;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <flow_file> <component_name>\n", argv[0]);
        return 1;
    }
    
    // Parse the flow file
    if (parse_flow_file(argv[1]) < 0) {
        return 1;
    }
    
    // Check for cycles
    if (detect_cycle(argv[2])) {
        fprintf(stderr, "Error: Cyclic dependency detected\n");
        return 1;
    }
    
    // Execute the specified component
    return execute_component(argv[2], STDIN_FILENO, STDOUT_FILENO);
}