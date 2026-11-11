local FunctionService = require("api/FunctionService")
local ubus = require("ubus")

local ovpnmon = FunctionService:new()

-- GET /api/ovpnmon/listservers
function ovpnmon:GET_TYPE_listservers()
	
	local ctx = ubus.connect()

	if ctx == nil then
	return self:ResponseOK({
	    result = {},
	    error = "Failed connecting to ubus"
	})
	end
	
	local objects = ctx:objects()
	
	if not objects then
		ctx:close()
		return self:ResponseOK({
			result = {},
			error = "Failed getting ubus objects"
		})
	end
	
	local openvpn_objects = {}



	for _, object_name in ipairs(objects) do
		if object_name:sub(1, 8) == "openvpn." then
			local response, err = ctx:call(object_name, "display_name", {})

			if response then
			    table.insert(openvpn_objects, {
				internal_name = object_name:sub(9),
				display_name = response.display_name
			    })
			end
		end
	end

	ctx:close()

	return self:ResponseOK({
		result = openvpn_objects
	})
end

-- GET /api/ovpnmon/listclients?name=inst1
function ovpnmon:GET_TYPE_listclients()
	
	local name = self.query_parameters.name
	
	if name == nil or name == "" then
	    return self:ResponseOK({
		result = {},
		error = "Missing server name"
	    })
	end
	
	local ubus_object_name = "openvpn." .. name
	
	local ctx = ubus.connect()

	if ctx == nil then
		return self:ResponseOK({
		    result = {},
		    error = "Failed connecting to ubus"
		})
	end

	local response, err = ctx:call(ubus_object_name, "list_clients", {})

	if response then
		ctx:close()
		return self:ResponseOK({
			result = response.clients
		})
	end

	ctx:close()
	
	return self:ResponseOK({
		result = {},
		error = "Failed calling ubus method: " .. err
	})
    
end


function ovpnmon:KickClientAction()
	
	local server_name = self.arguments.data.server_name
	local cl_name = self.arguments.data.client_name
	
	local ctx = ubus.connect()

	if ctx == nil then
		return self:ResponseOK({
		    result = {},
		    error = "Failed connecting to ubus"
		})
	end

	-- Find Ubus object with this name
	local ubus_object_name = "openvpn." .. server_name

	local response, err = ctx:call(ubus_object_name, "kick_client", {client_name = cl_name})
	if response then
		ctx:close()
		return self:ResponseOK({
			result = response.message
		})
	end

	ctx:close()
	
	return self:ResponseOK({
		result = {},
		error = "Failed calling ubus method: " .. err
	})
end

-- POST /api/ovpnmon/actions/kick_client
local kick_client_action = ovpnmon:action("kick_client", ovpnmon.KickClientAction)


	local server_name = kick_client_action:option("server_name")
		server_name.require = true
		server_name.maxlength = 256

	local client_name = kick_client_action:option("client_name")
		client_name.require = true
		client_name.maxlength = 256
	

return ovpnmon
