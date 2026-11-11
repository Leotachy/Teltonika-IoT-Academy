


local Log = {}

Log.info = function(message)
	print(message)
end

Log.error = function(error)
	io.stderr:write(error .. "\n")
end

return Log
