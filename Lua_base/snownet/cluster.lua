local ls = require "lovesnow"

local clusterd
local cluster = {}

function cluster.call(node, address, ...)
	-- ls.pack(...) will free by cluster.core.packrequest
	return ls.call(clusterd, "lua", "req", node, address, ls.pack(...))
end

function cluster.send(node, address, ...)
	-- push is the same with req, but no response
	ls.send(clusterd, "lua", "push", node, address, ls.pack(...))
end

function cluster.open(port)
	if type(port) == "string" then
		ls.call(clusterd, "lua", "listen", port)
	else
		ls.call(clusterd, "lua", "listen", "0.0.0.0", port)
	end
end

function cluster.reload(config)
	ls.call(clusterd, "lua", "reload", config)
end

function cluster.proxy(node, name)
	return ls.call(clusterd, "lua", "proxy", node, name)
end

function cluster.snax(node, name, address)
	local snax = require "lovesnow.snax"
	if not address then
		address = cluster.call(node, ".service", "QUERY", "snaxd" , name)
	end
	local handle = ls.call(clusterd, "lua", "proxy", node, address)
	return snax.bind(handle, name)
end

function cluster.register(name, addr)
	assert(type(name) == "string")
	assert(addr == nil or type(addr) == "number")
	return ls.call(clusterd, "lua", "register", name, addr)
end

function cluster.query(node, name)
	return ls.call(clusterd, "lua", "req", node, 0, ls.pack(name))
end

ls.init(function()
	clusterd = ls.uniqueservice("clusterd")
end)

return cluster
