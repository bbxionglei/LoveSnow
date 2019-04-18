local ls = require "lovesnow"
local harbor = require "lovesnow.harbor"
require "lovesnow.manager"	-- import ls.monitor

local function monitor_master()
	harbor.linkmaster()
	print("master is down")
	ls.exit()
end

ls.start(function()
	print("Log server start")
	ls.monitor "simplemonitor"
	local log = ls.newservice("globallog")
	ls.fork(monitor_master)
end)

