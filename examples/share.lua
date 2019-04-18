local ls = require "lovesnow"
local sharedata = require "lovesnow.sharedata"

local mode = ...

if mode == "host" then

ls.start(function()
	ls.error("new foobar")
	sharedata.new("foobar", { a=1, b= { "hello",  "world" } })

	ls.fork(function()
		ls.sleep(200)	-- sleep 2s
		ls.error("update foobar a = 2")
		sharedata.update("foobar", { a =2 })
		ls.sleep(200)	-- sleep 2s
		ls.error("update foobar a = 3")
		sharedata.update("foobar", { a = 3, b = { "change" } })
		ls.sleep(100)
		ls.error("delete foobar")
		sharedata.delete "foobar"
	end)
end)

else


ls.start(function()
	ls.newservice(SERVICE_NAME, "host")

	local obj = sharedata.query "foobar"

	local b = obj.b
	ls.error(string.format("a=%d", obj.a))

	for k,v in ipairs(b) do
		ls.error(string.format("b[%d]=%s", k,v))
	end

	-- test lua serialization
	local s = ls.packstring(obj)
	local nobj = ls.unpack(s)
	for k,v in pairs(nobj) do
		ls.error(string.format("nobj[%s]=%s", k,v))
	end
	for k,v in ipairs(nobj.b) do
		ls.error(string.format("nobj.b[%d]=%s", k,v))
	end

	for i = 1, 5 do
		ls.sleep(100)
		ls.error("second " ..i)
		for k,v in pairs(obj) do
			ls.error(string.format("%s = %s", k , tostring(v)))
		end
	end

	local ok, err = pcall(function()
		local tmp = { b[1], b[2] }	-- b is invalid , so pcall should failed
	end)

	if not ok then
		ls.error(err)
	end

	-- obj. b is not the same with local b
	for k,v in ipairs(obj.b) do
		ls.error(string.format("b[%d] = %s", k , tostring(v)))
	end

	collectgarbage()
	ls.error("sleep")
	ls.sleep(100)
	b = nil
	collectgarbage()
	ls.error("sleep")
	ls.sleep(100)

	ls.exit()
end)

end
