for _, plat in ipairs(vars.plats) do
	acklibrary {
		name = "lib_"..plat,
		srcs = {
			"./*.s", -- csb8.s
			"./*.c"
		},
		vars = { plat = plat },
		deps = {
			"h+emheaders"
		}
	}
end

