local ls = require "lovesnow"
local service = require "lovesnow.service"
local core = require "lovesnow.datasheet.core"

local datasheet_svr

ls.init(function()
	datasheet_svr = service.query "datasheet"
end)

local datasheet = {}
local sheets = setmetatable({}, {
	__gc = function(t)
		for _,v in pairs(t) do
			ls.send(datasheet_svr, "lua", "release", v.handle)
		end
	end,
})

local function querysheet(name)
	return ls.call(datasheet_svr, "lua", "query", name)
end

local function updateobject(name)
	local t = sheets[name]
	if not t.object then
		t.object = core.new(t.handle)
	end
	local function monitor()
		local handle = t.handle
		local newhandle = ls.call(datasheet_svr, "lua", "monitor", handle)
		core.update(t.object, newhandle)
		t.handle = newhandle
		ls.send(datasheet_svr, "lua", "release", handle)
		return monitor()
	end
	ls.fork(monitor)
end

function datasheet.query(name)
	local t = sheets[name]
	if not t then
		t = {}
		sheets[name] = t
	end
	if t.error then
		error(t.error)
	end
	if t.object then
		return t.object
	end
	if t.queue then
		local co = coroutine.running()
		table.insert(t.queue, co)
		ls.wait(co)
	else
		t.queue = {}	-- create wait queue for other query
		local ok, handle = pcall(querysheet, name)
		if ok then
			t.handle = handle
			updateobject(name)
		else
			t.error = handle
		end
		local q = t.queue
		t.queue = nil
		for _, co in ipairs(q) do
			ls.wakeup(co)
		end
	end
	if t.error then
		error(t.error)
	end
	return t.object
end

return datasheet
