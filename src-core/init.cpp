#define SATDUMP_DLL_EXPORT 1
#include "init.h"
#include "logger.h"
#include "core/module.h"
#include "core/pipeline.h"
#include <filesystem>
// #include "tle.h"
#include "core/plugin.h"
#include "satdump_vars.h"
#include "core/config.h"

#include "common/tracking/tle.h"
#include "products/products.h"

#include "core/opencl.h"

#include "common/dsp/buffer.h"

namespace satdump
{
    SATDUMP_DLL std::string user_path;
    SATDUMP_DLL std::string tle_file_override = "";
    SATDUMP_DLL bool tle_do_update_on_init = true;

    void initSatdump()
    {
        logger->info("   _____       __  ____                      ");
        logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
        logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
        logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
        logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
        logger->info("                                    /_/      ");
        logger->info("Starting SatDump v" + (std::string)SATDUMP_VERSION);
        logger->info("");

#ifdef _WIN32
        if (std::filesystem::exists("satdump_cfg.json"))
            user_path = "./config";
        else 
            user_path = std::string(getenv("APPDATA")) + "/satdump";
#elif __ANDROID__
        user_path = ".";
#else
        user_path = std::string(getenv("HOME")) + "/.config/satdump";
#endif

        if (std::filesystem::exists("satdump_cfg.json"))
            config::loadConfig("satdump_cfg.json", user_path);
        else
            config::loadConfig(satdump::RESPATH + "satdump_cfg.json", user_path);

        if (config::main_cfg["satdump_general"].contains("log_to_file"))
        {
            bool log_file = config::main_cfg["satdump_general"]["log_to_file"]["value"];
            if (log_file)
                initFileSink();
        }

        if (config::main_cfg["satdump_general"].contains("log_level"))
        {
            std::string log_level = config::main_cfg["satdump_general"]["log_level"]["value"];
            if (log_level == "trace")
                setConsoleLevel(slog::LOG_TRACE);
            else if (log_level == "debug")
                setConsoleLevel(slog::LOG_DEBUG);
            else if (log_level == "info")
                setConsoleLevel(slog::LOG_INFO);
            else if (log_level == "warn")
                setConsoleLevel(slog::LOG_WARN);
            else if (log_level == "error")
                setConsoleLevel(slog::LOG_ERROR);
            else if (log_level == "critical")
                setConsoleLevel(slog::LOG_CRIT);
        }

        loadPlugins();

        // Let plugins know we started
        eventBus->fire_event<config::RegisterPluginConfigHandlersEvent>({config::plugin_config_handlers});

        registerModules();

        // Load pipelines
        if (std::filesystem::exists("pipelines") && std::filesystem::is_directory("pipelines"))
            loadPipelines("pipelines");
        else
            loadPipelines(satdump::RESPATH + "pipelines");

        // List them
        logger->debug("Registered pipelines :");
        for (Pipeline &pipeline : pipelines)
            logger->debug(" - " + pipeline.name);

#ifdef USE_OPENCL
        // OpenCL
        opencl::initOpenCL();
#endif

        // TLEs
        if (tle_file_override == "")
        {
            loadTLEFileIntoRegistry(user_path + "/satdump_tles.txt");
            if (tle_do_update_on_init)
                autoUpdateTLE(user_path + "/satdump_tles.txt");
        }
        else
        {
            if (std::filesystem::exists(tle_file_override))
                loadTLEFileIntoRegistry(tle_file_override);
            else
                logger->error("TLE File doesn't exist : " + tle_file_override);
        }

        // Products
        registerProducts();

        // Set DSP buffer sizes if they have been changed
        if (config::main_cfg.contains("advanced_settings"))
        {
            if (config::main_cfg["advanced_settings"].contains("default_buffer_size"))
            {
                int new_sz = config::main_cfg["advanced_settings"]["default_buffer_size"].get<int>();
                dsp::STREAM_BUFFER_SIZE = new_sz;
                dsp::RING_BUF_SZ = new_sz;
                logger->warn("DSP Buffer size was changed to %d", new_sz);
            }
        }

        // Let plugins know we started
        eventBus->fire_event<SatDumpStartedEvent>({});

#ifdef BUILD_IS_DEBUG
        // If in debug, warn the user!
        logger->error("██████╗  █████╗ ███╗   ██╗ ██████╗ ███████╗██████╗ ");
        logger->error("██╔══██╗██╔══██╗████╗  ██║██╔════╝ ██╔════╝██╔══██╗");
        logger->error("██║  ██║███████║██╔██╗ ██║██║  ███╗█████╗  ██████╔╝");
        logger->error("██║  ██║██╔══██║██║╚██╗██║██║   ██║██╔══╝  ██╔══██╗");
        logger->error("██████╔╝██║  ██║██║ ╚████║╚██████╔╝███████╗██║  ██║");
        logger->error("╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝");
        logger->error("SatDump has NOT been built in Release mode.");
        logger->error("If you are not a developer but intending to use the software,");
        logger->error("you probably do not want this. If so, make sure to");
        logger->error("specify -DCMAKE_BUILD_TYPE=Release in CMake.");
#endif
    }
}