# ImGuiColorTextEdit
Syntax highlighting text editor for ImGui

This is my attempt to write a relatively simple widget which provides source code editing functionality with basic syntax highlighting.

While it relies on Omar Cornut's https://github.com/ocornut/imgui, it does not follow the "pure" one widget - one function approach. Since the editor has to maintain a relatively complex internal state, it did not seem to be practical to try and enforce that.

The code is work in progress, please report if you find any issues.

Main features are:
 - approximates typical code editor look and feel (essential mouse/keyboard commands work - I mean, the commands _I_ normally use :))
 - undo/redo support
 - extensible, multiple language syntax support
 - supports large files: there is no explicit limit set on file size or number of lines, performance is not affected when large files are loaded (except syntax coloring, see below)

Known issues:
 - syntax highligthing is based on std::regex, which is diasppointingly slow. Because of that, the highlighting process is amortized between multiple frames. Hand-written colorizers might help resolve this problem
 - 8 bit character only, no Unicode or Utf support (yet)
 - color palette is hardwired into the code atm....
 - there's no find/replace 

