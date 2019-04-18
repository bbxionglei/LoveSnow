local ls = require "lovesnow"

local service_name = (...)
local init = {}

function init.init(code, ...)
	local start_func
	ls.start = function(f)
		start_func = f
	end
	ls.dispatch("lua", function() error("No dispatch function")	end)
	local mainfunc = assert(load(code, service_name))
	assert(ls.pcall(mainfunc,...))
	if start_func then
		start_func()
	end
	ls.ret()
end

ls.start(function()
	ls.dispatch("lua", function(_,_,cmd,...)
		init[cmd](...)
	end)
end)
