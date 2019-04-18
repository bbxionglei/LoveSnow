local ls = require "lovesnow"
require "lovesnow.manager"	-- import ls.register

ls.start(function()
	ls.dispatch("lua", function(session, address, ...)
		print("[GLOBALLOG]", ls.address(address), ...)
	end)
	ls.register ".log"
	ls.register "LOG"
end)
