#include "module_products_processor.h"
#include "logger.h"
#include "imgui/imgui.h"

#include "products/processor/processor.h"

namespace products
{
    ProductsProcessorModule::ProductsProcessorModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        logger_sink = std::make_shared<widgets::LoggerSinkWidget>();
        // logger_sink->set_pattern("[%D - %T] %^(%L) %v%$");
        logger_sink->max_lines = 500;
    }

    std::vector<ModuleDataType> ProductsProcessorModule::getInputTypes()
    {
        return {DATA_FILE};
    }

    std::vector<ModuleDataType> ProductsProcessorModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    ProductsProcessorModule::~ProductsProcessorModule()
    {
    }

    void ProductsProcessorModule::process()
    {
        logger->add_sink(logger_sink);
        satdump::process_dataset(d_input_file);
        logger->del_sink(logger_sink);
    }

    void ProductsProcessorModule::drawUI(bool window)
    {
        ImGui::Begin("Products Processor", NULL, (window ? 0 : NOWINDOW_FLAGS) | ImGuiWindowFlags_NoScrollbar);
        logger_sink->draw();
        ImGui::End();
    }

    std::string ProductsProcessorModule::getID()
    {
        return "products_processor";
    }

    std::vector<std::string> ProductsProcessorModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> ProductsProcessorModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<ProductsProcessorModule>(input_file, output_file_hint, parameters);
    }
}