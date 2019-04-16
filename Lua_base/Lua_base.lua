
local x = require "mLualib"

function call_x()
	print("in call_x")
	x.dllHello()
	ls.exeHello()
end

call_x()
print("Lua_base.lua end")


