# Command-Line C Shell

**Developer:** Ayush Munjial  
**Technologies:** C · POSIX · Linux Systems · GCC · Makefile  
**Project Duration:** Jan 2024 – Feb 2024

---

## 📌 Overview

A fully functional, POSIX-compliant **command-line shell** built in C for UNIX-based systems. This shell supports both **interactive mode** and **batch script execution**, offering core features expected in a traditional shell: command parsing, I/O redirection, pipelining, and process control.

Designed with modular logic and low-level system calls, it emphasizes performance, reliability, and maintainability—making it a strong foundation for extended shell features or OS-level tooling.

---

## ✅ Features

- 🧠 **Interactive & Batch Modes**  
  Supports real-time terminal input or reading and executing from a script file.

- 🔀 **Robust Command Parsing**  
  Handles complex input with wildcards, special characters, and escape sequences.

- 📂 **Built-in Directory Navigation**  
  Implements `cd` and `pwd` with support for `~` and empty arguments to navigate the file system easily.

- 🔧 **I/O Redirection**  
  Enables standard input/output redirection (`<`, `>`) using `dup2()` for seamless file-based operations.

- 🔗 **Pipelining Support**  
  Allows chaining of multiple commands via pipes (`|`) with efficient inter-process communication.

- 🧩 **Escape Sequence Handling**  
  Users can escape whitespace and symbols like `<`, `>`, `|`, `*`, and `\` using backslashes.

- 🏠 **Home Directory Expansion**  
  Interprets `~` or `~/path` by dynamically resolving `$HOME`.

- ⚙️ **Efficient Subprocess Management**  
  Executes commands using `fork()`, `execv()`, and `wait()`, ensuring accurate exit status tracking.

- 📦 **Wildcard Expansion (Globbing)**  
  Implements custom logic to expand file patterns using `*`, supporting partial and nested matches.

---

## 🖥 Example Usage

### Interactive Mode
```bash
$ ./myshell
Welcome to my shell!
mysh> cd ~/Documents
mysh> echo Hello, world! > out.txt
mysh> cat < out.txt | rev
!dlrow ,olleH
mysh> exit
```

### Batch Mode
```bash
$ ./myshell commands.txt
```

---

## 🛠 How to Build & Run

1. **Clone the repository**
   ```bash
   git clone https://github.com/ayushmunjial/command-line-c-shell.git
   cd command-line-c-shell
   ```

2. **Build with Make**
   ```bash
   make
   ```

3. **Execute the shell**
   - Run interactively: `./myshell`
   - Run in batch mode: `./myshell script.txt`

4. **Clean build artifacts**
   ```bash
   make clean
   ```

---

## 📚 Sample Commands

```bash
mysh> ls -la
mysh> pwd | rev
mysh> sort < input.txt > output.txt
mysh> echo foo\ bar > spaced.txt
mysh> cd ~/Downloads
mysh> exit
```

---

## 🧩 Architecture & Key Components

- **myshell.c** — Contains the main loop, command parser, process launcher, and execution logic.
- **Makefile** — Automates compilation and cleanup.

### Key Functions
- `parse_command()` — Tokenizes command strings and handles escape sequences.
- `execute_Command()` — Handles I/O redirection, pipelining, and control flow.
- `execute_Process()` — Launches built-in or external commands with resolved paths.
- `storeArguments()` — Processes and expands wildcard arguments.

---

## 🧪 Testing Strategy

The shell was tested across diverse input cases to ensure correctness and robustness:
- Valid and invalid directory changes
- I/O redirection with varying placement in commands
- Complex piped commands
- Wildcard pattern matching across directory structures
- Mixed batch and interactive sessions
- Exit status tracking with failed commands (affecting prompt prefix)

---

## ⚖️ License

For personal and professional portfolio use only. All rights reserved by Ayush Munjial.
