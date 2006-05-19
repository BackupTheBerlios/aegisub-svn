-- Automation 4 test file
-- Create a Macro feature, that displays some text

script_name = "Add edgeblur macro"
script_description = "A testmacro showing how to do simple line modification in Automation 4"
script_author = "Niels Martin Hansen"
script_version = "1"


function add_edgeblur(subtitles, selected_lines, active_line)
	for z, i in ipairs(selected_lines) do
		local l = subtitles[i]
		l.text = "{\\be1}" .. l.text
		subtitles[i] = l
	end
	aegisub.set_undo_point("Add edgeblur")
end

aegisub.register_macro("Add edgeblur", "Adds \be1 tags to all selected lines", "edit", add_edgeblur)
