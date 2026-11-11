local ConfigService = require("api/ConfigService")

local Tuyad = ConfigService:new({
	delete = false,          -- Disable deletion of UCI sections
	create = false,          -- Disable creation of UCI sections
	-- general_section = "main",-- General UCI section name
	-- anonymous = true,        -- Create UCI anonymous sections
	-- increment_name = true,   -- Create UCI sections with numeric incremental names
})

local ConfigTuyad = Tuyad:section(
	"tuyad", -- UCI config name
	"tuyad"  -- UCI section type
)
ConfigTuyad:make_primary()
ConfigTuyad.default_options.id.maxlength = 8 -- Default id option can also have validations
-- ConfigTuyad.order_by = "option" -- Order UCI config by provided option
-- ConfigTuyad.sort_response_by = "option" -- Order API response by provided option

function ConfigTuyad:create_defaults(sid)
	-- Default values to be added with every creation
	return {
		test = "default-value-" .. sid -- sid is the default_options id
	}
end

	local opt_enabled = ConfigTuyad:option("enable")
		opt_enabled.datatype = "number"

	local opt_device_id = ConfigTuyad:option("device_id")
		opt_device_id.datatype = "string"
		opt_device_id.cfg_require = true -- Option is required
		opt_device_id.maxlength = 100
		opt_device_id.minlength = 1

	local opt_device_secret = ConfigTuyad:option("device_secret")
		opt_device_secret.datatype = "string"
		opt_device_secret.cfg_require = true -- Option is required
		opt_device_secret.maxlength = 100
		opt_device_secret.minlength = 1
		
	local opt_product_id = ConfigTuyad:option("product_id")
		opt_product_id.datatype = "string"
		opt_product_id.cfg_require = true -- Option is required
		opt_product_id.maxlength = 100
		opt_product_id.minlength = 1

-- Uploads, Actions, GET_TYPE are also possible and are the same as in function_example.lua file.

-----------------------------------------------HOOKS-----------------------------------------------

-- function Tuyad:GET_init_hook()
-- end

-- function Tuyad:GET_section_init_hook()
-- end

-- function Tuyad:GET_validate_section_hook()
-- end

-- function Tuyad:GET_after_data_hook(data)
-- end


-- function Tuyad:POST_init_hook()
-- end

-- function Tuyad:POST_validate_hook()
-- end

-- function Tuyad:POST_before_commit_hook()
-- end

-- function Tuyad:POST_after_commit_hook()
-- end

-- function Tuyad:POST_section_init_hook()
-- end

-- function Tuyad:POST_validate_section_hook()
-- end

-- function Tuyad:POST_after_data_hook()
-- end


-- function Tuyad:PUT_init_hook()
-- end

-- function Tuyad:PUT_validate_hook()
-- end

-- function Tuyad:PUT_before_commit_hook()
-- end

-- function Tuyad:PUT_after_commit_hook()
-- end

-- function Tuyad:PUT_section_init_hook()
-- end

-- function Tuyad:PUT_validate_section_hook()
-- end

-- function Tuyad:PUT_after_validate_section_hook()
-- end

-- function Tuyad:PUT_after_data_hook()
-- end


-- function Tuyad:DELETE_init_hook()
-- end

-- function Tuyad:DELETE_validate_hook()
-- end

-- function Tuyad:DELETE_before_commit_hook()
-- end

-- function Tuyad:DELETE_after_commit_hook()
-- end

-- function Tuyad:DELETE_section_init_hook()
-- end

-- function Tuyad:DELETE_before_section_delete_hook()
-- end

-- function Tuyad:DELETE_after_data_hook(data)
-- end

return Tuyad
