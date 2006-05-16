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
	showmessage(subtitles[1].class)
end

function dumper(subtitles, selected_lines, active_line)
	for i = 1, #subtitles do
		local l = subtitles[i]
		local s = l.raw .. "\n"
		s = s .. l.class .. "\n"
		if l.class == "comment" then
			s = s .. "text: " .. l.text .. "\n"
		elseif l.class == "info" then
			s = s .. string.format("key: %s\nvalue:%s\n", l.key, l.value)
		elseif l.class == "format" then
			-- skip
		elseif l.class == "style" then
			s = s .. string.format("name: %s\nfont: %s %d\ncolors: %s %s %s %s\n", l.name, l.fontname, l.fontsize, l.color1, l.color2, l.color3, l.color4)
		elseif l.class == "dialogue" then
			s = s .. string.format("layer: %d\nstyle: %s\ntext: %s\n", l.layer, l.style, l.text)
		end
		showmessage(s)
	end
end

function testcount(subtitles, selected_lines, active_line)
	return #selected_lines > 1
end

aegisub.register_macro("Hello", "Shows a message", "tools", macro_test1, testcount)

aegisub.register_macro("File line count", "Count the number of lines in the ASS file", "tools", macro_test2, nil)

aegisub.register_macro("Dump", "Dumps info on every line in the file", "tools", dumper, nil)
