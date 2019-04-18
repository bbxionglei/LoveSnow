local ls = require "lovesnow"

local harbor = {}

function harbor.globalname(name, handle)
	handle = handle or ls.self()
	ls.send(".cslave", "lua", "REGISTER", name, handle)
end

function harbor.queryname(name)
	return ls.call(".cslave", "lua", "QUERYNAME", name)
end

function harbor.link(id)
	ls.call(".cslave", "lua", "LINK", id)
end

function harbor.connect(id)
	ls.call(".cslave", "lua", "CONNECT", id)
end

function harbor.linkmaster()
	ls.call(".cslave", "lua", "LINKMASTER")
end

return harbor
