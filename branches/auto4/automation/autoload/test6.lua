-- Automation 4 test file
-- Create a Filter feature that does some kara stuff

script_name = "Automation 4 test 6"
script_description = "Test basic export filters"
script_author = "Niels Martin Hansen"
script_version = "1"

include("utils.lua")


function test6_2(subtitles, config)
	--[[for i = 1, #subtitles do
		local l = subtitles[i]
		if l.class == "dialogue" then
			local nl = table.copy(l)
			nl.text = "Copied!"
			subtitles.insert(i, nl)
			break
		end
	end]]
end


function test6(subtitles, config)
	--[[aegisub.progress.task("Collecting style data")
	local styles = {}
	for i = 1, #subtitles do
		local l = subtitles[i]
		if l.class == "dialogue" then
			break
		end
		if l.class == "style" then
			styles[l.name] = l
		end
		aegisub.progress.set(100 * i / #subtitles)
	end
	
	local res = {}
	
	local i = 1
	while i <= #subtitles do
		local l = subtitles[i]
		if l.class == "dialogue" then
			local res = {}
			do_line(styles, l, config, res)
			for j,nl in ipairs(res) do
				subtitles.insert(i+j, nl)
			end
			subtitles.delete(i)
			i = i + #res
		else
			i = i + 1
		end
		aegisub.progress.task("Producing effect (%d/%d)", i, #subtitles)
		aegisub.progress.set(100 * i / #subtitles)
	end]]
end

function do_line(styles, line, config, res)
	local k = aegisub.parse_karaoke_data(line)
	local left = 0
	for j = 1, #k do
		local nl = table.copy(line)
		l.text = string.format("{\\t(%d,%d,\\fscx50)\\pos(%d,20)}%s", k[j].start_time, k[j].end_time, left, k[j].text_stripped)
		left = left + (aegisub.text_extents(styles[l.style], k[j].text_stripped))
		table.insert(res, l)
	end
end


aegisub.register_filter("Stupid karaoke", "Makes some more karaoke-like stuff", 2000, test6, nil)
aegisub.register_filter("Lame test", "Simple test of filters, just inserting a new line", 2000, test6_2)
