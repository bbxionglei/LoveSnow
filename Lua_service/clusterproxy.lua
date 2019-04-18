local ls = require "lovesnow"
local cluster = require "lovesnow.cluster"
require "lovesnow.manager"	-- inject ls.forward_type

local node, address = ...

ls.register_protocol {
	name = "system",
	id = ls.PTYPE_SYSTEM,
	unpack = function (...) return ... end,
}

local forward_map = {
	[ls.PTYPE_SNAX] = ls.PTYPE_SYSTEM,
	[ls.PTYPE_LUA] = ls.PTYPE_SYSTEM,
	[ls.PTYPE_RESPONSE] = ls.PTYPE_RESPONSE,	-- don't free response message
}

ls.forward_type( forward_map ,function()
	local clusterd = ls.uniqueservice("clusterd")
	local n = tonumber(address)
	if n then
		address = n
	end
	ls.dispatch("system", function (session, source, msg, sz)
		if session == 0 then
			ls.send(clusterd, "lua", "push", node, address, msg, sz)
		else
			ls.ret(ls.rawcall(clusterd, "lua", ls.pack("req", node, address, msg, sz)))
		end
	end)
end)
