local ls = require "lovesnow"

local datacenter = {}

function datacenter.get(...)
	return ls.call("DATACENTER", "lua", "QUERY", ...)
end

function datacenter.set(...)
	return ls.call("DATACENTER", "lua", "UPDATE", ...)
end

function datacenter.wait(...)
	return ls.call("DATACENTER", "lua", "WAIT", ...)
end

return datacenter

