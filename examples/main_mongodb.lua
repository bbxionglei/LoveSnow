local ls = require "lovesnow"


ls.start(function()
	print("Main Server start")
	local console = ls.newservice(
		"testmongodb", "127.0.0.1", 27017, "testdb", "test", "test"
	)
	
	print("Main Server exit")
	ls.exit()
end)
