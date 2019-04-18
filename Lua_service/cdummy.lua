local ls = require "lovesnow"
require "lovesnow.manager"	-- import ls.launch, ...

local globalname = {}
local queryname = {}
local harbor = {}
local harbor_service

ls.register_protocol {
	name = "harbor",
	id = ls.PTYPE_HARBOR,
	pack = function(...) return ... end,
	unpack = ls.tostring,
}

ls.register_protocol {
	name = "text",
	id = ls.PTYPE_TEXT,
	pack = function(...) return ... end,
	unpack = ls.tostring,
}

local function response_name(name)
	local address = globalname[name]
	if queryname[name] then
		local tmp = queryname[name]
		queryname[name] = nil
		for _,resp in ipairs(tmp) do
			resp(true, address)
		end
	end
end

function harbor.REGISTER(name, handle)
	assert(globalname[name] == nil)
	globalname[name] = handle
	response_name(name)
	ls.redirect(harbor_service, handle, "harbor", 0, "N " .. name)
end

function harbor.QUERYNAME(name)
	if name:byte() == 46 then	-- "." , local name
		ls.ret(ls.pack(ls.localname(name)))
		return
	end
	local result = globalname[name]
	if result then
		ls.ret(ls.pack(result))
		return
	end
	local queue = queryname[name]
	if queue == nil then
		queue = { ls.response() }
		queryname[name] = queue
	else
		table.insert(queue, ls.response())
	end
end

function harbor.LINK(id)
	ls.ret()
end

function harbor.CONNECT(id)
	ls.error("Can't connect to other harbor in single node mode")
end

ls.start(function()
	local harbor_id = tonumber(ls.getenv "harbor")
	assert(harbor_id == 0)

	ls.dispatch("lua", function (session,source,command,...)
		local f = assert(harbor[command])
		f(...)
	end)
	ls.dispatch("text", function(session,source,command)
		-- ignore all the command
	end)

	harbor_service = assert(ls.launch("ls_harborm", harbor_id, ls.self()))
end)
