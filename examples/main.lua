local ls = require "lovesnow"
local sprotoloader = require "sprotoloader"

local max_client = 64

ls.start(function()
	ls.error("Server start")
	ls.uniqueservice("protoloader")
	-- TODO  console cmdline 为 false 时 程序出bug
	if not ls.getenv "daemon" then
		local console = ls.newservice("console")
	end
	ls.newservice("debug_console",17001)
	ls.newservice("simpledb")
	local watchdog = ls.newservice("watchdog")
	ls.call(watchdog, "lua", "start", {
		port = 17002,
		maxclient = max_client,
		nodelay = true,
	})
	ls.error("Watchdog listen on", 17002)
	ls.exit()
end)
