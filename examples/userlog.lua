local ls = require "lovesnow"
require "lovesnow.manager"

-- register protocol text before ls.start would be better.
ls.register_protocol {
	name = "text",
	id = ls.PTYPE_TEXT,
	unpack = ls.tostring,
	dispatch = function(_, address, msg)
		print(string.format(":%08x(%.2f): %s", address, ls.time(), msg))
	end
}

ls.register_protocol {
	name = "SYSTEM",
	id = ls.PTYPE_SYSTEM,
	unpack = function(...) return ... end,
	dispatch = function()
		-- reopen signal
		print("SIGHUP")
	end
}

ls.start(function()
end)