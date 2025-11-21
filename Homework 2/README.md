# os-hw2
OS Homework 2

This is the report for OS Homework 2 - NYU Fall 25 by Prof. Kamen Yotov. The assignment description and details can be found [here](https://docs.google.com/document/d/1XCBog6L_-6xNodVUviXSdELA2WCJVdCG-1gMVFV1iuQ/edit?tab=t.0).


## Instructions to Run

1. Run the following command to compile. 
`gcc flow.c -o flow`

2. Test the filecout.flow 
`./flow filecount.flow doit`

3. Test the complicated flow 
`./flow complicated.flow shenanigan`

4. Test the error flow
`./flow error.flow process_pipe`

5. Test a list of test cases mentioned in the `your_tests.flow`.
```
chmod +x run_tests.sh
./run_tests.sh
```

# Other Details. 
# Flow Language Interpreter - Implementation Documentation

## Overview
This program implements an interpreter for the `.flow` language, which allows users to define and chain processes in a directed acyclic graph (DAG) structure, similar to Unix shell piping but with more flexibility.

## Implementation Details

### Core Components Implemented

1. **node**: Represents a single process/command
   - Stores command string and parses it into argv array
   - Executes using fork() and execvp()
   - Handles stdin/stdout redirection

2. **pipe**: Connects stdout of one component to stdin of another
   - Creates pipe using pipe() system call
   - Forks two processes for source and destination
   - Properly manages file descriptors

3. **concatenate**: Runs multiple components sequentially
   - Executes each part independently
   - Outputs are streamed sequentially to the same destination
   - Parts do not feed into each other (as specified)

4. **stderr**: Captures stderr stream from a node
   - Redirects stderr to a pipe
   - Makes stderr available as stdout for further processing

5. **file** (Extra Credit): File input/output handling
   - Opens files using open() system call
   - Streams file content to output descriptor

### Key Design Decisions

1. **Component Storage**: All components are stored in a global array for easy lookup by name.

2. **Execution Model**: 
   - Recursive execution starting from the specified component
   - Each component type has its own execution function
   - Proper file descriptor management throughout

3. **Process Management**:
   - Uses fork() for creating child processes
   - waitpid() for synchronization
   - Avoids system() as required

4. **Cycle Detection**:
   - Implements DFS-based cycle detection algorithm
   - Uses visited and in_progress flags
   - Prevents infinite loops in execution

### Error Handling

- File not found errors are reported clearly
- Command execution failures are caught and reported
- Invalid component references are detected
- Cyclic dependencies are detected before execution

### Assumptions Made

1. Component names are unique within a flow file
2. Commands don't exceed MAX_COMMAND (32768) characters
3. Maximum 100 components per flow file
4. Maximum 50 parts in a concatenate directive
5. Input flow files are syntactically correct (as stated in requirements)

### Memory Management

- Dynamic memory allocated for command parsing is properly freed
- File descriptors are closed after use to prevent leaks
- Child processes exit cleanly after execution

### Limitations

- Does not implement complex shell features like globbing or variable expansion
- Commands are executed with execvp(), so PATH resolution applies
- No support for shell built-ins that don't exist as separate programs

## Extra Credit Implementation

The file component allows reading from files as input sources. Files can be used as sources in pipes, making it possible to implement input redirection (< operator equivalent).

Example flow for `wc < result.txt`:
```
file=input_file
name=result.txt

node=word_count
command=wc

pipe=count_file
from=input_file
to=word_count
```