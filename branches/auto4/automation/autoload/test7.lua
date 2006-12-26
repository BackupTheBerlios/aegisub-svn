-- Automation 4 test file
-- Create a Filter feature that does some kara stuff

script_name = "Automation 4 test 7"
script_description = "Test config dialogs"
script_author = "Niels Martin Hansen"
script_version = "1"

include("utils.lua")

function test7(subtitles, selected_lines, active_line)
	local a, b = aegisub.dialog.display({{class="label", label="Test..."}}, {})
	report_dialog_result(a, b)
	aegisub.progress.set(50)
	a, b = aegisub.dialog.display({{class="edit", name="foo", text=""}}, {"foo", "bar"})
	report_dialog_result(a, b)
end

function report_dialog_result(button, controls)
	aegisub.debug.out("Dialog closed: ")
	if button == false then
		aegisub.debug.out("cancelled\n")
	elseif button == true then
		aegisub.debug.out("clicked Ok\n")
	else
		aegisub.debug.out("clicked '" .. button .. "'\n")
	end
	for key, val in pairs(controls) do
		aegisub.debug.out(key .. ': ' .. val .. '\n')
	end
	aegisub.debug.out(" - - - - -\n")
end


aegisub.register_macro("Config Dialog 1", "Show a stupid config dialog", "video", test7, nil)
