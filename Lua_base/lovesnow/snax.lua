local ls = require "lovesnow"
local snax_interface = require "snax.interface"

local snax = {}
local typeclass = {}

local interface_g = ls.getenv("snax_interface_g")
local G = interface_g and require (interface_g) or { require = function() end }
interface_g = nil

ls.register_protocol {
	name = "snax",
	id = ls.PTYPE_SNAX,
	pack = ls.pack,
	unpack = ls.unpack,
}


function snax.interface(name)
	if typeclass[name] then
		return typeclass[name]
	end

	local si = snax_interface(name, G)

	local ret = {
		name = name,
		accept = {},
		response = {},
		system = {},
	}

	for _,v in ipairs(si) do
		local id, group, name, f = table.unpack(v)
		ret[group][name] = id
	end

	typeclass[name] = ret
	return ret
end

local meta = { __tostring = function(v) return string.format("[%s:%x]", v.type, v.handle) end}

local ls_send = ls.send
local ls_call = ls.call

local function gen_post(type, handle)
	return setmetatable({} , {
		__index = function( t, k )
			local id = type.accept[k]
			if not id then
				error(string.format("post %s:%s no exist", type.name, k))
			end
			return function(...)
				ls_send(handle, "snax", id, ...)
			end
		end })
end

local function gen_req(type, handle)
	return setmetatable({} , {
		__index = function( t, k )
			local id = type.response[k]
			if not id then
				error(string.format("request %s:%s no exist", type.name, k))
			end
			return function(...)
				return ls_call(handle, "snax", id, ...)
			end
		end })
end

local function wrapper(handle, name, type)
	return setmetatable ({
		post = gen_post(type, handle),
		req = gen_req(type, handle),
		type = name,
		handle = handle,
		}, meta)
end

local handle_cache = setmetatable( {} , { __mode = "kv" } )

function snax.rawnewservice(name, ...)
	local t = snax.interface(name)
	local handle = ls.newservice("snaxd", name)
	assert(handle_cache[handle] == nil)
	if t.system.init then
		ls.call(handle, "snax", t.system.init, ...)
	end
	return handle
end

function snax.bind(handle, type)
	local ret = handle_cache[handle]
	if ret then
		assert(ret.type == type)
		return ret
	end
	local t = snax.interface(type)
	ret = wrapper(handle, type, t)
	handle_cache[handle] = ret
	return ret
end

function snax.newservice(name, ...)
	local handle = snax.rawnewservice(name, ...)
	return snax.bind(handle, name)
end

function snax.uniqueservice(name, ...)
	local handle = assert(ls.call(".service", "lua", "LAUNCH", "snaxd", name, ...))
	return snax.bind(handle, name)
end

function snax.globalservice(name, ...)
	local handle = assert(ls.call(".service", "lua", "GLAUNCH", "snaxd", name, ...))
	return snax.bind(handle, name)
end

function snax.queryservice(name)
	local handle = assert(ls.call(".service", "lua", "QUERY", "snaxd", name))
	return snax.bind(handle, name)
end

function snax.queryglobal(name)
	local handle = assert(ls.call(".service", "lua", "GQUERY", "snaxd", name))
	return snax.bind(handle, name)
end

function snax.kill(obj, ...)
	local t = snax.interface(obj.type)
	ls_call(obj.handle, "snax", t.system.exit, ...)
end

function snax.self()
	return snax.bind(ls.self(), SERVICE_NAME)
end

function snax.exit(...)
	snax.kill(snax.self(), ...)
end

local function test_result(ok, ...)
	if ok then
		return ...
	else
		error(...)
	end
end

function snax.hotfix(obj, source, ...)
	local t = snax.interface(obj.type)
	return test_result(ls_call(obj.handle, "snax", t.system.hotfix, source, ...))
end

function snax.printf(fmt, ...)
	ls.error(string.format(fmt, ...))
end

function snax.profile_info(obj)
	local t = snax.interface(obj.type)
	return ls_call(obj.handle, "snax", t.system.profile)
end

return snax
