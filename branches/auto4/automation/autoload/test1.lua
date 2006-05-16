-- Automation 4 test file
-- Create a Macro feature, that displays some text

script_name = "Automation 4 test 1"
script_description = "Hello World in Automation 4/Lua"
script_author = "Niels Martin Hansen"
script_version = "1"


function macro_test1(subtitles, selected_lines, active_line)
	--aegisub.debug.out(3, "Hello World from %s", "Automation 4/Lua")
	showmessage("Hello Automation 4 World!")
end

function macro_test2(subtitles, selected_lines, active_line)
	showmessage(subtitles.n .. " and " .. #subtitles .. " should be the same value")
end

function longloop(subtitles, selected_lines, active_line)
	return #selected_lines > 1
end

aegisub.register_macro("Hello", "Shows a message", "tools", macro_test1, longloop)

aegisub.register_macro("File line count", "Count the number of lines in the ASS file", "tools", macro_test2, nil)
