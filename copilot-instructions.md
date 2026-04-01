# TCP Language Compiler Project Instructions

## Project Purpose
This project aims to produce a compiler for the TCP language, as specified in the file `tcp_lang.txt`.

## Key Files
- **Grammar**: The grammar for the TCP language is defined in `src/parser.y`.
- **Tree Structure**: The tree structure and all related methods are implemented in `src/tree.c`.

## Coding Conventions
- Follow the grammar and language rules as described in `tcp_lang.txt` and implemented in `src/parser.y`.
- All syntax tree manipulations and node creations must use the functions provided in `src/tree.c`.
- When adding new features or making changes, ensure compatibility with the existing grammar and tree structure.
- For semantic analysis, refer to the conventions and patterns established in the current codebase.

## Example Prompts
- "Add a new operator to the TCP grammar."
- "Implement a new semantic check for variable declarations."
- "Refactor tree node creation to support a new type."

## Related Customizations
- Consider creating file-specific instructions for `src/parser.y` and `src/tree.c` if you want to enforce stricter rules or patterns for grammar or tree logic.
- For advanced workflows, define custom agents or hooks for code generation, testing, or validation.

---
This instruction applies to the entire workspace and should be updated as the project evolves.
