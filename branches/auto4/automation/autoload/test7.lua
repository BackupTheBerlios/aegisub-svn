-- Automation 4 test file
-- Create a Filter feature that does some kara stuff

script_name = "Automation 4 test 7"
script_description = "Test config dialogs"
script_author = "Niels Martin Hansen"
script_version = "1"

include("utils.lua")

function test7(subtitles, selected_lines, active_line)
	aegisub.dialog.display({}, {})
	aegisub.progress.set(50)
	aegisub.dialog.display({}, {"foo", "bar"})
	for i = 0,1000 do
		local s = 'a' .. i
	end
end


aegisub.register_macro("Config Dialog 1", "Show a stupid config dialog", "video", test7, nil)
