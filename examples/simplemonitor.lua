local ls = require "lovesnow"

-- It's a simple service exit monitor, you can do something more when a service exit.

local service_map = {}

ls.register_protocol {
	name = "client",
	id = ls.PTYPE_CLIENT,	-- PTYPE_CLIENT = 3
	unpack = function() end,
	dispatch = function(_, address)
		local w = service_map[address]
		if w then
			for watcher in pairs(w) do
				ls.redirect(watcher, address, "error", 0, "")
			end
			service_map[address] = false
		end
	end
}

local function monitor(session, watcher, command, service)
	assert(command, "WATCH")
	local w = service_map[service]
	if not w then
		if w == false then
			ls.ret(ls.pack(false))
			return
		end
		w = {}
		service_map[service] = w
	end
	w[watcher] = true
	ls.ret(ls.pack(true))
end

ls.start(function()
	ls.dispatch("lua", monitor)
end)
