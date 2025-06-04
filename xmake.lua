add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

-- add_requires("levilamina x.x.x") for a specific version
-- add_requires("levilamina develop") to use develop version
-- please note that you should add bdslibrary yourself if using dev version
if is_config("target_type", "server") then
    add_requires("levilamina 1.2.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.2.0", {configs = {target_type = "client"}})
end

add_requires("legacyremotecall")

add_requires("hash-library")

add_requires("levibuildscript")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("publish")
    set_default(false)
    set_showmenu(false)
option_end()

option("test")
    set_default(false)
    set_showmenu(true)
    set_description("Enable test")
option_end()

option("target_type")
    set_default("server")
    -- set_showmenu(true)
    set_values("server", "client")
option_end()

target("LeviFakePlayer")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE")
    add_packages("levilamina","legacyremotecall")
    if has_config("test") then
        add_defines("LFP_TEST_MODE")
    end    
    add_packages("hash-library")

    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")

    add_defines("LFP_EXPORT")
    set_configvar("LFP_WORKSPACE_FOLDER", "$(projectdir)")
    add_configfiles("src/(lfp/Version.h.in)")
    add_includedirs("$(buildir)")

    add_headerfiles("src/(lfp/api/**.h)")
    add_files("src/lfp/**.cpp")
    add_includedirs("src")
    if has_config("publish") then
        add_defines("LFP_VERSION_PUBLISH")
    end

    if has_config("test") then
        add_defines("LFP_DEBUG")
        add_files("src/test/**.cpp")
    elseif is_mode("debug") then
        add_defines("LFP_DEBUG")
        add_files("src/debug/**.cpp")
    end

    
    on_load(function (target)
        import("core.base.json")
        local tag = os.iorun("git describe --tags --abbrev=0 --always")
        local major, minor, patch, suffix = tag:match("v(%d+)%.(%d+)%.(%d+)(.*)")
        if not major then
            print("Failed to parse version tag, using 0.0.0")
            major, minor, patch, suffix =0,0,0,""
            -- tag = json.loadfile("tooth.json")["version"]
            -- major, minor, patch, suffix = tag:match("(%d+)%.(%d+)%.(%d+)(.*)")
        end
        local versionStr =  major.."."..minor.."."..patch
        if suffix then
            prerelease = suffix:match("-(.*)")
            if prerelease then
                prerelease = prerelease:gsub("\n", "")
            end
            if prerelease then
                target:set("configvar", "LFP_VERSION_PRERELEASE", prerelease)
                versionStr = versionStr.."-"..prerelease
            end
        end
        target:set("configvar", "LFP_VERSION_MAJOR", major)
        target:set("configvar", "LFP_VERSION_MINOR", minor)
        target:set("configvar", "LFP_VERSION_PATCH", patch)

    --     if not has_config("publish") then
    --         local hash = os.iorun("git rev-parse --short HEAD")
    --         versionStr = versionStr.."+"..hash:gsub("\n", "")
    --     end

    --     target:add("rules", "@levibuildscript/modpacker",{
    --            modVersion = versionStr
    --        })
    end)
    -- on_install(function (target) 
    --     target:add_installfiles()
    -- end)
    if has_config("test") then 
        before_build(function (target)
            -- TODO
            os.run("powershell .\\scripts\\build-lse-test.ps1")
        end)
    end
    after_build(function (target) 
        local langPath = path.join(os.projectdir(), "lang/")
        local outputPath = path.join(os.projectdir(), "bin/" .. target:basename())
        os.mkdir(outputPath)
        os.cp(langPath, outputPath)
    end)

    -- if is_config("target_type", "server") then
    --     add_includedirs("src-server")
    --     add_files("src-server/**.cpp")
    -- else
    --     add_includedirs("src-client")
    --     add_files("src-client/**.cpp")
    -- end
