local ls = require "lovesnow"
local cluster = require "lovesnow.cluster"
local snax = require "lovesnow.snax"

ls.start(function()
	cluster.reload {
		db = "127.0.0.1:2528",
		db2 = "127.0.0.1:2529",
	}

	local sdb = ls.newservice("simpledb")
	-- register name "sdb" for simpledb, you can use cluster.query() later.
	-- See cluster2.lua
	cluster.register("sdb", sdb)

	print(ls.call(sdb, "lua", "SET", "a", "foobar"))
	print(ls.call(sdb, "lua", "SET", "b", "foobar2"))
	print(ls.call(sdb, "lua", "GET", "a"))
	print(ls.call(sdb, "lua", "GET", "b"))
	cluster.open "db"
	cluster.open "db2"
	-- unique snax service
	snax.uniqueservice "pingserver"
end)
