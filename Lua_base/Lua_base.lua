
local x = require "lovesnow"
local x1 = require "lovesnow.ff1"
local x2 = require "lovesnow.ff2"
--local x3 = require "lovesnow.ff3"
local core = require "lovesnow.core"
local pf = require "lovesnow.profile"

function call_x()
	print("in call_x")
	x.dllHello()
	x1.subff();
	x2.subff();
	print(core.now())
	ls.exeHello()
end
