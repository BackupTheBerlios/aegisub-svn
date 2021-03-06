--[[
 Copyright (c) 2005, Niels Martin Hansen
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   * Neither the name of the Aegisub Group nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 ]]

-- Aegisub Automation include file
-- This include file is an extension of karaskel.lua providing out-of-the-box support for per-syllable
-- positioning of karaoke.

-- It automatically includes and re-setups karaskel.lua, so you should not include that yourself!
include("karaskel-base.lua")
karaskel.engage_positioning = true

-- The interface here has been greatly simplified, there is only one function to override, do_syllable
-- The format for that one has changed.
-- The rest of the functions can be "emulated" through the do_syllable function.
-- Though you can still also override do_line and get an effect our of it, it's not advisable, since that
-- defeats the entire purpose of using this include file.

-- The arguments here mean the same as in karaskel.lua, and all tables have the same members
-- The return value is different now, though.
-- It is required to be in the same format as the do_line function:
-- A table with an "n" key, and keys 0..n-1 with line structures.
function karaskel.do_syllable(meta, styles, config, line, syl)
	karaskel.trace("default_do_syllable")
	return {n=0}
end
do_syllable = karaskel.do_syllable

function karaskel.do_line(meta, styles, config, line)
	karaskel.trace("adv_do_line")
	if line.kind ~= "dialogue" then
		return {n=1, [1]=line}
	end
	local result = {n=0}
	for i = 0, line.karaoke.n-1 do
		karaskel.trace("adv_do_line:2:"..i)
		local out = do_syllable(meta, styles, config, line, line.karaoke[i])
		for j = 1, out.n do
			table.insert(result, out[j])
		end
	end
	karaskel.trace("adv_do_line:3")
	return result
end
do_line = karaskel.do_line
