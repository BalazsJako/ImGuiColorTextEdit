# ImGuiColorTextEdit

A specialization of ImGuiColorTextEdit focusing solely on Lua 5.1.

Main Features:
 - A bar for breakpoints to the left of the line numbers.
 - Lexer for syntax highlighting and proper multiline comment and string support.
 - Parser that can detect syntax errors and output data for globals, upvalues and locals (scope included).

Drawbacks:
 - Does a full lexing and parsing pass every time a change is made to the text. But it's still pretty fast even for large files.
