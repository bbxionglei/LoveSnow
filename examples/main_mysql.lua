local ls = require "lovesnow"


ls.start(function()
	print("Main Server start")
	local console = ls.newservice("testmysql")
	
	print("Main Server exit")
	ls.exit()
end)
