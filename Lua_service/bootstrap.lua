local ls = require "lovesnow"
local harbor = require "lovesnow.harbor"
require "lovesnow.manager"	-- import ls.launch, ...
local memory = require "lovesnow.memory"

ls.start(function()
	local sharestring = tonumber(ls.getenv "sharestring" or 4096)
	memory.ssexpand(sharestring)

	local standalone = ls.getenv "standalone"

	local launcher = assert(ls.launch("ls_load_script","launcher"))
	ls.name(".launcher", launcher)

	local harbor_id = tonumber(ls.getenv "harbor" or 0)
	if harbor_id == 0 then
		assert(standalone ==  nil)
		standalone = true
		ls.setenv("standalone", "true")

		local ok, slave = pcall(ls.newservice, "cdummy")
		if not ok then
			ls.abort()
		end
		ls.name(".cslave", slave)

	else
		if standalone then
			if not pcall(ls.newservice,"cmaster") then
				ls.abort()
			end
		end

		local ok, slave = pcall(ls.newservice, "cslave")
		if not ok then
			ls.abort()
		end
		ls.name(".cslave", slave)
	end

	if standalone then
		local datacenter = ls.newservice "datacenterd"
		ls.name("DATACENTER", datacenter)
	end
	ls.newservice "service_mgr"
	pcall(ls.newservice,ls.getenv "start" or "main")
	ls.exit()
end)
