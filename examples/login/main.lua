local ls = require "lovesnow"

ls.start(function()
	local loginserver = ls.newservice("logind")
	local gate = ls.newservice("gated", loginserver)

	ls.call(gate, "lua", "open" , {
		port = 8888,
		maxclient = 64,
		servername = "sample",
	})
end)
