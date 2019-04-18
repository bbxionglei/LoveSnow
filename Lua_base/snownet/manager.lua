local ls = require "lovesnow"
local c = require "lovesnow.core"

function ls.launch(...)
	local addr = c.command("LAUNCH", table.concat({...}," "))
	if addr then
		return tonumber("0x" .. string.sub(addr , 2))
	end
end

function ls.kill(name)
	if type(name) == "number" then
		ls.send(".launcher","lua","REMOVE",name, true)
		name = ls.address(name)
	end
	c.command("KILL",name)
end

function ls.abort()
	c.command("ABORT")
end

local function globalname(name, handle)
	local c = string.sub(name,1,1)
	assert(c ~= ':')
	if c == '.' then
		return false
	end

	assert(#name <= 16)	-- GLOBALNAME_LENGTH is 16, defined in ls_harbor.h
	assert(tonumber(name) == nil)	-- global name can't be number

	local harbor = require "lovesnow.harbor"

	harbor.globalname(name, handle)

	return true
end

function ls.register(name)
	if not globalname(name) then
		c.command("REG", name)
	end
end

function ls.name(name, handle)
	if not globalname(name, handle) then
		c.command("NAME", name .. " " .. ls.address(handle))
	end
end

local dispatch_message = ls.dispatch_message

function ls.forward_type(map, start_func)
	c.callback(function(ptype, msg, sz, ...)
		local prototype = map[ptype]
		if prototype then
			dispatch_message(prototype, msg, sz, ...)
		else
			local ok, err = pcall(dispatch_message, ptype, msg, sz, ...)
			c.trash(msg, sz)
			if not ok then
				error(err)
			end
		end
	end, true)
	ls.timeout(0, function()
		ls.init_service(start_func)
	end)
end

function ls.filter(f ,start_func)
	c.callback(function(...)
		dispatch_message(f(...))
	end)
	ls.timeout(0, function()
		ls.init_service(start_func)
	end)
end

function ls.monitor(service, query)
	local monitor
	if query then
		monitor = ls.queryservice(true, service)
	else
		monitor = ls.uniqueservice(true, service)
	end
	assert(monitor, "Monitor launch failed")
	c.command("MONITOR", string.format(":%08x", monitor))
	return monitor
end

return ls
