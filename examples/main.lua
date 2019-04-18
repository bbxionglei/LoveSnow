local ls = require "lovesnow"
local sprotoloader = require "sprotoloader"

local max_client = 64

ls.start(function()
	ls.error("Server start")
	ls.uniqueservice("protoloader")
	if not ls.getenv "daemon" then
		local console = ls.newservice("console")
	end
	ls.newservice("debug_console",8000)
	ls.newservice("simpledb")
	local watchdog = ls.newservice("watchdog")
	ls.call(watchdog, "lua", "start", {
		port = 8888,
		maxclient = max_client,
		nodelay = true,
	})
	ls.error("Watchdog listen on", 8888)
	ls.exit()
end)