# ImGuiColorTextEdit
Syntax highlighting text editor for ImGui

![Screenshot](https://github.com/BalazsJako/ImGuiColorTextEdit/blob/master/ImGuiTextEdit.png "Screenshot")

Demo project: https://github.com/BalazsJako/ColorTextEditorDemo

This started as my attempt to write a relatively simple widget which provides source code editing functionality with basic syntax highlighting. Now there are other contributors who provide valuable additions.

While it relies on Omar Cornut's https://github.com/ocornut/imgui, it does not follow the "pure" one widget - one function approach. Since the editor has to maintain a relatively complex and large internal state, it did not seem to be practical to try and enforce fully immediate mode.

The code is (still) work in progress, please report if you find any issues.

# Main features
 - approximates typical code editor look and feel (essential mouse/keyboard commands work - I mean, the commands _I_ normally use :))
 - undo/redo support
 - extensible, multiple language syntax support
 - identifier declarations: a small piece of text associated with an identifier. The editor displays it in a tooltip when the mouse cursor is hovered over the identifier
 - error markers: the user can specify a list of error messages together the line of occurence, the editor will highligh the lines with red backround and display error message in a tooltip when the mouse cursor is hovered over the line
 - supports large files: there is no explicit limit set on file size or number of lines, performance is not affected when large files are loaded (except syntax coloring, see below)
 - color palette support: you can switch between different color palettes, or even define your own
 - supports both fixed and variable-width fonts
 
# Known issues
 - syntax highligthing of most languages - except C/C++ - is based on std::regex, which is diasppointingly slow. Because of that, the highlighting process is amortized between multiple frames. C/C++ has a hand-written tokenizer which is much faster. 
 - 8 bit character only, no Unicode or Utf support
 - there's no find/replace support

Don't forget to post your screenshots if you use this little piece of software in order to keep me us motivated. :)

# Contribute

If you want to contribute, please refer to CONTRIBUTE file.

Thank you. :)
